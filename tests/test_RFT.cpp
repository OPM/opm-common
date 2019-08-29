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

#include <opm/output/data/Solution.hpp>
#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/EclipseIO.hpp>
#include <opm/output/eclipse/InteHEAD.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

#include <cstddef>
#include <ctime>
#include <map>
#include <iomanip>
#include <ostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

using namespace Opm;

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
    class RSet
    {
    public:
        explicit RSet(std::string base)
            : odir_(boost::filesystem::temp_directory_path() /
                    boost::filesystem::unique_path("rset-%%%%"))
            , base_(std::move(base))
        {
            boost::filesystem::create_directories(this->odir_);
        }

        ~RSet()
        {
            boost::filesystem::remove_all(this->odir_);
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
        boost::filesystem::path odir_;
        std::string             base_;
    };

    class RFTRresults
    {
    public:
        explicit RFTRresults(const ::Opm::EclIO::ERft&          rft,
                             const std::string&                 well,
                             const ::Opm::EclIO::ERft::RftDate& date);

        float depth(const int i, const int j, const int k) const
        {
            return this->depth_[this->conIx(i, j, k)];
        }

        float pressure(const int i, const int j, const int k) const
        {
            return this->press_[this->conIx(i, j, k)];
        }

        float sgas(const int i, const int j, const int k) const
        {
            return this->sgas_[this->conIx(i, j, k)];
        }

        float swat(const int i, const int j, const int k) const
        {
            return this->swat_[this->conIx(i, j, k)];
        }

    private:
        std::vector<float> depth_;
        std::vector<float> press_;
        std::vector<float> sgas_;
        std::vector<float> swat_;

        std::map<std::tuple<int, int, int>, std::size_t> xConIx_;

        std::size_t conIx(const int i, const int j, const int k) const;
    };

    RFTRresults::RFTRresults(const ::Opm::EclIO::ERft&          rft,
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
            this->xConIx_[std::make_tuple(I[con], J[con], K[con])] = con;
        }

        BOOST_REQUIRE(rft.hasArray("DEPTH"   , well, date));
        BOOST_REQUIRE(rft.hasArray("PRESSURE", well, date));
        BOOST_REQUIRE(rft.hasArray("SGAS"    , well, date));
        BOOST_REQUIRE(rft.hasArray("SWAT"    , well, date));

        this->depth_ = rft.getRft<float>("DEPTH"   , well, date);
        this->press_ = rft.getRft<float>("PRESSURE", well, date);
        this->sgas_  = rft.getRft<float>("SGAS"    , well, date);
        this->swat_  = rft.getRft<float>("SWAT"    , well, date);
    }

    std::size_t RFTRresults::conIx(const int i, const int j, const int k) const
    {
        auto conIx = this->xConIx_.find(std::make_tuple(i, j, k));

        if (conIx == this->xConIx_.end()) {
            BOOST_TEST_FAIL("Invalid IJK Tuple (" << i << ", "
                            << j << ", " << k << ')');
        }

        return conIx->second;
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

    data::Solution createBlackoilState(int timeStepIdx, int numCells)
    {
        std::vector< double > pressure( numCells );
        std::vector< double > swat( numCells, 0 );
        std::vector< double > sgas( numCells, 0 );

        for (int i = 0; i < numCells; ++i) {
            pressure[i] = timeStepIdx*1e5 + 1e4 + i;
        }

        data::Solution sol;
        sol.insert( "PRESSURE", UnitSystem::measure::pressure, pressure , data::TargetType::RESTART_SOLUTION );
        sol.insert( "SWAT", UnitSystem::measure::identity, swat , data::TargetType::RESTART_SOLUTION );
        sol.insert( "SGAS", UnitSystem::measure::identity, sgas, data::TargetType::RESTART_SOLUTION );

        return sol;
    }

    std::time_t timeStamp(const ::Opm::EclIO::ERft::RftDate& date)
    {
        auto tp = std::tm{};

        tp.tm_year = std::get<0>(date) - 1900;
        tp.tm_mon  = std::get<1>(date) -    1; // 0..11
        tp.tm_mday = std::get<2>(date);        // 1..31

        return ::Opm::RestartIO::makeUTCTime(tp);
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(test_RFT)
{
    const auto rset = RSet{ "TESTRFT" };

    const auto eclipse_data_filename = std::string{ "testrft.DATA" };

    const auto deck = Parser{}.parseFile(eclipse_data_filename);
    auto eclipseState = EclipseState { deck };

    eclipseState.getIOConfig().setOutputDir(rset.outputDir());

    {
        /* eclipseWriter is scoped here to ensure it is destroyed after the
         * file itself has been written, because we're going to reload it
         * immediately. first upon destruction can we guarantee it being
         * written to disk and flushed.
         */

        const auto& grid = eclipseState.getInputGrid();
        const auto numCells = grid.getCartesianSize( );

        const Schedule schedule(deck, eclipseState);
        const SummaryConfig summary_config( deck, schedule, eclipseState.getTableManager( ));

        EclipseIO eclipseWriter( eclipseState, grid, schedule, summary_config );

        const auto start_time = schedule.posixStartTime();
        const auto step_time  = timeStamp(::Opm::EclIO::ERft::RftDate{ 2008, 10, 10 });

        SummaryState st;

        data::Rates r1, r2;
        r1.set( data::Rates::opt::wat, 4.11 );
        r1.set( data::Rates::opt::oil, 4.12 );
        r1.set( data::Rates::opt::gas, 4.13 );

        r2.set( data::Rates::opt::wat, 4.21 );
        r2.set( data::Rates::opt::oil, 4.22 );
        r2.set( data::Rates::opt::gas, 4.23 );

        std::vector<Opm::data::Connection> well1_comps(9);
        for (size_t i = 0; i < 9; ++i) {
            Opm::data::Connection well_comp { grid.getGlobalIndex(8,8,i) ,r1, 0.0 , 0.0, (double)i, 0.1*i,0.2*i, 1.2e3};
            well1_comps[i] = std::move(well_comp);
        }
        std::vector<Opm::data::Connection> well2_comps(6);
        for (size_t i = 0; i < 6; ++i) {
            Opm::data::Connection well_comp { grid.getGlobalIndex(3,3,i+3) ,r2, 0.0 , 0.0, (double)i, i*0.1,i*0.2, 0.15};
            well2_comps[i] = std::move(well_comp);
        }

        Opm::data::Solution solution = createBlackoilState(2, numCells);
        Opm::data::Wells wells;

        using SegRes = decltype(wells["w"].segments);

        wells["OP_1"] = { std::move(r1), 1.0, 1.1, 3.1, 1, std::move(well1_comps), SegRes{} };
        wells["OP_2"] = { std::move(r2), 1.0, 1.1, 3.2, 1, std::move(well2_comps), SegRes{} };

        RestartValue restart_value(std::move(solution), std::move(wells));

        eclipseWriter.writeTimeStep( st,
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
            dates[wellDate.first].push_back(wellDate.second);
        }

        // Well OP_1
        {
            auto op_1 = dates.find("OP_1");
            if (op_1 == dates.end()) {
                BOOST_TEST_FAIL("Missing RFT Data for Well OP_1");
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
                BOOST_TEST_FAIL("Missing RFT Data for Well OP_2");
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
    const auto rset = RSet{ "TESTRFT" };

    const auto eclipse_data_filename = std::string{ "testrft.DATA" };

    const auto deck = Parser().parseFile( eclipse_data_filename );
    auto eclipseState = EclipseState(deck);

    eclipseState.getIOConfig().setOutputDir(rset.outputDir());

    {
        /* eclipseWriter is scoped here to ensure it is destroyed after the
         * file itself has been written, because we're going to reload it
         * immediately. first upon destruction can we guarantee it being
         * written to disk and flushed.
         */

        const auto& grid = eclipseState.getInputGrid();
        const auto numCells = grid.getCartesianSize( );

        Schedule schedule(deck, eclipseState);
        SummaryConfig summary_config( deck, schedule, eclipseState.getTableManager( ));
        SummaryState st;

        const auto  start_time = schedule.posixStartTime();
        const auto& time_map   = schedule.getTimeMap( );

        for (int counter = 0; counter < 2; counter++) {
            EclipseIO eclipseWriter( eclipseState, grid, schedule, summary_config );
            for (size_t step = 0; step < time_map.size(); step++) {
                const auto step_time = time_map[step];

                data::Rates r1, r2;
                r1.set( data::Rates::opt::wat, 4.11 );
                r1.set( data::Rates::opt::oil, 4.12 );
                r1.set( data::Rates::opt::gas, 4.13 );

                r2.set( data::Rates::opt::wat, 4.21 );
                r2.set( data::Rates::opt::oil, 4.22 );
                r2.set( data::Rates::opt::gas, 4.23 );

                std::vector<Opm::data::Connection> well1_comps(9);
                for (size_t i = 0; i < 9; ++i) {
                    Opm::data::Connection well_comp { grid.getGlobalIndex(8,8,i) ,r1, 0.0 , 0.0, (double)i, 0.1*i,0.2*i, 3.14e5};
                    well1_comps[i] = std::move(well_comp);
                }
                std::vector<Opm::data::Connection> well2_comps(6);
                for (size_t i = 0; i < 6; ++i) {
                    Opm::data::Connection well_comp { grid.getGlobalIndex(3,3,i+3) ,r2, 0.0 , 0.0, (double)i, i*0.1,i*0.2, 355.113};
                    well2_comps[i] = std::move(well_comp);
                }

                Opm::data::Wells wells;
                Opm::data::Solution solution = createBlackoilState(2, numCells);

                using SegRes = decltype(wells["w"].segments);

                wells["OP_1"] = { std::move(r1), 1.0, 1.1, 3.1, 1, std::move(well1_comps), SegRes{} };
                wells["OP_2"] = { std::move(r2), 1.0, 1.1, 3.2, 1, std::move(well2_comps), SegRes{} };

                RestartValue restart_value(std::move(solution), std::move(wells));

                eclipseWriter.writeTimeStep( st,
                                             step,
                                             false,
                                             step_time - start_time,
                                             std::move(restart_value));
            }

            verifyRFTFile2(Opm::EclIO::OutputStream::outputFileName(rset, "RFT"));
        }
    }
}
