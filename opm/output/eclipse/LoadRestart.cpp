/*
  Copyright (c) 2018-2019 Equinor ASA
  Copyright (c) 2016 Statoil ASA
  Copyright (c) 2013-2015 Andreas Lauser
  Copyright (c) 2013 SINTEF ICT, Applied Mathematics.
  Copyright (c) 2013 Uni Research AS
  Copyright (c) 2015 IRIS AS

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

#include <opm/output/eclipse/RestartIO.hpp>

#include <opm/io/eclipse/ERst.hpp>
#include <opm/io/eclipse/RestartFileView.hpp>

#include <opm/output/data/Aquifer.hpp>
#include <opm/output/data/Cells.hpp>
#include <opm/output/data/Solution.hpp>
#include <opm/output/data/Wells.hpp>
#include <opm/output/data/Groups.hpp>

#include <opm/output/eclipse/VectorItems/aquifer.hpp>
#include <opm/output/eclipse/VectorItems/connection.hpp>
#include <opm/output/eclipse/VectorItems/doubhead.hpp>
#include <opm/output/eclipse/VectorItems/group.hpp>
#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/VectorItems/msw.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>

#include <opm/output/eclipse/RestartValue.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/TracerConfig.hpp>

#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/ScheduleTypes.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQEnums.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <functional>
#include <iterator>
#include <initializer_list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <boost/range.hpp>

namespace VI = ::Opm::RestartIO::Helpers::VectorItems;

namespace {
    template <typename T>
    boost::iterator_range<typename std::vector<T>::const_iterator>
    getDataWindow(const std::vector<T>& arr,
                  const std::size_t     windowSize,
                  const std::size_t     entity,
                  const std::size_t     subEntity               = 0,
                  const std::size_t     maxSubEntitiesPerEntity = 1)
    {
        const auto off =
            windowSize * (subEntity + maxSubEntitiesPerEntity*entity);

        auto begin = arr.begin() + off;
        auto end   = begin       + windowSize;

        return { begin, end };
    }
}

// ---------------------------------------------------------------------

class UDQVectors
{
public:
    explicit UDQVectors(std::shared_ptr<Opm::EclIO::RestartFileView> rst_view)
        : rstView_{ std::move(rst_view) }
    {
        const auto& intehead = this->rstView_->getKeyword<int>("INTEHEAD");

        this->maxNumMsWells_  = intehead[VI::intehead::NSWLMX];
        this->maxNumSegments_ = intehead[VI::intehead::NSEGMX];
        this->numGroups_      = intehead[VI::intehead::NGMAXZ];
        this->numWells_       = intehead[VI::intehead::NWMAXZ];
    }

    void prepareNextFieldUDQ()   { ++this->varIx_.field;   }
    void prepareNextGroupUDQ()   { ++this->varIx_.group;   }
    void prepareNextSegmentUDQ() { ++this->varIx_.segment; }
    void prepareNextWellUDQ()    { ++this->varIx_.well;    }

    const std::vector<std::string>& zudn() const
    {
        return this->rstView_->getKeyword<std::string>("ZUDN");
    }

    bool hasGroup()   const { return this->rstView_->hasKeyword<double>("DUDG"); }
    bool hasSegment() const { return this->rstView_->hasKeyword<double>("DUDS"); }
    bool hasWell()    const { return this->rstView_->hasKeyword<double>("DUDW"); }

    double currentFieldUDQValue() const
    {
        return this->rstView_->getKeyword<double>("DUDF")[ this->varIx_.field ];
    }

    auto currentGroupUDQValue() const
    {
        return getDataWindow(this->rstView_->getKeyword<double>("DUDG"),
                             this->numGroups_, this->varIx_.group);
    }

    auto currentSegmentUDQValue(const std::size_t msWellIx) const
    {
        return getDataWindow(this->rstView_->getKeyword<double>("DUDS"),
                             this->maxNumSegments_, this->varIx_.segment,
                             msWellIx, this->maxNumMsWells_);
    }

    auto currentWellUDQValue() const
    {
        return getDataWindow(this->rstView_->getKeyword<double>("DUDW"),
                             this->numWells_, this->varIx_.well);
    }

private:
    std::shared_ptr<Opm::EclIO::RestartFileView> rstView_;

    std::size_t maxNumMsWells_{0};
    std::size_t maxNumSegments_{0};
    std::size_t numGroups_{0};
    std::size_t numWells_{0};

    struct {
        std::size_t field{0};
        std::size_t group{0};
        std::size_t segment{0};
        std::size_t well{0};
    } varIx_;
};

// ---------------------------------------------------------------------

class WellVectors
{
public:
    template <typename T>
    using Window = boost::iterator_range<
        typename std::vector<T>::const_iterator
    >;

    explicit WellVectors(const std::vector<int>&                      intehead,
                         std::shared_ptr<Opm::EclIO::RestartFileView> rst_view);

    bool hasDefinedWellValues() const;
    bool hasDefinedConnectionValues() const;

    Window<int>    iwel(const std::size_t wellID) const;
    Window<double> xwel(const std::size_t wellID) const;

    Window<int>
    icon(const std::size_t wellID, const std::size_t connID) const;

    Window<double>
    xcon(const std::size_t wellID, const std::size_t connID) const;

private:
    std::size_t maxConnPerWell_;
    std::size_t numIWelElem_;
    std::size_t numXWelElem_;
    std::size_t numIConElem_;
    std::size_t numXConElem_;

    std::shared_ptr<Opm::EclIO::RestartFileView> rstView_;
};

WellVectors::WellVectors(const std::vector<int>&                      intehead,
                         std::shared_ptr<Opm::EclIO::RestartFileView> rst_view)
    : maxConnPerWell_(intehead[VI::intehead::NCWMAX])
    , numIWelElem_   (intehead[VI::intehead::NIWELZ])
    , numXWelElem_   (intehead[VI::intehead::NXWELZ])
    , numIConElem_   (intehead[VI::intehead::NICONZ])
    , numXConElem_   (intehead[VI::intehead::NXCONZ])
    , rstView_       (std::move(rst_view))
{}

bool WellVectors::hasDefinedWellValues() const
{
    return this->rstView_->hasKeyword<int>   ("IWEL")
        && this->rstView_->hasKeyword<double>("XWEL");
}

bool WellVectors::hasDefinedConnectionValues() const
{
    return this->rstView_->hasKeyword<int>   ("ICON")
        && this->rstView_->hasKeyword<double>("XCON");
}

WellVectors::Window<int>
WellVectors::iwel(const std::size_t wellID) const
{
    if (! this->hasDefinedWellValues()) {
        throw std::logic_error {
            "Cannot Request IWEL Values Unless Defined"
        };
    }

    return getDataWindow(this->rstView_->getKeyword<int>("IWEL"),
                         this->numIWelElem_, wellID);
}

WellVectors::Window<double>
WellVectors::xwel(const std::size_t wellID) const
{
    if (! this->hasDefinedWellValues()) {
        throw std::logic_error {
            "Cannot Request XWEL Values Unless Defined"
        };
    }

    return getDataWindow(this->rstView_->getKeyword<double>("XWEL"),
                         this->numXWelElem_, wellID);
}

WellVectors::Window<int>
WellVectors::icon(const std::size_t wellID, const std::size_t connID) const
{
    if (! this->hasDefinedConnectionValues()) {
        throw std::logic_error {
            "Cannot Request ICON Values Unless Defined"
        };
    }

    return getDataWindow(this->rstView_->getKeyword<int>("ICON"),
                         this->numIConElem_, wellID, connID,
                         this->maxConnPerWell_);
}

WellVectors::Window<double>
WellVectors::xcon(const std::size_t wellID, const std::size_t connID) const
{
    if (! this->hasDefinedConnectionValues()) {
        throw std::logic_error {
            "Cannot Request XCON Values Unless Defined"
        };
    }

    return getDataWindow(this->rstView_->getKeyword<double>("XCON"),
                         this->numXConElem_, wellID, connID,
                         this->maxConnPerWell_);
}

// ---------------------------------------------------------------------

class GroupVectors
{
public:
    template <typename T>
    using Window = boost::iterator_range<
        typename std::vector<T>::const_iterator
    >;

    explicit GroupVectors(const std::vector<int>&                      intehead,
                          std::shared_ptr<Opm::EclIO::RestartFileView> rst_view);

    bool hasDefinedValues() const;

    std::size_t maxGroups() const;

    Window<int>    igrp(const std::size_t groupID) const;
    Window<double> xgrp(const std::size_t groupID) const;

private:
    std::size_t maxNumGroups_;
    std::size_t numIGrpElem_;
    std::size_t numXGrpElem_;

    std::shared_ptr<Opm::EclIO::RestartFileView> rstView_;
};

GroupVectors::GroupVectors(const std::vector<int>&                      intehead,
                           std::shared_ptr<Opm::EclIO::RestartFileView> rst_view)
    : maxNumGroups_(intehead[VI::intehead::NGMAXZ] - 1) // -FIELD
    , numIGrpElem_ (intehead[VI::intehead::NIGRPZ])
    , numXGrpElem_ (intehead[VI::intehead::NXGRPZ])
    , rstView_     (std::move(rst_view))
{}

bool GroupVectors::hasDefinedValues() const
{
    return this->rstView_->hasKeyword<int>   ("IGRP")
        && this->rstView_->hasKeyword<double>("XGRP");
}

std::size_t GroupVectors::maxGroups() const
{
    return this->maxNumGroups_;
}

GroupVectors::Window<int>
GroupVectors::igrp(const std::size_t groupID) const
{
    if (! this->hasDefinedValues()) {
        throw std::logic_error {
            "Cannot Request IGRP Values Unless Defined"
        };
    }

    return getDataWindow(this->rstView_->getKeyword<int>("IGRP"),
                         this->numIGrpElem_, groupID);
}

GroupVectors::Window<double>
GroupVectors::xgrp(const std::size_t groupID) const
{
    if (! this->hasDefinedValues()) {
        throw std::logic_error {
            "Cannot Request XGRP Values Unless Defined"
        };
    }

    return getDataWindow(this->rstView_->getKeyword<double>("XGRP"),
                         this->numXGrpElem_, groupID);
}

// ---------------------------------------------------------------------

class SegmentVectors
{
public:
    template <typename T>
    using Window = boost::iterator_range<
        typename std::vector<T>::const_iterator
    >;

    explicit SegmentVectors(const std::vector<int>&                      intehead,
                            std::shared_ptr<Opm::EclIO::RestartFileView> rst_view);

    bool hasDefinedValues() const;

    Window<int>
    iseg(const std::size_t mswID, const std::size_t segID) const;

    Window<double>
    rseg(const std::size_t mswID, const std::size_t segID) const;

private:
    std::size_t maxSegPerWell_;
    std::size_t numISegElm_;
    std::size_t numRSegElm_;

    std::shared_ptr<Opm::EclIO::RestartFileView> rstView_;
};

SegmentVectors::SegmentVectors(const std::vector<int>&                      intehead,
                               std::shared_ptr<Opm::EclIO::RestartFileView> rst_view)
    : maxSegPerWell_(intehead[VI::intehead::NSEGMX])
    , numISegElm_   (intehead[VI::intehead::NISEGZ])
    , numRSegElm_   (intehead[VI::intehead::NRSEGZ])
    , rstView_      (std::move(rst_view))
{}

bool SegmentVectors::hasDefinedValues() const
{
    return this->rstView_->hasKeyword<int>   ("ISEG")
        && this->rstView_->hasKeyword<double>("RSEG");
}

SegmentVectors::Window<int>
SegmentVectors::iseg(const std::size_t mswID, const std::size_t segID) const
{
    if (! this->hasDefinedValues()) {
        throw std::logic_error {
            "Cannot Request ISEG Values Unless Defined"
        };
    }

    return getDataWindow(this->rstView_->getKeyword<int>("ISEG"),
                         this->numISegElm_, mswID, segID,
                         this->maxSegPerWell_);
}

SegmentVectors::Window<double>
SegmentVectors::rseg(const std::size_t mswID, const std::size_t segID) const
{
    if (! this->hasDefinedValues()) {
        throw std::logic_error {
            "Cannot Request RSEG Values Unless Defined"
        };
    }

    return getDataWindow(this->rstView_->getKeyword<double>("RSEG"),
                         this->numRSegElm_, mswID, segID,
                         this->maxSegPerWell_);
}

// ---------------------------------------------------------------------

class AquiferVectors
{
public:
    template <typename T>
    using Window = boost::iterator_range<
        typename std::vector<T>::const_iterator
    >;

    explicit AquiferVectors(const std::vector<int>&                      intehead,
                            std::shared_ptr<Opm::EclIO::RestartFileView> rst_view);

    bool hasDefinedValues() const;
    bool hasDefinedNumericAquiferValues() const;

    std::size_t maxAnalyticAquiferID() const;
    std::size_t numRecordsForNumericAquifers() const;

    Window<int>    iaaq(const std::size_t aquiferID) const;
    Window<float>  saaq(const std::size_t aquiferID) const;
    Window<double> xaaq(const std::size_t aquiferID) const;

    Window<int>    iaqn(const std::size_t recordID) const;
    Window<double> raqn(const std::size_t recordID) const;

private:
    std::size_t maxAnalyticAquiferID_;
    std::size_t numRecordsForNumericAquifers_;
    std::size_t numIntAnalyticAquiferElm_;
    std::size_t numIntNumericAquiferElm_;
    std::size_t numFloatAnalyticAquiferElm_;
    std::size_t numDoubleAnalyticAquiferElm_;
    std::size_t numDoubleNumericAquiferElm_;

    std::shared_ptr<Opm::EclIO::RestartFileView> rstView_;
};

AquiferVectors::AquiferVectors(const std::vector<int>&                      intehead,
                               std::shared_ptr<Opm::EclIO::RestartFileView> rst_view)
    : maxAnalyticAquiferID_        (intehead[VI::intehead::MAX_ANALYTIC_AQUIFERS])
    , numRecordsForNumericAquifers_(intehead[VI::intehead::NUM_AQUNUM_RECORDS])
    , numIntAnalyticAquiferElm_    (intehead[VI::intehead::NIAAQZ])
    , numIntNumericAquiferElm_     (intehead[VI::intehead::NIIAQN])
    , numFloatAnalyticAquiferElm_  (intehead[VI::intehead::NSAAQZ])
    , numDoubleAnalyticAquiferElm_ (intehead[VI::intehead::NXAAQZ])
    , numDoubleNumericAquiferElm_  (intehead[VI::intehead::NIRAQN])
    , rstView_                     (std::move(rst_view))
{}

bool AquiferVectors::hasDefinedValues() const
{
    return this->rstView_->hasKeyword<int>   ("IAAQ")
        && this->rstView_->hasKeyword<float> ("SAAQ")
        && this->rstView_->hasKeyword<double>("XAAQ");
}

bool AquiferVectors::hasDefinedNumericAquiferValues() const
{
    return this->rstView_->hasKeyword<int>   ("IAQN")
        && this->rstView_->hasKeyword<double>("RAQN");
}

std::size_t AquiferVectors::numRecordsForNumericAquifers() const
{
    if (! this->hasDefinedNumericAquiferValues()) {
        return 0;
    }

    return this->numRecordsForNumericAquifers_;
}

AquiferVectors::Window<int>
AquiferVectors::iaaq(const std::size_t aquiferID) const
{
    if (! this->hasDefinedValues()) {
        throw std::logic_error {
            "Cannot Request IAAQ Values Unless Defined"
        };
    }

    return getDataWindow(this->rstView_->getKeyword<int>("IAAQ"),
                         this->numIntAnalyticAquiferElm_, aquiferID);
}

AquiferVectors::Window<float>
AquiferVectors::saaq(const std::size_t aquiferID) const
{
    if (! this->hasDefinedValues()) {
        throw std::logic_error {
            "Cannot Request SAAQ Values Unless Defined"
        };
    }

    return getDataWindow(this->rstView_->getKeyword<float>("SAAQ"),
                         this->numFloatAnalyticAquiferElm_, aquiferID);
}

AquiferVectors::Window<double>
AquiferVectors::xaaq(const std::size_t aquiferID) const
{
    if (! this->hasDefinedValues()) {
        throw std::logic_error {
            "Cannot Request XAAQ Values Unless Defined"
        };
    }

    return getDataWindow(this->rstView_->getKeyword<double>("XAAQ"),
                         this->numDoubleAnalyticAquiferElm_, aquiferID);
}

AquiferVectors::Window<int>
AquiferVectors::iaqn(const std::size_t recordID) const
{
    if (! this->hasDefinedNumericAquiferValues()) {
        throw std::logic_error {
            "Cannot Request IAQN Values Unless Defined"
        };
    }

    return getDataWindow(this->rstView_->getKeyword<int>("IAQN"),
                         this->numIntNumericAquiferElm_, recordID);
}

AquiferVectors::Window<double>
AquiferVectors::raqn(const std::size_t recordID) const
{
    if (! this->hasDefinedNumericAquiferValues()) {
        throw std::logic_error {
            "Cannot Request RAQN Values Unless Defined"
        };
    }

    return getDataWindow(this->rstView_->getKeyword<double>("RAQN"),
                         this->numDoubleNumericAquiferElm_, recordID);
}

// ---------------------------------------------------------------------

namespace {
    void throwIfMissingRequired(const Opm::RestartKey& rst_key)
    {
        if (rst_key.required) {
            throw std::runtime_error {
                "Requisite restart vector '"
                + rst_key.key +
                "' is not available in restart file"
            };
        }
    }

    bool hasAquifers(const Opm::EclIO::RestartFileView& rst_view)
    {
        return rst_view.hasKeyword<double>("XAAQ")
            || rst_view.hasKeyword<double>("RAQN");
    }

    std::size_t maximumAnalyticAquiferID(const Opm::EclIO::RestartFileView& rst_view)
    {
        return rst_view.intehead()[VI::intehead::MAX_AN_AQUIFER_ID];
    }

    std::vector<double>
    double_vector(const std::string& key, const Opm::EclIO::RestartFileView& rst_view)
    {
        if (rst_view.hasKeyword<double>(key)) {
            // Data exists as type DOUB.  Return unchanged.
            return rst_view.getKeyword<double>(key);
        }
        else if (rst_view.hasKeyword<float>(key)) {
            // Data exists as type REAL.  Convert to double.
            const auto& data = rst_view.getKeyword<float>(key);

            return { data.begin(), data.end() };
        }

        // Data unavailable.  Return empty.
        return {};
    }

    void insertSolutionVector(const std::vector<double>&           vector,
                              const Opm::RestartKey&               value,
                              const std::vector<double>::size_type numcells,
                              Opm::data::Solution&                 sol)
    {
        if (vector.size() != numcells) {
            throw std::runtime_error {
                "Restart file: Could not restore '"
                + value.key
                + "', mismatched number of cells"
            };
        }

        sol.insert(value.key, value.dim, vector,
                   Opm::data::TargetType::RESTART_SOLUTION);
    }

    void loadIfAvailable(const Opm::RestartKey&               value,
                         const std::vector<double>::size_type numcells,
                         const Opm::EclIO::RestartFileView&   rst_view,
                         Opm::data::Solution&                 sol)
    {
        const auto& kwdata = double_vector(value.key, rst_view);

        if (kwdata.empty()) {
            throwIfMissingRequired(value);

            // If we get here, the requested value was not available in the
            // result set.  However, the client does not actually require
            // the value for restart purposes so we can safely skip this.
            return;
        }

        insertSolutionVector(kwdata, value, numcells, sol);
    }

    std::vector<double>
    getOpmExtraFromDoubHEAD(const bool                         required,
                            const Opm::UnitSystem&             usys,
                            const Opm::EclIO::RestartFileView& rst_view)
    {
        using M = Opm::UnitSystem::measure;

        const auto& doubhead = rst_view.getKeyword<double>("DOUBHEAD");

        const auto TsInit = doubhead[VI::doubhead::TsInit];

        if (TsInit < 0.0) {
            throwIfMissingRequired({ "OPMEXTRA", M::identity, required });
        }

        return { usys.to_si(M::time, TsInit) };
    }

    Opm::data::Solution
    restoreSOLUTION(const std::vector<Opm::RestartKey>& solution_keys,
                    const int                           numcells,
                    const Opm::EclIO::RestartFileView&  rst_view)
    {
        Opm::data::Solution sol(/* init_si = */ false);

        for (const auto& value : solution_keys) {
            // Load vector if available.
            loadIfAvailable(value, numcells, rst_view, sol);
        }

        return sol;
    }

    void restoreExtra(const std::vector<Opm::RestartKey>& extra_keys,
                      const Opm::UnitSystem&              usys,
                      const Opm::EclIO::RestartFileView&  rst_view,
                      Opm::RestartValue&                  rst_value)
    {
        for (const auto& extra : extra_keys) {
            const auto& vector = extra.key;
            auto        kwdata = double_vector(vector, rst_view);

            if (kwdata.empty()) {
                // Requested vector not available in result set.  Take
                // appropriate action depending on specific vector and
                // 'extra.required'.

                if (vector != "OPMEXTRA") {
                    throwIfMissingRequired(extra);

                    // Requested vector not available, but caller does not
                    // actually require the vector for restart purposes.
                    // Skip this.
                    continue;
                }
                else {
                    // Special case handling of OPMEXTRA.  Single item
                    // possibly stored in TSINIT item of DOUBHEAD.  Try to
                    // recover this.  Function throws if item is defaulted
                    // and caller requires that item be present through the
                    // 'extra.required' mechanism.

                    kwdata = getOpmExtraFromDoubHEAD(extra.required,
                                                     usys, rst_view);
                }
            }

            rst_value.addExtra(vector, extra.dim, std::move(kwdata));
        }

        for (auto& extra_value : rst_value.extra) {
            const auto& restart_key = extra_value.first;
            auto&       data        = extra_value.second;

            usys.to_si(restart_key.dim, data);
        }
    }

    template <typename AssignCumulative>
    void restoreConnCumulatives(const WellVectors::Window<double>& xcon,
                                AssignCumulative&&                 asgn)
    {
        asgn("COPT", xcon[VI::XConn::index::OilPrTotal]);
        asgn("COIT", xcon[VI::XConn::index::OilInjTotal]);

        asgn("CGPT", xcon[VI::XConn::index::GasPrTotal]);
        asgn("CGIT", xcon[VI::XConn::index::GasInjTotal]);

        asgn("CWPT", xcon[VI::XConn::index::WatPrTotal]);
        asgn("CWIT", xcon[VI::XConn::index::WatInjTotal]);

        asgn("CVPT", xcon[VI::XConn::index::VoidPrTotal]);
        asgn("CVIT", xcon[VI::XConn::index::VoidInjTotal]);
    }

    void restoreConnRates(const WellVectors::Window<double>& xcon,
                          const Opm::UnitSystem&             usys,
                          const bool                         oil,
                          const bool                         gas,
                          const bool                         wat,
                          Opm::data::Connection&             xc)
    {
        using M = ::Opm::UnitSystem::measure;

        if (wat) {
            xc.rates.set(Opm::data::Rates::opt::wat,
                         - usys.to_si(M::liquid_surface_rate,
                                      xcon[VI::XConn::index::WaterRate]));
        }

        if (oil) {
            xc.rates.set(Opm::data::Rates::opt::oil,
                         - usys.to_si(M::liquid_surface_rate,
                                      xcon[VI::XConn::index::OilRate]));
        }

        if (gas) {
            xc.rates.set(Opm::data::Rates::opt::gas,
                         - usys.to_si(M::gas_surface_rate,
                                      xcon[VI::XConn::index::GasRate]));
        }
    }

    void zeroConnRates(const bool             oil,
                       const bool             gas,
                       const bool             wat,
                       Opm::data::Connection& xc)
    {
        if (wat) { xc.rates.set(Opm::data::Rates::opt::wat, 0.0); }
        if (oil) { xc.rates.set(Opm::data::Rates::opt::oil, 0.0); }
        if (gas) { xc.rates.set(Opm::data::Rates::opt::gas, 0.0); }
    }

    void restoreConnResults(const Opm::Well&       well,
                            const std::size_t       wellID,
                            const Opm::EclipseGrid& grid,
                            const Opm::UnitSystem&  usys,
                            const Opm::Phases&      phases,
                            const WellVectors&      wellData,
                            Opm::SummaryState&      smry,
                            Opm::data::Well&        xw)
    {
        using M  = ::Opm::UnitSystem::measure;
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XConn::index;

        const auto iwel  = wellData.iwel(wellID);
        const auto nConn = static_cast<std::size_t>(iwel[VI::IWell::index::NConn]);

        const auto oil = phases.active(Opm::Phase::OIL);
        const auto gas = phases.active(Opm::Phase::GAS);
        const auto wat = phases.active(Opm::Phase::WATER);

        {
            const auto& connections = well.getConnections();
            xw.connections.resize(connections.size(), Opm::data::Connection{});

            auto simConnID = std::size_t{0};
            for (const auto& conn : connections) {
                auto& xc = xw.connections[simConnID];
                zeroConnRates(oil, gas, wat, xc);

                xc.index = conn.global_index();
                ++simConnID;
            }
        }

        if (! wellData.hasDefinedConnectionValues()) {
            // Result set does not provide certain pieces of information
            // which are needed to reconstruct connection flow rates.
            // Nothing to do except to return all zero rates.
            return;
        }

        const auto& wname = well.name();
        for (auto rstConnID = 0*nConn; rstConnID < nConn; ++rstConnID) {
            const auto icon = wellData.icon(wellID, rstConnID);

            const auto i = icon[VI::IConn::index::CellI] - 1;
            const auto j = icon[VI::IConn::index::CellJ] - 1;
            const auto k = icon[VI::IConn::index::CellK] - 1;

            const auto globCell = grid.getGlobalIndex(i, j, k);
            const auto xcon = wellData.xcon(wellID, rstConnID);

            restoreConnCumulatives(xcon, [globCell, &wname, &smry]
                (const std::string& vector, const double value)
            {
                smry.update_conn_var(wname, vector, globCell + 1, value);
            });

            auto* xc = xw.find_connection(globCell);
            if (xc == nullptr) { continue; }

            restoreConnRates(xcon, usys, oil, gas, wat, *xc);

            xc->pressure = usys.to_si(M::pressure, xcon[Ix::Pressure]);
        }
    }

    ::Opm::Well::ProducerCMode producerControlMode(const int curr)
    {
        using PMode = ::Opm::Well::ProducerCMode;
        using Ctrl  = VI::IWell::Value::WellCtrlMode;

        switch (curr) {
            case Ctrl::OilRate:  return PMode::ORAT;
            case Ctrl::WatRate:  return PMode::WRAT;
            case Ctrl::GasRate:  return PMode::GRAT;
            case Ctrl::LiqRate:  return PMode::LRAT;
            case Ctrl::ResVRate: return PMode::RESV;
            case Ctrl::THP:      return PMode::THP;
            case Ctrl::BHP:      return PMode::BHP;
            case Ctrl::CombRate: return PMode::CRAT;
            case Ctrl::Group:    return PMode::GRUP;

            default:
                return PMode::CMODE_UNDEFINED;
        }
    }

    ::Opm::Well::InjectorCMode
    injectorControlMode(const int curr, const int itype)
    {
        using IMode = ::Opm::Well::InjectorCMode;
        using Ctrl  = VI::IWell::Value::WellCtrlMode;

        switch (curr) {
            case Ctrl::OilRate:
                return Opm::WellType::oil_injector(itype)
                    ? IMode::RATE : IMode::CMODE_UNDEFINED;

            case Ctrl::WatRate:
                return Opm::WellType::water_injector(itype)
                    ? IMode::RATE : IMode::CMODE_UNDEFINED;

            case Ctrl::GasRate:
                return Opm::WellType::gas_injector(itype)
                    ? IMode::RATE : IMode::CMODE_UNDEFINED;

            case Ctrl::ResVRate: return IMode::RESV;
            case Ctrl::THP:      return IMode::THP;
            case Ctrl::BHP:      return IMode::BHP;
            case Ctrl::Group:    return IMode::GRUP;
        }

        return IMode::CMODE_UNDEFINED;
    }

    void restoreCurrentControl(const std::size_t  wellID,
                               const WellVectors& wellData,
                               Opm::data::Well&   xw)
    {
        const auto iwel = wellData.iwel(wellID);
        // For E100 it appears that +1 instead of -1 is written for group_controllable_flag when the 
        // group control is aactive, so using this to correct active_control (where ind.ctrl. is written)
        const auto grpc = iwel[VI::IWell::index::WGrupConControllable];
        const auto act  = grpc > 0 ? VI::IWell::Value::WellCtrlMode::Group : iwel[VI::IWell::index::ActWCtrl];
        const auto wtyp = iwel[VI::IWell::index::WType];

        auto& curr = xw.current_control;

        curr.isProducer = Opm::WellType::producer(wtyp);
        if (curr.isProducer) {
            curr.prod = producerControlMode(act);
        }
        else { // Assume injector
            curr.inj = injectorControlMode(act, wtyp);
        }
    }

    void restoreSegmentQuantities(const std::size_t        mswID,
                                  const Opm::WellSegments& segSet,
                                  const Opm::UnitSystem&   usys,
                                  const Opm::Phases&       phases,
                                  const SegmentVectors&    segData,
                                  Opm::data::Well&         xw)
    {
        // Note sign of flow rates.  RSEG stores flow rates as positive from
        // reservoir to well--i.e., towards the producer/platform.  The Flow
        // simulator uses the opposite sign convention.

        using M = ::Opm::UnitSystem::measure;

        const auto oil = phases.active(Opm::Phase::OIL);
        const auto gas = phases.active(Opm::Phase::GAS);
        const auto wat = phases.active(Opm::Phase::WATER);

        const auto numSeg = static_cast<std::size_t>(segSet.size());

        // Renormalisation constant for gas okay in non-field unit systems.
        // A bit more questionable for field units.
        const auto watRenormalisation =   10.0;
        const auto gasRenormalisation = 1000.0;

        for (auto segID = 0*numSeg; segID < numSeg; ++segID) {
            const auto segNumber = segSet[segID].segmentNumber(); // One-based
            const auto rseg      = segData.rseg(mswID, segNumber - 1);

            auto& segment = xw.segments[segNumber];

            segment.segNumber = segNumber;
            segment.pressures[Opm::data::SegmentPressures::Value::Pressure]  =
                usys.to_si(M::pressure, rseg[VI::RSeg::index::Pressure]);

            const auto totFlow     = rseg[VI::RSeg::index::TotFlowRate];
            const auto watFraction = rseg[VI::RSeg::index::WatFlowFract];
            const auto gasFraction = rseg[VI::RSeg::index::GasFlowFract];

            if (wat) {
                const auto qW = totFlow * watFraction * watRenormalisation;
                segment.rates.set(Opm::data::Rates::opt::wat,
                                  - usys.to_si(M::liquid_surface_rate, qW));
            }

            if (oil) {
                const auto qO = totFlow * (1.0 - (watFraction + gasFraction));
                segment.rates.set(Opm::data::Rates::opt::oil,
                                  - usys.to_si(M::liquid_surface_rate, qO));
            }

            if (gas) {
                const auto qG = totFlow * gasFraction * gasRenormalisation;
                segment.rates.set(Opm::data::Rates::opt::gas,
                                  - usys.to_si(M::gas_surface_rate, qG));
            }
        }
    }

    Opm::data::Well
    restore_well(const Opm::Well&        well,
                 const std::size_t       wellID,
                 const Opm::EclipseGrid& grid,
                 const Opm::UnitSystem&  usys,
                 const Opm::Phases&      phases,
                 const WellVectors&      wellData,
                 const SegmentVectors&   segData,
                 Opm::SummaryState&      smry)
    {
        if (! wellData.hasDefinedWellValues()) {
            // Result set does not provide well information.
            // No wells?  In any case, nothing to do here.
            return {};
        }

        using M = ::Opm::UnitSystem::measure;

        const auto xwel = wellData.xwel(wellID);

        const auto oil = phases.active(Opm::Phase::OIL);
        const auto gas = phases.active(Opm::Phase::GAS);
        const auto wat = phases.active(Opm::Phase::WATER);

        auto xw = ::Opm::data::Well{};

        // 1) Restore well rates (xw.rates)
        if (wat) {
            xw.rates.set(Opm::data::Rates::opt::wat,
                         - usys.to_si(M::liquid_surface_rate,
                                      xwel[VI::XWell::index::WatPrRate]));
        }

        if (oil) {
            xw.rates.set(Opm::data::Rates::opt::oil,
                         - usys.to_si(M::liquid_surface_rate,
                                      xwel[VI::XWell::index::OilPrRate]));
        }

        if (gas) {
            xw.rates.set(Opm::data::Rates::opt::gas,
                         - usys.to_si(M::gas_surface_rate,
                                      xwel[VI::XWell::index::GasPrRate]));
        }

        // 2) Restore guide rates
        if (well.isProducer()) {
            if (wat)
                xw.guide_rates.set(Opm::data::GuideRateValue::Item::Water,
                                   usys.to_si(M::liquid_surface_rate, xwel[VI::XWell::index::WatPrGuideRate]));

            if (oil)
                xw.guide_rates.set(Opm::data::GuideRateValue::Item::Oil,
                                   usys.to_si(M::liquid_surface_rate, xwel[VI::XWell::index::PrimGuideRate]));

            if (gas)
                xw.guide_rates.set(Opm::data::GuideRateValue::Item::Gas,
                                   usys.to_si(M::gas_surface_rate, xwel[VI::XWell::index::GasPrGuideRate]));

            xw.guide_rates.set(Opm::data::GuideRateValue::Item::ResV,
                               usys.to_si(M::rate, xwel[VI::XWell::index::VoidPrGuideRate]));
        } else {
            const auto& injector_type = well.injectorType();
            switch (injector_type) {
            case Opm::InjectorType::WATER:
                xw.guide_rates.set(Opm::data::GuideRateValue::Item::Water,
                                   -usys.to_si(M::liquid_surface_rate, xwel[VI::XWell::index::PrimGuideRate]));
                break;

            case Opm::InjectorType::GAS:
                xw.guide_rates.set(Opm::data::GuideRateValue::Item::Gas,
                                   -usys.to_si(M::gas_surface_rate, xwel[VI::XWell::index::PrimGuideRate]));
                break;
            default:
                throw std::logic_error("Only WATER and GAS injectors are supported when loading from restart file");
            }
        }


        // 3) Restore other well quantities (really only xw.bhp)
        xw.bhp = usys.to_si(M::pressure, xwel[VI::XWell::index::FlowBHP]);
        xw.thp = usys.to_si(M::pressure, xwel[VI::XWell::index::TubHeadPr]);
        xw.temperature = 0.0;

        // 4) Restore connection flow rates (xw.connections[i].rates),
        //    cumulatives (Cx{P,I}T), and pressure values
        //    (xw.connections[i].pressure).
        restoreConnResults(well, wellID, grid, usys,
                           phases, wellData, smry, xw);

        // 5) Restore well's active/current control
        restoreCurrentControl(wellID, wellData, xw);

        // 6) Restore segment quantities if applicable.
        if (well.isMultiSegment() && segData.hasDefinedValues())
        {
            const auto iwel   = wellData.iwel(wellID);
            const auto mswID  = iwel[VI::IWell::index::MsWID]; // One-based
            const auto numSeg = iwel[VI::IWell::index::NWseg];

            const auto& segSet = well.getSegments();

            if ((mswID > 0) && (numSeg > 0) &&
                (static_cast<int>(segSet.size()) == numSeg))
            {
                restoreSegmentQuantities(mswID - 1, segSet, usys,
                                         phases, segData, xw);
            }
        }

        return xw;
    }

    Opm::data::Wells
    restore_wells(const ::Opm::EclipseState&                   es,
                  const ::Opm::EclipseGrid&                    grid,
                  const ::Opm::Schedule&                       schedule,
                  Opm::SummaryState&                           smry,
                  std::shared_ptr<Opm::EclIO::RestartFileView> rst_view)
    {
        auto soln = ::Opm::data::Wells{};

        const auto& intehead = rst_view->intehead();;

        const auto wellData = WellVectors   { intehead, rst_view };
        const auto segData  = SegmentVectors{ intehead, rst_view };

        const auto& units  = es.getUnits();
        const auto& phases = es.runspec().phases();

        const auto& wells = schedule.getWells(rst_view->simStep());
        for (auto nWells = wells.size(), wellID = 0*nWells;
                  wellID < nWells; ++wellID)
        {
            const auto& well = wells[wellID];

            soln[well.name()] =
                restore_well(well, wellID, grid, units,
                             phases, wellData, segData, smry);
        }

        return soln;
    }

    Opm::data::GroupAndNetworkValues
    restore_grp_nwrk(const Opm::Schedule&                         schedule,
                     const Opm::UnitSystem&                       usys,
                     std::shared_ptr<Opm::EclIO::RestartFileView> rst_view)
    {
        using M = Opm::UnitSystem::measure;
        using xIx = VI::XGroup::index;
        using GRVI = Opm::data::GuideRateValue::Item;

        auto xg_nwrk = Opm::data::GroupAndNetworkValues{};

        const auto& intehead  = rst_view->intehead();
        const auto  sim_step  = rst_view->simStep();
        const auto  nwgmax    = intehead[VI::NWGMAX];
        const auto  groupData = GroupVectors{ intehead, rst_view };

        for (const auto& gname : schedule.groupNames(sim_step)) {
            const auto& group = schedule.getGroup(gname, sim_step);
            const auto group_index = (gname == "FIELD")
                ? groupData.maxGroups() : group.insert_index() - 1;

            const auto igrp = groupData.igrp(group_index);
            const auto xgrp = groupData.xgrp(group_index);

            auto& gr_data = xg_nwrk.groupData[gname];

            gr_data.currentControl
                .set(Opm::Group::ProductionCModeFromInt(igrp[nwgmax + VI::IGroup::index::ProdActiveCMode]),
                     Opm::Group::InjectionCModeFromInt (igrp[nwgmax + VI::IGroup::index::GInjActiveCMode]),
                     Opm::Group::InjectionCModeFromInt (igrp[nwgmax + VI::IGroup::index::WInjActiveCMode]));

            if (igrp[nwgmax + VI::IGroup::GuideRateDef] != VI::IGroup::Value::None) {
                gr_data.guideRates.production
                    .set(GRVI::Oil,   usys.to_si(M::liquid_surface_rate, xgrp[xIx::OilPrGuideRate]))
                    .set(GRVI::Gas,   usys.to_si(M::gas_surface_rate,    xgrp[xIx::GasPrGuideRate]))
                    .set(GRVI::Water, usys.to_si(M::liquid_surface_rate, xgrp[xIx::WatPrGuideRate]))
                    .set(GRVI::ResV,  usys.to_si(M::rate,                xgrp[xIx::VoidPrGuideRate]));

                gr_data.guideRates.injection
                    .set(GRVI::Oil,   usys.to_si(M::liquid_surface_rate, xgrp[xIx::OilInjGuideRate]))
                    .set(GRVI::Gas,   usys.to_si(M::gas_surface_rate,    xgrp[xIx::GasInjGuideRate]))
                    .set(GRVI::Water, usys.to_si(M::liquid_surface_rate, xgrp[xIx::WatInjGuideRate]));
            }

            const auto node_pressure = -1.0;
            xg_nwrk.nodeData[gname].pressure = node_pressure;
        }

        return xg_nwrk;
    }

    Opm::data::AquiferType
    determineAquiferType(const AquiferVectors::Window<int>& iaaq)
    {
        using MType = Opm::RestartIO::Helpers::VectorItems::
            IAnalyticAquifer::Value::ModelType;

        using AType = Opm::data::AquiferType;

        switch (iaaq[VI::IAnalyticAquifer::TypeRelated1]) {
        case MType::Fetkovich:    return AType::Fetkovich;
        case MType::CarterTracy:  return AType::CarterTracy;
        case MType::ConstantFlux: return AType::ConstantFlux;
        }

        throw std::runtime_error {
            "Unknown Aquifer Type: T1 = " +
            std::to_string(iaaq[VI::IAnalyticAquifer::TypeRelated1])
        };
    }

    Opm::data::FetkovichData
    extractFetkovichData(const Opm::UnitSystem&               usys,
                         const AquiferVectors::Window<float>& saaq)
    {
        using M = ::Opm::UnitSystem::measure;

        auto data = Opm::data::FetkovichData{};

        data.initVolume =
            usys.to_si(M::liquid_surface_volume,
                       saaq[VI::SAnalyticAquifer::FetInitVol]);

        data.prodIndex =
            usys.to_si(M::liquid_surface_rate,
            usys.from_si(M::pressure,
                         saaq[VI::SAnalyticAquifer::FetProdIndex]));

        data.timeConstant = saaq[VI::SAnalyticAquifer::FetTimeConstant];

        return data;
    }

    void restore_analytic_aquifer(const std::size_t      aquiferID,
                                  const AquiferVectors&  aquiferData,
                                  const Opm::UnitSystem& units,
                                  Opm::data::Aquifers&   aquifers)
    {
        using M  = ::Opm::UnitSystem::measure;
        using Ix = VI::XAnalyticAquifer::index;

        const auto saaq = aquiferData.saaq(aquiferID);
        const auto xaaq = aquiferData.xaaq(aquiferID);

        auto& aqData = aquifers[1 + static_cast<int>(aquiferID)];

        aqData.aquiferID = 1 + static_cast<int>(aquiferID);
        aqData.pressure  = units.to_si(M::pressure, xaaq[Ix::Pressure]);
        aqData.volume    = units.to_si(M::liquid_surface_volume,
                                       xaaq[Ix::ProdVolume]);

        aqData.initPressure =
            units.to_si(M::pressure, saaq[VI::SAnalyticAquifer::InitPressure]);

        aqData.datumDepth =
            units.to_si(M::length, saaq[VI::SAnalyticAquifer::DatumDepth]);

        const auto type = determineAquiferType(aquiferData.iaaq(aquiferID));
        if (type == Opm::data::AquiferType::Fetkovich) {
            auto* tData = aqData.typeData.create<Opm::data::AquiferType::Fetkovich>();
            *tData = extractFetkovichData(units, saaq);
        }
    }

    void restore_numeric_aquifers(const AquiferVectors&  aquiferData,
                                  const Opm::UnitSystem& units,
                                  Opm::data::Aquifers&   aquifers)
    {
        using M = ::Opm::UnitSystem::measure;
        using Opm::data::AquiferType;

        const auto IxAqID = VI::INumericAquifer::index::AquiferID;
        const auto IxANQT = VI::RNumericAquifer::index::ProdVolume;
        const auto IxIPR  = VI::RNumericAquifer::index::Pressure;

        const auto numRecords = aquiferData.numRecordsForNumericAquifers();
        for (auto recordID = 0*numRecords; recordID < numRecords; ++recordID) {
            const auto aquiferID = aquiferData.iaqn(recordID)[IxAqID];
            auto& aqData = aquifers[aquiferID];

            if (! aqData.typeData.is<AquiferType::Numerical>()) {
                aqData.typeData.create<AquiferType::Numerical>();
                aqData.aquiferID = aquiferID;
            }

            const auto raqn = aquiferData.raqn(recordID);
            auto* typeData = aqData.typeData.getMutable<AquiferType::Numerical>();

            typeData->initPressure.push_back(units.to_si(M::pressure, raqn[IxIPR]));
            if (const auto volume = raqn[IxANQT]; volume > 0.0) {
                aqData.volume = units.to_si(M::liquid_surface_volume, volume);
            }
        }
    }

    Opm::data::Aquifers
    restore_aquifers(const ::Opm::EclipseState&                   es,
                     std::shared_ptr<Opm::EclIO::RestartFileView> rst_view)
    {
        auto aquifers = Opm::data::Aquifers{};

        const auto& intehead    = rst_view->intehead();
        const auto  aquiferData = AquiferVectors{ intehead, rst_view };

        const auto maxAqID = maximumAnalyticAquiferID(*rst_view);
        for (auto aquiferID = 0*maxAqID; aquiferID < maxAqID; ++aquiferID) {
            restore_analytic_aquifer(aquiferID, aquiferData, es.getUnits(), aquifers);
        }

        restore_numeric_aquifers(aquiferData, es.getUnits(), aquifers);

        return aquifers;
    }

    void assign_well_cumulatives(const std::string& well,
                                 const std::size_t  wellID,
                                 const Opm::Tracers& tracer_dims,
                                 const Opm::TracerConfig& tracer_config,
                                 const WellVectors& wellData,
                                 Opm::SummaryState& smry)
    {
        if (! wellData.hasDefinedWellValues()) {
            // Result set does not provide well information.
            // No wells?  In any case, nothing to do here.
            return;
        }

        const auto xwel = wellData.xwel(wellID);

        // No unit conversion here.  Smry expects output units.
        smry.update_well_var(well, "WOPT", xwel[VI::XWell::index::OilPrTotal]);
        smry.update_well_var(well, "WWPT", xwel[VI::XWell::index::WatPrTotal]);
        smry.update_well_var(well, "WGPT", xwel[VI::XWell::index::GasPrTotal]);
        smry.update_well_var(well, "WVPT", xwel[VI::XWell::index::VoidPrTotal]);

        // Cumulative liquid production = WOPT + WWPT
        smry.update_well_var(well, "WLPT",
                             xwel[VI::XWell::index::OilPrTotal] +
                             xwel[VI::XWell::index::WatPrTotal]);

        smry.update_well_var(well, "WWIT", xwel[VI::XWell::index::WatInjTotal]);
        smry.update_well_var(well, "WGIT", xwel[VI::XWell::index::GasInjTotal]);
        smry.update_well_var(well, "WVIT", xwel[VI::XWell::index::VoidInjTotal]);

        smry.update_well_var(well, "WOPTS", xwel[VI::XWell::index::OilPrTotalSolution]);
        smry.update_well_var(well, "WGPTS", xwel[VI::XWell::index::GasPrTotalSolution]);

        // Free oil cumulative production = WOPT - WOPTS
        smry.update_well_var(well, "WOPTF",
                             xwel[VI::XWell::index::OilPrTotal] -
                             xwel[VI::XWell::index::OilPrTotalSolution]);

        // Free gas cumulative production = WGPT - WGPTS
        smry.update_well_var(well, "WGPTF",
                             xwel[VI::XWell::index::GasPrTotal] -
                             xwel[VI::XWell::index::GasPrTotalSolution]);

        smry.update_well_var(well, "WOPTH", xwel[VI::XWell::index::HistOilPrTotal]);
        smry.update_well_var(well, "WWPTH", xwel[VI::XWell::index::HistWatPrTotal]);
        smry.update_well_var(well, "WGPTH", xwel[VI::XWell::index::HistGasPrTotal]);

        smry.update_well_var(well, "WWITH", xwel[VI::XWell::index::HistWatInjTotal]);
        smry.update_well_var(well, "WGITH", xwel[VI::XWell::index::HistGasInjTotal]);

        for (std::size_t tracer_index = 0; tracer_index < tracer_config.size(); tracer_index++) {
            const auto& tracer_name = tracer_config[tracer_index].name;
            auto wtpt_offset = VI::XWell::index::TracerOffset +   tracer_dims.water_tracers();
            auto wtit_offset = VI::XWell::index::TracerOffset + 2*tracer_dims.water_tracers();

            smry.update_well_var(well, fmt::format("WTPT{}", tracer_name), xwel[wtpt_offset + tracer_index]);
            smry.update_well_var(well, fmt::format("WTIT{}", tracer_name), xwel[wtit_offset + tracer_index]);
        }
    }

    void assign_group_cumulatives(const std::string&  group,
                                  const std::size_t   groupID,
                                  const GroupVectors& groupData,
                                  Opm::SummaryState&  smry)
    {
        if (! groupData.hasDefinedValues()) {
            // Result set does not provide group information.
            // No groups?  In any case, nothing to do here.
            return;
        }

        auto update = [isField = group == "FIELD", &group, &smry]
            (const std::string& vector, const double value) -> void
        {
            if (isField) {
                // Initialise the F* vectors for FIELD
                smry.update('F' + vector, value);
            }
            else {
                // Initialise the G* vectors for all non-FIELD groups
                smry.update_group_var(group, 'G' + vector, value);
            }
        };

        const auto xgrp = groupData.xgrp(groupID);

        // No unit conversion here.  Smry expects output units.
        update("OPT", xgrp[VI::XGroup::index::OilPrTotal]);
        update("WPT", xgrp[VI::XGroup::index::WatPrTotal]);
        update("GPT", xgrp[VI::XGroup::index::GasPrTotal]);
        update("VPT", xgrp[VI::XGroup::index::VoidPrTotal]);

        // Cumulative liquid production = xOPT + xWPT
        update("LPT",
               xgrp[VI::XGroup::index::OilPrTotal] +
               xgrp[VI::XGroup::index::WatPrTotal]);

        update("WIT", xgrp[VI::XGroup::index::WatInjTotal]);
        update("GIT", xgrp[VI::XGroup::index::GasInjTotal]);
        update("VIT", xgrp[VI::XGroup::index::VoidInjTotal]);

        update("OPTS", xgrp[VI::XGroup::index::OilPrTotalSolution]);
        update("GPTS", xgrp[VI::XGroup::index::GasPrTotalSolution]);

        // Free oil cumulative production = xOPT - xOPTS
        update("OPTF",
               xgrp[VI::XGroup::index::OilPrTotal] -
               xgrp[VI::XGroup::index::OilPrTotalSolution]);

        // Free gas cumulative production = xGPT - xGPTS
        update("GPTF",
               xgrp[VI::XGroup::index::GasPrTotal] -
               xgrp[VI::XGroup::index::GasPrTotalSolution]);

        update("OPTH", xgrp[VI::XGroup::index::HistOilPrTotal]);
        update("WPTH", xgrp[VI::XGroup::index::HistWatPrTotal]);
        update("GPTH", xgrp[VI::XGroup::index::HistGasPrTotal]);
        update("WITH", xgrp[VI::XGroup::index::HistWatInjTotal]);
        update("GITH", xgrp[VI::XGroup::index::HistGasInjTotal]);

        update("GCT",  xgrp[VI::XGroup::index::GasConsumptionTotal]);
        update("GIMT", xgrp[VI::XGroup::index::GasImportTotal]);
    }

    bool isDefaultedUDQ(const double x)
    {
        return x == Opm::UDQ::restart_default;
    }

    void restoreFieldUDQValue(const UDQVectors&  udqs,
                              const std::string& quantity,
                              Opm::SummaryState& smry)
    {
        const auto dudf = udqs.currentFieldUDQValue();

        if (! isDefaultedUDQ(dudf)) {
            smry.update(quantity, dudf);
        }
    }

    void restoreGroupUDQValue(const UDQVectors&                     udqs,
                              const std::vector<const Opm::Group*>& groups,
                              const std::string&                    quantity,
                              Opm::SummaryState&                    smry)
    {
        const auto nGrp = groups.size();
        const auto dudg = udqs.currentGroupUDQValue();

        for (auto iGrp = 0*nGrp; iGrp < nGrp; ++iGrp) {
            if ((groups[iGrp] == nullptr) || isDefaultedUDQ(dudg[iGrp])) {
                continue;
            }

            smry.update_group_var(groups[iGrp]->name(), quantity, dudg[iGrp]);
        }
    }

    void restoreSegmentUDQValue(const UDQVectors&               udqs,
                                const std::vector<std::string>& msWells,
                                const std::string&              quantity,
                                Opm::SummaryState&              smry)
    {
        const auto nWells = msWells.size();

        for (auto iWell = 0*nWells; iWell < nWells; ++iWell) {
            const auto duds = udqs.currentSegmentUDQValue(iWell);
            const auto nSeg = duds.size();

            for (auto iSeg = 0*nSeg; iSeg < nSeg; ++iSeg) {
                if (isDefaultedUDQ(duds[iSeg])) {
                    continue;
                }

                smry.update_segment_var(msWells[iWell],
                                        quantity,
                                        iSeg + 1, // One-based segment number.
                                        duds[iSeg]);
            }
        }
    }

    void restoreWellUDQValue(const UDQVectors&               udqs,
                             const std::vector<std::string>& wells,
                             const std::string&              quantity,
                             Opm::SummaryState&              smry)
    {
        const auto nWell = wells.size();
        const auto dudw  = udqs.currentWellUDQValue();

        for (auto iWell = 0*nWell; iWell < nWell; ++iWell) {
            if (isDefaultedUDQ(dudw[iWell])) {
                continue;
            }

            smry.update_well_var(wells[iWell], quantity, dudw[iWell]);
        }
    }

    std::vector<std::string>
    multiSegmentWells(const Opm::ScheduleState&       scheduleBlock,
                      const std::vector<std::string>& allWells)
    {
        auto msWells = std::vector<std::string>{};
        msWells.reserve(allWells.size());

        std::copy_if(allWells.begin(), allWells.end(),
                     std::back_inserter(msWells),
                     [&scheduleBlock](const std::string& wname)
                     {
                         auto wptr = scheduleBlock.wells.get_ptr(wname);
                         return (wptr != nullptr) && wptr->isMultiSegment();
                     });

        return msWells;
    }

    void restoreUDQValues(const Opm::Schedule&                         schedule,
                          std::shared_ptr<Opm::EclIO::RestartFileView> rst_view,
                          Opm::SummaryState&                           smry)
    {
        const auto simStep = rst_view->simStep();

        auto udqs = UDQVectors { std::move(rst_view) };

        const auto groups = udqs.hasGroup()
            ? schedule.restart_groups(simStep)
            : std::vector<const Opm::Group*>{};

        const auto allWells = (udqs.hasWell() || udqs.hasSegment())
            ? schedule.wellNames(simStep)
            : std::vector<std::string>{};

        const auto msWells = udqs.hasSegment()
            ? multiSegmentWells(schedule[simStep], allWells)
            : std::vector<std::string>{};

        std::size_t i = 0;
        for (const auto& udq : udqs.zudn()) {
            if(i++ % 2) continue; // Odd elements are the UDQ unit strings

            switch (udq.front()) {
            case 'F':
                restoreFieldUDQValue(udqs, udq, smry);
                udqs.prepareNextFieldUDQ();
                break;

            case 'G':
                restoreGroupUDQValue(udqs, groups, udq, smry);
                udqs.prepareNextGroupUDQ();
                break;

            case 'S':
                restoreSegmentUDQValue(udqs, msWells, udq, smry);
                udqs.prepareNextSegmentUDQ();
                break;

            case 'W':
                restoreWellUDQValue(udqs, allWells, udq, smry);
                udqs.prepareNextWellUDQ();
                break;
            }
        }
    }

    void restore_cumulative(::Opm::SummaryState&                         smry,
                            const ::Opm::Schedule&                       schedule,
                            const Opm::TracerConfig&                     tracer_config,
                            std::shared_ptr<Opm::EclIO::RestartFileView> rst_view)
    {
        const auto  sim_step = rst_view->simStep();
        const auto& intehead = rst_view->intehead();

        smry.update_elapsed(schedule.seconds(rst_view->reportStep()));

        // Well cumulatives
        {
            const auto  wellData = WellVectors { intehead, rst_view };
            const auto& wells    = schedule.wellNames(sim_step);

            for (auto nWells = wells.size(), wellID = 0*nWells;
                 wellID < nWells; ++wellID)
            {
                assign_well_cumulatives(wells[wellID], wellID, schedule.runspec().tracers(), tracer_config, wellData, smry);
            }
        }

        // Group cumulatives, including FIELD.
        {
            const auto groupData = GroupVectors {
                intehead, std::move(rst_view)
            };

            for (const auto& gname : schedule.groupNames(sim_step)) {
                const auto& group = schedule.getGroup(gname, sim_step);
                // Note: Order of group values in {I,X}GRP arrays mostly
                // matches group's order of occurrence in .DATA file.
                // Values pertaining to FIELD are stored at zero-based order
                // index NGMAXZ (maximum number of groups in model).  The
                // latter value is groupData.maxGroups().
                //
                // As a final wrinkle, Flow internally stores FIELD at
                // insert_index() == 0, so subtract one to account for this
                // fact.  Max(insert_index(), 1) - 1 is just a bit of future
                // proofing and robustness in case that ever changes.
                const auto groupOrderIx = (gname == "FIELD")
                    ? groupData.maxGroups() // NGMAXZ -- Item 3 of WELLDIMS
                    : std::max(group.insert_index(), std::size_t{1}) - 1;

                assign_group_cumulatives(gname, groupOrderIx, groupData, smry);
            }
        }
    }
} // Anonymous namespace

namespace Opm::RestartIO  {

    RestartValue
    load(const std::string&             filename,
         int                            report_step,
         Action::State&                 /*  action_state  */,
         SummaryState&                  summary_state,
         const std::vector<RestartKey>& solution_keys,
         const EclipseState&            es,
         const EclipseGrid&             grid,
         const Schedule&                schedule,
         const std::vector<RestartKey>& extra_keys)
    {
        auto rst_view = std::make_shared<Opm::EclIO::RestartFileView>
            (std::make_shared<Opm::EclIO::ERst>(filename), report_step);

        auto xr = restoreSOLUTION(solution_keys, grid.getNumActive(), *rst_view);
        xr.convertToSI(es.getUnits());

        auto xw = restore_wells(es, grid, schedule, summary_state, rst_view);
        auto xgrp_nwrk = restore_grp_nwrk(schedule, es.getUnits(), rst_view);

        auto aquifers = hasAquifers(*rst_view)
            ? restore_aquifers(es, rst_view) : data::Aquifers{};

        auto rst_value = RestartValue {
            std::move(xr), std::move(xw), std::move(xgrp_nwrk), std::move(aquifers)
        };

        if (! extra_keys.empty()) {
            restoreExtra(extra_keys, es.getUnits(), *rst_view, rst_value);
        }

        if (rst_view->hasKeyword<std::string>("ZUDN")) {
            restoreUDQValues(schedule, rst_view, summary_state);
        }

        restore_cumulative(summary_state, schedule, es.tracer(), std::move(rst_view));

        return rst_value;
    }

    data::Solution
    load_solution_only(const std::string&             filename,
                       int                            report_step,
                       const std::vector<RestartKey>& solution_keys,
                       const EclipseState&            es,
                       const EclipseGrid&             grid)
    {
        auto rst_view = std::make_shared<Opm::EclIO::RestartFileView>
            (std::make_shared<Opm::EclIO::ERst>(filename), report_step);

        if (!rst_view->valid()) {
            return {};
        }

        auto sol =  restoreSOLUTION(solution_keys, grid.getNumActive(), *rst_view);
        sol.convertToSI(es.getUnits());

        return sol;
    }

} // Opm::RestartIO
