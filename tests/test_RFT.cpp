/*
  Copyright 2015 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "config.h"

#define BOOST_TEST_MODULE EclipseRFTWriter

#include <boost/test/unit_test.hpp>

#include <opm/io/eclipse/ERft.hpp>
#include <opm/io/eclipse/OutputStream.hpp>

#include <opm/output/data/Groups.hpp>
#include <opm/output/data/Solution.hpp>
#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/EclipseIO.hpp>
#include <opm/output/eclipse/InteHEAD.hpp>
#include <opm/output/eclipse/WriteRFT.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/input/eclipse/Python/Python.hpp>
#include <opm/input/eclipse/Schedule/Action/State.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQState.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTestState.hpp>

#include <opm/input/eclipse/Units/Units.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <opm/common/utility/FileSystem.hpp>
#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <ctime>
#include <iomanip>
#include <iterator>
#include <map>
#include <ostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace std { // hack...
    // For printing ERft::RftDate objects.  Needed by EQUAL_COLLECTIONS.
    static ostream& operator<<(ostream& os, const tuple<int,int,int>& d)
    {
        os <<        setw(4)                 << get<0>(d)
           << "-" << setw(2) << setfill('0') << get<1>(d)
           << "-" << setw(2) << setfill('0') << get<2>(d);

        return os;
    }
}

namespace {
    struct Setup
    {
        explicit Setup(const std::string& deckfile)
            : Setup{ ::Opm::Parser{}.parseFile(deckfile) }
        {}

        explicit Setup(const ::Opm::Deck& deck)
            : es    { deck }
            , sched { deck, es, std::make_shared<const ::Opm::Python>() }
        {}

        ::Opm::EclipseState es;
        ::Opm::Schedule     sched;
    };

    class RSet
    {
    public:
        explicit RSet(std::string base)
            : odir_(std::filesystem::temp_directory_path() /
                    Opm::unique_path("rset-%%%%"))
            , base_(std::move(base))
        {
            std::filesystem::create_directories(this->odir_);
        }

        ~RSet()
        {
            std::filesystem::remove_all(this->odir_);
        }

        std::string outputDir() const
        {
            return this->odir_.string();
        }

        operator ::Opm::EclIO::OutputStream::ResultSet() const
        {
            return { this->odir_.string(), this->base_ };
        }

    private:
        std::filesystem::path odir_;
        std::string             base_;
    };

    class RFTResultIndex
    {
    public:
        explicit RFTResultIndex(const ::Opm::EclIO::ERft&          rft,
                                const std::string&                 well,
                                const ::Opm::EclIO::ERft::RftDate& date);

        std::size_t operator()(const int i, const int j, const int k) const
        {
            auto conIx = this->xConIx_.find(std::make_tuple(i, j, k));
            if (conIx == this->xConIx_.end()) {
                BOOST_FAIL("Invalid IJK Tuple (" << i << ", "
                           << j << ", " << k << ')');
            }

            return conIx->second;
        }

    private:
        std::map<std::tuple<int, int, int>, std::size_t> xConIx_;
    };

    RFTResultIndex::RFTResultIndex(const ::Opm::EclIO::ERft&          rft,
                                   const std::string&                 well,
                                   const ::Opm::EclIO::ERft::RftDate& date)
    {
        BOOST_REQUIRE(rft.hasRft(well, date));

        BOOST_REQUIRE(rft.hasArray("CONIPOS", well, date));
        BOOST_REQUIRE(rft.hasArray("CONJPOS", well, date));
        BOOST_REQUIRE(rft.hasArray("CONKPOS", well, date));

        const auto& I = rft.getRft<int>("CONIPOS", well, date);
        const auto& J = rft.getRft<int>("CONJPOS", well, date);
        const auto& K = rft.getRft<int>("CONKPOS", well, date);

        for (auto ncon = I.size(), con = 0*ncon; con < ncon; ++con) {
            this->xConIx_.emplace(std::piecewise_construct,
                                  std::forward_as_tuple(I[con], J[con], K[con]),
                                  std::forward_as_tuple(con));
        }
    }

    class RFTRresults
    {
    public:
        explicit RFTRresults(const ::Opm::EclIO::ERft&          rft,
                             const std::string&                 well,
                             const ::Opm::EclIO::ERft::RftDate& date);

        float depth(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->depth_);
        }

        float pressure(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->press_);
        }

        float sgas(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->sgas_);
        }

        float swat(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->swat_);
        }

    private:
        RFTResultIndex resIx_;

        std::vector<float> depth_{};
        std::vector<float> press_{};
        std::vector<float> sgas_{};
        std::vector<float> swat_{};

        template <typename T, class A>
        T value(const int i, const int j, const int k,
                const std::vector<T, A>& vector) const
        {
            return vector[ this->resIx_(i, j, k) ];
        }
    };

    RFTRresults::RFTRresults(const ::Opm::EclIO::ERft&          rft,
                             const std::string&                 well,
                             const ::Opm::EclIO::ERft::RftDate& date)
        : resIx_{ rft, well, date }
    {
        BOOST_REQUIRE(rft.hasArray("DEPTH"   , well, date));
        BOOST_REQUIRE(rft.hasArray("PRESSURE", well, date));
        BOOST_REQUIRE(rft.hasArray("SGAS"    , well, date));
        BOOST_REQUIRE(rft.hasArray("SWAT"    , well, date));

        this->depth_ = rft.getRft<float>("DEPTH"   , well, date);
        this->press_ = rft.getRft<float>("PRESSURE", well, date);
        this->sgas_  = rft.getRft<float>("SGAS"    , well, date);
        this->swat_  = rft.getRft<float>("SWAT"    , well, date);
    }

    class PLTResults
    {
    public:
        explicit PLTResults(const ::Opm::EclIO::ERft&          rft,
                            const std::string&                 well,
                            const ::Opm::EclIO::ERft::RftDate& date);

        int next(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->neighbour_id_);
        }

        float depth(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->depth_);
        }

        float pressure(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->press_);
        }

        float conntrans(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->trans_);
        }

        float kh(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->kh_);
        }

        float orat(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->orat_);
        }

        float wrat(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->wrat_);
        }

        float grat(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->grat_);
        }

    protected:
        template <typename T, class A>
        T value(const int i, const int j, const int k,
                const std::vector<T, A>& vector) const
        {
            return vector[ this->resIx_(i, j, k) ];
        }

    private:
        RFTResultIndex resIx_;

        std::vector<int> neighbour_id_{};

        std::vector<float> depth_{};
        std::vector<float> press_{};
        std::vector<float> trans_{};
        std::vector<float> kh_{};

        std::vector<float> orat_{};
        std::vector<float> wrat_{};
        std::vector<float> grat_{};
    };

    PLTResults::PLTResults(const ::Opm::EclIO::ERft&          rft,
                           const std::string&                 well,
                           const ::Opm::EclIO::ERft::RftDate& date)
        : resIx_{ rft, well, date }
    {
        BOOST_REQUIRE(rft.hasArray("CONNXT"  , well, date));

        BOOST_REQUIRE(rft.hasArray("CONDEPTH", well, date));
        BOOST_REQUIRE(rft.hasArray("CONPRES" , well, date));
        BOOST_REQUIRE(rft.hasArray("CONFAC"  , well, date));
        BOOST_REQUIRE(rft.hasArray("CONKH"   , well, date));

        BOOST_REQUIRE(rft.hasArray("CONORAT" , well, date));
        BOOST_REQUIRE(rft.hasArray("CONWRAT" , well, date));
        BOOST_REQUIRE(rft.hasArray("CONGRAT" , well, date));

        this->neighbour_id_ = rft.getRft<int>("CONNXT", well, date);

        this->depth_ = rft.getRft<float>("CONDEPTH", well, date);
        this->press_ = rft.getRft<float>("CONPRES" , well, date);
        this->trans_ = rft.getRft<float>("CONFAC"  , well, date);
        this->kh_    = rft.getRft<float>("CONKH"   , well, date);

        this->orat_ = rft.getRft<float>("CONORAT", well, date);
        this->wrat_ = rft.getRft<float>("CONWRAT", well, date);
        this->grat_ = rft.getRft<float>("CONGRAT", well, date);
    }

    class PLTResultsMSW : public PLTResults
    {
    public:
        explicit PLTResultsMSW(const ::Opm::EclIO::ERft&          rft,
                               const std::string&                 well,
                               const ::Opm::EclIO::ERft::RftDate& date);

        int segment(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->segment_id_);
        }

        int branch(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->branch_id_);
        }

        float start(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->start_length_);
        }

        float end(const int i, const int j, const int k) const
        {
            return this->value(i, j, k, this->end_length_);
        }

    private:
        std::vector<int> segment_id_{};
        std::vector<int> branch_id_{};

        std::vector<float> start_length_{};
        std::vector<float> end_length_{};
    };

    PLTResultsMSW::PLTResultsMSW(const ::Opm::EclIO::ERft&          rft,
                                 const std::string&                 well,
                                 const ::Opm::EclIO::ERft::RftDate& date)
        : PLTResults{ rft, well, date }
    {
        BOOST_REQUIRE(rft.hasArray("CONLENST", well, date));
        BOOST_REQUIRE(rft.hasArray("CONLENEN", well, date));
        BOOST_REQUIRE(rft.hasArray("CONSEGNO", well, date));
        BOOST_REQUIRE(rft.hasArray("CONBRNO" , well, date));

        this->segment_id_ = rft.getRft<int>("CONSEGNO", well, date);
        this->branch_id_  = rft.getRft<int>("CONBRNO" , well, date);

        this->start_length_ = rft.getRft<float>("CONLENST", well, date);
        this->end_length_   = rft.getRft<float>("CONLENEN", well, date);
    }

    void verifyRFTFile(const std::string& rft_filename)
    {
        using RftDate = ::Opm::EclIO::ERft::RftDate;

        const auto rft = ::Opm::EclIO::ERft{ rft_filename };

        const auto xRFT = RFTRresults {
            rft, "OP_1", RftDate{ 2008, 10, 10 }
        };

        const auto tol = 1.0e-5;

        BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 1), 0.0   , tol);
        BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 2), 1.0e-5, tol);
        BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 3), 2.0e-5, tol);

        BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 1), 0.0, tol);
        BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 2), 0.2, tol);
        BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 3), 0.4, tol);

        BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 1), 0.0, tol);
        BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 2), 0.1, tol);
        BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 3), 0.2, tol);

        BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 1), 1*0.250 + 0.250/2, tol);
        BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 2), 2*0.250 + 0.250/2, tol);
        BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 3), 3*0.250 + 0.250/2, tol);
    }

    Opm::data::Solution createBlackoilState(int timeStepIdx, int numCells)
    {
        std::vector< double > pressure( numCells );
        std::vector< double > swat( numCells, 0 );
        std::vector< double > sgas( numCells, 0 );

        for (int i = 0; i < numCells; ++i) {
            pressure[i] = timeStepIdx*1e5 + 1e4 + i;
        }

        Opm::data::Solution sol;
        sol.insert( "PRESSURE", Opm::UnitSystem::measure::pressure, pressure, Opm::data::TargetType::RESTART_SOLUTION );
        sol.insert( "SWAT"    , Opm::UnitSystem::measure::identity, swat    , Opm::data::TargetType::RESTART_SOLUTION );
        sol.insert( "SGAS"    , Opm::UnitSystem::measure::identity, sgas    , Opm::data::TargetType::RESTART_SOLUTION );

        return sol;
    }

    std::time_t timeStamp(const ::Opm::EclIO::ERft::RftDate& date)
    {
        auto tp = std::tm{};

        tp.tm_year = std::get<0>(date) - 1900;
        tp.tm_mon  = std::get<1>(date) -    1; // 0..11
        tp.tm_mday = std::get<2>(date);        // 1..31

        return Opm::TimeService::makeUTCTime(tp);
    }
} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Using_EclipseIO)

BOOST_AUTO_TEST_CASE(test_RFT)
{
    auto python = std::make_shared<Opm::Python>();
    const auto rset = RSet{ "TESTRFT" };

    const auto eclipse_data_filename = std::string{ "testrft.DATA" };

    const auto deck = Opm::Parser{}.parseFile(eclipse_data_filename);
    auto eclipseState = Opm::EclipseState { deck };

    eclipseState.getIOConfig().setOutputDir(rset.outputDir());

    {
        /* eclipseWriter is scoped here to ensure it is destroyed after the
         * file itself has been written, because we're going to reload it
         * immediately. first upon destruction can we guarantee it being
         * written to disk and flushed.
         */

        const auto& grid = eclipseState.getInputGrid();
        const auto numCells = grid.getCartesianSize( );

        const Opm::Schedule schedule(deck, eclipseState, python);
        const Opm::SummaryConfig summary_config( deck, schedule, eclipseState.fieldProps(), eclipseState.aquifer() );

        Opm::EclipseIO eclipseWriter( eclipseState, grid, schedule, summary_config );

        const auto start_time = schedule.posixStartTime();
        const auto step_time  = timeStamp(::Opm::EclIO::ERft::RftDate{ 2008, 10, 10 });

        Opm::SummaryState st(Opm::TimeService::now());
        Opm::Action::State action_state;
        Opm::UDQState udq_state(1234);
        Opm::WellTestState wtest_state;

        Opm::data::Rates r1, r2;
        r1.set( Opm::data::Rates::opt::wat, 4.11 );
        r1.set( Opm::data::Rates::opt::oil, 4.12 );
        r1.set( Opm::data::Rates::opt::gas, 4.13 );

        r2.set( Opm::data::Rates::opt::wat, 4.21 );
        r2.set( Opm::data::Rates::opt::oil, 4.22 );
        r2.set( Opm::data::Rates::opt::gas, 4.23 );

        std::vector<Opm::data::Connection> well1_comps(9);
        for (size_t i = 0; i < 9; ++i) {
            Opm::data::Connection well_comp { grid.getGlobalIndex(8,8,i) ,r1, 0.0 , 0.0, (double)i, 0.1*i,0.2*i, 1.2e3, 4.321};
            well1_comps[i] = std::move(well_comp);
        }
        std::vector<Opm::data::Connection> well2_comps(6);
        for (size_t i = 0; i < 6; ++i) {
            Opm::data::Connection well_comp { grid.getGlobalIndex(3,3,i+3) ,r2, 0.0 , 0.0, (double)i, i*0.1,i*0.2, 0.15, 0.54321};
            well2_comps[i] = std::move(well_comp);
        }

        Opm::data::Solution solution = createBlackoilState(2, numCells);
        Opm::data::Wells wells;
        Opm::data::GroupAndNetworkValues group_nwrk;

        using SegRes = decltype(wells["w"].segments);
        using Ctrl = decltype(wells["w"].current_control);

        wells["OP_1"] = {
            std::move(r1), 1.0, 1.1, 3.1, 1,
            ::Opm::Well::Status::OPEN,
            std::move(well1_comps), SegRes{}, Ctrl{}
        };
        wells["OP_2"] = {
            std::move(r2), 1.0, 1.1, 3.2, 1,
            ::Opm::Well::Status::OPEN,
            std::move(well2_comps), SegRes{}, Ctrl{}
        };

        Opm::RestartValue restart_value(std::move(solution), std::move(wells), std::move(group_nwrk), {});

        eclipseWriter.writeTimeStep( action_state,
                                     wtest_state,
                                     st,
                                     udq_state,
                                     2,
                                     false,
                                     step_time - start_time,
                                     std::move(restart_value));
    }

    verifyRFTFile(Opm::EclIO::OutputStream::outputFileName(rset, "RFT"));
}

namespace {
    void verifyRFTFile2(const std::string& rft_filename)
    {
        using RftDate = Opm::EclIO::ERft::RftDate;
        const auto rft = ::Opm::EclIO::ERft{ rft_filename };

        auto dates = std::unordered_map<
            std::string, std::vector<RftDate>
        >{};

        for (const auto& wellDate : rft.listOfRftReports()) {
            dates[std::get<0>(wellDate)].push_back(std::get<1>(wellDate));
        }

        // Well OP_1
        {
            auto op_1 = dates.find("OP_1");
            if (op_1 == dates.end()) {
                BOOST_FAIL("Missing RFT Data for Well OP_1");
            }

            const auto expect = std::vector<Opm::EclIO::ERft::RftDate> {
                RftDate{ 2008, 10, 10 },
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(op_1->second.begin(),
                                          op_1->second.end(),
                                          expect.begin(), expect.end());
        }

        // Well OP_2
        {
            auto op_2 = dates.find("OP_2");
            if (op_2 == dates.end()) {
                BOOST_FAIL("Missing RFT Data for Well OP_2");
            }

            const auto expect = std::vector<RftDate> {
                RftDate{ 2008, 10, 10 },
                RftDate{ 2008, 11, 10 },
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(op_2->second.begin(),
                                          op_2->second.end(),
                                          expect.begin(), expect.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(test_RFT2)
{
    auto python = std::make_shared<Opm::Python>();
    const auto rset = RSet{ "TESTRFT" };

    const auto eclipse_data_filename = std::string{ "testrft.DATA" };

    const auto deck = Opm::Parser().parseFile( eclipse_data_filename );
    auto eclipseState = Opm::EclipseState(deck);

    eclipseState.getIOConfig().setOutputDir(rset.outputDir());

    {
        /* eclipseWriter is scoped here to ensure it is destroyed after the
         * file itself has been written, because we're going to reload it
         * immediately. first upon destruction can we guarantee it being
         * written to disk and flushed.
         */

        const auto& grid = eclipseState.getInputGrid();
        const auto numCells = grid.getCartesianSize( );

        Opm::Schedule schedule(deck, eclipseState, python);
        Opm::SummaryConfig summary_config( deck, schedule, eclipseState.fieldProps(), eclipseState.aquifer() );
        Opm::SummaryState st(Opm::TimeService::now());
        Opm::Action::State action_state;
        Opm::UDQState udq_state(10);
        Opm::WellTestState wtest_state;

        const auto  start_time = schedule.posixStartTime();
        for (int counter = 0; counter < 2; counter++) {
            Opm::EclipseIO eclipseWriter( eclipseState, grid, schedule, summary_config );
            for (size_t step = 0; step < schedule.size(); step++) {
                const auto step_time = schedule.simTime(step);

                Opm::data::Rates r1, r2;
                r1.set( Opm::data::Rates::opt::wat, 4.11 );
                r1.set( Opm::data::Rates::opt::oil, 4.12 );
                r1.set( Opm::data::Rates::opt::gas, 4.13 );

                r2.set( Opm::data::Rates::opt::wat, 4.21 );
                r2.set( Opm::data::Rates::opt::oil, 4.22 );
                r2.set( Opm::data::Rates::opt::gas, 4.23 );

                std::vector<Opm::data::Connection> well1_comps(9);
                for (size_t i = 0; i < 9; ++i) {
                    Opm::data::Connection well_comp { grid.getGlobalIndex(8,8,i) ,r1, 0.0 , 0.0, (double)i, 0.1*i,0.2*i, 3.14e5, 0.1234};
                    well1_comps[i] = std::move(well_comp);
                }
                std::vector<Opm::data::Connection> well2_comps(6);
                for (size_t i = 0; i < 6; ++i) {
                    Opm::data::Connection well_comp { grid.getGlobalIndex(3,3,i+3) ,r2, 0.0 , 0.0, (double)i, i*0.1,i*0.2, 355.113, 0.9876};
                    well2_comps[i] = std::move(well_comp);
                }

                Opm::data::Wells wells;
                Opm::data::Solution solution = createBlackoilState(2, numCells);

                using SegRes = decltype(wells["w"].segments);
                using Ctrl = decltype(wells["w"].current_control);

                wells["OP_1"] = {
                    std::move(r1), 1.0, 1.1, 3.1, 1,
                    ::Opm::Well::Status::OPEN,
                    std::move(well1_comps), SegRes{}, Ctrl{}
                };
                wells["OP_2"] = {
                    std::move(r2), 1.0, 1.1, 3.2, 1,
                    ::Opm::Well::Status::OPEN,
                    std::move(well2_comps), SegRes{}, Ctrl{}
                };

                Opm::RestartValue restart_value(std::move(solution), std::move(wells),
                                                Opm::data::GroupAndNetworkValues(), {});

                eclipseWriter.writeTimeStep( action_state,
                                             wtest_state,
                                             st,
                                             udq_state,
                                             step,
                                             false,
                                             step_time - start_time,
                                             std::move(restart_value));
            }

            verifyRFTFile2(Opm::EclIO::OutputStream::outputFileName(rset, "RFT"));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END() // Using_EclipseIO

// =====================================================================

BOOST_AUTO_TEST_SUITE(Using_Direct_Write)

namespace {
    std::vector<Opm::data::Connection>
    connRes_OP1(const ::Opm::EclipseGrid& grid)
    {
        auto xcon = std::vector<Opm::data::Connection>{};
        xcon.reserve(9);

        for (auto con = 0; con < 9; ++con) {
            xcon.emplace_back();

            auto& c = xcon.back();
            c.index = grid.getGlobalIndex(8, 8, con);

            c.cell_pressure = (120 + con*10)*::Opm::unit::barsa;

            c.cell_saturation_gas   = 0.15;
            c.cell_saturation_water = 0.3 + con/20.0;
            c.trans_factor          = 0.98765;
        }

        return xcon;
    }

    ::Opm::data::Well wellSol_OP1(const ::Opm::EclipseGrid& grid)
    {
        auto xw = ::Opm::data::Well{};
        xw.connections = connRes_OP1(grid);

        return xw;
    }

    std::vector<Opm::data::Connection>
    connRes_OP2(const ::Opm::EclipseGrid& grid)
    {
        auto xcon = std::vector<Opm::data::Connection>{};
        xcon.reserve(6);

        for (auto con = 3; con < 9; ++con) {
            xcon.emplace_back();

            auto& c = xcon.back();
            c.index = grid.getGlobalIndex(3, 3, con);

            c.cell_pressure = (120 + con*10)*::Opm::unit::barsa;

            c.cell_saturation_gas   = 0.6 - con/20.0;
            c.cell_saturation_water = 0.25;
            c.trans_factor          = 0.12345;
        }

        return xcon;
    }

    ::Opm::data::Well wellSol_OP2(const ::Opm::EclipseGrid& grid)
    {
        auto xw = ::Opm::data::Well{};
        xw.connections = connRes_OP2(grid);

        return xw;
    }

    ::Opm::data::Wells wellSol(const ::Opm::EclipseGrid& grid)
    {
        auto xw = ::Opm::data::Wells{};

        xw["OP_1"] = wellSol_OP1(grid);
        xw["OP_2"] = wellSol_OP2(grid);

        return xw;
    }
}

BOOST_AUTO_TEST_CASE(Basic_Unformatted)
{
    using RftDate = ::Opm::EclIO::ERft::RftDate;

    const auto rset  = RSet { "TESTRFT" };
    const auto model = Setup{ "testrft.DATA" };

    {
        auto rftFile = ::Opm::EclIO::OutputStream::RFT {
            rset, ::Opm::EclIO::OutputStream::Formatted  { false },
            ::Opm::EclIO::OutputStream::RFT::OpenExisting{ false }
        };

        const auto  reportStep = 2;
        const auto  elapsed    = model.sched.seconds(reportStep);
        const auto& grid       = model.es.getInputGrid();

        ::Opm::RftIO::write(reportStep, elapsed, model.es.getUnits(),
                            grid, model.sched, wellSol(grid), rftFile);
    }

    {
        const auto rft = ::Opm::EclIO::ERft {
            ::Opm::EclIO::OutputStream::outputFileName(rset, "RFT")
        };

        {
            const auto xRFT = RFTRresults {
                rft, "OP_1", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 0.25f;

            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 1), 1*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 2), 2*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 3), 3*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 1), 120.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 2), 130.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 3), 140.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 4), 150.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 5), 160.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 6), 170.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 7), 180.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 8), 190.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 9), 200.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 1), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 2), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 3), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 4), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 5), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 6), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 7), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 8), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 9), 0.15f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 1), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 2), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 3), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 5), 0.50f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 6), 0.55f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 7), 0.60f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 8), 0.65f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 9), 0.70f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 0.25f;

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 150.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 160.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 170.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 180.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 190.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 200.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_1", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_1", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_1");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  BARSA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  BARSA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }
    }

    {
        auto rftFile = ::Opm::EclIO::OutputStream::RFT {
            rset, ::Opm::EclIO::OutputStream::Formatted  { false },
            ::Opm::EclIO::OutputStream::RFT::OpenExisting{ true }
        };

        const auto  reportStep = 3;
        const auto  elapsed    = model.sched.seconds(reportStep);
        const auto& grid       = model.es.getInputGrid();

        ::Opm::RftIO::write(reportStep, elapsed, model.es.getUnits(),
                            grid, model.sched, wellSol(grid), rftFile);
    }

    {
        const auto rft = ::Opm::EclIO::ERft {
            ::Opm::EclIO::OutputStream::outputFileName(rset, "RFT")
        };

        {
            const auto xRFT = RFTRresults {
                rft, "OP_1", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 0.25f;

            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 1), 1*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 2), 2*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 3), 3*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 1), 120.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 2), 130.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 3), 140.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 4), 150.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 5), 160.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 6), 170.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 7), 180.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 8), 190.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 9), 200.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 1), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 2), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 3), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 4), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 5), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 6), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 7), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 8), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 9), 0.15f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 1), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 2), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 3), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 5), 0.50f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 6), 0.55f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 7), 0.60f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 8), 0.65f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 9), 0.70f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 0.25f;

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 150.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 160.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 170.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 180.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 190.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 200.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 11, 10 }
            };

            const auto thick = 0.25f;

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 150.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 160.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 170.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 180.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 190.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 200.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_1", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_1", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_1");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  BARSA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  BARSA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }

        {
            const auto date = RftDate{2008, 11, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  BARSA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }
    }
}

BOOST_AUTO_TEST_CASE(Basic_Formatted)
{
    using RftDate = ::Opm::EclIO::ERft::RftDate;

    const auto rset  = RSet { "TESTRFT" };
    const auto model = Setup{ "testrft.DATA" };

    {
        auto rftFile = ::Opm::EclIO::OutputStream::RFT {
            rset, ::Opm::EclIO::OutputStream::Formatted  { true  },
            ::Opm::EclIO::OutputStream::RFT::OpenExisting{ false }
        };

        const auto  reportStep = 2;
        const auto  elapsed    = model.sched.seconds(reportStep);
        const auto& grid       = model.es.getInputGrid();

        ::Opm::RftIO::write(reportStep, elapsed, model.es.getUnits(),
                            grid, model.sched, wellSol(grid), rftFile);
    }

    {
        const auto rft = ::Opm::EclIO::ERft {
            ::Opm::EclIO::OutputStream::outputFileName(rset, "FRFT")
        };

        {
            const auto xRFT = RFTRresults {
                rft, "OP_1", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 0.25f;

            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 1), 1*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 2), 2*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 3), 3*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 1), 120.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 2), 130.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 3), 140.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 4), 150.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 5), 160.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 6), 170.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 7), 180.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 8), 190.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 9), 200.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 1), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 2), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 3), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 4), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 5), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 6), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 7), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 8), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 9), 0.15f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 1), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 2), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 3), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 5), 0.50f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 6), 0.55f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 7), 0.60f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 8), 0.65f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 9), 0.70f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 0.25f;

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 150.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 160.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 170.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 180.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 190.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 200.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_1", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_1", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_1");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  BARSA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  BARSA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }
    }

    {
        auto rftFile = ::Opm::EclIO::OutputStream::RFT {
            rset, ::Opm::EclIO::OutputStream::Formatted  { true },
            ::Opm::EclIO::OutputStream::RFT::OpenExisting{ true }
        };

        const auto  reportStep = 3;
        const auto  elapsed    = model.sched.seconds(reportStep);
        const auto& grid       = model.es.getInputGrid();

        ::Opm::RftIO::write(reportStep, elapsed, model.es.getUnits(),
                            grid, model.sched, wellSol(grid), rftFile);
    }

    {
        const auto rft = ::Opm::EclIO::ERft {
            ::Opm::EclIO::OutputStream::outputFileName(rset, "FRFT")
        };

        {
            const auto xRFT = RFTRresults {
                rft, "OP_1", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 0.25f;

            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 1), 1*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 2), 2*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 3), 3*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 1), 120.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 2), 130.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 3), 140.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 4), 150.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 5), 160.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 6), 170.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 7), 180.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 8), 190.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 9), 200.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 1), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 2), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 3), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 4), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 5), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 6), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 7), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 8), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 9), 0.15f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 1), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 2), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 3), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 5), 0.50f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 6), 0.55f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 7), 0.60f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 8), 0.65f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 9), 0.70f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 0.25f;

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 150.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 160.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 170.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 180.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 190.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 200.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 11, 10 }
            };

            const auto thick = 0.25f;

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 150.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 160.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 170.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 180.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 190.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 200.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_1", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_1", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_1");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  BARSA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  BARSA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }

        {
            const auto date = RftDate{2008, 11, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  BARSA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }
    }
}

BOOST_AUTO_TEST_CASE(Field_Units)
{
    using RftDate = ::Opm::EclIO::ERft::RftDate;

    const auto rset  = RSet { "TESTRFT" };
    const auto model = Setup{ "testrft.DATA" };
    const auto usys  = ::Opm::UnitSystem::newFIELD();

    {
        auto rftFile = ::Opm::EclIO::OutputStream::RFT {
            rset, ::Opm::EclIO::OutputStream::Formatted  { false },
            ::Opm::EclIO::OutputStream::RFT::OpenExisting{ false }
        };

        const auto  reportStep = 2;
        const auto  elapsed    = model.sched.seconds(reportStep);
        const auto& grid       = model.es.getInputGrid();

        ::Opm::RftIO::write(reportStep, elapsed, usys, grid,
                            model.sched, wellSol(grid), rftFile);
    }

    {
        const auto rft = ::Opm::EclIO::ERft {
            ::Opm::EclIO::OutputStream::outputFileName(rset, "RFT")
        };

        {
            const auto xRFT = RFTRresults {
                rft, "OP_1", RftDate{ 2008, 10, 10 }
            };

            const auto thick = ::Opm::unit::convert::to(0.25f, ::Opm::unit::feet);

            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 1), 1*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 2), 2*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 3), 3*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 4), 4*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 5), 5*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 6), 6*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 7), 7*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 8), 8*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 9), 9*thick + thick/2.0f, 5.0e-6f);

            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 1), 1.740452852762511e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 2), 1.885490590492720e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 3), 2.030528328222930e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 4), 2.175566065953139e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 5), 2.320603803683348e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 6), 2.465641541413557e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 7), 2.610679279143767e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 8), 2.755717016873976e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 9), 2.900754754604185e+03f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 1), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 2), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 3), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 4), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 5), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 6), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 7), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 8), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 9), 0.15f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 1), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 2), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 3), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 5), 0.50f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 6), 0.55f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 7), 0.60f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 8), 0.65f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 9), 0.70f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 10, 10 }
            };

            const auto thick = ::Opm::unit::convert::to(0.25f, ::Opm::unit::feet);

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 5.0e-6f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 2.175566065953139e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 2.320603803683348e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 2.465641541413557e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 2.610679279143767e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 2.755717016873976e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 2.900754754604185e+03f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_1", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_1", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_1");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], "  FEET");
            BOOST_CHECK_EQUAL(welletc[ 4], "  PSIA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " STB/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], "MSCF/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RB/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " FT/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " LB/STB");
            BOOST_CHECK_EQUAL(welletc[14], " LB/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  LB/LB");
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], "  FEET");
            BOOST_CHECK_EQUAL(welletc[ 4], "  PSIA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " STB/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], "MSCF/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RB/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " FT/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " LB/STB");
            BOOST_CHECK_EQUAL(welletc[14], " LB/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  LB/LB");
        }
    }

    {
        auto rftFile = ::Opm::EclIO::OutputStream::RFT {
            rset, ::Opm::EclIO::OutputStream::Formatted  { false },
            ::Opm::EclIO::OutputStream::RFT::OpenExisting{ true }
        };

        const auto  reportStep = 3;
        const auto  elapsed    = model.sched.seconds(reportStep);
        const auto& grid       = model.es.getInputGrid();

        ::Opm::RftIO::write(reportStep, elapsed, usys, grid,
                            model.sched, wellSol(grid), rftFile);
    }

    {
        const auto rft = ::Opm::EclIO::ERft {
            ::Opm::EclIO::OutputStream::outputFileName(rset, "RFT")
        };

        {
            const auto xRFT = RFTRresults {
                rft, "OP_1", RftDate{ 2008, 10, 10 }
            };

            const auto thick = ::Opm::unit::convert::to(0.25f, ::Opm::unit::feet);

            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 1), 1*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 2), 2*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 3), 3*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 4), 4*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 5), 5*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 6), 6*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 7), 7*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 8), 8*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 9), 9*thick + thick/2.0f, 5.0e-6f);

            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 1), 1.740452852762511e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 2), 1.885490590492720e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 3), 2.030528328222930e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 4), 2.175566065953139e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 5), 2.320603803683348e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 6), 2.465641541413557e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 7), 2.610679279143767e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 8), 2.755717016873976e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 9), 2.900754754604185e+03f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 1), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 2), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 3), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 4), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 5), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 6), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 7), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 8), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 9), 0.15f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 1), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 2), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 3), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 5), 0.50f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 6), 0.55f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 7), 0.60f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 8), 0.65f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 9), 0.70f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 10, 10 }
            };

            const auto thick = ::Opm::unit::convert::to(0.25f, ::Opm::unit::feet);

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 5.0e-6f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 2.175566065953139e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 2.320603803683348e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 2.465641541413557e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 2.610679279143767e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 2.755717016873976e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 2.900754754604185e+03f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 11, 10 }
            };

            const auto thick = ::Opm::unit::convert::to(0.25f, ::Opm::unit::feet);

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 5.0e-6f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 5.0e-6f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 2.175566065953139e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 2.320603803683348e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 2.465641541413557e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 2.610679279143767e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 2.755717016873976e+03f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 2.900754754604185e+03f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_1", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_1", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_1");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], "  FEET");
            BOOST_CHECK_EQUAL(welletc[ 4], "  PSIA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " STB/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], "MSCF/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RB/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " FT/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " LB/STB");
            BOOST_CHECK_EQUAL(welletc[14], " LB/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  LB/LB");
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], "  FEET");
            BOOST_CHECK_EQUAL(welletc[ 4], "  PSIA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " STB/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], "MSCF/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RB/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " FT/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " LB/STB");
            BOOST_CHECK_EQUAL(welletc[14], " LB/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  LB/LB");
        }

        {
            const auto date = RftDate{2008, 11, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], "  FEET");
            BOOST_CHECK_EQUAL(welletc[ 4], "  PSIA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " STB/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], "MSCF/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RB/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " FT/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " LB/STB");
            BOOST_CHECK_EQUAL(welletc[14], " LB/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  LB/LB");
        }
    }
}

BOOST_AUTO_TEST_CASE(Lab_Units)
{
    using RftDate = ::Opm::EclIO::ERft::RftDate;

    const auto rset  = RSet { "TESTRFT" };
    const auto model = Setup{ "testrft.DATA" };
    const auto usys  = ::Opm::UnitSystem::newLAB();

    {
        auto rftFile = ::Opm::EclIO::OutputStream::RFT {
            rset, ::Opm::EclIO::OutputStream::Formatted  { false },
            ::Opm::EclIO::OutputStream::RFT::OpenExisting{ false }
        };

        const auto  reportStep = 2;
        const auto  elapsed    = model.sched.seconds(reportStep);
        const auto& grid       = model.es.getInputGrid();

        ::Opm::RftIO::write(reportStep, elapsed, usys, grid,
                            model.sched, wellSol(grid), rftFile);
    }

    {
        const auto rft = ::Opm::EclIO::ERft {
            ::Opm::EclIO::OutputStream::outputFileName(rset, "RFT")
        };

        {
            const auto xRFT = RFTRresults {
                rft, "OP_1", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 25.0f; // cm

            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 1), 1*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 2), 2*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 3), 3*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 1), 1.184307920059215e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 2), 1.283000246730817e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 3), 1.381692573402418e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 4), 1.480384900074019e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 5), 1.579077226745621e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 6), 1.677769553417222e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 7), 1.776461880088823e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 8), 1.875154206760424e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 9), 1.973846533432026e+02f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 1), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 2), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 3), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 4), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 5), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 6), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 7), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 8), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 9), 0.15f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 1), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 2), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 3), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 5), 0.50f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 6), 0.55f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 7), 0.60f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 8), 0.65f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 9), 0.70f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 25.0f; // cm

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 1.480384900074019e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 1.579077226745621e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 1.677769553417222e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 1.776461880088823e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 1.875154206760424e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 1.973846533432026e+02f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_1", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_1", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "   HR");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_1");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], "   CM");
            BOOST_CHECK_EQUAL(welletc[ 4], "  ATMA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SCC/HR");
            BOOST_CHECK_EQUAL(welletc[ 8], " SCC/HR");
            BOOST_CHECK_EQUAL(welletc[ 9], " RCC/HR");
            BOOST_CHECK_EQUAL(welletc[10], " CM/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " GM/SCC");
            BOOST_CHECK_EQUAL(welletc[14], " GM/HR");
            BOOST_CHECK_EQUAL(welletc[15], "  GM/GM");
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "   HR");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], "   CM");
            BOOST_CHECK_EQUAL(welletc[ 4], "  ATMA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SCC/HR");
            BOOST_CHECK_EQUAL(welletc[ 8], " SCC/HR");
            BOOST_CHECK_EQUAL(welletc[ 9], " RCC/HR");
            BOOST_CHECK_EQUAL(welletc[10], " CM/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " GM/SCC");
            BOOST_CHECK_EQUAL(welletc[14], " GM/HR");
            BOOST_CHECK_EQUAL(welletc[15], "  GM/GM");
        }
    }

    {
        auto rftFile = ::Opm::EclIO::OutputStream::RFT {
            rset, ::Opm::EclIO::OutputStream::Formatted  { false },
            ::Opm::EclIO::OutputStream::RFT::OpenExisting{ true }
        };

        const auto  reportStep = 3;
        const auto  elapsed    = model.sched.seconds(reportStep);
        const auto& grid       = model.es.getInputGrid();

        ::Opm::RftIO::write(reportStep, elapsed, usys, grid,
                            model.sched, wellSol(grid), rftFile);
    }

    {
        const auto rft = ::Opm::EclIO::ERft {
            ::Opm::EclIO::OutputStream::outputFileName(rset, "RFT")
        };

        {
            const auto xRFT = RFTRresults {
                rft, "OP_1", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 25.0f; // cm

            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 1), 1*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 2), 2*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 3), 3*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 1), 1.184307920059215e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 2), 1.283000246730817e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 3), 1.381692573402418e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 4), 1.480384900074019e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 5), 1.579077226745621e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 6), 1.677769553417222e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 7), 1.776461880088823e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 8), 1.875154206760424e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 9), 1.973846533432026e+02f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 1), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 2), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 3), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 4), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 5), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 6), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 7), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 8), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 9), 0.15f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 1), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 2), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 3), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 5), 0.50f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 6), 0.55f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 7), 0.60f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 8), 0.65f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 9), 0.70f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 25.0f; // cm

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 1.480384900074019e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 1.579077226745621e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 1.677769553417222e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 1.776461880088823e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 1.875154206760424e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 1.973846533432026e+02f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 11, 10 }
            };

            const auto thick = 25.0f; // cm

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 1.480384900074019e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 1.579077226745621e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 1.677769553417222e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 1.776461880088823e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 1.875154206760424e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 1.973846533432026e+02f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_1", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_1", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "   HR");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_1");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], "   CM");
            BOOST_CHECK_EQUAL(welletc[ 4], "  ATMA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SCC/HR");
            BOOST_CHECK_EQUAL(welletc[ 8], " SCC/HR");
            BOOST_CHECK_EQUAL(welletc[ 9], " RCC/HR");
            BOOST_CHECK_EQUAL(welletc[10], " CM/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " GM/SCC");
            BOOST_CHECK_EQUAL(welletc[14], " GM/HR");
            BOOST_CHECK_EQUAL(welletc[15], "  GM/GM");
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "   HR");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], "   CM");
            BOOST_CHECK_EQUAL(welletc[ 4], "  ATMA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SCC/HR");
            BOOST_CHECK_EQUAL(welletc[ 8], " SCC/HR");
            BOOST_CHECK_EQUAL(welletc[ 9], " RCC/HR");
            BOOST_CHECK_EQUAL(welletc[10], " CM/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " GM/SCC");
            BOOST_CHECK_EQUAL(welletc[14], " GM/HR");
            BOOST_CHECK_EQUAL(welletc[15], "  GM/GM");
        }

        {
            const auto date = RftDate{2008, 11, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "   HR");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], "   CM");
            BOOST_CHECK_EQUAL(welletc[ 4], "  ATMA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SCC/HR");
            BOOST_CHECK_EQUAL(welletc[ 8], " SCC/HR");
            BOOST_CHECK_EQUAL(welletc[ 9], " RCC/HR");
            BOOST_CHECK_EQUAL(welletc[10], " CM/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " GM/SCC");
            BOOST_CHECK_EQUAL(welletc[14], " GM/HR");
            BOOST_CHECK_EQUAL(welletc[15], "  GM/GM");
        }
    }
}

BOOST_AUTO_TEST_CASE(PVT_M_Units)
{
    using RftDate = ::Opm::EclIO::ERft::RftDate;

    const auto rset  = RSet { "TESTRFT" };
    const auto model = Setup{ "testrft.DATA" };
    const auto usys  = ::Opm::UnitSystem::newPVT_M();

    {
        auto rftFile = ::Opm::EclIO::OutputStream::RFT {
            rset, ::Opm::EclIO::OutputStream::Formatted  { false },
            ::Opm::EclIO::OutputStream::RFT::OpenExisting{ false }
        };

        const auto  reportStep = 2;
        const auto  elapsed    = model.sched.seconds(reportStep);
        const auto& grid       = model.es.getInputGrid();

        ::Opm::RftIO::write(reportStep, elapsed, usys, grid,
                            model.sched, wellSol(grid), rftFile);
    }

    {
        const auto rft = ::Opm::EclIO::ERft {
            ::Opm::EclIO::OutputStream::outputFileName(rset, "RFT")
        };

        {
            const auto xRFT = RFTRresults {
                rft, "OP_1", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 0.25f;

            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 1), 1*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 2), 2*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 3), 3*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 1), 1.184307920059215e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 2), 1.283000246730817e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 3), 1.381692573402418e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 4), 1.480384900074019e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 5), 1.579077226745621e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 6), 1.677769553417222e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 7), 1.776461880088823e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 8), 1.875154206760424e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 9), 1.973846533432026e+02f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 1), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 2), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 3), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 4), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 5), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 6), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 7), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 8), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 9), 0.15f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 1), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 2), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 3), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 5), 0.50f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 6), 0.55f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 7), 0.60f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 8), 0.65f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 9), 0.70f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 0.25f;

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 1.480384900074019e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 1.579077226745621e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 1.677769553417222e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 1.776461880088823e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 1.875154206760424e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 1.973846533432026e+02f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_1", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_1", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_1");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  ATMA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  ATMA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }
    }

    {
        auto rftFile = ::Opm::EclIO::OutputStream::RFT {
            rset, ::Opm::EclIO::OutputStream::Formatted  { false },
            ::Opm::EclIO::OutputStream::RFT::OpenExisting{ true }
        };

        const auto  reportStep = 3;
        const auto  elapsed    = model.sched.seconds(reportStep);
        const auto& grid       = model.es.getInputGrid();

        ::Opm::RftIO::write(reportStep, elapsed, usys, grid,
                            model.sched, wellSol(grid), rftFile);
    }

    {
        const auto rft = ::Opm::EclIO::ERft {
            ::Opm::EclIO::OutputStream::outputFileName(rset, "RFT")
        };

        {
            const auto xRFT = RFTRresults {
                rft, "OP_1", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 0.25;

            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 1), 1*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 2), 2*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 3), 3*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(9, 9, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 1), 1.184307920059215e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 2), 1.283000246730817e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 3), 1.381692573402418e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 4), 1.480384900074019e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 5), 1.579077226745621e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 6), 1.677769553417222e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 7), 1.776461880088823e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 8), 1.875154206760424e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(9, 9, 9), 1.973846533432026e+02f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 1), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 2), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 3), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 4), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 5), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 6), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 7), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 8), 0.15f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(9, 9, 9), 0.15f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 1), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 2), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 3), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 5), 0.50f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 6), 0.55f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 7), 0.60f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 8), 0.65f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(9, 9, 9), 0.70f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 10, 10 }
            };

            const auto thick = 0.25;

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 1.480384900074019e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 1.579077226745621e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 1.677769553417222e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 1.776461880088823e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 1.875154206760424e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 1.973846533432026e+02f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto xRFT = RFTRresults {
                rft, "OP_2", RftDate{ 2008, 11, 10 }
            };

            const auto thick = 0.25;

            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 4), 4*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 5), 5*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 6), 6*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 7), 7*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 8), 8*thick + thick/2.0f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.depth(4, 4, 9), 9*thick + thick/2.0f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 4), 1.480384900074019e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 5), 1.579077226745621e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 6), 1.677769553417222e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 7), 1.776461880088823e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 8), 1.875154206760424e+02f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.pressure(4, 4, 9), 1.973846533432026e+02f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 4), 0.45f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 5), 0.40f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 6), 0.35f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 7), 0.30f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.sgas(4, 4, 9), 0.20f, 1.0e-10f);

            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 4), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 5), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 6), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 7), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 8), 0.25f, 1.0e-10f);
            BOOST_CHECK_CLOSE(xRFT.swat(4, 4, 9), 0.25f, 1.0e-10f);
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_1", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_1", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_1");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  ATMA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }

        {
            const auto date = RftDate{2008, 10, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  ATMA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }

        {
            const auto date = RftDate{2008, 11, 10};

            BOOST_CHECK(rft.hasArray("WELLETC", "OP_2", date));

            const auto& welletc = rft.getRft<std::string>("WELLETC", "OP_2", date);

            BOOST_CHECK_EQUAL(welletc[ 0], "  DAYS");
            BOOST_CHECK_EQUAL(welletc[ 1], "OP_2");
            BOOST_CHECK_EQUAL(welletc[ 2], "");
            BOOST_CHECK_EQUAL(welletc[ 3], " METRES");
            BOOST_CHECK_EQUAL(welletc[ 4], "  ATMA");
            BOOST_CHECK_EQUAL(welletc[ 5], "R");
            BOOST_CHECK_EQUAL(welletc[ 6], "STANDARD");
            BOOST_CHECK_EQUAL(welletc[ 7], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 8], " SM3/DAY");
            BOOST_CHECK_EQUAL(welletc[ 9], " RM3/DAY");
            BOOST_CHECK_EQUAL(welletc[10], " M/SEC");
            // No check for welletc[11]
            BOOST_CHECK_EQUAL(welletc[12], "   CP");
            BOOST_CHECK_EQUAL(welletc[13], " KG/SM3");
            BOOST_CHECK_EQUAL(welletc[14], " KG/DAY");
            BOOST_CHECK_EQUAL(welletc[15], "  KG/KG");
        }
    }
}

BOOST_AUTO_TEST_SUITE_END() // Using_Direct_Write

// =====================================================================

BOOST_AUTO_TEST_SUITE(PLTData)

namespace {
    Opm::Deck pltDataSet()
    {
        return ::Opm::Parser{}.parseString(R"(RUNSPEC
TITLE
  'BASE1' 'MSW' 'HFA'

NOECHO

DIMENS
 6 8 7 /

START
 1 'JAN' 2000 /

OIL
WATER
GAS
DISGAS
VAPOIL
METRIC

TABDIMS
 1 1 5 20 1* 20 /

EQLDIMS
 1 /

REGDIMS
 1 1 /

WELLDIMS
 2 7 2 2 /

WSEGDIMS
 1 12 1 /

UNIFIN
UNIFOUT

-- =====================================================================

GRID

GRIDFILE
 0 1 /

INIT
NEWTRAN

GRIDUNIT
 'METRES' /

SPECGRID
 6 8 7 1 'F' /

DXV
 6*100 /

DYV
 8*100 /

DZV
 7*10 /

DEPTHZ
 63*2700 /

PERMX
 48*72 48*135 48*355 48*50 48*200 48*130 48*55 /

PORO
 48*0.25 48*0.2 48*0.2 48*0.2 48*0.2 48*0.18 48*0.18 /

COPY
 'PERMX' 'PERMY' /
 'PERMX' 'PERMZ' /
/

MULTIPLY
 'PERMZ' 0.1 /
/

MULTZ
 48*1 48*1 48*1
 48*0
 48*1 48*1 48*1 /

MULTNUM
 48*1 48*1
 48*2 48*2 48*2
 48*3 48*3 /

-- =====================================================================

PROPS

SWOF
 0 0 1 0
 1 1 0 0 /

SGOF
 0 0 1 0
 1 1 0 0 /

ROCK
 280 5.6e-05 /

PVTW
 247.7 1.03665 4.1726e-05 0.2912 9.9835e-05 /

DENSITY
 861 999.1 1.01735 /

PVTO
 0   1   1.07033 0.645
    25   1.06657 0.668
    50   1.06293 0.691
    75   1.05954 0.714
   100   1.05636 0.736 /

 17.345  25   1.14075 0.484
         50   1.1351  0.506
         75   1.12989 0.527
        100   1.12508 0.548 /

 31.462  50   1.1843  0.439
         75   1.178   0.459
        100   1.17219 0.479 /

 45.089  75   1.22415 0.402
        100   1.21728 0.421
        150   1.2051  0.458
        200   1.19461 0.494 /

 58.99 100   1.26373 0.37
       150   1.24949 0.405
       200   1.23732 0.439
       225   1.23186 0.456 /

 88.618 150   1.34603 0.316
        200   1.32975 0.346
        225   1.32253 0.361
        250   1.31582 0.376 /

 120.85 200   1.43292 0.273
        225   1.42343 0.286
        250   1.41467 0.299
        275   1.40656 0.312 /

 138.134 225   1.47867 0.255
         250   1.46868 0.267
         275   1.45945 0.279
         294.6 1.45269 0.288 /

 156.324 250   1.52632 0.239
         275   1.51583 0.25
         294.6 1.50816 0.258
         300   1.50613 0.261 /

 175.509 275   1.5761  0.224
         294.6 1.56741 0.232
         300   1.5651  0.234
         324   1.55533 0.244 /

 191.323 294.6 1.61682 0.214
         300   1.61428 0.216
         324   1.60352 0.225
         350   1.59271 0.235 /

 195.818 300 1.62835 0.211
         324 1.6173  0.22
         350 1.60621 0.23
         400 1.58707 0.248 /

 216.43 324 1.68095 0.199
        350 1.66851 0.208
        400 1.64713 0.226
        450 1.62847 0.243
        500 1.612   0.26 /
 /

PVTG
   1   2.123e-06    1.877001 0.01037
       0            1.352546 0.011247 /
  25   5.99e-06     0.050493 0.012925
       0            0.050477 0.012932 /
  50   4.9422e-06   0.024609 0.01373
       0            0.024612 0.013734 /
  75   6.1628e-06   0.016094 0.014475
       0            0.016102 0.014475 /
 100   8.6829e-06   0.011902 0.015347
       0            0.011915 0.015334 /
 150   1.91019e-05  0.007838 0.017699
       0            0.00786  0.017591 /
 200   4.14858e-05  0.005938 0.020947
       0            0.005967 0.020506 /
 225   5.95434e-05  0.005349 0.022888
       0            0.005377 0.022116 /
 250   8.3633e-05   0.004903 0.025025
       0            0.004925 0.023767 /
 275   0.0001148977 0.004561 0.027355
       0            0.004571 0.025418 /
 294.6 0.0001452455 0.00435  0.029325
       0            0.004344 0.026696 /
 300   0.0001546223 0.004299 0.029893
       0            0.004288 0.027044 /
 324   0.000202062  0.004107 0.032559
       0.0001546223 0.004098 0.031456
       0.0001452455 0.004097 0.031237
       0.0001148977 0.004093 0.030521
       8.3633e-05   0.004089 0.029767
       5.95434e-05  0.004088 0.029165
       4.14858e-05  0.004087 0.028702
       1.91019e-05  0.004085 0.028173
       8.6829e-06   0.004068 0.028353
       0            0.004066 0.028567 /
 /

-- =====================================================================

REGIONS

SATNUM
 48*1 48*1 48*1 48*1 48*1 48*1 48*1 /

EQLNUM
 48*1 48*1 48*1 48*1 48*1 48*1 48*1 /

PVTNUM
 48*1 48*1 48*1 48*1 48*1 48*1 48*1 /

-- =====================================================================

SOLUTION

EQUIL
 2730 300 2750 0 1650 0 1 1 0 /

RSVD
 2650 156.324
 2750 138.134 /

RVVD
 2600 0.00739697
 2750 0.00639697 /

RPTSOL
 'THPRES' 'FIP=2' /

RPTRST
 'BASIC=5' FREQ=6 /

-- =====================================================================

SUMMARY

ALL

-- =====================================================================

SCHEDULE

GRUPTREE
 'TEST' 'FIELD' /
/

WELSPECS
 'P1' 'TEST' 1 2 1* 'OIL' 0 'STD' 'STOP' 'YES' 0 'SEG' 0 /
 'I1' 'TEST' 6 8 1* 'WATER' /
/

COMPDAT
 'P1' 2 3 2 2 'OPEN' 1* 52.08337 0.216 1* 0 1* 'Z' /
 'P1' 2 3 3 3 'OPEN' 1* 366.2544 0.216 1* 0 1* 'Y' /
 'P1' 2 4 3 3 'OPEN' 1* 388.4829 0.216 1* 0 1* 'Y' /
 'P1' 3 4 3 3 'OPEN' 1* 203.6268 0.216 1* 0 1* 'Y' /
 'P1' 3 5 3 3 'OPEN' 1* 571.7222 0.216 1* 0 1* 'Y' /
 'P1' 3 6 3 3 'OPEN' 1* 389.4535 0.216 1* 0 1* 'Y' /
 'I1' 6 8 5 7 'OPEN' 1* 1*       0.216 1* 0 1* 'Z' /
/

WELSEGS
 'P1' 2620.17107 0 1* 'INC' 'HFA' /
  2  2 1  1  38.17432  3.32249 0.102 1e-05 /
  3  3 1  2  62.22322  5.41558 0.102 1e-05 /
  4  4 1  3  54.33161  4.72874 0.102 1e-05 /
  5  5 1  4 119.18735 10.34614 0.102 1e-05 /
  6  6 1  5 263.64361 14.87775 0.102 1e-05 /
  7  7 1  6 360.47928 11.28317 0.102 1e-05 /
  8  8 1  7 282.92022  5.30723 0.102 1e-05 /
  9  9 1  8 370.26595  5.85843 0.102 1e-05 /
 10 10 1  9 458.85844  9.23286 0.102 1e-05 /
 11 11 1 10 266.98559  6.56172 0.102 1e-05 /
/

COMPSEGS
 'P1' /
 2 3 2 1  233.61     362.82114 /
 2 3 3 1  362.82114  712.29909 /
 2 4 3 1  712.29909 1083.7797  /
 3 4 3 1 1083.7797  1278.13953 /
 3 5 3 1 1278.13953 1824.3116  /
 3 6 3 1 1824.3116  2195.85641 /
/

WCONPROD
 'P1' 'OPEN' 'ORAT' 8000 4* 65 /
/

WCONINJE
 'I1' 'WATER' 'OPEN' 'RATE' 5000 1* 450 /
/

TSTEP
 1 /

WRFTPLT
 'P1' 'YES' 'YES' 'NO' /
 'I1' 'YES' 'YES' 'NO' /
/

TSTEP
 2 3 5 10*10 20*20 30*30 /

END

)");
    }

    std::vector<int> cellIndex(const ::Opm::EclipseGrid&             grid,
                               const std::vector<std::array<int,3>>& ijk)
    {
        auto cellIx = std::vector<int>{};
        cellIx.reserve(ijk.size());

        std::transform(ijk.begin(), ijk.end(), std::back_inserter(cellIx),
                       [&grid](const auto& ijk_entry)
                       {
                           return grid.getGlobalIndex(ijk_entry[0] - 1,
                                                      ijk_entry[1] - 1,
                                                      ijk_entry[2] - 1);
                       });

        return cellIx;
    }

    std::vector<int> cellIndex_P1(const ::Opm::EclipseGrid& grid)
    {
        return cellIndex(grid, {
                {2, 3, 2},
                {2, 3, 3},
                {2, 4, 3},
                {3, 4, 3},
                {3, 5, 3},
                {3, 6, 3},
            });
    }

    std::vector<int> cellIndex_I1(const ::Opm::EclipseGrid& grid)
    {
        return cellIndex(grid, {
                {6, 8, 5},
                {6, 8, 6},
                {6, 8, 7},
            });
    }

    std::vector<Opm::data::Connection>
    connRes_P1(const ::Opm::EclipseGrid& grid)
    {
        using rt = ::Opm::data::Rates::opt;

        const auto cellIx = cellIndex_P1(grid);

        const auto ncon = static_cast<int>(cellIx.size());

        auto xcon = std::vector<Opm::data::Connection>{};
        xcon.reserve(ncon);

        const auto m3_d = ::Opm::UnitSystem::newMETRIC()
            .to_si(::Opm::UnitSystem::measure::liquid_surface_rate, 1.0);

        const auto m3cp_db = ::Opm::UnitSystem::newMETRIC()
            .to_si(::Opm::UnitSystem::measure::transmissibility, 1.0);

        for (auto con = 0; con < ncon; ++con) {
            auto& c = xcon.emplace_back();

            c.index = cellIx[con];

            c.cell_pressure = (120 + con*10)*::Opm::unit::barsa;
            c.pressure = (120 - (ncon - con)*10)*::Opm::unit::barsa;

            // Negative rates for producing connections
            c.rates.set(rt::oil, -  100*con*m3_d)
                   .set(rt::gas, - 1000*con*m3_d)
                   .set(rt::wat, -   10*con*m3_d);

            c.cell_saturation_gas   = 0.15;
            c.cell_saturation_water = 0.3 + con/static_cast<double>(2 * ncon);
            c.trans_factor          = 0.98765*m3cp_db;
        }

        return xcon;
    }

    ::Opm::data::Well wellSol_P1(const ::Opm::EclipseGrid& grid)
    {
        auto xw = ::Opm::data::Well{};
        xw.connections = connRes_P1(grid);

        return xw;
    }

    std::vector<Opm::data::Connection>
    connRes_I1(const ::Opm::EclipseGrid& grid)
    {
        using rt = ::Opm::data::Rates::opt;

        const auto cellIx = cellIndex_I1(grid);

        const auto ncon = static_cast<int>(cellIx.size());

        auto xcon = std::vector<Opm::data::Connection>{};
        xcon.reserve(ncon);

        const auto m3_d = ::Opm::UnitSystem::newMETRIC()
            .to_si(::Opm::UnitSystem::measure::liquid_surface_rate, 1.0);

        const auto m3cp_db = ::Opm::UnitSystem::newMETRIC()
            .to_si(::Opm::UnitSystem::measure::transmissibility, 1.0);

        for (auto con = 0; con < ncon; ++con) {
            auto& c = xcon.emplace_back();

            c.index = cellIx[con];

            c.cell_pressure = (120 + con*10)*::Opm::unit::barsa;
            c.pressure = (120 + (3 + con)*10)*::Opm::unit::barsa;

            // Positive rates for injecting connections
            c.rates.set(rt::wat, 123.4*con*m3_d);

            c.cell_saturation_gas   = 0.6 - (con + 3)/static_cast<double>(2 * ncon);
            c.cell_saturation_water = 0.25;
            c.trans_factor          = 0.12345*m3cp_db;
        }

        return xcon;
    }

    ::Opm::data::Well wellSol_I1(const ::Opm::EclipseGrid& grid)
    {
        auto xw = ::Opm::data::Well{};
        xw.connections = connRes_I1(grid);

        return xw;
    }

    ::Opm::data::Wells wellSol(const ::Opm::EclipseGrid& grid)
    {
        auto xw = ::Opm::data::Wells{};

        xw["P1"] = wellSol_P1(grid);
        xw["I1"] = wellSol_I1(grid);

        return xw;
    }
}

BOOST_AUTO_TEST_CASE(Standard_Well)
{
    using RftDate = ::Opm::EclIO::ERft::RftDate;

    const auto rset  = RSet { "TESTPLT" };
    const auto model = Setup{ pltDataSet() };

    {
        auto rftFile = ::Opm::EclIO::OutputStream::RFT {
            rset, ::Opm::EclIO::OutputStream::Formatted  { false },
            ::Opm::EclIO::OutputStream::RFT::OpenExisting{ false }
        };

        const auto  reportStep = 1;
        const auto  elapsed    = model.sched.seconds(reportStep);
        const auto& grid       = model.es.getInputGrid();

        ::Opm::RftIO::write(reportStep, elapsed, model.es.getUnits(),
                            grid, model.sched, wellSol(grid), rftFile);
    }

    const auto rft = ::Opm::EclIO::ERft {
        ::Opm::EclIO::OutputStream::outputFileName(rset, "RFT")
    };

    const auto xPLT = PLTResults {
        rft, "I1", RftDate{ 2000, 1, 2 }
    };

    BOOST_CHECK_EQUAL(xPLT.next(6, 8, 5), 0);
    BOOST_CHECK_EQUAL(xPLT.next(6, 8, 6), 1);
    BOOST_CHECK_EQUAL(xPLT.next(6, 8, 7), 2);

    BOOST_CHECK_CLOSE(xPLT.depth(6, 8, 5), 2745.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.depth(6, 8, 6), 2755.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.depth(6, 8, 7), 2765.0f, 1.0e-5f);

    BOOST_CHECK_CLOSE(xPLT.pressure(6, 8, 5), 150.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.pressure(6, 8, 6), 160.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.pressure(6, 8, 7), 170.0f, 1.0e-5f);

    BOOST_CHECK_CLOSE(xPLT.orat(6, 8, 5), 0.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.orat(6, 8, 6), 0.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.orat(6, 8, 7), 0.0f, 1.0e-5f);

    BOOST_CHECK_CLOSE(xPLT.wrat(6, 8, 5), 0.0f * (- 123.4f), 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.wrat(6, 8, 6), 1.0f * (- 123.4f), 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.wrat(6, 8, 7), 2.0f * (- 123.4f), 1.0e-5f);

    BOOST_CHECK_CLOSE(xPLT.grat(6, 8, 5), 0.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.grat(6, 8, 6), 0.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.grat(6, 8, 7), 0.0f, 1.0e-5f);

    BOOST_CHECK_CLOSE(xPLT.conntrans(6, 8, 5), 0.12345f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.conntrans(6, 8, 6), 0.12345f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.conntrans(6, 8, 7), 0.12345f, 1.0e-5f);

    BOOST_CHECK_CLOSE(xPLT.kh(6, 8, 5), 2000.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.kh(6, 8, 6), 1300.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.kh(6, 8, 7),  550.0f, 1.0e-5f);
}

BOOST_AUTO_TEST_CASE(Multisegment_Well)
{
    using RftDate = ::Opm::EclIO::ERft::RftDate;

    const auto rset  = RSet { "TESTPLT" };
    const auto model = Setup{ pltDataSet() };

    {
        auto rftFile = ::Opm::EclIO::OutputStream::RFT {
            rset, ::Opm::EclIO::OutputStream::Formatted  { false },
            ::Opm::EclIO::OutputStream::RFT::OpenExisting{ false }
        };

        const auto  reportStep = 1;
        const auto  elapsed    = model.sched.seconds(reportStep);
        const auto& grid       = model.es.getInputGrid();

        ::Opm::RftIO::write(reportStep, elapsed, model.es.getUnits(),
                            grid, model.sched, wellSol(grid), rftFile);
    }

    const auto rft = ::Opm::EclIO::ERft {
        ::Opm::EclIO::OutputStream::outputFileName(rset, "RFT")
    };

    const auto xPLT = PLTResultsMSW {
        rft, "P1", RftDate{ 2000, 1, 2 }
    };

    BOOST_CHECK_EQUAL(xPLT.next(2, 3, 2), 0);
    BOOST_CHECK_EQUAL(xPLT.next(2, 3, 3), 1);
    BOOST_CHECK_EQUAL(xPLT.next(2, 4, 3), 2);
    BOOST_CHECK_EQUAL(xPLT.next(3, 4, 3), 3);
    BOOST_CHECK_EQUAL(xPLT.next(3, 5, 3), 4);
    BOOST_CHECK_EQUAL(xPLT.next(3, 6, 3), 5);

    BOOST_CHECK_CLOSE(xPLT.depth(2, 3, 2), 2645.3552f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.depth(2, 3, 3), 2658.8618f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.depth(2, 4, 3), 2670.1450f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.depth(3, 4, 3), 2675.4521f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.depth(3, 5, 3), 2681.3105f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.depth(3, 6, 3), 2690.5435f, 1.0e-5f);

    BOOST_CHECK_CLOSE(xPLT.pressure(2, 3, 2),  60.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.pressure(2, 3, 3),  70.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.pressure(2, 4, 3),  80.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.pressure(3, 4, 3),  90.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.pressure(3, 5, 3), 100.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.pressure(3, 6, 3), 110.0f, 1.0e-5f);

    BOOST_CHECK_CLOSE(xPLT.orat(2, 3, 2), 0.0f * 100.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.orat(2, 3, 3), 1.0f * 100.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.orat(2, 4, 3), 2.0f * 100.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.orat(3, 4, 3), 3.0f * 100.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.orat(3, 5, 3), 4.0f * 100.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.orat(3, 6, 3), 5.0f * 100.0f, 1.0e-5f);

    BOOST_CHECK_CLOSE(xPLT.wrat(2, 3, 2), 0.0f * 10.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.wrat(2, 3, 3), 1.0f * 10.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.wrat(2, 4, 3), 2.0f * 10.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.wrat(3, 4, 3), 3.0f * 10.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.wrat(3, 5, 3), 4.0f * 10.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.wrat(3, 6, 3), 5.0f * 10.0f, 1.0e-5f);

    BOOST_CHECK_CLOSE(xPLT.grat(2, 3, 2), 0.0f * 1000.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.grat(2, 3, 3), 1.0f * 1000.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.grat(2, 4, 3), 2.0f * 1000.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.grat(3, 4, 3), 3.0f * 1000.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.grat(3, 5, 3), 4.0f * 1000.0f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.grat(3, 6, 3), 5.0f * 1000.0f, 1.0e-5f);

    BOOST_CHECK_CLOSE(xPLT.conntrans(2, 3, 2), 0.98765f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.conntrans(2, 3, 3), 0.98765f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.conntrans(2, 4, 3), 0.98765f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.conntrans(3, 4, 3), 0.98765f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.conntrans(3, 5, 3), 0.98765f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.conntrans(3, 6, 3), 0.98765f, 1.0e-5f);

    BOOST_CHECK_CLOSE(xPLT.kh(2, 3, 2), 5.0659907e3f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.kh(2, 3, 3), 2.8570773e4f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.kh(2, 4, 3), 3.0304773e4f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.kh(3, 4, 3), 1.5884520e4f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.kh(3, 5, 3), 4.4598906e4f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.kh(3, 6, 3), 3.0380488e4f, 1.0e-5f);

    BOOST_CHECK_EQUAL(xPLT.segment(2, 3, 2),  5);
    BOOST_CHECK_EQUAL(xPLT.segment(2, 3, 3),  6);
    BOOST_CHECK_EQUAL(xPLT.segment(2, 4, 3),  7);
    BOOST_CHECK_EQUAL(xPLT.segment(3, 4, 3),  8);
    BOOST_CHECK_EQUAL(xPLT.segment(3, 5, 3),  9);
    BOOST_CHECK_EQUAL(xPLT.segment(3, 6, 3), 10);

    BOOST_CHECK_EQUAL(xPLT.branch(2, 3, 2), 1);
    BOOST_CHECK_EQUAL(xPLT.branch(2, 3, 3), 1);
    BOOST_CHECK_EQUAL(xPLT.branch(2, 4, 3), 1);
    BOOST_CHECK_EQUAL(xPLT.branch(3, 4, 3), 1);
    BOOST_CHECK_EQUAL(xPLT.branch(3, 5, 3), 1);
    BOOST_CHECK_EQUAL(xPLT.branch(3, 6, 3), 1);

    BOOST_CHECK_CLOSE(xPLT.start(2, 3, 2),  233.61f   , 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.start(2, 3, 3),  362.82114f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.start(2, 4, 3),  712.29909f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.start(3, 4, 3), 1083.7797f , 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.start(3, 5, 3), 1278.13953f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.start(3, 6, 3), 1824.3116f , 1.0e-5f);

    BOOST_CHECK_CLOSE(xPLT.end(2, 3, 2),  362.82114f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.end(2, 3, 3),  712.29909f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.end(2, 4, 3), 1083.7797f , 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.end(3, 4, 3), 1278.13953f, 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.end(3, 5, 3), 1824.3116f , 1.0e-5f);
    BOOST_CHECK_CLOSE(xPLT.end(3, 6, 3), 2195.85641f, 1.0e-5f);
}

BOOST_AUTO_TEST_SUITE_END() // PLTData
