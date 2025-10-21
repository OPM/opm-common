/*
  Copyright 2021 Equinor ASA.
  Copyright 2019 Equinor ASA.
  Copyright 2016 Statoil ASA.

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

#include <opm/output/eclipse/Summary.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/KeywordLocation.hpp>
#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/AquiferCT.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/AquiferConfig.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/Aquifetp.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/input/eclipse/EclipseState/Phase.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

#include <opm/input/eclipse/Schedule/Action/Actions.hpp>
#include <opm/input/eclipse/Schedule/GasLiftOpt.hpp>
#include <opm/input/eclipse/Schedule/Group/GConSump.hpp>
#include <opm/input/eclipse/Schedule/Group/Group.hpp>
#include <opm/input/eclipse/Schedule/Group/GroupSatelliteInjection.hpp>
#include <opm/input/eclipse/Schedule/Group/GSatProd.hpp>
#include <opm/input/eclipse/Schedule/MSW/Segment.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQConfig.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQContext.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQParams.hpp>
#include <opm/input/eclipse/Schedule/VFPProdTable.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/io/eclipse/EclUtil.hpp>
#include <opm/io/eclipse/EclOutput.hpp>
#include <opm/io/eclipse/OutputStream.hpp>
#include <opm/io/eclipse/ExtSmryOutput.hpp>

#include <opm/output/data/Aquifer.hpp>
#include <opm/output/data/Groups.hpp>
#include <opm/output/data/GuideRateValue.hpp>
#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/Inplace.hpp>
#include <opm/output/eclipse/RegionCache.hpp>
#include <opm/output/eclipse/WStat.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <exception>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>

template <>
struct fmt::formatter<Opm::EclIO::SummaryNode::Category> : fmt::formatter<string_view>
{
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(Opm::EclIO::SummaryNode::Category c, FormatContext& ctx) const
    {
        using Category = Opm::EclIO::SummaryNode::Category;

        string_view name = "unknown";

        switch (c) {
        case Category::Well:          name = "Well";          break;
        case Category::Group:         name = "Group";         break;
        case Category::Field:         name = "Field";         break;
        case Category::Region:        name = "Region";        break;
        case Category::Block:         name = "Block";         break;
        case Category::Connection:    name = "Connection";    break;
        case Category::Completion:    name = "Completion";    break;
        case Category::Segment:       name = "Segment";       break;
        case Category::Aquifer:       name = "Aquifer";       break;
        case Category::Node:          name = "Node";          break;
        case Category::Miscellaneous: name = "Miscellaneous"; break;
        }

        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<Opm::EclIO::SummaryNode::Type> : fmt::formatter<string_view>
{
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(Opm::EclIO::SummaryNode::Type t, FormatContext& ctx) const
    {
        using Type = Opm::EclIO::SummaryNode::Type;

        string_view name = "unknown";

        switch (t) {
        case Type::Rate:      name = "Rate";      break;
        case Type::Total:     name = "Total";     break;
        case Type::Ratio:     name = "Ratio";     break;
        case Type::Pressure:  name = "Pressure";  break;
        case Type::Count:     name = "Count";     break;
        case Type::Mode:      name = "Mode";      break;
        case Type::ProdIndex: name = "PI/II";     break;
        case Type::Undefined: name = "Undefined"; break;
        }

        return formatter<string_view>::format(name, ctx);
    }
};

namespace {
    struct ParamCTorArgs
    {
        std::string kw;
        Opm::EclIO::SummaryNode::Type type;
    };

    std::vector<ParamCTorArgs> requiredRestartVectors()
    {
        using Type = ::Opm::EclIO::SummaryNode::Type;

        return {
            // Production
            ParamCTorArgs{ "OPR" , Type::Rate },
            ParamCTorArgs{ "WPR" , Type::Rate },
            ParamCTorArgs{ "GPR" , Type::Rate },
            ParamCTorArgs{ "VPR" , Type::Rate },
            ParamCTorArgs{ "OPP" , Type::Rate },
            ParamCTorArgs{ "WPP" , Type::Rate },
            ParamCTorArgs{ "GPP" , Type::Rate },
            ParamCTorArgs{ "OPT" , Type::Total },
            ParamCTorArgs{ "WPT" , Type::Total },
            ParamCTorArgs{ "GPT" , Type::Total },
            ParamCTorArgs{ "VPT" , Type::Total },
            ParamCTorArgs{ "OPTS", Type::Total },
            ParamCTorArgs{ "GPTS", Type::Total },
            ParamCTorArgs{ "OPTH", Type::Total },
            ParamCTorArgs{ "WPTH", Type::Total },
            ParamCTorArgs{ "GPTH", Type::Total },

            // Flow rate ratios (production)
            ParamCTorArgs{ "WCT" , Type::Ratio },
            ParamCTorArgs{ "GOR" , Type::Ratio },

            // injection
            ParamCTorArgs{ "OIR" , Type::Rate },
            ParamCTorArgs{ "WIR" , Type::Rate },
            ParamCTorArgs{ "GIR" , Type::Rate },
            ParamCTorArgs{ "VIR" , Type::Rate },
            ParamCTorArgs{ "OPI" , Type::Rate },
            ParamCTorArgs{ "WPI" , Type::Rate },
            ParamCTorArgs{ "GPI" , Type::Rate },
            ParamCTorArgs{ "OIT" , Type::Total },
            ParamCTorArgs{ "WIT" , Type::Total },
            ParamCTorArgs{ "GIT" , Type::Total },
            ParamCTorArgs{ "VIT" , Type::Total },
            ParamCTorArgs{ "WITH", Type::Total },
            ParamCTorArgs{ "GITH", Type::Total },
        };
    }

    std::vector<Opm::EclIO::SummaryNode>
    requiredRestartVectors(const ::Opm::Schedule& sched)
    {
        auto entities = std::vector<Opm::EclIO::SummaryNode> {};

        const auto vectors = requiredRestartVectors();
        const auto extra_well_vectors = std::vector<ParamCTorArgs> {
            { "WTHP",  Opm::EclIO::SummaryNode::Type::Pressure },
            { "WBHP",  Opm::EclIO::SummaryNode::Type::Pressure },
            { "WGVIR", Opm::EclIO::SummaryNode::Type::Rate     },
            { "WWVIR", Opm::EclIO::SummaryNode::Type::Rate     },
            { "WOPGR", Opm::EclIO::SummaryNode::Type::Rate     },
            { "WGPGR", Opm::EclIO::SummaryNode::Type::Rate     },
            { "WWPGR", Opm::EclIO::SummaryNode::Type::Rate     },
            { "WGIGR", Opm::EclIO::SummaryNode::Type::Rate     },
            { "WWIGR", Opm::EclIO::SummaryNode::Type::Rate     },
            { "WMCTL", Opm::EclIO::SummaryNode::Type::Mode     },
            { "WGLIR", Opm::EclIO::SummaryNode::Type::Rate     },

            { "WOPP",  Opm::EclIO::SummaryNode::Type::Pressure },
            { "WWPP",  Opm::EclIO::SummaryNode::Type::Pressure },

            { "WOPR",  Opm::EclIO::SummaryNode::Type::Rate     },
            { "WWPR",  Opm::EclIO::SummaryNode::Type::Rate     },
            { "WGPR",  Opm::EclIO::SummaryNode::Type::Rate     },
            { "WVPR",  Opm::EclIO::SummaryNode::Type::Rate     },

            { "WWCT",  Opm::EclIO::SummaryNode::Type::Ratio    },
            { "WGOR",  Opm::EclIO::SummaryNode::Type::Ratio    },
            //{ "WGCR",  Opm::EclIO::SummaryNode::Type::Ratio    },
            { "WGCT",  Opm::EclIO::SummaryNode::Type::Ratio    },

            { "WOPT",  Opm::EclIO::SummaryNode::Type::Total    },
            { "WWPT",  Opm::EclIO::SummaryNode::Type::Total    },
            { "WGPT",  Opm::EclIO::SummaryNode::Type::Total    },
            { "WVPT",  Opm::EclIO::SummaryNode::Type::Total    },

            { "WOPTS", Opm::EclIO::SummaryNode::Type::Total    },
            { "WGPTS", Opm::EclIO::SummaryNode::Type::Total    },

            { "WWIT",  Opm::EclIO::SummaryNode::Type::Total    },
            { "WGIT",  Opm::EclIO::SummaryNode::Type::Total    },
            { "WVIT",  Opm::EclIO::SummaryNode::Type::Total    },

            { "WGIMR", Opm::EclIO::SummaryNode::Type::Rate     },
            { "WGIMT", Opm::EclIO::SummaryNode::Type::Total    },

            { "WOPTH", Opm::EclIO::SummaryNode::Type::Total    },
            { "WWPTH", Opm::EclIO::SummaryNode::Type::Total    },
            { "WGPTH", Opm::EclIO::SummaryNode::Type::Total    },

            { "WWITH", Opm::EclIO::SummaryNode::Type::Total    },
            { "WGITH", Opm::EclIO::SummaryNode::Type::Total    },



        };
        const auto extra_group_vectors = std::vector<ParamCTorArgs> {
            { "GOPGR", Opm::EclIO::SummaryNode::Type::Rate },
            { "GGPGR", Opm::EclIO::SummaryNode::Type::Rate },
            { "GWPGR", Opm::EclIO::SummaryNode::Type::Rate },
            { "GGIGR", Opm::EclIO::SummaryNode::Type::Rate },
            { "GWIGR", Opm::EclIO::SummaryNode::Type::Rate },
            { "GMCTG", Opm::EclIO::SummaryNode::Type::Mode },
            { "GMCTP", Opm::EclIO::SummaryNode::Type::Mode },
            { "GMCTW", Opm::EclIO::SummaryNode::Type::Mode },
            { "GMWPR", Opm::EclIO::SummaryNode::Type::Mode },
            { "GMWIN", Opm::EclIO::SummaryNode::Type::Mode },
            { "GPR",   Opm::EclIO::SummaryNode::Type::Pressure },
            { "GGCR",   Opm::EclIO::SummaryNode::Type::Rate },
            { "GGIMR",   Opm::EclIO::SummaryNode::Type::Rate },
        };

        const auto extra_field_vectors = std::vector<ParamCTorArgs> {
            { "FMCTG", Opm::EclIO::SummaryNode::Type::Mode },
            { "FMCTP", Opm::EclIO::SummaryNode::Type::Mode },
            { "FMCTW", Opm::EclIO::SummaryNode::Type::Mode },
            { "FMWPR", Opm::EclIO::SummaryNode::Type::Mode },
            { "FMWIN", Opm::EclIO::SummaryNode::Type::Mode },
        };

        const auto extra_connection_vectors = std::vector<ParamCTorArgs> {
            {"COPR", Opm::EclIO::SummaryNode::Type::Rate},
            {"CWPR", Opm::EclIO::SummaryNode::Type::Rate},
            {"CGPR", Opm::EclIO::SummaryNode::Type::Rate},
            {"CVPR", Opm::EclIO::SummaryNode::Type::Rate},
            {"COPT", Opm::EclIO::SummaryNode::Type::Total},
            {"CWPT", Opm::EclIO::SummaryNode::Type::Total},
            {"CGPT", Opm::EclIO::SummaryNode::Type::Total},
            {"CVPT", Opm::EclIO::SummaryNode::Type::Total},
            {"COIR", Opm::EclIO::SummaryNode::Type::Rate},
            {"CWIR", Opm::EclIO::SummaryNode::Type::Rate},
            {"CGIR", Opm::EclIO::SummaryNode::Type::Rate},
            {"CVIR", Opm::EclIO::SummaryNode::Type::Rate},
            {"COIT", Opm::EclIO::SummaryNode::Type::Total},
            {"CWIT", Opm::EclIO::SummaryNode::Type::Total},
            {"CGIT", Opm::EclIO::SummaryNode::Type::Total},
            {"CVIT", Opm::EclIO::SummaryNode::Type::Total},
            {"CPR",  Opm::EclIO::SummaryNode::Type::Pressure},
            {"CGOR", Opm::EclIO::SummaryNode::Type::Ratio},
            {"CWCT", Opm::EclIO::SummaryNode::Type::Ratio},
        };

        using Cat = Opm::EclIO::SummaryNode::Category;

        auto makeEntities = [&vectors, &entities]
            (const char                        kwpref,
             const Cat                         cat,
             const std::vector<ParamCTorArgs>& extra_vectors,
             const std::string&                name) -> void
        {
            const auto dflt_num = Opm::EclIO::SummaryNode::default_number;

            std::string kwp(1, kwpref);
            auto toNode =
                [&kwp, &cat, &dflt_num, &name](const auto& vector) -> Opm::EclIO::SummaryNode
                {
                    return {kwp + vector.kw, cat,
                            vector.type, name, dflt_num, {}, {} };
                };

            // Recall: Cannot use emplace_back() for PODs.
            std::transform(vectors.begin(), vectors.end(),
                           std::back_inserter(entities), toNode);

            kwp = "";
            std::transform(extra_vectors.begin(), extra_vectors.end(),
                           std::back_inserter(entities), toNode);
        };

        for (const auto& well_name : sched.wellNames()) {
            makeEntities('W', Cat::Well, extra_well_vectors, well_name);

            const auto& well = sched.getWellatEnd(well_name);
            for (const auto& conn : well.getConnections()) {
                std::transform(extra_connection_vectors.begin(), extra_connection_vectors.end(),
                               std::back_inserter(entities),
                               [&well, &conn](const auto& conn_vector) -> Opm::EclIO::SummaryNode
                               {
                                   return {conn_vector.kw,
                                           Cat::Connection,
                                           conn_vector.type,
                                           well.name(),
                                           static_cast<int>(conn.global_index() + 1), {}, {}};
                               });
            }
        }

        for (const auto& grp_name : sched.groupNames()) {
            if (grp_name == "FIELD") { continue; }

            makeEntities('G', Cat::Group, extra_group_vectors, grp_name);
        }

        makeEntities('F', Cat::Field, extra_field_vectors, "FIELD");

        return entities;
    }

    std::vector<Opm::EclIO::SummaryNode>
    requiredSegmentVectors(const ::Opm::Schedule& sched)
    {
        auto entities = std::vector<Opm::EclIO::SummaryNode> {};

        const auto vectors = std::vector<ParamCTorArgs> {
            { "SOFR", Opm::EclIO::SummaryNode::Type::Rate     },
            { "SGFR", Opm::EclIO::SummaryNode::Type::Rate     },
            { "SWFR", Opm::EclIO::SummaryNode::Type::Rate     },
            { "SPR",  Opm::EclIO::SummaryNode::Type::Pressure },
            { "SPRDH",  Opm::EclIO::SummaryNode::Type::Pressure },
            { "SPRDF",  Opm::EclIO::SummaryNode::Type::Pressure },
            { "SPRDA",  Opm::EclIO::SummaryNode::Type::Pressure },
            { "SOHF",  Opm::EclIO::SummaryNode::Type::Ratio},
            { "SOFV",  Opm::EclIO::SummaryNode::Type::Undefined},
            { "SWHF",  Opm::EclIO::SummaryNode::Type::Ratio},
            { "SWFV",  Opm::EclIO::SummaryNode::Type::Undefined},
            { "SGHF",  Opm::EclIO::SummaryNode::Type::Ratio},
            { "SGFV",  Opm::EclIO::SummaryNode::Type::Undefined},
        };

        using Cat = Opm::EclIO::SummaryNode::Category;

        auto makeVectors = [&](const Opm::Well& well) -> void
        {
            const auto& wname = well.name();
            const auto  nSeg  = static_cast<int>(well.getSegments().size());

            for (auto segID = 0*nSeg + 1; segID <= nSeg; ++segID) {
                std::transform(vectors.begin(), vectors.end(),
                               std::back_inserter(entities),
                               [&wname, &segID](const auto& vector) -> Opm::EclIO::SummaryNode
                               {
                                   return {vector.kw, Cat::Segment,
                                           vector.type, wname, segID, {}, {} };
                               });
            }
        };

        for (const auto& wname : sched.wellNames()) {
            const auto& well = sched.getWellatEnd(wname);

            if (! well.isMultiSegment()) {
                // Don't allocate MS summary vectors for non-MS wells.
                continue;
            }

            makeVectors(well);
        }

        return entities;
    }

    std::vector<Opm::EclIO::SummaryNode>
    requiredAquiferVectors(const std::vector<int>& aquiferIDs)
    {
        auto entities = std::vector<Opm::EclIO::SummaryNode> {};

        const auto vectors = std::vector<ParamCTorArgs> {
            { "AAQR" , Opm::EclIO::SummaryNode::Type::Rate      },
            { "AAQP" , Opm::EclIO::SummaryNode::Type::Pressure  },
            { "AAQT" , Opm::EclIO::SummaryNode::Type::Total     },
            { "AAQTD", Opm::EclIO::SummaryNode::Type::Undefined },
            { "AAQPD", Opm::EclIO::SummaryNode::Type::Undefined },
        };

        using Cat = Opm::EclIO::SummaryNode::Category;

        for (const auto& aquiferID : aquiferIDs) {
            std::transform(vectors.begin(), vectors.end(),
                           std::back_inserter(entities),
                           [&aquiferID](const auto& vector) -> Opm::EclIO::SummaryNode
                           {
                               return {vector.kw, Cat::Aquifer,
                                       vector.type, "", aquiferID, {}, {}};
                           });
        }

        return entities;
    }

    std::vector<Opm::EclIO::SummaryNode>
    requiredNumericAquiferVectors(const std::vector<int>& aquiferIDs)
    {
        auto entities = std::vector<Opm::EclIO::SummaryNode> {};

        const auto vectors = std::vector<ParamCTorArgs> {
            { "ANQR" , Opm::EclIO::SummaryNode::Type::Rate     },
            { "ANQP" , Opm::EclIO::SummaryNode::Type::Pressure },
            { "ANQT" , Opm::EclIO::SummaryNode::Type::Total    },
        };

        using Cat = Opm::EclIO::SummaryNode::Category;

        for (const auto& aquiferID : aquiferIDs) {
            std::transform(vectors.begin(), vectors.end(),
                           std::back_inserter(entities),
                           [&aquiferID](const auto& vector) -> Opm::EclIO::SummaryNode
                           {
                               return {vector.kw, Cat::Aquifer,
                                       vector.type, "", aquiferID, {}, {}};
                           });
        }

        return entities;
    }

Opm::TimeStampUTC make_sim_time(const Opm::Schedule& sched, const Opm::SummaryState& st, double sim_step) {
    auto elapsed = st.get_elapsed() + sim_step;
    return Opm::TimeStampUTC( sched.getStartTime() )  + std::chrono::duration<double>(elapsed);
}



/*
 * This class takes simulator state and parser-provided information and
 * orchestrates ert to write simulation results as requested by the SUMMARY
 * section in eclipse. The implementation is somewhat compact as a lot of the
 * requested output may be similar-but-not-quite. Through various techniques
 * the compiler writes a lot of this code for us.
 */


using rt = Opm::data::Rates::opt;
using measure = Opm::UnitSystem::measure;
constexpr const bool injector = true;
constexpr const bool producer = false;

/* Some numerical value with its unit tag embedded to enable caller to apply
 * unit conversion. This removes a lot of boilerplate. ad-hoc solution to poor
 * unit support in general.
 */
measure div_unit( measure numer, measure denom ) {
    if( numer == measure::gas_surface_rate &&
        denom == measure::liquid_surface_rate )
        return measure::gas_oil_ratio;

    if( numer == measure::liquid_surface_rate &&
        denom == measure::gas_surface_rate )
        return measure::oil_gas_ratio;

    if( numer == measure::liquid_surface_rate &&
        denom == measure::liquid_surface_rate )
        return measure::water_cut;

    if( numer == measure::liquid_surface_volume &&
        denom == measure::time )
        return measure::liquid_surface_rate;

    if( numer == measure::gas_surface_volume &&
        denom == measure::time )
        return measure::gas_surface_rate;

    if( numer == measure::mass &&
        denom == measure::time )
        return measure::mass_rate;

    if( numer == measure::mass_rate &&
        denom == measure::liquid_surface_rate )
        return measure::concentration;

    if( numer == measure::energy &&
        denom == measure::time )
        return measure::energy_rate;

    return measure::identity;
}

measure mul_unit( measure lhs, measure rhs ) {
    if( lhs == rhs ) return lhs;

    if( ( lhs == measure::liquid_surface_rate && rhs == measure::time ) ||
        ( rhs == measure::liquid_surface_rate && lhs == measure::time ) )
        return measure::liquid_surface_volume;

    if( ( lhs == measure::gas_surface_rate && rhs == measure::time ) ||
        ( rhs == measure::gas_surface_rate && lhs == measure::time ) )
        return measure::gas_surface_volume;

    if( ( lhs == measure::rate && rhs == measure::time ) ||
        ( rhs == measure::rate && lhs == measure::time ) )
        return measure::volume;

    if(  lhs == measure::mass_rate && rhs == measure::time)
        return measure::mass;

    if(  lhs == measure::energy_rate && rhs == measure::time)
        return measure::energy;

    return lhs;
}

struct quantity {
    double value;
    Opm::UnitSystem::measure unit;

    quantity operator+( const quantity& rhs ) const {
        assert( this->unit == rhs.unit );
        return { this->value + rhs.value, this->unit };
    }

    quantity operator*( const quantity& rhs ) const {
        return { this->value * rhs.value, mul_unit( this->unit, rhs.unit ) };
    }

    quantity operator/( const quantity& rhs ) const {
        const auto res_unit = div_unit( this->unit, rhs.unit );

        if( rhs.value == 0 ) return { 0.0, res_unit };
        return { this->value / rhs.value, res_unit };
    }

    quantity operator/( double divisor ) const {
        if( divisor == 0 ) return { 0.0, this->unit };
        return { this->value / divisor , this->unit };
    }

    quantity& operator/=( double divisor ) {
        if( divisor == 0 )
            this->value = 0;
        else
            this->value /= divisor;

        return *this;
    }


    quantity operator-( const quantity& rhs) const {
        return { this->value - rhs.value, this->unit };
    }
};


/*
 * All functions must have the same parameters, so they're gathered in a struct
 * and functions use whatever information they care about.
 *
 * schedule_wells are wells from the deck, provided by opm-parser. active_index
 * is the index of the block in question. wells is simulation data.
 */
struct fn_args
{
    const std::vector<const Opm::Well*>& schedule_wells;
    const std::string group_name;
    const std::string keyword_name;
    double duration;
    const int sim_step;
    int  num;
    const std::optional<std::variant<std::string, int>> extra_data;
    const Opm::SummaryState& st;
    const Opm::data::Wells& wells;
    const Opm::data::WellBlockAveragePressures& wbp;
    const Opm::data::GroupAndNetworkValues& grp_nwrk;
    const Opm::out::RegionCache& regionCache;
    const Opm::EclipseGrid& grid;
    const Opm::Schedule& schedule;
    const std::vector< std::pair< std::string, double > > eff_factors;
    const std::optional<Opm::Inplace>& initial_inplace;
    const Opm::Inplace& inplace;
    const Opm::UnitSystem& unit_system;
};

/* Since there are several enums in opm scattered about more-or-less
 * representing the same thing. Since functions use template parameters to
 * expand into the actual implementations we need a static way to determine
 * what unit to tag the return value with.
 */
template< rt > constexpr
measure rate_unit() { return measure::liquid_surface_rate; }
template< Opm::Phase > constexpr
measure rate_unit() { return measure::liquid_surface_rate; }

template <Opm::data::GuideRateValue::Item>
measure rate_unit() { return measure::liquid_surface_rate; }

template<> constexpr
measure rate_unit< rt::gas >() { return measure::gas_surface_rate; }
template<> constexpr
measure rate_unit< Opm::Phase::GAS >() { return measure::gas_surface_rate; }

template<> constexpr
measure rate_unit< rt::dissolved_gas >() { return measure::gas_surface_rate; }

template<> constexpr
measure rate_unit< rt::solvent >() { return measure::gas_surface_rate; }

template<> constexpr
measure rate_unit< rt::reservoir_water >() { return measure::rate; }

template<> constexpr
measure rate_unit< rt::reservoir_oil >() { return measure::rate; }

template<> constexpr
measure rate_unit< rt::reservoir_gas >() { return measure::rate; }

template<> constexpr
measure rate_unit< rt::mass_gas >() { return measure::mass_rate; }

template<> constexpr
measure rate_unit< rt::mass_wat >() { return measure::mass_rate; }

template<> constexpr
measure rate_unit< rt::microbial >() { return measure::mass_rate; }

template<> constexpr
measure rate_unit< rt::oxygen >() { return measure::mass_rate; }

template<> constexpr
measure rate_unit< rt::urea >() { return measure::mass_rate; }

template<> constexpr
measure rate_unit < rt::productivity_index_water > () { return measure::liquid_productivity_index; }

template<> constexpr
measure rate_unit < rt::productivity_index_oil > () { return measure::liquid_productivity_index; }

template<> constexpr
measure rate_unit < rt::productivity_index_gas > () { return measure::gas_productivity_index; }

template<> constexpr
measure rate_unit< rt::well_potential_water >() { return measure::liquid_surface_rate; }

template<> constexpr
measure rate_unit< rt::well_potential_oil >() { return measure::liquid_surface_rate; }

template<> constexpr
measure rate_unit< rt::well_potential_gas >() { return measure::gas_surface_rate; }

template<> constexpr
measure rate_unit< rt::energy >() { return measure::energy_rate; }

template <> constexpr
measure rate_unit<Opm::data::GuideRateValue::Item::Gas>() { return measure::gas_surface_rate; }

template <> constexpr
measure rate_unit<Opm::data::GuideRateValue::Item::ResV>() { return measure::rate; }

template <Opm::data::WellControlLimits::Item>
[[nodiscard]] constexpr measure controlLimitUnit() noexcept
{
    return measure::liquid_surface_rate;
}

template <>
[[nodiscard]] constexpr measure controlLimitUnit<Opm::data::WellControlLimits::Item::Bhp>() noexcept
{
    return measure::pressure;
}

template <>
[[nodiscard]] constexpr measure controlLimitUnit<Opm::data::WellControlLimits::Item::GasRate>() noexcept
{
    return measure::gas_surface_rate;
}

template <>
[[nodiscard]] constexpr measure controlLimitUnit<Opm::data::WellControlLimits::Item::ResVRate>() noexcept
{
    return measure::rate;
}

double efac( const std::vector<std::pair<std::string,double>>& eff_factors, const std::string& name)
{
    auto it = std::find_if(eff_factors.begin(), eff_factors.end(),
        [&name](const std::pair<std::string, double>& elem)
    {
        return elem.first == name;
    });

    return (it != eff_factors.end()) ? it->second : 1.0;
}

inline bool
has_vfp_table(const Opm::ScheduleState&            sched_state,
              int vfp_table_number)
{
    return sched_state.vfpprod.has(vfp_table_number);
}

inline Opm::VFPProdTable::ALQ_TYPE
alq_type(const Opm::ScheduleState&            sched_state,
         int vfp_table_number)
{
    return sched_state.vfpprod(vfp_table_number).getALQType();
}

template <rt phase>
double satellite_prod(const Opm::SummaryState&  st,
                      const Opm::ScheduleState& sched,
                      const std::string&        group)
{
    using Rate = Opm::GSatProd::GSatProdGroupProp::Rate;

    const auto& gsatprod = sched.gsatprod();

    if (! gsatprod.has(group)) {
        return 0.0;
    }

    const auto gs = gsatprod.get(group, st);

    if constexpr (phase == rt::oil) {
        return gs.rate[Rate::Oil];
    }
    else if constexpr (phase == rt::gas) {
        return gs.rate[Rate::Gas];
    }
    else if constexpr (phase == rt::wat) {
        return gs.rate[Rate::Water];
    }

    return 0.0;
}

template <rt phase>
double satellite_inj(const Opm::ScheduleState& sched, const std::string& group)
{
    if (! sched.satelliteInjection.has(group)) {
        return 0.0;
    }

    auto irate = [&gsatinje = sched.satelliteInjection(group)](const Opm::Phase p)
    {
        const auto ix = gsatinje.rateIndex(p);
        if (! ix.has_value()) { return 0.0; }

        const auto& q = gsatinje[*ix].surface();

        return q.has_value() ? *q : 0.0;
    };

    if constexpr (phase == rt::oil) {
        return irate(Opm::Phase::OIL);
    }
    else if constexpr (phase == rt::gas) {
        return irate(Opm::Phase::GAS);
    }
    else if constexpr (phase == rt::wat) {
        return irate(Opm::Phase::WATER);
    }

    return 0.0;
}

inline double cumulativeSatelliteEffFactor(const Opm::ScheduleState& sched,
                                           const std::string&        gr_name)
{
    if (!sched.groups.has(gr_name)) { return 1.0; }

    auto isField = [](const std::string& group)
    { return group.empty() || (group == "FIELD"); };

    if (isField(gr_name)) { return 1.0; }

    const auto* grp = &sched.groups(gr_name);

    auto efac = grp->getGroupEfficiencyFactor();

    while (true) {
        const auto& parent = grp->flow_group();

        if (!parent.has_value() || isField(*parent)) {
            // No efficiency factor on FIELD.
            break;
        }

        grp = &sched.groups(*parent);

        efac *= grp->getGroupEfficiencyFactor();
    }

    return efac;
}

template <typename SatelliteRate>
double accum_groups(const Opm::ScheduleState& sched,
                    const std::string&        gr_name,
                    const double              efac,
                    SatelliteRate&&           sat_rate)
{
    if (! sched.groups.has(gr_name)) {
        return 0.0;
    }

    const auto& children = sched.groups(gr_name).groups();

    return efac * std::accumulate(children.begin(), children.end(),
                                  sat_rate(gr_name),
                                  [&sched, &sat_rate]
                                  (const double acc, const auto& child)
                                  {
                                      const auto efacDowntree = sched.groups(child)
                                          .getGroupEfficiencyFactor();

                                      return acc + accum_groups(sched, child,
                                                                efacDowntree,
                                                                sat_rate);
                                  });
}

template <rt phase, bool injection, bool cumulativeSatellite>
double satellite_rate(const fn_args& args)
{
    const auto& sched = args.schedule[args.sim_step];

    const auto satRate = [is_cumulative = cumulativeSatellite,
                          &sched, &gname = args.group_name]
        (const auto& group_sat_rate)
    {
        // Distinguish between the rate and cumulative cases when including
        // rates from satellite groups.  In particular, when computing
        // cumulatives, we need to include all efficiency factors imposed at
        // higher levels in the group tree.  Otherwise, we need only include
        // down-tree efficiency factors.

        const auto efac = !is_cumulative
            ? 1.0
            : cumulativeSatelliteEffFactor(sched, gname);

        return accum_groups(sched, gname, efac, group_sat_rate);
    };

    if (!injection && !sched.gsatprod().empty()) {
        // Down-tree satellite production rates.

        return satRate([&st = args.st, &sched](const std::string& gname)
        { return satellite_prod<phase>(st, sched, gname); });
    }
    else if (injection && (sched.satelliteInjection.size() > 0)) {
        // Down-tree satellite injection rates.

        return satRate([&sched](const std::string& gname)
        { return satellite_inj<phase>(sched, gname); });
    }

    // Neither satellite injection nor satellite production.  Rate is zero.
    return 0.0;
}

inline quantity artificial_lift_quantity( const fn_args& args ) {
    // Note: This function is intentionally supported only at the well level
    // (meaning there's no loop over args.schedule_wells by intention).  Its
    // purpose is to calculate WALQ only.

    // Note: in order to determine the correct dimension to use the Summary code
    // calls the various evaluator functions with a default constructed fn_args
    // instance. In the case of the WALQ function this does not really work,
    // because the correct output dimension depends on exactly what physical
    // quantity is represented by the ALQ - and that again requires quite some
    // context to determine correctly. The current hack is that if WLIFTOPT is
    // configured for at least one well we use dimension
    // measure::gas_surface_rate - otherwise we use measure::identity.

    auto dimension = measure::identity;
    const auto& glo = args.schedule[args.sim_step].glo();
    if (glo.num_wells() != 0)
        dimension = measure::gas_surface_rate;

    auto zero = quantity{0, dimension};
    if (args.schedule_wells.empty()) {
        return zero;
    }

    const auto* well = args.schedule_wells.front();
    if (well->isInjector()) {
        return zero;
    }

    auto xwPos = args.wells.find(well->name());
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
    {
        return zero;
    }

    const auto& production = well->productionControls(args.st);
    if (!glo.has_well(well->name()))
        return { production.alq_value, dimension};

    const auto& sched_state = args.schedule[args.sim_step];
    if (alq_type(sched_state, production.vfp_table_number) != Opm::VFPProdTable::ALQ_TYPE::ALQ_GRAT)
        return zero;

    const double eff_fac = efac(args.eff_factors, well->name());
    auto alq_rate = eff_fac * xwPos->second.rates.get(rt::alq, production.alq_value);
    return { alq_rate, dimension };
}

inline quantity glir( const fn_args& args ) {
    if (args.schedule_wells.empty()) {
        return { 0.0, measure::gas_surface_rate };
    }

    const auto& sched_state = args.schedule[args.sim_step];

    double alq_rate = 0.0;
    for (const auto* well : args.schedule_wells) {
        if (well->isInjector()) {
            continue;
        }

        auto xwPos = args.wells.find(well->name());
        if ((xwPos == args.wells.end()) ||
            (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
        {
            continue;
        }

        const auto& production = well->productionControls(args.st);
        if (! has_vfp_table(sched_state, production.vfp_table_number)) {
            const double eff_fac = efac(args.eff_factors, well->name());
            alq_rate += args.unit_system.to_si( measure::gas_surface_rate, eff_fac * xwPos->second.rates.get(rt::alq, production.alq_value ));
            continue;
        }

        const auto thisAlqType = alq_type(sched_state, production.vfp_table_number);
        if (thisAlqType == Opm::VFPProdTable::ALQ_TYPE::ALQ_GRAT) {
            const double eff_fac = efac(args.eff_factors, well->name());
            alq_rate += eff_fac * xwPos->second.rates.get(rt::alq, production.alq_value);
        }

        if (thisAlqType == Opm::VFPProdTable::ALQ_TYPE::ALQ_IGLR) {
            const double eff_fac = efac(args.eff_factors, well->name());
            auto glr = production.alq_value;
            auto wpr = xwPos->second.rates.get(rt::wat);
            auto opr = xwPos->second.rates.get(rt::oil);
            alq_rate -= eff_fac * glr * (wpr + opr);
        }
    }

    return { alq_rate, measure::gas_surface_rate };
}

template <rt phase, bool injection = true, bool cumulativeSatellite = false>
inline quantity rate(const fn_args& args)
{
    double sum = 0.0;

    for (const auto* sched_well : args.schedule_wells) {
        const auto& name = sched_well->name();

        auto xwPos = args.wells.find(name);
        if ((xwPos == args.wells.end()) ||
            (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
        {
            continue;
        }

        const double eff_fac = efac(args.eff_factors, name);
        const auto v = xwPos->second.rates.get(phase, 0.0) * eff_fac;

        if ((v > 0.0) == injection) {
            sum += v;
        }
    }

    if (! injection) {
        sum *= -1.0;
    }

    if (! args.group_name.empty()) {
        sum += satellite_rate<phase, injection, cumulativeSatellite>(args);
    }

    if ((phase == rt::polymer) || (phase == rt::brine)) {
        return { sum, measure::mass_rate };
    }

    return { sum, rate_unit< phase >() };
}

template <bool injection>
const Opm::data::Connection* findConnectionResults(const fn_args& args)
{
    if (args.schedule_wells.empty()) {
        // Typically in the first call which configures the summary nodes.
        return nullptr;
    }

    const auto xwPos = args.wells.find(args.schedule_wells.front()->name());
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT) ||
        (xwPos->second.current_control.isProducer == injection))
    {
        return nullptr;
    }

    return xwPos->second.find_connection(args.num - 1);
}

template <double Opm::data::ConnectionFracture::* q, measure unit, bool injection = true>
inline quantity fracture_connection_quantities(const fn_args& args)
{
    const auto* connection = findConnectionResults<injection>(args);

    return (connection == nullptr)
        ? quantity { 0.0, unit }
        : quantity { connection->fracture.*q, unit };
}

template <double Opm::data::ConnectionFiltrate::* q, measure unit, bool injection = true>
inline quantity filtrate_connection_quantities(const fn_args& args)
{
    const auto* connection = findConnectionResults<injection>(args);

    return (connection == nullptr)
        ? quantity { 0.0, unit }
        : quantity { connection->filtrate.*q, unit };
}

template <double Opm::data::WellFiltrate::* q, measure unit, bool injection = true>
inline quantity filtrate_well_quantities(const fn_args& args)
{
    const auto zero = quantity { 0.0, unit };

    if (args.schedule_wells.empty()) {
        // Typically in the first call which configures the summary nodes.
        return zero;
    }

    auto xwPos = args.wells.find(args.schedule_wells.front()->name());
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT) ||
        (xwPos->second.current_control.isProducer == injection))
    {
        return zero;
    }

    return { xwPos->second.filtrate.*q, unit };
}

template< rt tracer, rt phase, bool injection = true >
inline quantity ratetracer( const fn_args& args ) {
    double sum = 0.0;

    // All well-related tracer keywords, e.g. WTPCxx, WTPRxx, FTPTxx, have a 4-letter prefix length
    constexpr auto prefix_len = 4;
    std::string tracer_name = args.keyword_name.substr(prefix_len);

    for (const auto* sched_well : args.schedule_wells) {
        const auto& name = sched_well->name();

        auto xwPos = args.wells.find(name);
        if ((xwPos == args.wells.end()) ||
            (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
        {
            continue;
        }

        const double eff_fac = efac(args.eff_factors, name);
        const auto v = xwPos->second.rates.get(tracer, 0.0, tracer_name) * eff_fac;

        if ((v > 0.0) == injection) {
            sum += v;
        }
    }

    if (! injection) {
        sum *= -1.0;
    }

    return { sum, rate_unit< phase >() };
}

template< rt phase, bool injection = true >
inline quantity ratel( const fn_args& args ) {
    const auto unit = ((phase == rt::polymer) || (phase == rt::brine))
        ? measure::mass_rate : rate_unit<phase>();

    const quantity zero = { 0.0, unit };

    if (args.schedule_wells.empty())
        return zero;

    const auto* well = args.schedule_wells.front();
    const auto& name = well->name();

    auto xwPos = args.wells.find(name);
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT) ||
        (xwPos->second.current_control.isProducer == injection))
    {
        return zero;
    }

    const double eff_fac = efac(args.eff_factors, name);
    const auto& well_data = xwPos->second;

    double sum = 0;
    const auto& connections = well->getConnections( args.num );
    for (const auto* conn_ptr : connections) {
        const size_t global_index = conn_ptr->global_index();
        const auto& conn_data =
            std::find_if(well_data.connections.begin(),
                         well_data.connections.end(),
                [global_index](const Opm::data::Connection& cdata)
            {
                return cdata.index == global_index;
            });

        if (conn_data != well_data.connections.end()) {
            sum += conn_data->rates.get(phase, 0.0) * eff_fac;
        }
    }

    if (! injection) {
        sum *= -1;
    }

    return { sum, unit };
}

inline quantity cpr( const fn_args& args ) {
    const quantity zero = { 0, measure::pressure };
    // The args.num value is the literal value which will go to the
    // NUMS array in the eclipse SMSPEC file; the values in this array
    // are offset 1 - whereas we need to use this index here to look
    // up a connection with offset 0.
    const size_t global_index = args.num - 1;
    if (args.schedule_wells.empty())
        return zero;

    const auto& name = args.schedule_wells.front()->name();
    auto xwPos = args.wells.find(name);
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
        return zero;

    const auto& well_data = xwPos->second;
    const auto& connection =
        std::find_if(well_data.connections.begin(),
                     well_data.connections.end(),
            [global_index](const Opm::data::Connection& c)
        {
            return c.index == global_index;
        });

    if (connection == well_data.connections.end())
        return zero;

    return { connection->pressure, measure::pressure };
}

template< rt phase, bool injection = true >
inline quantity cratel( const fn_args& args ) {
    const auto unit = ((phase == rt::polymer) || (phase == rt::brine))
        ? measure::mass_rate : rate_unit<phase>();

    const quantity zero = { 0.0, unit };

    if (args.schedule_wells.empty())
        return zero;

    const auto* well = args.schedule_wells.front();
    const auto& name = well->name();

    auto xwPos = args.wells.find(name);
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT) ||
        (xwPos->second.current_control.isProducer == injection))
    {
        return zero;
    }

    const auto complnum = getCompletionNumberFromGlobalConnectionIndex(well->getConnections(), args.num - 1);
    if (!complnum.has_value())
        // Connection might not yet have come online.
        return zero;

    const auto& well_data = xwPos->second;
    const double eff_fac = efac(args.eff_factors, name);

    double sum = 0;
    const auto& connections = well->getConnections(*complnum);
    for (const auto& conn_ptr : connections) {
        const size_t global_index = conn_ptr->global_index();
        const auto& conn_data =
            std::find_if(well_data.connections.begin(),
                         well_data.connections.end(),
                [global_index] (const Opm::data::Connection& cdata)
            {
                return cdata.index == global_index;
            });

        if (conn_data != well_data.connections.end()) {
            sum += conn_data->rates.get( phase, 0.0 ) * eff_fac;
        }
    }

    if (! injection) {
        sum *= -1;
    }

    return { sum, unit };
}

template <Opm::data::ConnectionFracturing::Statistics Opm::data::ConnectionFracturing::* q,
          double Opm::data::ConnectionFracturing::Statistics::* stat,
          measure unit>
quantity connFracStatistics(const fn_args& args)
{
    const auto zero = quantity { 0.0, unit };

    if (args.schedule_wells.empty()) {
        return zero;
    }

    const auto& name = args.schedule_wells.front()->name();
    auto xwPos = args.wells.find(name);
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
    {
        return zero;
    }

    const auto global_index = static_cast<std::size_t>(args.num - 1);

    const auto& well_data = xwPos->second;
    const auto connPos =
        std::find_if(well_data.connections.begin(),
                     well_data.connections.end(),
            [global_index](const Opm::data::Connection& c)
        {
            return c.index == global_index;
        });

    if ((connPos == well_data.connections.end()) ||
        (connPos->fract.numCells == 0))
    {
        return zero;
    }

    return { connPos->fract.*q.*stat, unit };
}

template< bool injection >
inline quantity flowing( const fn_args& args ) {
    const auto& wells = args.wells;
    auto pred = [&wells]( const Opm::Well* w ) -> bool
    {
        auto xwPos = wells.find(w->name());
        return (xwPos != wells.end())
            && (w->isInjector( ) == injection)
            && (xwPos->second.dynamicStatus == Opm::Well::Status::OPEN)
            && xwPos->second.flowing();
    };

    return { double( std::count_if( args.schedule_wells.begin(),
                                    args.schedule_wells.end(),
                                    pred ) ),
             measure::identity };
}

template< rt phase, bool injection = true>
inline quantity crate( const fn_args& args ) {
    const quantity zero = { 0, rate_unit< phase >() };
    // The args.num value is the literal value which will go to the
    // NUMS array in the eclipse SMSPEC file; the values in this array
    // are offset 1 - whereas we need to use this index here to look
    // up a connection with offset 0.
    const size_t global_index = args.num - 1;
    if (args.schedule_wells.empty())
        return zero;

    const auto& name = args.schedule_wells.front()->name();
    auto xwPos = args.wells.find(name);
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT) ||
        (xwPos->second.current_control.isProducer == injection))
    {
        return zero;
    }

    const auto& well_data = xwPos->second;
    const auto& completion =
        std::find_if(well_data.connections.begin(),
                     well_data.connections.end(),
            [global_index](const Opm::data::Connection& c)
        {
            return c.index == global_index;
        });

    if (completion == well_data.connections.end())
        return zero;

    const double eff_fac = efac( args.eff_factors, name );
    auto v = completion->rates.get( phase, 0.0 ) * eff_fac;
    if (! injection)
        v *= -1;

    if (phase == rt::polymer || phase == rt::brine)
        return { v, measure::mass_rate };

    return { v, rate_unit< phase >() };
}

template <bool injection = true>
quantity crate_resv( const fn_args& args ) {
    const quantity zero = { 0.0, rate_unit<rt::reservoir_oil>() };
    if (args.schedule_wells.empty())
        return zero;

    const auto& name = args.schedule_wells.front()->name();
    auto xwPos = args.wells.find(name);
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT) ||
        (xwPos->second.current_control.isProducer == injection))
    {
        return zero;
    }

    // The args.num value is the literal value which will go to the
    // NUMS array in the eclipse SMSPEC file; the values in this array
    // are offset 1 - whereas we need to use this index here to look
    // up a connection with offset 0.
    const auto global_index = static_cast<std::size_t>(args.num - 1);

    const auto& well_data = xwPos->second;
    const auto completion =
        std::find_if(well_data.connections.begin(),
                     well_data.connections.end(),
            [global_index](const Opm::data::Connection& c)
        {
            return c.index == global_index;
        });

    if (completion == well_data.connections.end())
        return zero;

    const auto eff_fac = efac( args.eff_factors, name );
    auto v = completion->reservoir_rate * eff_fac;
    if (! injection)
        v *= -1;

    return { v, rate_unit<rt::reservoir_oil>() };
}

template <typename GetValue>
quantity segment_quantity(const fn_args& args,
                          const measure  m,
                          GetValue&&     getValue)
{
    const auto zero = quantity { 0.0, m };
    if (args.schedule_wells.empty()) {
        return zero;
    }

    const auto& name = args.schedule_wells.front()->name();
    auto xwPos = args.wells.find(name);
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
    {
        return zero;
    }

    const auto& well_data = xwPos->second;

    const auto segNumber = static_cast<std::size_t>(args.num);
    auto segPos = well_data.segments.find(segNumber);
    if (segPos == well_data.segments.end()) {
        return zero;
    }

    return { getValue(segPos->second), m };
}

template <Opm::data::SegmentPressures::Value ix>
inline quantity segpress(const fn_args& args)
{
    return segment_quantity(args, measure::pressure,
                            [](const Opm::data::Segment& segment)
                            {
                                return segment.pressures[ix];
                            });
}

template <rt phase>
inline quantity srate(const fn_args& args)
{
    const auto m = ((phase == rt::polymer) || (phase == rt::brine))
        ? measure::mass_rate
        : rate_unit<phase>();

    return segment_quantity(args, m,
        [&args](const Opm::data::Segment& segment)
    {
        // Note: Opposite flow rate sign conventions in Flow vs. ECLIPSE.
        return - segment.rates.get(phase, 0.0)
            * efac(args.eff_factors, args.schedule_wells.front()->name());
    });
}

template< rt tracer, rt phase>
inline quantity sratetracer(const fn_args& args)
{
    return segment_quantity(args, rate_unit<phase>(),
        [&args](const Opm::data::Segment& segment)
    {
        // Tracer-related keywords, STFRx and STFCx, have a 4-letter prefix length
        constexpr auto prefix_len = 4;
        std::string tracer_name = args.keyword_name.substr(prefix_len);

        return - segment.rates.get(tracer, 0.0, tracer_name)
            * efac(args.eff_factors, args.schedule_wells.front()->name());
    });
}

template <typename Items>
double segment_phase_quantity_value(const Opm::data::QuantityCollection<Items>& q,
                                    const typename Items::Item                  p)
{
    return q.has(p) ? q.get(p) : 0.0;
}

template <Opm::data::SegmentPhaseDensity::Item p>
quantity segment_density(const fn_args& args)
{
    return segment_quantity(args, measure::density,
        [](const Opm::data::Segment& segment)
    {
        return segment_phase_quantity_value(segment.density, p);
    });
}

template <Opm::data::SegmentPhaseQuantity::Item p>
quantity segment_flow_velocity(const fn_args& args)
{
    return segment_quantity(args, measure::pipeflow_velocity,
        [](const Opm::data::Segment& segment)
    {
        // Note: Opposite velocity sign conventions in Flow vs. ECLIPSE.
        return - segment_phase_quantity_value(segment.velocity, p);
    });
}

template <Opm::data::SegmentPhaseQuantity::Item p>
quantity segment_holdup_fraction(const fn_args& args)
{
    return segment_quantity(args, measure::identity,
        [](const Opm::data::Segment& segment)
    {
        return segment_phase_quantity_value(segment.holdup, p);
    });
}

template <Opm::data::SegmentPhaseQuantity::Item p>
quantity segment_viscosity(const fn_args& args)
{
    return segment_quantity(args, measure::viscosity,
        [](const Opm::data::Segment& segment)
    {
        return segment_phase_quantity_value(segment.viscosity, p);
    });
}

inline quantity trans_factors ( const fn_args& args ) {
    const quantity zero = { 0.0, measure::transmissibility };

    if (args.schedule_wells.empty())
        // No wells.  Before simulation starts?
        return zero;

    auto xwPos = args.wells.find(args.schedule_wells.front()->name());
    if (xwPos == args.wells.end())
        // No dynamic results for this well.  Not open?
        return zero;

    // Like connection rate we need to look up a connection with offset 0.
    const size_t global_index = args.num - 1;
    const auto& connections = xwPos->second.connections;
    auto connPos = std::find_if(connections.begin(), connections.end(),
        [global_index](const Opm::data::Connection& c)
    {
        return c.index == global_index;
    });

    if (connPos == connections.end())
        // No dynamic results for this connection.
        return zero;

    // Dynamic connection result's "trans_factor" includes PI-adjustment.
    return { connPos->trans_factor, measure::transmissibility };
}

inline quantity d_factors ( const fn_args& args ) {
    const quantity zero = { 0.0, measure::dfactor };

    if (args.schedule_wells.empty())
        // No wells.  Before simulation starts?
        return zero;

    auto xwPos = args.wells.find(args.schedule_wells.front()->name());
    if (xwPos == args.wells.end())
        // No dynamic results for this well.  Not open?
        return zero;

    // Like connection rate we need to look up a connection with offset 0.
    const size_t global_index = args.num - 1;
    const auto& connections = xwPos->second.connections;
    auto connPos = std::find_if(connections.begin(), connections.end(),
        [global_index](const Opm::data::Connection& c)
    {
        return c.index == global_index;
    });

    if (connPos == connections.end())
        // No dynamic results for this connection.
        return zero;

    // Connection "d_factor".
    return { connPos->d_factor, measure::dfactor };
}

inline quantity wstat( const fn_args& args ) {
    const quantity zero = { Opm::WStat::numeric::UNKNOWN, measure::identity};
    if (args.schedule_wells.empty())
        return zero;
    const auto& sched_well = args.schedule_wells.front();
    const auto& arg_well = args.wells.find(sched_well->name());

    if (arg_well == args.wells.end() || arg_well->second.dynamicStatus == Opm::Well::Status::SHUT)
        return {Opm::WStat::numeric::SHUT, measure::identity};

    if (arg_well->second.dynamicStatus == Opm::Well::Status::STOP)
        return {Opm::WStat::numeric::STOP, measure::identity};

    if (sched_well->isInjector())
        return {Opm::WStat::numeric::INJ, measure::identity};

    return {Opm::WStat::numeric::PROD, measure::identity};
}

inline quantity bhp( const fn_args& args ) {
    const quantity zero = { 0, measure::pressure };
    if (args.schedule_wells.empty())
        return zero;

    const auto p = args.wells.find(args.schedule_wells.front()->name());
    if ((p == args.wells.end()) ||
        (p->second.dynamicStatus == Opm::Well::Status::SHUT))
    {
        return zero;
    }

    return { p->second.bhp, measure::pressure };
}

/*
  This function is slightly ugly - the evaluation of ROEW uses the already
  calculated COPT results. We do not really have any formalism for such
  dependencies between the summary vectors. For this particualar case there is a
  hack in SummaryConfig which should ensure that this is safe.
*/

quantity roew(const fn_args& args) {
    const quantity zero = { 0, measure::identity };
    const auto& region_name = std::get<std::string>(*args.extra_data);
    if (!args.initial_inplace.has_value())
        return zero;

    const auto& initial_inplace = args.initial_inplace.value();
    if (!initial_inplace.has( region_name, Opm::Inplace::Phase::OIL, args.num))
        return zero;

    double oil_prod = 0;
    for (const auto& [well, global_index] : args.regionCache.connections(region_name, args.num)) {
        const auto copt_key = fmt::format("COPT:{}:{}" , well, global_index + 1);
        if (args.st.has(copt_key))
            oil_prod += args.st.get(copt_key);
    }
    oil_prod = args.unit_system.to_si(Opm::UnitSystem::measure::volume, oil_prod);
    return { oil_prod / initial_inplace.get( region_name, Opm::Inplace::Phase::OIL, args.num ) , measure::identity };
}

template <bool injection = true>
quantity temperature(const fn_args& args)
{
    const auto zero = quantity {
        // Note: We use to_si(0.0) to properly handle different temperature
        // scales.  This value will convert back to 0.0 of the appropriate
        // unit when we later call .from_si().  If a plain value 0.0 is
        // entered here, it will be treated as 0.0 K which is typically not
        // what we want in our output files (i.e., -273.15 C or -459.67 F).
        args.unit_system.to_si(measure::temperature, 0.0),
        measure::temperature
    };

    if (args.schedule_wells.empty()) {
        return zero;
    }

    const auto p = args.wells.find(args.schedule_wells.front()->name());
    if ((p == args.wells.end()) ||
        (p->second.dynamicStatus == Opm::Well::Status::SHUT) ||
        (p->second.current_control.isProducer == injection))
    {
        return zero;
    }

    return { p->second.temperature, measure::temperature };
}

inline quantity thp( const fn_args& args ) {
    const quantity zero = { 0, measure::pressure };
    if (args.schedule_wells.empty())
        return zero;

    const auto p = args.wells.find(args.schedule_wells.front()->name());
    if ((p == args.wells.end()) ||
        (p->second.dynamicStatus == Opm::Well::Status::SHUT))
    {
        return zero;
    }

    return { p->second.thp, measure::pressure };
}

inline quantity bhp_history( const fn_args& args ) {
    if( args.schedule_wells.empty() ) return { 0.0, measure::pressure };

    const auto* sched_well = args.schedule_wells.front();

    const auto bhp_hist = sched_well->isProducer()
        ? sched_well->getProductionProperties().BHPH
        : sched_well->getInjectionProperties() .BHPH;

    return { bhp_hist, measure::pressure };
}

inline quantity thp_history( const fn_args& args ) {
    if( args.schedule_wells.empty() ) return { 0.0, measure::pressure };

    const auto* sched_well = args.schedule_wells.front();

    const auto thp_hist = sched_well->isProducer()
        ? sched_well->getProductionProperties().THPH
        : sched_well->getInjectionProperties() .THPH;

    return { thp_hist, measure::pressure };
}

inline quantity node_pressure(const fn_args& args)
{
    auto nodePos = args.grp_nwrk.nodeData.find(args.group_name);
    if (nodePos == args.grp_nwrk.nodeData.end()) {
        return { 0.0, measure::pressure };
    }

    return { nodePos->second.pressure, measure::pressure };
}

inline quantity converged_node_pressure(const fn_args& args)
{
    auto nodePos = args.grp_nwrk.nodeData.find(args.group_name);
    if (nodePos == args.grp_nwrk.nodeData.end()) {
        return { 0.0, measure::pressure };
    }

    return { nodePos->second.converged_pressure, measure::pressure };
}

template <Opm::data::WellBlockAvgPress::Quantity wbp_quantity>
quantity well_block_average_pressure(const fn_args& args)
{
    // Note: This WBP evaluation function is supported only at the well
    // level.  There is intentionally no loop over args.schedule_wells.

    const quantity zero = { 0.0, measure::pressure };

    if (args.schedule_wells.empty())
        return zero;

    // No need to exclude status == SHUT here as the WBP quantity is well
    // defined for shut wells too.
    auto p = args.wbp.values.find(args.schedule_wells.front()->name());
    if (p == args.wbp.values.end()) {
        return zero;
    }

    return { p->second[wbp_quantity], measure::pressure };
}

template <Opm::Phase phase>
inline quantity production_history(const fn_args& args)
{
    // Looking up historical well production rates before simulation starts
    // or the well is flowing is meaningless.  We therefore default to
    // outputting zero in this case.

    double sum = 0.0;
    for (const auto* sched_well : args.schedule_wells) {
        const auto& name = sched_well->name();

        auto xwPos = args.wells.find(name);
        if ((xwPos == args.wells.end()) ||
            (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
        {
            // Well's not flowing.  Ignore contribution regardless of what's
            // in WCONHIST.
            continue;
        }

        const double eff_fac = efac(args.eff_factors, name);
        sum += sched_well->production_rate(args.st, phase) * eff_fac;
    }

    return { sum, rate_unit<phase>() };
}

template <Opm::Phase phase>
inline quantity injection_history(const fn_args& args)
{
    // Looking up historical well injection rates before simulation starts
    // or the well is flowing is meaningless.  We therefore default to
    // outputting zero in this case.

    double sum = 0.0;
    for (const auto* sched_well : args.schedule_wells) {
        const auto& name = sched_well->name();

        auto xwPos = args.wells.find(name);
        if ((xwPos == args.wells.end()) ||
            (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
        {
            // Well's not flowing.  Ignore contribution regardless of what's
            // in WCONINJH.
            continue;
        }

        const double eff_fac = efac(args.eff_factors, name);
        sum += sched_well->injection_rate(args.st, phase) * eff_fac;
    }

    return { sum, rate_unit<phase>() };
}

template< bool injection >
inline quantity abandoned_well( const fn_args& args ) {
    std::size_t count = 0;

    for (const auto* sched_well : args.schedule_wells) {
        if (injection && !sched_well->hasInjected())
            continue;

        if (!injection && !sched_well->hasProduced())
            continue;

        const auto& well_name = sched_well->name();
        auto well_iter = args.wells.find( well_name );
        if (well_iter == args.wells.end()) {
            count += 1;
            continue;
        }

        count += !well_iter->second.flowing();
    }

    return { 1.0 * count, measure::identity };
}

inline quantity res_vol_production_target( const fn_args& args )
{
    const double sum = std::accumulate(args.schedule_wells.begin(),
                                       args.schedule_wells.end(), 0.0,
                                       [](const auto acc, const auto& sched_well)
                                       {
                                           return
                                                sched_well->getProductionProperties().predictionMode
                                                ? acc + sched_well->getProductionProperties().ResVRate.SI_value_or(0.0)
                                                : acc;
                                       });

    return { sum, measure::rate };
}

inline quantity group_oil_production_target( const fn_args& args )
{
    const auto& groups = args.schedule[args.sim_step].groups;
    const double value = groups.has(args.group_name) ? groups.get(args.group_name).productionControls(args.st).oil_target : 0.0;

    return { value, measure::rate };
}

inline quantity group_gas_production_target( const fn_args& args )
{
    const auto& groups = args.schedule[args.sim_step].groups;
    const double value = groups.has(args.group_name) ? groups.get(args.group_name).productionControls(args.st).gas_target : 0.0;

    return { value, measure::rate };
}

inline quantity group_water_production_target( const fn_args& args )
{
    const auto& groups = args.schedule[args.sim_step].groups;
    const double value = groups.has(args.group_name) ? groups.get(args.group_name).productionControls(args.st).water_target : 0.0;

    return { value, measure::rate };
}

inline quantity group_liquid_production_target( const fn_args& args )
{
    const auto& groups = args.schedule[args.sim_step].groups;
    const double value = groups.has(args.group_name) ? groups.get(args.group_name).productionControls(args.st).liquid_target : 0.0;

    return { value, measure::rate };
}

inline quantity group_gas_injection_target( const fn_args& args )
{
    double value = 0.0;
    const auto& groups = args.schedule[args.sim_step].groups;
    if (groups.has(args.group_name)) {
        const auto& group = groups.get(args.group_name);
        if (group.hasInjectionControl(Opm::Phase::GAS))
            value = group.injectionControls(Opm::Phase::GAS, args.st).surface_max_rate;
    }

    return { value, measure::rate };
}

inline quantity group_water_injection_target( const fn_args& args )
{
    double value = 0.0;
    const auto& groups = args.schedule[args.sim_step].groups;
    if (groups.has(args.group_name)) {
        const auto& group = groups.get(args.group_name);
        if (group.hasInjectionControl(Opm::Phase::WATER))
            value = group.injectionControls(Opm::Phase::WATER, args.st).surface_max_rate;
    }

    return { value, measure::rate };
}

inline quantity group_res_vol_injection_target( const fn_args& args )
{
    double value = 0.0;
    const auto& groups = args.schedule[args.sim_step].groups;
    if (groups.has(args.group_name)) {
        const auto& group = groups.get(args.group_name);
        if (group.hasInjectionControl(Opm::Phase::GAS))
            value += group.injectionControls(Opm::Phase::GAS, args.st).resv_max_rate;
        if (group.hasInjectionControl(Opm::Phase::WATER))
            value += group.injectionControls(Opm::Phase::WATER, args.st).resv_max_rate;
    }

    return { value, measure::rate };
}



template <bool injection, Opm::data::WellControlLimits::Item i>
quantity well_control_limit(const fn_args& args)
{
    constexpr auto m = controlLimitUnit<i>();
    const auto zero = quantity { 0.0, m };

    if (args.schedule_wells.empty() ||
        ((i != Opm::data::WellControlLimits::Item::Bhp) &&
         (args.schedule_wells.front()->isProducer() == injection)))
    {
        return zero;
    }

    const auto& name = args.schedule_wells.front()->name();

    auto xwPos = args.wells.find(name);
    if (xwPos == args.wells.end()) {
        return zero;
    }

    return xwPos->second.limits.has(i)
        ? quantity { xwPos->second.limits.get(i), m }
        : zero;
}

inline quantity duration( const fn_args& args ) {
    return { args.duration, measure::time };
}

template<rt phase , bool injection>
quantity region_rate( const fn_args& args ) {
    double sum = 0;
    const auto& well_connections = args.regionCache.connections( std::get<std::string>(*args.extra_data), args.num );

    for (const auto& pair : well_connections) {

        double eff_fac = efac( args.eff_factors, pair.first );

        double Rate = args.wells.get( pair.first , pair.second , phase ) * eff_fac;

        // We are asking for the production rate in an injector - or
        // opposite. We just clamp to zero.
        if ((Rate > 0) != injection) {
            Rate = 0;
        }

        sum += Rate;
    }

    if( injection )
        return { sum, rate_unit< phase >() };
    else
        return { -sum, rate_unit< phase >() };
}

quantity rhpv(const fn_args& args) {
    const auto& inplace = args.inplace;
    const auto& region_name = std::get<std::string>(*args.extra_data);
    if (inplace.has( region_name, Opm::Inplace::Phase::HydroCarbonPV, args.num ))
        return { inplace.get( region_name, Opm::Inplace::Phase::HydroCarbonPV, args.num ), measure::volume };
    else
        return {0, measure::volume};
}

template < rt phase, bool outputProducer = true, bool outputInjector = true>
inline quantity potential_rate( const fn_args& args )
{
    double sum = 0.0;

    for (const auto* sched_well : args.schedule_wells) {
        const auto& name = sched_well->name();

        auto xwPos = args.wells.find(name);
        if ((xwPos == args.wells.end()) ||
            (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
        {
            continue;
        }

        if (sched_well->isInjector() && outputInjector) {
            const auto v = xwPos->second.rates.get(phase, 0.0);
            sum += v * efac(args.eff_factors, name);
        }
        else if (sched_well->isProducer() && outputProducer) {
            const auto v = xwPos->second.rates.get(phase, 0.0);
            sum += v * efac(args.eff_factors, name);
        }
    }

    return { sum, rate_unit< phase >() };
}

template <Opm::data::WellBlockAvgPress::Quantity wbp_quantity>
quantity well_block_average_prod_index(const fn_args& args)
{
    // Note: This WPIn evaluation function is supported only at the well
    // level.  There is intentionally no loop over args.schedule_wells.

    const auto zero = quantity { 0.0, rate_unit<rt::productivity_index_oil>() };

    if (args.schedule_wells.empty()) {
        return zero;
    }

    const auto* well = args.schedule_wells.front();

    auto xwPos = args.wells.find(well->name());
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
    {
        return zero;
    }

    auto p = args.wbp.values.find(well->name());
    if (p == args.wbp.values.end()) {
        return zero;
    }

    const auto& [rate_quant, unit] = [preferred_phase = well->getPreferredPhase()]()
    {
        switch (preferred_phase) {
        case Opm::Phase::GAS:
            return std::pair { rt::gas, rate_unit<rt::productivity_index_gas>() };

        case Opm::Phase::WATER:
            return std::pair { rt::wat, rate_unit<rt::productivity_index_water>() };

        default:
            return std::pair { rt::oil, rate_unit<rt::productivity_index_oil>() };
        }
    }();

    const auto q = xwPos->second.rates.get(rate_quant, 0.0)
        * efac(args.eff_factors, well->name());

    const auto dp = p->second[wbp_quantity] - xwPos->second.bhp;

    return { - q / dp, unit };
}

inline quantity preferred_phase_productivty_index(const fn_args& args)
{
    if (args.schedule_wells.empty())
        return {0.0, rate_unit<rt::productivity_index_oil>()};

    const auto* well = args.schedule_wells.front();
    const auto preferred_phase = well->getPreferredPhase();
    if (well->getStatus() == Opm::Well::Status::OPEN) {
        switch (preferred_phase) {
        case Opm::Phase::OIL:
            return potential_rate<rt::productivity_index_oil>(args);

        case Opm::Phase::GAS:
            return potential_rate<rt::productivity_index_gas>(args);

        case Opm::Phase::WATER:
            return potential_rate<rt::productivity_index_water>(args);

        default:
            break;
        }
    }
    else {
        switch (preferred_phase) {
        case Opm::Phase::OIL:
            return {0.0, rate_unit<rt::productivity_index_oil>()};

        case Opm::Phase::GAS:
            return {0.0, rate_unit<rt::productivity_index_gas>()};

        case Opm::Phase::WATER:
            return {0.0, rate_unit<rt::productivity_index_water>()};

        default:
            break;
        }
    }

    throw std::invalid_argument {
        fmt::format("Unsupported \"preferred\" phase: {}",
                    static_cast<int>(args.schedule_wells.front()->getPreferredPhase()))
    };
}

inline quantity connection_productivity_index(const fn_args& args)
{
    const quantity zero = { 0.0, rate_unit<rt::productivity_index_oil>() };

    if (args.schedule_wells.empty())
        return zero;

    auto xwPos = args.wells.find(args.schedule_wells.front()->name());
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
    {
        return zero;
    }

    // The args.num value is the literal value which will go to the
    // NUMS array in the eclipse SMSPEC file; the values in this array
    // are offset 1 - whereas we need to use this index here to look
    // up a connection with offset 0.
    const auto global_index = static_cast<std::size_t>(args.num) - 1;

    const auto& xcon = xwPos->second.connections;
    const auto& completion =
        std::find_if(xcon.begin(), xcon.end(),
            [global_index](const Opm::data::Connection& c)
        {
            return c.index == global_index;
        });

    if (completion == xcon.end())
        return zero;

    switch (args.schedule_wells.front()->getPreferredPhase()) {
    case Opm::Phase::OIL:
        return { completion->rates.get(rt::productivity_index_oil, 0.0),
                 rate_unit<rt::productivity_index_oil>() };

    case Opm::Phase::GAS:
        return { completion->rates.get(rt::productivity_index_gas, 0.0),
                 rate_unit<rt::productivity_index_gas>() };

    case Opm::Phase::WATER:
        return { completion->rates.get(rt::productivity_index_water, 0.0),
                 rate_unit<rt::productivity_index_water>() };

    default:
        break;
    }

    throw std::invalid_argument {
        fmt::format("Unsupported \"preferred\" phase: {}",
                    static_cast<int>(args.schedule_wells.front()->getPreferredPhase()))
    };
}

template < bool isGroup, bool Producer, bool waterInjector, bool gasInjector>
inline quantity group_control( const fn_args& args )
{
    std::string g_name = "";
    if (isGroup) {
        const quantity zero = { static_cast<double>(0), Opm::UnitSystem::measure::identity};
        if( args.group_name.empty() ) return zero;

        g_name = args.group_name;
    }
    else {
        g_name = "FIELD";
    }

    int cntl_mode = 0;

    // production control
    if (Producer) {
        auto it_g = args.grp_nwrk.groupData.find(g_name);
        if (it_g != args.grp_nwrk.groupData.end())
            cntl_mode = Opm::Group::ProductionCMode2Int(it_g->second.currentControl.currentProdConstraint);
    }
    // water injection control
    else if (waterInjector){
        auto it_g = args.grp_nwrk.groupData.find(g_name);
        if (it_g != args.grp_nwrk.groupData.end())
            cntl_mode = Opm::Group::InjectionCMode2Int(it_g->second.currentControl.currentWaterInjectionConstraint);
    }

    // gas injection control
    else if (gasInjector){
        auto it_g = args.grp_nwrk.groupData.find(g_name);
        if (it_g != args.grp_nwrk.groupData.end())
            cntl_mode = Opm::Group::InjectionCMode2Int(it_g->second.currentControl.currentGasInjectionConstraint);
    }

    return {static_cast<double>(cntl_mode), Opm::UnitSystem::measure::identity};
}

namespace {
    bool well_control_mode_defined(const ::Opm::data::Well& xw)
    {
        using PMode = ::Opm::Well::ProducerCMode;
        using IMode = ::Opm::Well::InjectorCMode;

        const auto& curr = xw.current_control;

        return (curr.isProducer && (curr.prod != PMode::CMODE_UNDEFINED))
            || (!curr.isProducer && (curr.inj != IMode::CMODE_UNDEFINED));
    }
}

inline quantity well_control_mode( const fn_args& args )
{
    const auto unit = Opm::UnitSystem::measure::identity;

    if (args.schedule_wells.empty()) {
        // No wells.  Possibly determining pertinent unit of measure
        // during SMSPEC configuration.
        return { 0.0, unit };
    }

    const auto* well = args.schedule_wells.front();
    auto xwPos = args.wells.find(well->name());
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT ||
         xwPos->second.dynamicStatus == Opm::Well::Status::STOP )) {
        // No dynamic results for 'well'.  Treat as shut/stopped.
        return { 0.0, unit };
    }

    if (! well_control_mode_defined(xwPos->second)) {
        // No dynamic control mode defined.  Use input control.
        const auto wmctl = Opm::Well::eclipseControlMode(*well, args.st);

        return { static_cast<double>(wmctl), unit };
    }

    // Well has simulator-provided active control mode.  Pick the
    // appropriate value depending on well type (producer/injector).
    const auto& curr = xwPos->second.current_control;
    const auto wmctl = curr.isProducer
        ? Opm::Well::eclipseControlMode(curr.prod)
        : Opm::Well::eclipseControlMode(curr.inj, well->injectorType());

    return { static_cast<double>(wmctl), unit };
}

template <Opm::data::GuideRateValue::Item i>
quantity guiderate_value(const ::Opm::data::GuideRateValue& grvalue)
{
    return { !grvalue.has(i) ? 0.0 : grvalue.get(i), rate_unit<i>() };
}

template <bool injection, Opm::data::GuideRateValue::Item i>
quantity group_guiderate(const fn_args& args)
{
    auto xgPos = args.grp_nwrk.groupData.find(args.group_name);
    if (xgPos == args.grp_nwrk.groupData.end()) {
        return { 0.0, rate_unit<i>() };
    }

    return injection
        ? guiderate_value<i>(xgPos->second.guideRates.injection)
        : guiderate_value<i>(xgPos->second.guideRates.production);
}

template <bool injection, Opm::data::GuideRateValue::Item i>
quantity well_guiderate(const fn_args& args)
{
    if (args.schedule_wells.empty()) {
        return { 0.0, rate_unit<i>() };
    }

    const auto* well = args.schedule_wells.front();
    if (well->isInjector() != injection) {
        return { 0.0, rate_unit<i>() };
    }

    auto xwPos = args.wells.find(well->name());
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
    {
        return { 0.0, rate_unit<i>() };
    }

    return guiderate_value<i>(xwPos->second.guide_rates);
}

quantity well_efficiency_factor(const fn_args& args)
{
    const auto zero = quantity { 0.0, measure::identity };

    if (args.schedule_wells.empty()) {
        return zero;
    }

    const auto* well = args.schedule_wells.front();

    auto xwPos = args.wells.find(well->name());
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
    {
        // Non-flowing wells have a zero efficiency factor
        return zero;
    }

    return { well->getEfficiencyFactor() *
             xwPos->second.efficiency_scaling_factor, measure::identity };
}

quantity well_efficiency_factor_grouptree(const fn_args& args)
{
    const auto zero = quantity { 0.0, measure::identity };

    if (args.schedule_wells.empty()) {
        return zero;
    }

    const auto* well = args.schedule_wells.front();

    auto xwPos = args.wells.find(well->name());
    if ((xwPos == args.wells.end()) ||
        (xwPos->second.dynamicStatus == Opm::Well::Status::SHUT))
    {
        // Non-flowing wells have a zero efficiency factor
        return zero;
    }

    auto factor = well->getEfficiencyFactor() * xwPos->second.efficiency_scaling_factor;
    auto parent = well->groupName();
    while (parent != "FIELD") {
        const auto& grp = args.schedule[args.sim_step].groups(parent);
        factor *= grp.getGroupEfficiencyFactor();

        const auto prnt = grp.control_group();
        parent = prnt.has_value() ? prnt.value() : "FIELD";
    }

    return { factor, measure::identity };
}

quantity group_efficiency_factor(const fn_args& args)
{
    const auto zero = quantity { 0.0, measure::identity };

    if (args.schedule_wells.empty()) {
        return zero;
    }
    const auto& sched = args.schedule[args.sim_step];
    const auto gefac = sched.groups(args.group_name).getGroupEfficiencyFactor();

    return { gefac, measure::identity };
}

double gconsump_rate(const std::string& gname,
                     const Opm::ScheduleState& schedule,
                     const Opm::SummaryState& st,
                     double Opm::GConSump::GCONSUMPGroupProp::* rate)
{
    double tot_rate = 0.0;
    if (schedule.groups.has(gname)) {
        for (const auto& child : schedule.groups(gname).groups()) {
            const auto fac = schedule.groups(child).getGroupEfficiencyFactor();
            tot_rate += fac * gconsump_rate(child, schedule, st, rate);
        }
    }

    if (const auto& gconsump = schedule.gconsump(); gconsump.has(gname)) {
        tot_rate += gconsump.get(gname, st).*rate;
    }

    return tot_rate;
}

quantity gas_consumption_rate(const fn_args& args)
{
    return {
        gconsump_rate(args.group_name,
                      args.schedule[args.sim_step], args.st,
                      &Opm::GConSump::GCONSUMPGroupProp::consumption_rate),
        measure::gas_surface_rate
    };
}

quantity gas_import_rate(const fn_args& args)
{
    return {
        gconsump_rate(args.group_name,
                      args.schedule[args.sim_step], args.st,
                      &Opm::GConSump::GCONSUMPGroupProp::import_rate),
        measure::gas_surface_rate
    };
}

/*
 * A small DSL, really poor man's function composition, to avoid massive
 * repetition when declaring the handlers for each individual keyword. bin_op
 * and its aliases will apply the pair of functions F and G that all take const
 * fn_args& and return quantity, making them composable.
 */
template< typename F, typename G, typename Op >
struct bin_op {
    bin_op( F fn, G gn = {} ) : f( fn ), g( gn ) {}
    quantity operator()( const fn_args& args ) const {
        return Op()( f( args ), g( args ) );
    }

    private:
        F f;
        G g;
};

template< typename F, typename G >
auto mul( F f, G g ) -> bin_op< F, G, std::multiplies< quantity > >
{ return { f, g }; }

template< typename F, typename G >
auto sum( F f, G g ) -> bin_op< F, G, std::plus< quantity > >
{ return { f, g }; }

template< typename F, typename G >
auto div( F f, G g ) -> bin_op< F, G, std::divides< quantity > >
{ return { f, g }; }

template< typename F, typename G >
auto sub( F f, G g ) -> bin_op< F, G, std::minus< quantity > >
{ return { f, g }; }

using ofun = std::function<quantity(const fn_args&)>;
using UnitTable = std::unordered_map<std::string, Opm::UnitSystem::measure>;

static const auto funs = std::unordered_map<std::string, ofun> {
    { "WWIR", rate< rt::wat, injector > },
    { "WOIR", rate< rt::oil, injector > },
    { "WGIR", rate< rt::gas, injector > },
    { "WEIR", rate< rt::energy, injector > },
    { "WTIRHEA", rate< rt::energy, injector > },
    { "WNIR", rate< rt::solvent, injector > },
    { "WCIR", rate< rt::polymer, injector > },
    { "WSIR", rate< rt::brine, injector > },
    // Allow phase specific interpretation of tracer related summary keywords
    { "WTIR#W", ratetracer< rt::tracer, rt::wat, injector > }, // #W: Water tracers
    { "WTIR#O", ratetracer< rt::tracer, rt::oil, injector > }, // #O: Oil tracers
    { "WTIR#G", ratetracer< rt::tracer, rt::gas, injector > }, // #G: Gas tracers
    { "WTIRF#W", ratetracer< rt::tracer, rt::wat, injector > }, // #W: Water tracers
    { "WTIRF#O", ratetracer< rt::tracer, rt::oil, injector > }, // #O: Oil tracers
    { "WTIRF#G", ratetracer< rt::tracer, rt::gas, injector > }, // #G: Gas tracers
    { "WTIRS#W", ratetracer< rt::tracer, rt::wat, injector > }, // #W: Water tracers
    { "WTIRS#O", ratetracer< rt::tracer, rt::oil, injector > }, // #O: Oil tracers
    { "WTIRS#G", ratetracer< rt::tracer, rt::gas, injector > }, // #G: Gas tracers
    { "WTIC#W", div( ratetracer< rt::tracer, rt::wat, injector >, rate< rt::wat, injector >) },
    { "WTIC#O", div( ratetracer< rt::tracer, rt::oil, injector >, rate< rt::oil, injector >) },
    { "WTIC#G", div( ratetracer< rt::tracer, rt::gas, injector >, rate< rt::gas, injector >) },
    { "WTICF#W", div( ratetracer< rt::tracer, rt::wat, injector >, rate< rt::wat, injector >) },
    { "WTICF#O", div( ratetracer< rt::tracer, rt::oil, injector >, rate< rt::oil, injector >) },
    { "WTICF#G", div( ratetracer< rt::tracer, rt::gas, injector >, rate< rt::gas, injector >) },
    { "WTICS#W", div( ratetracer< rt::tracer, rt::wat, injector >, rate< rt::wat, injector >) },
    { "WTICS#O", div( ratetracer< rt::tracer, rt::oil, injector >, rate< rt::oil, injector >) },
    { "WTICS#G", div( ratetracer< rt::tracer, rt::gas, injector >, rate< rt::gas, injector >) },
    { "WVIR", sum( sum( rate< rt::reservoir_water, injector >, rate< rt::reservoir_oil, injector > ),
                       rate< rt::reservoir_gas, injector > ) },
    { "WGIGR", well_guiderate<injector, Opm::data::GuideRateValue::Item::Gas> },
    { "WWIGR", well_guiderate<injector, Opm::data::GuideRateValue::Item::Water> },

    { "WWIT", mul( rate< rt::wat, injector >, duration ) },
    { "WOIT", mul( rate< rt::oil, injector >, duration ) },
    { "WGIT", mul( rate< rt::gas, injector >, duration ) },
    { "WEIT", mul( rate< rt::energy, injector >, duration ) },
    { "WTITHEA", mul( rate< rt::energy, injector >, duration ) },
    { "WNIT", mul( rate< rt::solvent, injector >, duration ) },
    { "WCIT", mul( rate< rt::polymer, injector >, duration ) },
    { "WSIT", mul( rate< rt::brine, injector >, duration ) },
    { "WTIT#W", mul( ratetracer< rt::tracer, rt::wat, injector >, duration ) },
    { "WTIT#O", mul( ratetracer< rt::tracer, rt::oil, injector >, duration ) },
    { "WTIT#G", mul( ratetracer< rt::tracer, rt::gas, injector >, duration ) },
    { "WTITF#W", mul( ratetracer< rt::tracer, rt::wat, injector >, duration ) },
    { "WTITF#O", mul( ratetracer< rt::tracer, rt::oil, injector >, duration ) },
    { "WTITF#G", mul( ratetracer< rt::tracer, rt::gas, injector >, duration ) },
    { "WTITS#W", mul( ratetracer< rt::tracer, rt::wat, injector >, duration ) },
    { "WTITS#O", mul( ratetracer< rt::tracer, rt::oil, injector >, duration ) },
    { "WTITS#G", mul( ratetracer< rt::tracer, rt::gas, injector >, duration ) },
    { "WVIT", mul( sum( sum( rate< rt::reservoir_water, injector >, rate< rt::reservoir_oil, injector > ),
                        rate< rt::reservoir_gas, injector > ), duration ) },

    { "WWPR", rate< rt::wat, producer > },
    { "WOPR", rate< rt::oil, producer > },
    { "WWPTL",mul(ratel< rt::wat, producer >, duration) },
    { "WGPTL",mul(ratel< rt::gas, producer >, duration) },
    { "WOPTL",mul(ratel< rt::oil, producer >, duration) },
    { "WWPRL",ratel< rt::wat, producer > },
    { "WGPRL",ratel< rt::gas, producer > },
    { "WOPRL",ratel< rt::oil, producer > },
    { "WOFRL",ratel< rt::oil, producer > },
    { "WWIRL",ratel< rt::wat, injector> },
    { "WWITL",mul(ratel< rt::wat, injector>, duration) },
    { "WGIRL",ratel< rt::gas, injector> },
    { "WGITL",mul(ratel< rt::gas, injector>, duration) },
    { "WLPTL",mul( sum(ratel<rt::wat, producer>, ratel<rt::oil, producer>), duration)},
    { "WWCTL", div( ratel< rt::wat, producer >,
                    sum( ratel< rt::wat, producer >, ratel< rt::oil, producer > ) ) },
    { "WGORL", div( ratel< rt::gas, producer >, ratel< rt::oil, producer > ) },
    { "WGPR", rate< rt::gas, producer > },
    { "WEPR", rate< rt::energy, producer > },
    { "WTPRHEA", rate< rt::energy, producer > },
    { "WGLIR", glir},
    { "WGLIT", mul ( glir, duration ) },
    { "WALQ", artificial_lift_quantity },
    { "WNPR", rate< rt::solvent, producer > },
    { "WCPR", rate< rt::polymer, producer > },
    { "WSPR", rate< rt::brine, producer > },
    { "WTPR#W", ratetracer< rt::tracer, rt::wat, producer > },
    { "WTPR#O", ratetracer< rt::tracer, rt::oil, producer > },
    { "WTPR#G", ratetracer< rt::tracer, rt::gas, producer > },
    { "WTPRF#W", ratetracer< rt::tracer, rt::wat, producer> },
    { "WTPRF#O", ratetracer< rt::tracer, rt::oil, producer> },
    { "WTPRF#G", ratetracer< rt::tracer, rt::gas, producer> },
    { "WTPRS#W", ratetracer< rt::tracer, rt::wat, producer> },
    { "WTPRS#O", ratetracer< rt::tracer, rt::oil, producer> },
    { "WTPRS#G", ratetracer< rt::tracer, rt::gas, producer> },
    { "WTPC#W", div( ratetracer< rt::tracer, rt::wat, producer >, rate< rt::wat, producer >) },
    { "WTPC#O", div( ratetracer< rt::tracer, rt::oil, producer >, rate< rt::oil, producer >) },
    { "WTPC#G", div( ratetracer< rt::tracer, rt::gas, producer >, rate< rt::gas, producer >) },
    { "WTPCF#W", div( ratetracer< rt::tracer, rt::wat, producer >, rate< rt::wat, producer >) },
    { "WTPCF#O", div( ratetracer< rt::tracer, rt::oil, producer >, rate< rt::oil, producer >) },
    { "WTPCF#G", div( ratetracer< rt::tracer, rt::gas, producer >, rate< rt::gas, producer >) },
    { "WTPCS#W", div( ratetracer< rt::tracer, rt::wat, producer >, rate< rt::wat, producer >) },
    { "WTPCS#O", div( ratetracer< rt::tracer, rt::oil, producer >, rate< rt::oil, producer >) },
    { "WTPCS#G", div( ratetracer< rt::tracer, rt::gas, producer >, rate< rt::gas, producer >) },
    { "WCPC", div( rate< rt::polymer, producer >, rate< rt::wat, producer >) },
    { "WSPC", div( rate< rt::brine, producer >, rate< rt::wat, producer >) },

    { "WOPGR", well_guiderate<producer, Opm::data::GuideRateValue::Item::Oil> },
    { "WGPGR", well_guiderate<producer, Opm::data::GuideRateValue::Item::Gas> },
    { "WWPGR", well_guiderate<producer, Opm::data::GuideRateValue::Item::Water> },
    { "WVPGR", well_guiderate<producer, Opm::data::GuideRateValue::Item::ResV> },

    { "WGPRS", rate< rt::dissolved_gas, producer > },
    { "WGPRF", sub( rate< rt::gas, producer >, rate< rt::dissolved_gas, producer > ) },
    { "WOPRS", rate< rt::vaporized_oil, producer > },
    { "WOPRF", sub (rate < rt::oil, producer >, rate< rt::vaporized_oil, producer > )  },
    { "WVPR", sum( sum( rate< rt::reservoir_water, producer >, rate< rt::reservoir_oil, producer > ),
                   rate< rt::reservoir_gas, producer > ) },
    { "WGVPR", rate< rt::reservoir_gas, producer > },

    { "WLPR", sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) },
    { "WWPT", mul( rate< rt::wat, producer >, duration ) },
    { "WOPT", mul( rate< rt::oil, producer >, duration ) },
    { "WGPT", mul( rate< rt::gas, producer >, duration ) },
    { "WEPT", mul( rate< rt::energy, producer >, duration ) },
    { "WTPTHEA", mul( rate< rt::energy, producer >, duration ) },
    { "WNPT", mul( rate< rt::solvent, producer >, duration ) },
    { "WCPT", mul( rate< rt::polymer, producer >, duration ) },
    { "WSPT", mul( rate< rt::brine, producer >, duration ) },
    { "WTPT#W", mul( ratetracer< rt::tracer, rt::wat, producer >, duration ) },
    { "WTPT#O", mul( ratetracer< rt::tracer, rt::oil, producer >, duration ) },
    { "WTPT#G", mul( ratetracer< rt::tracer, rt::gas, producer >, duration ) },
    { "WTPTF#W", mul( ratetracer< rt::tracer, rt::wat, producer >, duration ) },
    { "WTPTF#O", mul( ratetracer< rt::tracer, rt::oil, producer >, duration ) },
    { "WTPTF#G", mul( ratetracer< rt::tracer, rt::gas, producer >, duration ) },
    { "WTPTS#W", mul( ratetracer< rt::tracer, rt::wat, producer >, duration ) },
    { "WTPTS#O", mul( ratetracer< rt::tracer, rt::oil, producer >, duration ) },
    { "WTPTS#G", mul( ratetracer< rt::tracer, rt::gas, producer >, duration ) },
    { "WLPT", mul( sum( rate< rt::wat, producer >, rate< rt::oil, producer > ),
                   duration ) },
    { "WGPTS", mul( rate< rt::dissolved_gas, producer >, duration )},
    { "WGPTF", sub( mul( rate< rt::gas, producer >, duration ),
                        mul( rate< rt::dissolved_gas, producer >, duration ))},
    { "WOPTS", mul( rate< rt::vaporized_oil, producer >, duration )},
    { "WOPTF", sub( mul( rate< rt::oil, producer >, duration ),
                        mul( rate< rt::vaporized_oil, producer >, duration ))},
    { "WVPT", mul( sum( sum( rate< rt::reservoir_water, producer >, rate< rt::reservoir_oil, producer > ),
                        rate< rt::reservoir_gas, producer > ), duration ) },

    { "WWCT", div( rate< rt::wat, producer >,
                   sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) ) },
    { "GWCT", div( rate< rt::wat, producer >,
                   sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) ) },
    { "WGOR", div( rate< rt::gas, producer >, rate< rt::oil, producer > ) },
    { "WOGR", div( rate< rt::oil, producer >, rate< rt::gas, producer > ) },
    { "WWGR", div( rate< rt::wat, producer >, rate< rt::gas, producer > ) },
    { "GGOR", div( rate< rt::gas, producer >, rate< rt::oil, producer > ) },
    { "WGLR", div( rate< rt::gas, producer >,
                   sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) ) },

    { "WSTAT", wstat },
    { "WBHP", bhp },
    { "WTHP", thp },

    // Well level filter cake quantities (OPM extension)
    { "WINJFVR", filtrate_well_quantities<&Opm::data::WellFiltrate::rate,
      measure::geometric_volume_rate, injector> },
    { "WINJFVT", filtrate_well_quantities<&Opm::data::WellFiltrate::total,
      measure::geometric_volume, injector> },
    { "WINJFC", filtrate_well_quantities<&Opm::data::WellFiltrate::concentration,
      measure::ppm, injector> },

    { "WBP" , well_block_average_pressure<Opm::data::WellBlockAvgPress::Quantity::WBP>  },
    { "WBP4", well_block_average_pressure<Opm::data::WellBlockAvgPress::Quantity::WBP4> },
    { "WBP5", well_block_average_pressure<Opm::data::WellBlockAvgPress::Quantity::WBP5> },
    { "WBP9", well_block_average_pressure<Opm::data::WellBlockAvgPress::Quantity::WBP9> },
    { "WTPCHEA", temperature< producer >},
    { "WTICHEA", temperature< injector >},

    { "WBHPT", well_control_limit<producer, Opm::data::WellControlLimits::Item::Bhp> },
    { "WOIRT", well_control_limit<injector, Opm::data::WellControlLimits::Item::OilRate> },
    { "WOPRT", well_control_limit<producer, Opm::data::WellControlLimits::Item::OilRate> },
    { "WWIRT", well_control_limit<injector, Opm::data::WellControlLimits::Item::WaterRate> },
    { "WWPRT", well_control_limit<producer, Opm::data::WellControlLimits::Item::WaterRate> },
    { "WGIRT", well_control_limit<injector, Opm::data::WellControlLimits::Item::GasRate> },
    { "WGPRT", well_control_limit<producer, Opm::data::WellControlLimits::Item::GasRate> },
    { "WVIRT", well_control_limit<injector, Opm::data::WellControlLimits::Item::ResVRate> },
    { "WVPRT", well_control_limit<producer, Opm::data::WellControlLimits::Item::ResVRate> },
    { "WLPRT", well_control_limit<producer, Opm::data::WellControlLimits::Item::LiquidRate> },

    { "WMCTL", well_control_mode },

    { "GWIR", rate< rt::wat, injector > },
    { "WGVIR", rate< rt::reservoir_gas, injector >},
    { "WWVIR", rate< rt::reservoir_water, injector >},
    { "GOIR", rate< rt::oil, injector > },
    { "GGIR", rate< rt::gas, injector > },
    { "GEIR", rate< rt::energy, injector > },
    { "GTIRHEA", rate< rt::energy, injector > },
    { "GNIR", rate< rt::solvent, injector > },
    { "GCIR", rate< rt::polymer, injector > },
    { "GSIR", rate< rt::brine, injector > },
    { "GVIR", sum( sum( rate< rt::reservoir_water, injector >, rate< rt::reservoir_oil, injector > ),
                        rate< rt::reservoir_gas, injector > ) },

    { "GGIGR", group_guiderate<injector, Opm::data::GuideRateValue::Item::Gas> },
    { "GWIGR", group_guiderate<injector, Opm::data::GuideRateValue::Item::Water> },

    { "GWIT", mul(rate<rt::wat, injector, /* cumulativeSatellite = */ true>, duration) },
    { "GOIT", mul(rate<rt::oil, injector, /* cumulativeSatellite = */ true>, duration) },
    { "GGIT", mul(rate<rt::gas, injector, /* cumulativeSatellite = */ true>, duration) },

    { "GEIT", mul( rate< rt::energy, injector >, duration ) },
    { "GTITHEA", mul( rate< rt::energy, injector >, duration ) },
    { "GNIT", mul( rate< rt::solvent, injector >, duration ) },
    { "GCIT", mul( rate< rt::polymer, injector >, duration ) },
    { "GSIT", mul( rate< rt::brine, injector >, duration ) },
    { "GVIT", mul( sum( sum( rate< rt::reservoir_water, injector >, rate< rt::reservoir_oil, injector > ),
                        rate< rt::reservoir_gas, injector > ), duration ) },

    { "GWPR", rate< rt::wat, producer > },
    { "GOPR", rate< rt::oil, producer > },
    { "GGPR", rate< rt::gas, producer > },
    { "GEPR", rate< rt::energy, producer > },
    { "GTPRHEA", rate< rt::energy, producer > },
    { "GGLIR", glir },
    { "GGLIT", mul( glir, duration) },
    { "GNPR", rate< rt::solvent, producer > },
    { "GCPR", rate< rt::polymer, producer > },
    { "GSPR", rate< rt::brine, producer > },
    { "GCPC", div( rate< rt::polymer, producer >, rate< rt::wat, producer >) },
    { "GSPC", div( rate< rt::brine, producer >, rate< rt::wat, producer >) },
    { "GOPRS", rate< rt::vaporized_oil, producer > },
    { "GOPRF", sub (rate < rt::oil, producer >, rate< rt::vaporized_oil, producer > ) },
    { "GLPR", sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) },
    { "GVPR", sum( sum( rate< rt::reservoir_water, producer >, rate< rt::reservoir_oil, producer > ),
                        rate< rt::reservoir_gas, producer > ) },

    { "GOPGR", group_guiderate<producer, Opm::data::GuideRateValue::Item::Oil> },
    { "GGPGR", group_guiderate<producer, Opm::data::GuideRateValue::Item::Gas> },
    { "GWPGR", group_guiderate<producer, Opm::data::GuideRateValue::Item::Water> },
    { "GVPGR", group_guiderate<producer, Opm::data::GuideRateValue::Item::ResV> },

    { "GGCR", gas_consumption_rate },
    { "GGCT", mul( mul( gas_consumption_rate, group_efficiency_factor ), duration ) },
    { "GGIMR", gas_import_rate },
    { "GGIMT", mul( mul( gas_import_rate, group_efficiency_factor ), duration ) },

    { "GSGR", sum(  sub( rate< rt::gas, producer >, rate< rt::gas, injector > ),
                    sub( gas_import_rate, gas_consumption_rate ) ) },
    { "GGSR", sum(  sub( rate< rt::gas, producer >, rate< rt::gas, injector > ),
                    sub( gas_import_rate, gas_consumption_rate ) ) },
    { "GSGT", mul( mul( sum(  sub( rate< rt::gas, producer >, rate< rt::gas, injector > ),
                    sub( gas_import_rate, gas_consumption_rate ) ), group_efficiency_factor ), duration ) },
    { "GGST", mul( mul( sum(  sub( rate< rt::gas, producer >, rate< rt::gas, injector > ),
                    sub( gas_import_rate, gas_consumption_rate ) ), group_efficiency_factor ), duration ) },


    { "GPR", node_pressure },
    { "NPR", converged_node_pressure },
    { "GNETPR", converged_node_pressure },

    { "GWPT", mul(rate<rt::wat, producer, /* cumulativeSatellite = */ true>, duration) },
    { "GOPT", mul(rate<rt::oil, producer, /* cumulativeSatellite = */ true>, duration) },
    { "GGPT", mul(rate<rt::gas, producer, /* cumulativeSatellite = */ true>, duration) },

    { "GEPT", mul( rate< rt::energy, producer >, duration ) },
    { "GTPTHEA", mul( rate< rt::energy, producer >, duration ) },
    { "GNPT", mul( rate< rt::solvent, producer >, duration ) },
    { "GCPT", mul( rate< rt::polymer, producer >, duration ) },
    { "GOPTS", mul( rate< rt::vaporized_oil, producer >, duration ) },
    { "GOPTF", mul( sub (rate < rt::oil, producer >,
                         rate< rt::vaporized_oil, producer > ),
                    duration ) },
    { "GLPT", mul( sum( rate< rt::wat, producer >, rate< rt::oil, producer > ),
                   duration ) },
    { "GVPT", mul( sum( sum( rate< rt::reservoir_water, producer >, rate< rt::reservoir_oil, producer > ),
                        rate< rt::reservoir_gas, producer > ), duration ) },
    // Group potential
    { "GWPP", potential_rate< rt::well_potential_water , true, false>},
    { "GOPP", potential_rate< rt::well_potential_oil , true, false>},
    { "GGPP", potential_rate< rt::well_potential_gas , true, false>},
    { "GWPI", potential_rate< rt::well_potential_water , false, true>},
    { "GOPI", potential_rate< rt::well_potential_oil , false, true>},
    { "GGPI", potential_rate< rt::well_potential_gas , false, true>},

    //Group control mode
    { "GMCTP", group_control< true, true,  false, false >},
    { "GMCTW", group_control< true, false, true,  false >},
    { "GMCTG", group_control< true, false, false, true  >},

    { "WWPRH", production_history< Opm::Phase::WATER > },
    { "WOPRH", production_history< Opm::Phase::OIL > },
    { "WGPRH", production_history< Opm::Phase::GAS > },
    { "WLPRH", sum( production_history< Opm::Phase::WATER >,
                    production_history< Opm::Phase::OIL > ) },

    { "WWPTH", mul( production_history< Opm::Phase::WATER >, duration ) },
    { "WOPTH", mul( production_history< Opm::Phase::OIL >, duration ) },
    { "WGPTH", mul( production_history< Opm::Phase::GAS >, duration ) },
    { "WLPTH", mul( sum( production_history< Opm::Phase::WATER >,
                         production_history< Opm::Phase::OIL > ),
                    duration ) },

    { "WWIRH", injection_history< Opm::Phase::WATER > },
    { "WOIRH", injection_history< Opm::Phase::OIL > },
    { "WGIRH", injection_history< Opm::Phase::GAS > },
    { "WWITH", mul( injection_history< Opm::Phase::WATER >, duration ) },
    { "WOITH", mul( injection_history< Opm::Phase::OIL >, duration ) },
    { "WGITH", mul( injection_history< Opm::Phase::GAS >, duration ) },

    /* From our point of view, injectors don't have water cuts and div/sum will return 0.0 */
    { "WWCTH", div( production_history< Opm::Phase::WATER >,
                    sum( production_history< Opm::Phase::WATER >,
                         production_history< Opm::Phase::OIL > ) ) },

    /* We do not support mixed injections, and gas/oil is undefined when oil is
     * zero (i.e. pure gas injector), so always output 0 if this is an injector
     */
    { "WGORH", div( production_history< Opm::Phase::GAS >,
                    production_history< Opm::Phase::OIL > ) },
    { "WWGRH", div( production_history< Opm::Phase::WATER >,
                    production_history< Opm::Phase::GAS > ) },
    { "WGLRH", div( production_history< Opm::Phase::GAS >,
                    sum( production_history< Opm::Phase::WATER >,
                         production_history< Opm::Phase::OIL > ) ) },

    { "WTHPH", thp_history },
    { "WBHPH", bhp_history },

    { "GWPRH", production_history< Opm::Phase::WATER > },
    { "GOPRH", production_history< Opm::Phase::OIL > },
    { "GGPRH", production_history< Opm::Phase::GAS > },
    { "GLPRH", sum( production_history< Opm::Phase::WATER >,
                    production_history< Opm::Phase::OIL > ) },
    { "GWIRH", injection_history< Opm::Phase::WATER > },
    { "GOIRH", injection_history< Opm::Phase::OIL > },
    { "GGIRH", injection_history< Opm::Phase::GAS > },
    { "GGORH", div( production_history< Opm::Phase::GAS >,
                    production_history< Opm::Phase::OIL > ) },
    { "GWCTH", div( production_history< Opm::Phase::WATER >,
                    sum( production_history< Opm::Phase::WATER >,
                         production_history< Opm::Phase::OIL > ) ) },

    { "GWPTH", mul( production_history< Opm::Phase::WATER >, duration ) },
    { "GOPTH", mul( production_history< Opm::Phase::OIL >, duration ) },
    { "GGPTH", mul( production_history< Opm::Phase::GAS >, duration ) },
    { "GGPRF", sub( rate < rt::gas, producer >, rate< rt::dissolved_gas, producer > )},
    { "GGPRS", rate< rt::dissolved_gas, producer> },
    { "GGPTF", mul( sub( rate < rt::gas, producer >, rate< rt::dissolved_gas, producer > ),
                         duration ) },
    { "GGPTS", mul( rate< rt::dissolved_gas, producer>, duration ) },
    { "GGLR",  div( rate< rt::gas, producer >,
                    sum( rate< rt::wat, producer >,
                         rate< rt::oil, producer > ) ) },
    { "GGLRH", div( production_history< Opm::Phase::GAS >,
                    sum( production_history< Opm::Phase::WATER >,
                         production_history< Opm::Phase::OIL > ) ) },
    { "GLPTH", mul( sum( production_history< Opm::Phase::WATER >,
                         production_history< Opm::Phase::OIL > ),
                    duration ) },
    { "GWITH", mul( injection_history< Opm::Phase::WATER >, duration ) },
    { "GGITH", mul( injection_history< Opm::Phase::GAS >, duration ) },
    { "GMWIN", flowing< injector > },
    { "GMWPR", flowing< producer > },

    { "GWPRT", group_water_production_target },
    { "GOPRT", group_oil_production_target },
    { "GGPRT", group_gas_production_target },
    { "GLPRT", group_liquid_production_target },
    { "GVPRT", res_vol_production_target },

    { "GWIRT", group_water_injection_target },
    { "GGIRT", group_gas_injection_target },
    { "GVIRT", group_res_vol_injection_target },


    { "CPR", cpr  },
    { "CGIRL", cratel< rt::gas, injector> },
    { "CGITL", mul( cratel< rt::gas, injector>, duration) },
    { "CWIRL", cratel< rt::wat, injector> },
    { "CWITL", mul( cratel< rt::wat, injector>, duration) },
    { "CWPRL", cratel< rt::wat, producer > },
    { "CWPTL", mul( cratel< rt::wat, producer >, duration) },
    { "COPRL", cratel< rt::oil, producer > },
    { "COPTL", mul( cratel< rt::oil, producer >, duration) },
    { "CGPRL", cratel< rt::gas, producer > },
    { "CGPTL", mul( cratel< rt::gas, producer >, duration) },
    { "COFRL", cratel< rt::oil, producer > },
    { "CGORL", div( cratel< rt::gas, producer >, cratel< rt::oil, producer > ) },
    { "CWCTL", div( cratel< rt::wat, producer >,
                    sum( cratel< rt::wat, producer >, cratel< rt::oil, producer > ) ) },
    { "CWIR", crate< rt::wat, injector > },
    { "CGIR", crate< rt::gas, injector > },
    { "COIR", crate< rt::oil, injector > },
    { "CVIR", crate_resv<injector> },
    { "CCIR", crate< rt::polymer, injector > },
    { "CSIR", crate< rt::brine, injector > },

    // Filter cake quantities (OPM extension)
    { "CINJFVR", filtrate_connection_quantities<&Opm::data::ConnectionFiltrate::rate,
      measure::geometric_volume_rate, injector> },
    { "CINJFVT", filtrate_connection_quantities<&Opm::data::ConnectionFiltrate::total,
      measure::geometric_volume, injector> },
    { "CFCWIDTH", filtrate_connection_quantities<&Opm::data::ConnectionFiltrate::thickness,
      measure::length, injector> },
    { "CFCSKIN", filtrate_connection_quantities<&Opm::data::ConnectionFiltrate::skin_factor,
      measure::identity, injector> },
    { "CFCPORO", filtrate_connection_quantities<&Opm::data::ConnectionFiltrate::poro,
      measure::identity, injector> },
    { "CFCPERM", filtrate_connection_quantities<&Opm::data::ConnectionFiltrate::perm,
      measure::permeability, injector> },
    { "CFCRAD", filtrate_connection_quantities<&Opm::data::ConnectionFiltrate::radius,
      measure::length, injector> },
    { "CFCAOF", filtrate_connection_quantities<&Opm::data::ConnectionFiltrate::area_of_flow,
      measure::area, injector> },
    { "CFCFFRAC", filtrate_connection_quantities<&Opm::data::ConnectionFiltrate::flow_factor,
      measure::identity, injector> },
    { "CFCFRATE", filtrate_connection_quantities<&Opm::data::ConnectionFiltrate::fracture_rate,
      measure::geometric_volume_rate, injector> },

    // Hydraulic fracturing (OPM extension)
    //
    // Per fracture characteristics
    { "CFRAREA", fracture_connection_quantities<&Opm::data::ConnectionFracture::area,
      measure::area, injector> },
    { "CFRFLUX", fracture_connection_quantities<&Opm::data::ConnectionFracture::flux,
      measure::geometric_volume_rate, injector> },
    { "CFRHEIGH", fracture_connection_quantities<&Opm::data::ConnectionFracture::height,
      measure::length, injector> },
    { "CFRLENGT", fracture_connection_quantities<&Opm::data::ConnectionFracture::length,
      measure::length, injector> },
    { "CFRWI", fracture_connection_quantities<&Opm::data::ConnectionFracture::WI,
      measure::transmissibility, injector> },
    { "CFRVOLUM", fracture_connection_quantities<&Opm::data::ConnectionFracture::volume,
      measure::geometric_volume, injector> },
    { "CFRFVOLU", fracture_connection_quantities<&Opm::data::ConnectionFracture::filter_volume,
      measure::geometric_volume, injector> },
    { "CFRAVGW", fracture_connection_quantities<&Opm::data::ConnectionFracture::avg_width,
      measure::length, injector> },
    { "CFRAVGFW", fracture_connection_quantities<&Opm::data::ConnectionFracture::avg_filter_width,
      measure::length, injector> },

    // Fracture pressure statistics
    { "CFRPMAX", connFracStatistics<&Opm::data::ConnectionFracturing::press,
      &Opm::data::ConnectionFracturing::Statistics::max, measure::pressure> },
    { "CFRPMIN", connFracStatistics<&Opm::data::ConnectionFracturing::press,
      &Opm::data::ConnectionFracturing::Statistics::min, measure::pressure> },
    { "CFRPAVG", connFracStatistics<&Opm::data::ConnectionFracturing::press,
      &Opm::data::ConnectionFracturing::Statistics::avg, measure::pressure> },
    { "CFRPSTD", connFracStatistics<&Opm::data::ConnectionFracturing::press,
      &Opm::data::ConnectionFracturing::Statistics::stdev, measure::pressure> },

    // Fracture injection rate statistics
    { "CFRIRMAX", connFracStatistics<&Opm::data::ConnectionFracturing::rate,
      &Opm::data::ConnectionFracturing::Statistics::max, measure::rate> },
    { "CFRIRMIN", connFracStatistics<&Opm::data::ConnectionFracturing::rate,
      &Opm::data::ConnectionFracturing::Statistics::min, measure::rate> },
    { "CFRIRAVG", connFracStatistics<&Opm::data::ConnectionFracturing::rate,
      &Opm::data::ConnectionFracturing::Statistics::avg, measure::rate> },
    { "CFRIRSTD", connFracStatistics<&Opm::data::ConnectionFracturing::rate,
      &Opm::data::ConnectionFracturing::Statistics::stdev, measure::rate> },

    // Fracture width statistics
    { "CFRWDMAX", connFracStatistics<&Opm::data::ConnectionFracturing::width,
      &Opm::data::ConnectionFracturing::Statistics::max, measure::length> },
    { "CFRWDMIN", connFracStatistics<&Opm::data::ConnectionFracturing::width,
      &Opm::data::ConnectionFracturing::Statistics::min, measure::length> },
    { "CFRWDAVG", connFracStatistics<&Opm::data::ConnectionFracturing::width,
      &Opm::data::ConnectionFracturing::Statistics::avg, measure::length> },
    { "CFRWDSTD", connFracStatistics<&Opm::data::ConnectionFracturing::width,
      &Opm::data::ConnectionFracturing::Statistics::stdev, measure::length> },

    { "COIT", mul( crate< rt::oil, injector >, duration ) },
    { "CWIT", mul( crate< rt::wat, injector >, duration ) },
    { "CGIT", mul( crate< rt::gas, injector >, duration ) },
    { "CVIT", mul( crate_resv<injector>, duration ) },
    { "CNIT", mul( crate< rt::solvent, injector >, duration ) },

    { "CWPR", crate< rt::wat, producer > },
    { "COPR", crate< rt::oil, producer > },
    { "CGPR", crate< rt::gas, producer > },
    { "CVPR", crate_resv<producer> },
    { "CCPR", crate< rt::polymer, producer > },
    { "CSPR", crate< rt::brine, producer > },
    { "CGFR", sub(crate<rt::gas, producer>, crate<rt::gas, injector>) },
    { "COFR", sub(crate<rt::oil, producer>, crate<rt::oil, injector>) },
    { "CWFR", sub(crate<rt::wat, producer>, crate<rt::wat, injector>) },
    { "CWCT", div( crate< rt::wat, producer >,
                   sum( crate< rt::wat, producer >, crate< rt::oil, producer > ) ) },
    { "CGOR", div( crate< rt::gas, producer >, crate< rt::oil, producer > ) },
    // Minus for injection rates and pluss for production rate
    { "CNFR", sub( crate< rt::solvent, producer >, crate<rt::solvent, injector >) },
    { "CWPT", mul( crate< rt::wat, producer >, duration ) },
    { "COPT", mul( crate< rt::oil, producer >, duration ) },
    { "CGPT", mul( crate< rt::gas, producer >, duration ) },
    { "CVPT", mul( crate_resv<producer>, duration ) },
    { "CNPT", mul( crate< rt::solvent, producer >, duration ) },
    { "CCIT", mul( crate< rt::polymer, injector >, duration ) },
    { "CCPT", mul( crate< rt::polymer, producer >, duration ) },
    { "CSIT", mul( crate< rt::brine, injector >, duration ) },
    { "CSPT", mul( crate< rt::brine, producer >, duration ) },
    { "CTFAC", trans_factors },
    { "CDFAC", d_factors },
    { "CPI", connection_productivity_index },
    { "CGFRF", sub(crate<rt::gas, producer>, crate<rt::dissolved_gas, producer>) }, // Free gas flow
    { "CGFRS", crate<rt::dissolved_gas, producer> },                                // Solution gas flow
    { "COFRF", sub(crate<rt::oil, producer>, crate<rt::vaporized_oil, producer>) }, // Liquid oil flow
    { "COFRS", crate<rt::vaporized_oil, producer> },                                // Vaporized oil
    { "FWPR", rate< rt::wat, producer > },
    { "FOPR", rate< rt::oil, producer > },
    { "FGPR", rate< rt::gas, producer > },
    { "FEPR", rate< rt::energy, producer > },
    { "FTPRHEA", rate< rt::energy, producer > },
    { "FGLIR", glir },
    { "FGLIT", mul( glir, duration ) },
    { "FNPR", rate< rt::solvent, producer > },
    { "FCPR", rate< rt::polymer, producer > },
    { "FSPR", rate< rt::brine, producer > },
    { "FCPC", div( rate< rt::polymer, producer >, rate< rt::wat, producer >) },
    { "FSPC", div( rate< rt::brine, producer >, rate< rt::wat, producer >) },
    { "FTPR#W", ratetracer< rt::tracer, rt::wat, producer > },
    { "FTPR#O", ratetracer< rt::tracer, rt::oil, producer > },
    { "FTPR#G", ratetracer< rt::tracer, rt::gas, producer > },
    { "FTPRF#W", ratetracer< rt::tracer, rt::wat, producer > },
    { "FTPRF#O", ratetracer< rt::tracer, rt::oil, producer > },
    { "FTPRF#G", ratetracer< rt::tracer, rt::gas, producer > },
    { "FTPRS#W", ratetracer< rt::tracer, rt::wat, producer > },
    { "FTPRS#O", ratetracer< rt::tracer, rt::oil, producer > },
    { "FTPRS#G", ratetracer< rt::tracer, rt::gas, producer > },
    { "FTPC#W", div( ratetracer< rt::tracer, rt::wat, producer >, rate< rt::wat, producer >) },
    { "FTPC#O", div( ratetracer< rt::tracer, rt::oil, producer >, rate< rt::oil, producer >) },
    { "FTPC#G", div( ratetracer< rt::tracer, rt::gas, producer >, rate< rt::gas, producer >) },
    { "FTPCF#W", div( ratetracer< rt::tracer, rt::wat, producer >, rate< rt::wat, producer >) },
    { "FTPCF#O", div( ratetracer< rt::tracer, rt::oil, producer >, rate< rt::oil, producer >) },
    { "FTPCF#G", div( ratetracer< rt::tracer, rt::gas, producer >, rate< rt::gas, producer >) },
    { "FTPCS#W", div( ratetracer< rt::tracer, rt::wat, producer >, rate< rt::wat, producer >) },
    { "FTPCS#O", div( ratetracer< rt::tracer, rt::oil, producer >, rate< rt::oil, producer >) },
    { "FTPCS#G", div( ratetracer< rt::tracer, rt::gas, producer >, rate< rt::gas, producer >) },
    { "FVPR", sum( sum( rate< rt::reservoir_water, producer>, rate< rt::reservoir_oil, producer >),
                   rate< rt::reservoir_gas, producer>)},
    { "FGPRS", rate< rt::dissolved_gas, producer > },
    { "FGPRF", sub( rate< rt::gas, producer >, rate< rt::dissolved_gas, producer > ) },
    { "FOPRS", rate< rt::vaporized_oil, producer > },
    { "FOPRF", sub (rate < rt::oil, producer >, rate< rt::vaporized_oil, producer > ) },

    { "FLPR", sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) },

    { "FWPT", mul(rate<rt::wat, producer, /* cumSatProd = */ true>, duration) },
    { "FOPT", mul(rate<rt::oil, producer, /* cumSatProd = */ true>, duration) },
    { "FGPT", mul(rate<rt::gas, producer, /* cumSatProd = */ true>, duration) },

    { "FEPT", mul( rate< rt::energy, producer >, duration ) },
    { "FTPTHEA", mul( rate< rt::energy, producer >, duration ) },
    { "FNPT", mul( rate< rt::solvent, producer >, duration ) },
    { "FCPT", mul( rate< rt::polymer, producer >, duration ) },
    { "FSPT", mul( rate< rt::brine, producer >, duration ) },
    { "FTPT#W", mul( ratetracer< rt::tracer, rt::wat, producer >, duration ) },
    { "FTPT#O", mul( ratetracer< rt::tracer, rt::oil, producer >, duration ) },
    { "FTPT#G", mul( ratetracer< rt::tracer, rt::gas, producer >, duration ) },
    { "FTPTF#W", mul( ratetracer< rt::tracer, rt::wat, producer >, duration ) },
    { "FTPTF#O", mul( ratetracer< rt::tracer, rt::oil, producer >, duration ) },
    { "FTPTF#G", mul( ratetracer< rt::tracer, rt::gas, producer >, duration ) },
    { "FTPTS#W", mul( ratetracer< rt::tracer, rt::wat, producer >, duration ) },
    { "FTPTS#O", mul( ratetracer< rt::tracer, rt::oil, producer >, duration ) },
    { "FTPTS#G", mul( ratetracer< rt::tracer, rt::gas, producer >, duration ) },
    { "FLPT", mul( sum( rate< rt::wat, producer >, rate< rt::oil, producer > ),
                   duration ) },
    { "FVPT", mul(sum (sum( rate< rt::reservoir_water, producer>, rate< rt::reservoir_oil, producer >),
                       rate< rt::reservoir_gas, producer>), duration)},
    { "FGPTS", mul( rate< rt::dissolved_gas, producer > , duration )},
    { "FGPTF", mul( sub( rate< rt::gas, producer >, rate< rt::dissolved_gas, producer > ), duration )},
    { "FOPTS", mul( rate< rt::vaporized_oil, producer >, duration ) },
    { "FOPTF", mul( sub (rate < rt::oil, producer >,
                         rate< rt::vaporized_oil, producer > ),
                    duration ) },

    { "FWIR", rate< rt::wat, injector > },
    { "FOIR", rate< rt::oil, injector > },
    { "FGIR", rate< rt::gas, injector > },
    { "FEIR", rate< rt::energy, injector > },
    { "FTIRHEA", rate< rt::energy, injector > },
    { "FNIR", rate< rt::solvent, injector > },
    { "FCIR", rate< rt::polymer, injector > },
    { "FSIR", rate< rt::brine, injector > },
    { "FTIR#W", ratetracer< rt::tracer, rt::wat, injector > },
    { "FTIR#O", ratetracer< rt::tracer, rt::oil, injector > },
    { "FTIR#G", ratetracer< rt::tracer, rt::gas, injector > },
    { "FTIRF#W", ratetracer< rt::tracer, rt::wat, injector > },
    { "FTIRF#O", ratetracer< rt::tracer, rt::oil, injector > },
    { "FTIRF#G", ratetracer< rt::tracer, rt::gas, injector > },
    { "FTIRS#W", ratetracer< rt::tracer, rt::wat, injector > },
    { "FTIRS#O", ratetracer< rt::tracer, rt::oil, injector > },
    { "FTIRS#G", ratetracer< rt::tracer, rt::gas, injector > },
    { "FTIC#W", div( ratetracer< rt::tracer, rt::wat, injector >, rate< rt::wat, injector >) },
    { "FTIC#O", div( ratetracer< rt::tracer, rt::oil, injector >, rate< rt::oil, injector >) },
    { "FTIC#G", div( ratetracer< rt::tracer, rt::gas, injector >, rate< rt::gas, injector >) },
    { "FTICF#W", div( ratetracer< rt::tracer, rt::wat, injector >, rate< rt::wat, injector >) },
    { "FTICF#O", div( ratetracer< rt::tracer, rt::oil, injector >, rate< rt::oil, injector >) },
    { "FTICF#G", div( ratetracer< rt::tracer, rt::gas, injector >, rate< rt::gas, injector >) },
    { "FTICS#W", div( ratetracer< rt::tracer, rt::wat, injector >, rate< rt::wat, injector >) },
    { "FTICS#O", div( ratetracer< rt::tracer, rt::oil, injector >, rate< rt::oil, injector >) },
    { "FTICS#G", div( ratetracer< rt::tracer, rt::gas, injector >, rate< rt::gas, injector >) },
    { "FVIR", sum( sum( rate< rt::reservoir_water, injector>, rate< rt::reservoir_oil, injector >),
                   rate< rt::reservoir_gas, injector>)},

    { "FLIR", sum( rate< rt::wat, injector >, rate< rt::oil, injector > ) },
    { "FWIT", mul( rate< rt::wat, injector >, duration ) },
    { "FOIT", mul( rate< rt::oil, injector >, duration ) },
    { "FGIT", mul( rate< rt::gas, injector >, duration ) },
    { "FEIT", mul( rate< rt::energy, injector >, duration ) },
    { "FTITHEA", mul( rate< rt::energy, injector >, duration ) },
    { "FNIT", mul( rate< rt::solvent, injector >, duration ) },
    { "FCIT", mul( rate< rt::polymer, injector >, duration ) },
    { "FSIT", mul( rate< rt::brine, injector >, duration ) },
    { "FTIT#W", mul( ratetracer< rt::tracer, rt::wat, injector >, duration ) },
    { "FTIT#O", mul( ratetracer< rt::tracer, rt::oil, injector >, duration ) },
    { "FTIT#G", mul( ratetracer< rt::tracer, rt::gas, injector >, duration ) },
    { "FTITF#W", mul( ratetracer< rt::tracer, rt::wat, injector >, duration ) },
    { "FTITF#O", mul( ratetracer< rt::tracer, rt::oil, injector >, duration ) },
    { "FTITF#G", mul( ratetracer< rt::tracer, rt::gas, injector >, duration ) },
    { "FTITS#W", mul( ratetracer< rt::tracer, rt::wat, injector >, duration ) },
    { "FTITS#O", mul( ratetracer< rt::tracer, rt::oil, injector >, duration ) },
    { "FTITS#G", mul( ratetracer< rt::tracer, rt::gas, injector >, duration ) },
    { "FLIT", mul( sum( rate< rt::wat, injector >, rate< rt::oil, injector > ),
                   duration ) },
    { "FVIT", mul( sum( sum( rate< rt::reservoir_water, injector>, rate< rt::reservoir_oil, injector >),
                   rate< rt::reservoir_gas, injector>), duration)},

    { "FGCR", gas_consumption_rate },
    { "FGCT", mul( gas_consumption_rate, duration ) },
    { "FGIMR", gas_import_rate },
    { "FGIMT", mul( gas_import_rate, duration ) },

    { "FSGR", sum(  sub( rate< rt::gas, producer >, rate< rt::gas, injector > ),
                    sub( gas_import_rate, gas_consumption_rate ) ) },
    { "FGSR", sum(  sub( rate< rt::gas, producer >, rate< rt::gas, injector > ),
                    sub( gas_import_rate, gas_consumption_rate ) ) },
    { "FSGT", mul( sum(  sub( rate< rt::gas, producer >, rate< rt::gas, injector > ),
                    sub( gas_import_rate, gas_consumption_rate ) ), duration ) },
    { "FGST", mul( sum(  sub( rate< rt::gas, producer >, rate< rt::gas, injector > ),
                    sub( gas_import_rate, gas_consumption_rate ) ), duration ) },


    // Field potential
    { "FWPP", potential_rate< rt::well_potential_water , true, false>},
    { "FOPP", potential_rate< rt::well_potential_oil , true, false>},
    { "FGPP", potential_rate< rt::well_potential_gas , true, false>},
    { "FWPI", potential_rate< rt::well_potential_water , false, true>},
    { "FOPI", potential_rate< rt::well_potential_oil , false, true>},
    { "FGPI", potential_rate< rt::well_potential_gas , false, true>},


    { "FWPRH", production_history< Opm::Phase::WATER > },
    { "FOPRH", production_history< Opm::Phase::OIL > },
    { "FGPRH", production_history< Opm::Phase::GAS > },
    { "FLPRH", sum( production_history< Opm::Phase::WATER >,
                    production_history< Opm::Phase::OIL > ) },
    { "FWPTH", mul( production_history< Opm::Phase::WATER >, duration ) },
    { "FOPTH", mul( production_history< Opm::Phase::OIL >, duration ) },
    { "FGPTH", mul( production_history< Opm::Phase::GAS >, duration ) },
    { "FLPTH", mul( sum( production_history< Opm::Phase::WATER >,
                         production_history< Opm::Phase::OIL > ),
                    duration ) },

    { "FWIRH", injection_history< Opm::Phase::WATER > },
    { "FOIRH", injection_history< Opm::Phase::OIL > },
    { "FGIRH", injection_history< Opm::Phase::GAS > },
    { "FWITH", mul( injection_history< Opm::Phase::WATER >, duration ) },
    { "FOITH", mul( injection_history< Opm::Phase::OIL >, duration ) },
    { "FGITH", mul( injection_history< Opm::Phase::GAS >, duration ) },

    { "FWCT", div( rate< rt::wat, producer >,
                   sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) ) },
    { "FGOR", div( rate< rt::gas, producer >, rate< rt::oil, producer > ) },
    { "FGLR", div( rate< rt::gas, producer >,
                   sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) ) },
    { "FWCTH", div( production_history< Opm::Phase::WATER >,
                    sum( production_history< Opm::Phase::WATER >,
                         production_history< Opm::Phase::OIL > ) ) },
    { "FGORH", div( production_history< Opm::Phase::GAS >,
                    production_history< Opm::Phase::OIL > ) },
    { "FGLRH", div( production_history< Opm::Phase::GAS >,
                    sum( production_history< Opm::Phase::WATER >,
                         production_history< Opm::Phase::OIL > ) ) },
    { "FMWIN", flowing< injector > },
    { "FMWPR", flowing< producer > },

    { "FWPRT", group_water_production_target },
    { "FOPRT", group_oil_production_target },
    { "FGPRT", group_gas_production_target },
    { "FLPRT", group_liquid_production_target },
    { "FVPRT", res_vol_production_target },

    { "FWIRT", group_water_injection_target },
    { "FGIRT", group_gas_injection_target },
    { "FVIRT", group_res_vol_injection_target },

    { "FMWPA", abandoned_well< producer > },
    { "FMWIA", abandoned_well< injector >},

    //Field control mode
    { "FMCTP", group_control< false, true,  false, false >},
    { "FMCTW", group_control< false, false, true,  false >},
    { "FMCTG", group_control< false, false, false, true  >},

    /* Region properties */
    { "ROIR"  , region_rate< rt::oil, injector > },
    { "RGIR"  , region_rate< rt::gas, injector > },
    { "RWIR"  , region_rate< rt::wat, injector > },
    { "ROPR"  , region_rate< rt::oil, producer > },
    { "RGPR"  , region_rate< rt::gas, producer > },
    { "RWPR"  , region_rate< rt::wat, producer > },
    { "ROIT"  , mul( region_rate< rt::oil, injector >, duration ) },
    { "RGIT"  , mul( region_rate< rt::gas, injector >, duration ) },
    { "RWIT"  , mul( region_rate< rt::wat, injector >, duration ) },
    { "ROPT"  , mul( region_rate< rt::oil, producer >, duration ) },
    { "RGPT"  , mul( region_rate< rt::gas, producer >, duration ) },
    { "RWPT"  , mul( region_rate< rt::wat, producer >, duration ) },
    { "RHPV"  , rhpv },

    // Segment summary vectors for multi-segmented wells.
    { "SDENM", segment_density<Opm::data::SegmentPhaseDensity::Item::Mixture> },
    { "SMDEN", segment_density<Opm::data::SegmentPhaseDensity::Item::MixtureWithExponents> },
    { "SODEN", segment_density<Opm::data::SegmentPhaseDensity::Item::Oil> },
    { "SOFR" , srate<rt::oil> },
    { "SOFT" , mul(srate<rt::oil>, duration) },
    { "SOFRF", sub(srate<rt::oil>, srate<rt::vaporized_oil>) }, // Free oil flow
    { "SOFRS", srate<rt::vaporized_oil> },                      // Solution oil flow
    { "SOFV" , segment_flow_velocity<Opm::data::SegmentPhaseQuantity::Item::Oil> },
    { "SOHF" , segment_holdup_fraction<Opm::data::SegmentPhaseQuantity::Item::Oil> },
    { "SOVIS", segment_viscosity<Opm::data::SegmentPhaseQuantity::Item::Oil> },
    { "SGDEN", segment_density<Opm::data::SegmentPhaseDensity::Item::Gas> },
    { "SGFR" , srate<rt::gas> },
    { "SGFT" , mul(srate<rt::gas>, duration) },
    { "SGFRF", sub(srate<rt::gas>, srate<rt::dissolved_gas>) }, // Free gas flow
    { "SGFRS", srate<rt::dissolved_gas> },                      // Solution gas flow
    { "SGFV" , segment_flow_velocity<Opm::data::SegmentPhaseQuantity::Item::Gas> },
    { "SGHF" , segment_holdup_fraction<Opm::data::SegmentPhaseQuantity::Item::Gas> },
    { "SGVIS", segment_viscosity<Opm::data::SegmentPhaseQuantity::Item::Gas> },
    { "SWDEN", segment_density<Opm::data::SegmentPhaseDensity::Item::Water> },
    { "SWFR" , srate<rt::wat> },
    { "SWFT" , mul(srate<rt::wat>, duration) },
    { "SWFV" , segment_flow_velocity<Opm::data::SegmentPhaseQuantity::Item::Water> },
    { "SWHF" , segment_holdup_fraction<Opm::data::SegmentPhaseQuantity::Item::Water> },
    { "SWVIS", segment_viscosity<Opm::data::SegmentPhaseQuantity::Item::Water> },
    { "SGOR" , div(srate<rt::gas>, srate<rt::oil>) },
    { "SOGR" , div(srate<rt::oil>, srate<rt::gas>) },
    { "SWCT" , div(srate<rt::wat>, sum(srate<rt::wat>, srate<rt::oil>)) },
    { "SWGR" , div(srate<rt::wat>, srate<rt::gas>) },
    { "SPR"  , segpress<Opm::data::SegmentPressures::Value::Pressure> },
    { "SPRD" , segpress<Opm::data::SegmentPressures::Value::PDrop> },
    { "SPRDH", segpress<Opm::data::SegmentPressures::Value::PDropHydrostatic> },
    { "SPRDF", segpress<Opm::data::SegmentPressures::Value::PDropFriction> },
    { "SPRDA", segpress<Opm::data::SegmentPressures::Value::PDropAccel> },
    { "STFR#W", sratetracer<rt::tracer, rt::wat> }, // #W: Water tracers
    { "STFR#O", sratetracer<rt::tracer, rt::oil> }, // #O: Oil tracers
    { "STFR#G", sratetracer<rt::tracer, rt::gas> }, // #G: Gas tracers
    { "STFC#W", div( sratetracer<rt::tracer, rt::wat>, srate<rt::wat> ) }, // #W: Water tracers
    { "STFC#O", div( sratetracer<rt::tracer, rt::oil>, srate<rt::oil> ) }, // #O: Oil tracers
    { "STFC#G", div( sratetracer<rt::tracer, rt::gas>, srate<rt::gas> ) }, // #G: Gas tracers

    // Well productivity index
    { "WPI", preferred_phase_productivty_index },
    { "WPIW", potential_rate< rt::productivity_index_water >},
    { "WPIO", potential_rate< rt::productivity_index_oil >},
    { "WPIG", potential_rate< rt::productivity_index_gas >},
    { "WPIL", sum( potential_rate< rt::productivity_index_water, true, false >,
                   potential_rate< rt::productivity_index_oil, true, false >)},

    { "WPI1", well_block_average_prod_index<Opm::data::WellBlockAvgPress::Quantity::WBP>  },
    { "WPI4", well_block_average_prod_index<Opm::data::WellBlockAvgPress::Quantity::WBP4> },
    { "WPI5", well_block_average_prod_index<Opm::data::WellBlockAvgPress::Quantity::WBP5> },
    { "WPI9", well_block_average_prod_index<Opm::data::WellBlockAvgPress::Quantity::WBP9> },

    // Well potential
    { "WWPP", potential_rate< rt::well_potential_water , true, false>},
    { "WOPP", potential_rate< rt::well_potential_oil , true, false>},
    { "WGPP", potential_rate< rt::well_potential_gas , true, false>},
    { "WWPI", potential_rate< rt::well_potential_water , false, true>},
    { "WWIP", potential_rate< rt::well_potential_water , false, true>}, // Alias for 'WWPI'
    { "WOPI", potential_rate< rt::well_potential_oil , false, true>},
    { "WGPI", potential_rate< rt::well_potential_gas , false, true>},
    { "WGIP", potential_rate< rt::well_potential_gas , false, true>}, // Alias for 'WGPI'
    { "ROEW", roew },

    // Efficiency factors
    {"GEFF" , group_efficiency_factor},
    {"WEFF" , well_efficiency_factor},
    {"WEFFG", well_efficiency_factor_grouptree},

    // Mass water
    { "FAMIR",  rate< rt::mass_wat, injector > },
    { "GAMIR",  rate< rt::mass_wat, injector > },
    { "WAMIR",  rate< rt::mass_wat, injector > },
    { "CAMIR",  crate< rt::mass_wat, injector > },
    { "CAMIRL", cratel< rt::mass_wat, injector > },
    { "FAMIT",  mul( rate< rt::mass_wat, injector >, duration ) },
    { "GAMIT",  mul( rate< rt::mass_wat, injector >, duration ) },
    { "WAMIT",  mul( rate< rt::mass_wat, injector >, duration ) },
    { "CAMIT",  mul( crate< rt::mass_wat, injector >, duration ) },
    { "CAMITL", mul( cratel< rt::mass_wat, injector >, duration ) },
    { "FAMPR",  rate< rt::mass_wat, producer > },
    { "GAMPR",  rate< rt::mass_wat, producer > },
    { "WAMPR",  rate< rt::mass_wat, producer > },
    { "CAMPR",  crate< rt::mass_wat, producer > },
    { "CAMPRL", cratel< rt::mass_wat, producer > },
    { "FAMPT",  mul( rate< rt::mass_wat, producer >, duration ) },
    { "GAMPT",  mul( rate< rt::mass_wat, producer >, duration ) },
    { "WAMPT",  mul( rate< rt::mass_wat, producer >, duration ) },
    { "CAMPT",  mul( crate< rt::mass_wat, producer >, duration ) },
    { "CAMPTL", mul( cratel< rt::mass_wat, producer >, duration ) },

    // co2/h2store
    { "FGMIR",  rate< rt::mass_gas, injector > },
    { "GGMIR",  rate< rt::mass_gas, injector > },
    { "WGMIR",  rate< rt::mass_gas, injector > },
    { "CGMIR",  crate< rt::mass_gas, injector > },
    { "CGMIRL", cratel< rt::mass_gas, injector > },
    { "FGMIT",  mul( rate< rt::mass_gas, injector >, duration ) },
    { "GGMIT",  mul( rate< rt::mass_gas, injector >, duration ) },
    { "WGMIT",  mul( rate< rt::mass_gas, injector >, duration ) },
    { "CGMIT",  mul( crate< rt::mass_gas, injector >, duration ) },
    { "CGMITL", mul( cratel< rt::mass_gas, injector >, duration ) },
    { "FGMPR",  rate< rt::mass_gas, producer > },
    { "GGMPR",  rate< rt::mass_gas, producer > },
    { "WGMPR",  rate< rt::mass_gas, producer > },
    { "CGMPR",  crate< rt::mass_gas, producer > },
    { "CGMPRL", cratel< rt::mass_gas, producer > },
    { "FGMPT",  mul( rate< rt::mass_gas, producer >, duration ) },
    { "GGMPT",  mul( rate< rt::mass_gas, producer >, duration ) },
    { "WGMPT",  mul( rate< rt::mass_gas, producer >, duration ) },
    { "CGMPT",  mul( crate< rt::mass_gas, producer >, duration ) },
    { "CGMPTL", mul( cratel< rt::mass_gas, producer >, duration ) },

    // Biofilms
    { "WMMIR", rate< rt::microbial, injector > },
    { "WMMIT", mul( rate< rt::microbial, injector >, duration ) },
    { "GMMIT", mul( rate< rt::microbial, injector >, duration ) },
    { "CMMIR", crate< rt::microbial, injector > },
    { "CMMIT",  mul( crate< rt::microbial, injector >, duration) },
    { "CMMIRL", cratel< rt::microbial, injector> },
    { "CMMITL", mul( cratel< rt::microbial, injector>, duration) },
    { "FMMIR", rate< rt::microbial, injector > },
    { "FMMIT", mul( rate< rt::microbial, injector >, duration ) },
    { "WMMPR", rate< rt::microbial, producer > },
    { "WMMPT", mul( rate< rt::microbial, producer >, duration ) },
    { "GMMPT", mul( rate< rt::microbial, producer >, duration ) },
    { "CMMPR", crate< rt::microbial, producer > },
    { "CMMPT",  mul( crate< rt::microbial, producer >, duration) },
    { "CMMPRL", cratel< rt::microbial, producer > },
    { "CMMPTL", mul( cratel< rt::microbial, producer >, duration) },
    { "FMMPR", rate< rt::microbial, producer > },
    { "FMMPT", mul( rate< rt::microbial, producer >, duration ) },
    { "WMOIR", rate< rt::oxygen, injector > },
    { "WMOIT", mul( rate< rt::oxygen, injector >, duration ) },
    { "GMOIT", mul( rate< rt::oxygen, injector >, duration ) },
    { "CMOIR", crate< rt::oxygen, injector > },
    { "CMOIT",  mul( crate< rt::oxygen, injector >, duration) },
    { "CMOIRL", cratel< rt::oxygen, injector> },
    { "CMOITL", mul( cratel< rt::oxygen, injector>, duration) },
    { "FMOIR", rate< rt::oxygen, injector > },
    { "FMOIT", mul( rate< rt::oxygen, injector >, duration ) },
    { "WMOPR", rate< rt::oxygen, producer > },
    { "WMOPT", mul( rate< rt::oxygen, producer >, duration ) },
    { "GMOPT", mul( rate< rt::oxygen, producer >, duration ) },
    { "CMOPR", crate< rt::oxygen, producer > },
    { "CMOPT",  mul( crate< rt::oxygen, producer >, duration) },
    { "CMOPRL", cratel< rt::oxygen, producer > },
    { "CMOPTL", mul( cratel< rt::oxygen, producer >, duration) },
    { "FMOPR", rate< rt::oxygen, producer > },
    { "FMOPT", mul( rate< rt::oxygen, producer >, duration ) },
    { "WMUIR", rate< rt::urea, injector > },
    { "WMUIT", mul( rate< rt::urea, injector >, duration ) },
    { "GMUIT", mul( rate< rt::urea, injector >, duration ) },
    { "CMUIR", crate< rt::urea, injector > },
    { "CMUIT",  mul( crate< rt::urea, injector >, duration) },
    { "CMUIRL", cratel< rt::urea, injector> },
    { "CMUITL", mul( cratel< rt::urea, injector>, duration) },
    { "FMUIR", rate< rt::urea, injector > },
    { "FMUIT", mul( rate< rt::urea, injector >, duration ) },
    { "WMUPR", rate< rt::urea, producer > },
    { "WMUPT", mul( rate< rt::urea, producer >, duration ) },
    { "GMUPT", mul( rate< rt::urea, producer >, duration ) },
    { "CMUPR", crate< rt::urea, producer > },
    { "CMUPT",  mul( crate< rt::urea, producer >, duration) },
    { "CMUPRL", cratel< rt::urea, producer > },
    { "CMUPTL", mul( cratel< rt::urea, producer >, duration) },
    { "FMUPR", rate< rt::urea, producer > },
    { "FMUPT", mul( rate< rt::urea, producer >, duration ) },
};

static const auto single_values_units = UnitTable {
    {"TCPU"     , Opm::UnitSystem::measure::runtime },
    {"ELAPSED"  , Opm::UnitSystem::measure::identity },
    {"NEWTON"   , Opm::UnitSystem::measure::identity },
    {"NLINERS"  , Opm::UnitSystem::measure::identity },
    {"NLINSMIN" , Opm::UnitSystem::measure::identity },
    {"NLINSMAX" , Opm::UnitSystem::measure::identity },
    {"MLINEARS" , Opm::UnitSystem::measure::identity },
    {"NLINEARS" , Opm::UnitSystem::measure::identity },
    {"MSUMLINS" , Opm::UnitSystem::measure::identity },
    {"MSUMNEWT" , Opm::UnitSystem::measure::identity },
    {"TCPUTS"   , Opm::UnitSystem::measure::identity },
    {"TIMESTEP" , Opm::UnitSystem::measure::time },
    {"TCPUDAY"  , Opm::UnitSystem::measure::time },
    {"STEPTYPE" , Opm::UnitSystem::measure::identity },
    {"TELAPLIN" , Opm::UnitSystem::measure::time },
    {"FRPV"     , Opm::UnitSystem::measure::volume },
    {"FWIP"     , Opm::UnitSystem::measure::liquid_surface_volume },
    {"FWIPR"    , Opm::UnitSystem::measure::volume },
    {"FOIP"     , Opm::UnitSystem::measure::liquid_surface_volume },
    {"FOIPR"    , Opm::UnitSystem::measure::volume },
    {"FOE"      , Opm::UnitSystem::measure::identity },
    {"FGIP"     , Opm::UnitSystem::measure::gas_surface_volume },
    {"FGIPR"    , Opm::UnitSystem::measure::volume },
    {"FSIP"     , Opm::UnitSystem::measure::mass },
    {"FOIPL"    , Opm::UnitSystem::measure::liquid_surface_volume },
    {"FOIPG"    , Opm::UnitSystem::measure::liquid_surface_volume },
    {"FGIPL"    , Opm::UnitSystem::measure::gas_surface_volume },
    {"FGIPG"    , Opm::UnitSystem::measure::gas_surface_volume },
    {"FPR"      , Opm::UnitSystem::measure::pressure },
    {"FPRP"     , Opm::UnitSystem::measure::pressure },
    {"FPRH"     , Opm::UnitSystem::measure::pressure },
    {"FHPV"     , Opm::UnitSystem::measure::volume },
    {"FGCDI"    , Opm::UnitSystem::measure::moles },
    {"FGCDM"    , Opm::UnitSystem::measure::moles },
    {"FGKDI"    , Opm::UnitSystem::measure::moles },
    {"FGKDM"    , Opm::UnitSystem::measure::moles },
    {"FWCD"     , Opm::UnitSystem::measure::moles },
    {"FWIPG"    , Opm::UnitSystem::measure::liquid_surface_volume },
    {"FWIPL"    , Opm::UnitSystem::measure::liquid_surface_volume },
    {"FGMIP"    , Opm::UnitSystem::measure::mass },
    {"FGMGP"    , Opm::UnitSystem::measure::mass },
    {"FGMDS"    , Opm::UnitSystem::measure::mass },
    {"FGMTR"    , Opm::UnitSystem::measure::mass },
    {"FGMMO"    , Opm::UnitSystem::measure::mass },
    {"FGKTR"    , Opm::UnitSystem::measure::mass },
    {"FGKMO"    , Opm::UnitSystem::measure::mass },
    {"FGMST"    , Opm::UnitSystem::measure::mass },
    {"FGMUS"    , Opm::UnitSystem::measure::mass },
    {"FMMIP"    , Opm::UnitSystem::measure::mass },
    {"FMOIP"    , Opm::UnitSystem::measure::mass },
    {"FMUIP"    , Opm::UnitSystem::measure::mass },
    {"FMBIP"    , Opm::UnitSystem::measure::mass },
    {"FMCIP"    , Opm::UnitSystem::measure::mass },
    {"FAMIP"    , Opm::UnitSystem::measure::mass },
};

static const auto region_units = UnitTable {
    {"RPR"   , Opm::UnitSystem::measure::pressure},
    {"RPRP"  , Opm::UnitSystem::measure::pressure},
    {"RPRH"  , Opm::UnitSystem::measure::pressure},
    {"RRPV"  , Opm::UnitSystem::measure::volume },
    {"ROIP"  , Opm::UnitSystem::measure::liquid_surface_volume },
    {"ROIPL" , Opm::UnitSystem::measure::liquid_surface_volume },
    {"ROIPG" , Opm::UnitSystem::measure::liquid_surface_volume },
    {"RGIP"  , Opm::UnitSystem::measure::gas_surface_volume },
    {"RGIPL" , Opm::UnitSystem::measure::gas_surface_volume },
    {"RGIPG" , Opm::UnitSystem::measure::gas_surface_volume },
    {"RWIP"  , Opm::UnitSystem::measure::liquid_surface_volume },
    {"RRPV"  , Opm::UnitSystem::measure::geometric_volume },
    {"RGCDI" , Opm::UnitSystem::measure::moles },
    {"RGCDM" , Opm::UnitSystem::measure::moles },
    {"RGKDI" , Opm::UnitSystem::measure::moles },
    {"RGKDM" , Opm::UnitSystem::measure::moles },
    {"RWCD"  , Opm::UnitSystem::measure::moles },
    {"RWIPG" , Opm::UnitSystem::measure::liquid_surface_volume },
    {"RWIPL" , Opm::UnitSystem::measure::liquid_surface_volume },
    {"RGMIP" , Opm::UnitSystem::measure::mass },
    {"RGMGP" , Opm::UnitSystem::measure::mass },
    {"RGMDS" , Opm::UnitSystem::measure::mass },
    {"RGMTR" , Opm::UnitSystem::measure::mass },
    {"RGMMO" , Opm::UnitSystem::measure::mass },
    {"RGKTR" , Opm::UnitSystem::measure::mass },
    {"RGKMO" , Opm::UnitSystem::measure::mass },
    {"RGMST" , Opm::UnitSystem::measure::mass },
    {"RGMUS" , Opm::UnitSystem::measure::mass },
    {"RMMIP" , Opm::UnitSystem::measure::mass },
    {"RMOIP" , Opm::UnitSystem::measure::mass },
    {"RMUIP" , Opm::UnitSystem::measure::mass },
    {"RMBIP" , Opm::UnitSystem::measure::mass },
    {"RMCIP" , Opm::UnitSystem::measure::mass },
    {"RAMIP" , Opm::UnitSystem::measure::mass },
};

static const auto interregion_units = UnitTable {
    // Flow rates (surface volume)
    { "ROFR"  , Opm::UnitSystem::measure::liquid_surface_rate },
    { "ROFR+" , Opm::UnitSystem::measure::liquid_surface_rate },
    { "ROFR-" , Opm::UnitSystem::measure::liquid_surface_rate },
    { "RGFR"  , Opm::UnitSystem::measure::gas_surface_rate    },
    { "RGFR+" , Opm::UnitSystem::measure::gas_surface_rate    },
    { "RGFR-" , Opm::UnitSystem::measure::gas_surface_rate    },
    { "RWFR"  , Opm::UnitSystem::measure::liquid_surface_rate },
    { "RWFR+" , Opm::UnitSystem::measure::liquid_surface_rate },
    { "RWFR-" , Opm::UnitSystem::measure::liquid_surface_rate },

    // Cumulatives (surface volume)
    { "ROFT"  , Opm::UnitSystem::measure::liquid_surface_volume },
    { "ROFT+" , Opm::UnitSystem::measure::liquid_surface_volume },
    { "ROFT-" , Opm::UnitSystem::measure::liquid_surface_volume },
    { "ROFTG" , Opm::UnitSystem::measure::liquid_surface_volume },
    { "ROFTL" , Opm::UnitSystem::measure::liquid_surface_volume },
    { "RGFT"  , Opm::UnitSystem::measure::gas_surface_volume    },
    { "RGFT+" , Opm::UnitSystem::measure::gas_surface_volume    },
    { "RGFT-" , Opm::UnitSystem::measure::gas_surface_volume    },
    { "RGFTG" , Opm::UnitSystem::measure::gas_surface_volume    },
    { "RGFTL" , Opm::UnitSystem::measure::gas_surface_volume    },
    { "RWFT"  , Opm::UnitSystem::measure::liquid_surface_volume },
    { "RWFT+" , Opm::UnitSystem::measure::liquid_surface_volume },
    { "RWFT-" , Opm::UnitSystem::measure::liquid_surface_volume },
};

static const auto block_units = UnitTable {
    // Gas quantities
    {"BGDEN"    , Opm::UnitSystem::measure::density},
    {"BDENG"    , Opm::UnitSystem::measure::density},
    {"BGIP"     , Opm::UnitSystem::measure::gas_surface_volume},
    {"BGIPG"    , Opm::UnitSystem::measure::gas_surface_volume},
    {"BGIPL"    , Opm::UnitSystem::measure::gas_surface_volume},
    {"BGKR"     , Opm::UnitSystem::measure::identity},
    {"BKRG"     , Opm::UnitSystem::measure::identity},
    {"BGPC"     , Opm::UnitSystem::measure::pressure},
    {"BGPR"     , Opm::UnitSystem::measure::pressure},
    {"BGPV"     , Opm::UnitSystem::measure::volume},
    {"BGSAT"    , Opm::UnitSystem::measure::identity},
    {"BSGAS"    , Opm::UnitSystem::measure::identity},
    {"BGVIS"    , Opm::UnitSystem::measure::viscosity},
    {"BVGAS"    , Opm::UnitSystem::measure::viscosity},

    // Oil quantities
    {"BODEN"    , Opm::UnitSystem::measure::density},
    {"BDENO"    , Opm::UnitSystem::measure::density},
    {"BOKR"     , Opm::UnitSystem::measure::identity},
    {"BKRO"     , Opm::UnitSystem::measure::identity},
    {"BKROG"    , Opm::UnitSystem::measure::identity},
    {"BKROW"    , Opm::UnitSystem::measure::identity},
    {"BOIP"     , Opm::UnitSystem::measure::liquid_surface_volume},
    {"BOIPG"    , Opm::UnitSystem::measure::liquid_surface_volume},
    {"BOIPL"    , Opm::UnitSystem::measure::liquid_surface_volume},
    {"BOPV"     , Opm::UnitSystem::measure::volume},
    {"BOSAT"    , Opm::UnitSystem::measure::identity},
    {"BSOIL"    , Opm::UnitSystem::measure::identity},
    {"BOVIS"    , Opm::UnitSystem::measure::viscosity},
    {"BVOIL"    , Opm::UnitSystem::measure::viscosity},

    // Water quantities
    {"BWDEN"    , Opm::UnitSystem::measure::density},
    {"BDENW"    , Opm::UnitSystem::measure::density},
    {"BFLOWI"   , Opm::UnitSystem::measure::liquid_surface_rate},
    {"BFLOWJ"   , Opm::UnitSystem::measure::liquid_surface_rate},
    {"BFLOWK"   , Opm::UnitSystem::measure::liquid_surface_rate},
    {"BWIP"     , Opm::UnitSystem::measure::liquid_surface_volume},
    {"BWKR"     , Opm::UnitSystem::measure::identity},
    {"BKRW"     , Opm::UnitSystem::measure::identity},
    {"BWPC"     , Opm::UnitSystem::measure::pressure},
    {"BWPR"     , Opm::UnitSystem::measure::pressure},
    {"BWPV"     , Opm::UnitSystem::measure::volume},
    {"BWSAT"    , Opm::UnitSystem::measure::identity},
    {"BSWAT"    , Opm::UnitSystem::measure::identity},
    {"BWVIS"    , Opm::UnitSystem::measure::viscosity},
    {"BVWAT"    , Opm::UnitSystem::measure::viscosity},
    {"BAMIP"    , Opm::UnitSystem::measure::mass},

    // Pressure quantities
    {"BPR"      , Opm::UnitSystem::measure::pressure},
    {"BPRESSUR" , Opm::UnitSystem::measure::pressure},
    {"BPPO"     , Opm::UnitSystem::measure::pressure},
    {"BPPG"     , Opm::UnitSystem::measure::pressure},
    {"BPPW"     , Opm::UnitSystem::measure::pressure},

    // Volumes and ratios
    {"BRPV"     , Opm::UnitSystem::measure::volume},
    {"BRS"      , Opm::UnitSystem::measure::gas_oil_ratio},
    {"BRV"      , Opm::UnitSystem::measure::oil_gas_ratio},

    {"BNSAT"    , Opm::UnitSystem::measure::identity},

    // Temperature/energy
    {"BTCNFHEA" , Opm::UnitSystem::measure::temperature},
    {"BTEMP"    , Opm::UnitSystem::measure::temperature},

    // mechanics
    {"BSTRSSXX" , Opm::UnitSystem::measure::pressure},
    {"BSTRSSYY" , Opm::UnitSystem::measure::pressure},
    {"BSTRSSZZ" , Opm::UnitSystem::measure::pressure},
    {"BSTRSSXY" , Opm::UnitSystem::measure::pressure},
    {"BSTRSSXZ" , Opm::UnitSystem::measure::pressure},
    {"BSTRSSYZ" , Opm::UnitSystem::measure::pressure},

    // co2/h2store
    {"BWCD" , Opm::UnitSystem::measure::moles},
    {"BGCDI" , Opm::UnitSystem::measure::moles},
    {"BGCDM" , Opm::UnitSystem::measure::moles},
    {"BGKDI" , Opm::UnitSystem::measure::moles},
    {"BGKDM" , Opm::UnitSystem::measure::moles},
    {"BGKMO" , Opm::UnitSystem::measure::mass},
    {"BGKTR" , Opm::UnitSystem::measure::mass},
    {"BGMDS" , Opm::UnitSystem::measure::mass},
    {"BGMGP" , Opm::UnitSystem::measure::mass},
    {"BGMIP" , Opm::UnitSystem::measure::mass},
    {"BGMMO" , Opm::UnitSystem::measure::mass},
    {"BGMST" , Opm::UnitSystem::measure::mass},
    {"BGMTR" , Opm::UnitSystem::measure::mass},
    {"BGMUS" , Opm::UnitSystem::measure::mass},
    {"BWIPG" , Opm::UnitSystem::measure::liquid_surface_volume},
    {"BWIPL" , Opm::UnitSystem::measure::liquid_surface_volume},

    // Biofilms
    {"BMMIP"     , Opm::UnitSystem::measure::mass},
    {"BMOIP"     , Opm::UnitSystem::measure::mass},
    {"BMUIP"     , Opm::UnitSystem::measure::mass},
    {"BMBIP"     , Opm::UnitSystem::measure::mass},
    {"BMCIP"     , Opm::UnitSystem::measure::mass},
};

static const auto aquifer_units = UnitTable {
    {"AAQT", Opm::UnitSystem::measure::liquid_surface_volume},
    {"AAQR", Opm::UnitSystem::measure::liquid_surface_rate},
    {"AAQP", Opm::UnitSystem::measure::pressure},
    {"ANQP", Opm::UnitSystem::measure::pressure},
    {"ANQT", Opm::UnitSystem::measure::liquid_surface_volume},
    {"ANQR", Opm::UnitSystem::measure::liquid_surface_rate},

    // Dimensionless time and pressure values for CT aquifers
    {"AAQTD", Opm::UnitSystem::measure::identity},
    {"AAQPD", Opm::UnitSystem::measure::identity},
};

void sort_wells_by_insert_index(std::vector<const Opm::Well*>& wells)
{
    std::sort(wells.begin(), wells.end(),
        [](const Opm::Well* w1, const Opm::Well* w2)
    {
        return w1->seqIndex() < w2->seqIndex();
    });
}

std::vector<const Opm::Well*>
find_single_well(const Opm::Schedule& schedule,
                 const std::string&   well_name,
                 const int            sim_step)
{
    auto single_well = std::vector<const Opm::Well*>{};

    if (schedule.hasWell(well_name, sim_step)) {
        single_well.push_back(&schedule.getWell(well_name, sim_step));
    }

    return single_well;
}

std::vector<const Opm::Well*>
find_region_wells(const Opm::Schedule&           schedule,
                  const Opm::EclIO::SummaryNode& node,
                  const int                      sim_step,
                  const Opm::out::RegionCache&   regionCache)
{
    auto result = std::vector<const Opm::Well*>{};
    auto regionwells = std::set<const Opm::Well*>{};

    const auto region = node.number;

    for (const auto& connection : regionCache.connections(*node.fip_region, region)) {
        const auto& w_name = connection.first;
        if (! schedule.hasWell(w_name, sim_step)) {
            continue;
        }

        regionwells.insert(&schedule.getWell(w_name, sim_step));
    }

    result.assign(regionwells.begin(), regionwells.end());
    sort_wells_by_insert_index(result);

    return result;
}

std::vector<const Opm::Well*>
find_group_wells(const Opm::Schedule& schedule,
                 const std::string&   group_name,
                 const int            sim_step)
{
    auto groupwells = std::vector<const Opm::Well*>{};

    const auto& schedState = schedule[sim_step];
    if (! schedState.groups.has(group_name)) {
        return groupwells;      // Empty
    }

    auto downtree = std::vector<std::string>{group_name};
    for (auto i = 0*downtree.size(); i < downtree.size(); ++i) {
        const auto& group = schedState.groups.get(downtree[i]);

        if (group.wellgroup()) {
            std::transform(group.wells().begin(), group.wells().end(),
                           std::back_inserter(groupwells),
                           [&schedState](const auto& wname)
                           {
                               return &schedState.wells.get(wname);
                           });
        }
        else {
            const auto& children = group.groups();
            downtree.insert(downtree.end(), children.begin(), children.end());
        }
    }

    sort_wells_by_insert_index(groupwells);

    return groupwells;
}

std::vector<const Opm::Well*>
find_field_wells(const Opm::Schedule& schedule,
                 const int            sim_step)
{
    auto fieldwells = std::vector<const Opm::Well*>{};

    const auto& wells = schedule[sim_step].wells;
    const auto keys = wells.keys();
    std::transform(keys.begin(), keys.end(),
                   std::back_inserter(fieldwells),
                   [&wells](const auto& well)
                   {
                       return &wells.get(well);
                   });

    sort_wells_by_insert_index(fieldwells);

    return fieldwells;
}

inline std::vector<const Opm::Well*>
find_wells(const Opm::Schedule&           schedule,
           const Opm::EclIO::SummaryNode& node,
           const int                      sim_step,
           const Opm::out::RegionCache&   regionCache)
{
    switch (node.category) {
    case Opm::EclIO::SummaryNode::Category::Well:
    case Opm::EclIO::SummaryNode::Category::Connection:
    case Opm::EclIO::SummaryNode::Category::Completion:
    case Opm::EclIO::SummaryNode::Category::Segment:
        return find_single_well(schedule, node.wgname, sim_step);

    case Opm::EclIO::SummaryNode::Category::Group:
        return find_group_wells(schedule, node.wgname, sim_step);

    case Opm::EclIO::SummaryNode::Category::Field:
        return find_field_wells(schedule, sim_step);

    case Opm::EclIO::SummaryNode::Category::Region:
        return find_region_wells(schedule, node, sim_step, regionCache);

    case Opm::EclIO::SummaryNode::Category::Aquifer:
    case Opm::EclIO::SummaryNode::Category::Block:
    case Opm::EclIO::SummaryNode::Category::Node:
    case Opm::EclIO::SummaryNode::Category::Miscellaneous:
        return {};
    }

    throw std::runtime_error {
        fmt::format("Unhandled summary node category \"{}\" in find_wells()",
                    static_cast<int>(node.category))
    };
}

bool need_wells(const Opm::EclIO::SummaryNode& node)
{
    static const std::regex region_keyword_regex {
        "R[OGW][IP][RT](_[A-Z0-9_]{1,3})?"
    };
    static const std::regex group_guiderate_regex { "G[OGWV][IP]GR" };

    using Cat = Opm::EclIO::SummaryNode::Category;

    switch (node.category) {
    case Cat::Connection: [[fallthrough]];
    case Cat::Completion: [[fallthrough]];
    case Cat::Field:      [[fallthrough]];
    case Cat::Group:      [[fallthrough]];
    case Cat::Segment:    [[fallthrough]];
    case Cat::Well:
        // Need to capture wells for anything other than guiderates at group
        // level.  Those are directly available in the solution values from
        // the simulator and don't need aggregation from well level.
        return (node.category != Cat::Group)
            || !std::regex_match(node.keyword, group_guiderate_regex);

    case Cat::Region:
        return std::regex_match(node.keyword, region_keyword_regex);

    case Cat::Aquifer:       [[fallthrough]];
    case Cat::Miscellaneous: [[fallthrough]];
    case Cat::Node:          [[fallthrough]];
        // Node values directly available in solution.
    case Cat::Block:
        return false;
    }

    throw std::runtime_error {
        fmt::format("Unhandled summary node category \"{}\" in need_wells()",
                    static_cast<int>(node.category))
    };
}

void updateValue(const Opm::EclIO::SummaryNode& node, const double value, Opm::SummaryState& st)
{
    using Cat = Opm::EclIO::SummaryNode::Category;

    switch (node.category) {
    case Cat::Well:
        st.update_well_var(node.wgname, node.keyword, value);
        break;

    case Cat::Group:
    case Cat::Node:
        st.update_group_var(node.wgname, node.keyword, value);
        break;

    case Cat::Connection:
        st.update_conn_var(node.wgname, node.keyword, node.number, value);
        break;

    case Cat::Segment:
        st.update_segment_var(node.wgname, node.keyword, node.number, value);
        break;

    case Cat::Region:
        st.update_region_var(node.fip_region.value_or("FIPNUM"),
                             node.keyword, node.number, value);
        break;

    default:
        st.update(node.unique_key(), value);
        break;
    }
}

/*
 * The well efficiency factor will not impact the well rate itself, but is
 * rather applied for accumulated values.The WEFAC can be considered to shut
 * and open the well for short intervals within the same timestep, and the well
 * is therefore solved at full speed.
 *
 * Groups are treated similarly as wells. The group's GEFAC is not applied for
 * rates, only for accumulated volumes. When GEFAC is set for a group, it is
 * considered that all wells are taken down simultaneously, and GEFAC is
 * therefore not applied to the group's rate. However, any efficiency factors
 * applied to the group's wells or sub-groups must be included.
 *
 * Regions and fields will have the well and group efficiency applied for both
 * rates and accumulated values.
 *
 */
struct EfficiencyFactor
{
    using Factor  = std::pair<std::string, double>;
    using FacColl = std::vector<Factor>;

    FacColl factors{};

    void setFactors(const Opm::EclIO::SummaryNode&       node,
                    const Opm::Schedule&                 schedule,
                    const std::vector<const Opm::Well*>& schedule_wells,
                    const int                            sim_step,
                    const Opm::data::Wells&              sim_res);
};

void EfficiencyFactor::setFactors(const Opm::EclIO::SummaryNode&       node,
                                  const Opm::Schedule&                 schedule,
                                  const std::vector<const Opm::Well*>& schedule_wells,
                                  const int                            sim_step,
                                  const Opm::data::Wells&              sim_res)
{
    this->factors.clear();

    const bool is_field  { node.category == Opm::EclIO::SummaryNode::Category::Field  } ;
    const bool is_group  { node.category == Opm::EclIO::SummaryNode::Category::Group  } ;
    const bool is_region { node.category == Opm::EclIO::SummaryNode::Category::Region } ;
    const bool is_rate   { node.type     != Opm::EclIO::SummaryNode::Type::Total      } ;

    if (!is_field && !is_group && !is_region && is_rate)
        return;

    for (const auto* well : schedule_wells) {
        if (!well->hasBeenDefined(sim_step))
            continue;

        const auto res_it = sim_res.find(well->name());
        double efficiency_scaling_factor = 1.0;
        if (res_it != sim_res.end()) {
            efficiency_scaling_factor = res_it->second.efficiency_scaling_factor;
        }

        double eff_factor = well->getEfficiencyFactor() * efficiency_scaling_factor;
        const auto* group_ptr = std::addressof(schedule.getGroup(well->groupName(), sim_step));

        while (group_ptr) {
            if (is_group && is_rate && (group_ptr->name() == node.wgname))
                break;

            eff_factor *= group_ptr->getGroupEfficiencyFactor();

            const auto parent_group = group_ptr->flow_group();

            if (parent_group.has_value())
                group_ptr = std::addressof(schedule.getGroup( parent_group.value(), sim_step ));
            else
                group_ptr = nullptr;
        }

        this->factors.emplace_back(well->name(), eff_factor);
    }
}

namespace Evaluator {
    struct InputData
    {
        const Opm::EclipseState& es;
        const Opm::Schedule& sched;
        const Opm::EclipseGrid& grid;
        const Opm::out::RegionCache& reg;
        const std::optional<Opm::Inplace>& initial_inplace;
    };

    struct SimulatorResults
    {
        const Opm::data::Wells& wellSol;
        const Opm::data::WellBlockAveragePressures& wbp;
        const Opm::data::GroupAndNetworkValues& grpNwrkSol;
        const std::map<std::string, double>& single;
        const Opm::Inplace inplace;
        const std::map<std::string, std::vector<double>>& region;
        const std::map<std::pair<std::string, int>, double>& block;
        const Opm::data::Aquifers& aquifers;
        const std::unordered_map<std::string, Opm::data::InterRegFlowMap>& ireg;
    };

    class Base
    {
    public:
        virtual ~Base() {}

        virtual void update(const std::size_t       sim_step,
                            const double            stepSize,
                            const InputData&        input,
                            const SimulatorResults& simRes,
                            Opm::SummaryState&      st) const = 0;
    };

    class FunctionRelation : public Base
    {
    public:
        explicit FunctionRelation(Opm::EclIO::SummaryNode node, ofun fcn)
            : node_(std::move(node))
            , fcn_ (std::move(fcn))
        {
            if (this->use_number()) {
                this->number_ = std::max(0, this->node_.number);
            }
        }

        void update(const std::size_t       sim_step,
                    const double            stepSize,
                    const InputData&        input,
                    const SimulatorResults& simRes,
                    Opm::SummaryState&      st) const override
        {
            const auto wells = need_wells(this->node_)
                ? find_wells(input.sched, this->node_,
                             static_cast<int>(sim_step), input.reg)
                : std::vector<const Opm::Well*>{};

            EfficiencyFactor eFac{};
            eFac.setFactors(this->node_, input.sched, wells, sim_step, simRes.wellSol);

            const fn_args args {
                wells, this->group_name(), this->node_.keyword,
                stepSize, static_cast<int>(sim_step),
                this->number_, this->node_.fip_region,
                st,
                simRes.wellSol, simRes.wbp, simRes.grpNwrkSol,
                input.reg, input.grid, input.sched,
                std::move(eFac.factors),
                input.initial_inplace, simRes.inplace,
                input.sched.getUnits()
            };

            const auto& usys = input.es.getUnits();
            const auto  prm  = this->fcn_(args);

            updateValue(this->node_, usys.from_si(prm.unit, prm.value), st);
        }

    private:
        Opm::EclIO::SummaryNode node_;
        ofun                    fcn_;
        int                     number_{0};

        std::string group_name() const
        {
            using Cat = ::Opm::EclIO::SummaryNode::Category;

            if (this->node_.category == Cat::Field) return std::string{"FIELD"};

            const auto need_grp_name =
                (this->node_.category == Cat::Group) ||
                (this->node_.category == Cat::Node);

            const auto def_gr_name = this->node_.category == Cat::Field ? std::string{"FIELD"} : std::string{""};
            return need_grp_name
                ? this->node_.wgname : def_gr_name;
        }

        bool use_number() const
        {
            using Cat = ::Opm::EclIO::SummaryNode::Category;
            const auto cat = this->node_.category;

            return ! ((cat == Cat::Well) ||
                      (cat == Cat::Group) ||
                      (cat == Cat::Field) ||
                      (cat == Cat::Node) ||
                      (cat == Cat::Miscellaneous));
        }
    };

    class BlockValue : public Base
    {
    public:
        explicit BlockValue(Opm::EclIO::SummaryNode node,
                            const Opm::UnitSystem::measure m)
            : node_(std::move(node))
            , m_   (m)
        {}

        void update(const std::size_t    /* sim_step */,
                    const double         /* stepSize */,
                    const InputData&        input,
                    const SimulatorResults& simRes,
                    Opm::SummaryState&      st) const override
        {
            auto xPos = simRes.block.find(this->lookupKey());
            if (xPos == simRes.block.end()) {
                return;
            }

            const auto& usys = input.es.getUnits();
            updateValue(this->node_, usys.from_si(this->m_, xPos->second), st);
        }

    private:
        Opm::EclIO::SummaryNode  node_;
        Opm::UnitSystem::measure m_;

        Opm::out::Summary::BlockValues::key_type lookupKey() const
        {
            return { this->node_.keyword, this->node_.number };
        }
    };

    class AquiferValue: public Base
    {
    public:
        explicit AquiferValue(Opm::EclIO::SummaryNode node,
                              const Opm::UnitSystem::measure m)
        : node_(std::move(node))
        , m_   (m)
        {}

        void update(const std::size_t    /* sim_step */,
                    const double         /* stepSize */,
                    const InputData&        input,
                    const SimulatorResults& simRes,
                    Opm::SummaryState&      st) const override
        {
            auto xPos = simRes.aquifers.find(this->node_.number);
            if (xPos == simRes.aquifers.end()) {
                return;
            }

            const auto& usys = input.es.getUnits();
            updateValue(this->node_, usys.from_si(this->m_, xPos->second.get(this->node_.keyword)), st);
        }
    private:
        Opm::EclIO::SummaryNode  node_;
        Opm::UnitSystem::measure m_;
    };

    class RegionValue : public Base
    {
    public:
        explicit RegionValue(Opm::EclIO::SummaryNode node,
                             const Opm::UnitSystem::measure m)
            : node_(std::move(node))
            , m_   (m)
        {}

        void update(const std::size_t    /* sim_step */,
                    const double         /* stepSize */,
                    const InputData&        input,
                    const SimulatorResults& simRes,
                    Opm::SummaryState&      st) const override
        {
            if (this->node_.number < 0) {
                return;
            }

            auto xPos = simRes.region.find(this->node_.keyword);
            if (xPos == simRes.region.end()) {
                // Vector (e.g., RPR) not available from simulator.
                // Typically at time zero.
                return;
            }

            const auto ix = this->index();
            if (ix >= xPos->second.size()) {
                // Region ID outside active set (e.g., the node specifies
                // region ID 12 when max(FIPNUM) == 10)
                return;
            }

            const auto  val  = xPos->second[ix];
            const auto& usys = input.es.getUnits();

            updateValue(this->node_, usys.from_si(this->m_, val), st);
        }

    private:
        Opm::EclIO::SummaryNode  node_;
        Opm::UnitSystem::measure m_;

        std::vector<double>::size_type index() const
        {
            return this->node_.number - 1;
        }
    };

    class InterRegionValue : public Base
    {
    public:
        explicit InterRegionValue(const Opm::EclIO::SummaryNode& node,
                                  const Opm::UnitSystem::measure m)
            : node_   (node)
            , m_      (m)
            , regname_(node_.fip_region.has_value()
                       ? node_.fip_region.value()
                       : std::string{ "FIPNUM" })
        {
            this->analyzeKeyword();
        }

        void update(const std::size_t    /* sim_step */,
                    const double            stepSize,
                    const InputData&        input,
                    const SimulatorResults& simRes,
                    Opm::SummaryState&      st) const override
        {
            if (this->component_ == Component::NumComponents) {
                return;
            }

            auto flows = simRes.ireg.find(this->regname_);
            if (flows == simRes.ireg.end()) {
                return;
            }

            auto flow = flows->second.getInterRegFlows(this->r1_, this->r2_);
            if (! flow.has_value()) {
                return;
            }

            const auto& usys = input.es.getUnits();
            const auto  val  = this->getValue(flow->first, flow->second, stepSize);

            updateValue(this->node_, usys.from_si(this->m_, val), st);
        }

    private:
        using RateWindow = Opm::data::InterRegFlowMap::ReadOnlyWindow;
        using Component  = RateWindow::Component;
        using Direction  = RateWindow::Direction;

        Opm::EclIO::SummaryNode node_;
        Opm::UnitSystem::measure m_;
        std::string regname_{};

        Component component_{ Component::NumComponents };
        Component subtract_ { Component::NumComponents };
        Direction direction_{ Direction::Positive };
        bool useDirection_{ false };
        bool isCumulative_{ false };
        int r1_{ -1 };
        int r2_{ -1 };

        void analyzeKeyword()
        {
            // Valid keywords are
            //
            // - R[OGW]F[TR]
            //     Basic oil/gas/water flow rates and cumulatives.  FIPNUM
            //     region set.
            //
            // - R[OGW]F[TR][-+]
            //     Directional versions of basic oil/gas/water flow rates
            //     and cumulatives.  FIPNUM region set.
            //
            // - R[OG]F[TR][GL]
            //     Flow rates and cumulatives of free oil (ROF[TR]L),
            //     vaporised oil (ROF[TR]G), free gas (RGF[TR]G), and gas
            //     dissolved in liquid (RGF[TR]L).  FIPNUM region set.
            //
            // - R[OGW]F[TR]_[A-Z0-9]{3}
            //     Basic oil/gas/water flow rates and cumulatives.  User
            //     defined region set (FIP* keyword).
            //
            // - R[OGW]F[TR][-+][A-Z0-9]{3}
            //     Directional versions of basic oil/gas/water flow rates
            //     and cumulatives.  User defined region set (FIP* keyword).
            //
            // - R[OG]F[TR][GL][A-Z0-9]{3}
            //     Flow rates and cumulatives of free oil (ROF[TR]L),
            //     vaporised oil (ROF[TR]G), free gas (RGF[TR]G), and gas
            //     dissolved in liquid (RGF[TR]L).  User defined region set
            //     (FIP* keyword).
            //
            // We don't need a full keyword verification here, only to
            // extract the pertinent keyword pieces, because the input
            // keyword validity is enforced at the parser level.  See json
            // descriptions REGION2REGION_PROBE and REGION2REGION_PROBE_E300
            // in input/eclipse/share/keywords.
            //
            // Note that we explicitly disregard the region set name here as
            // this name does not influence the interpretation of the
            // summary vector keyword-only the definition of the individual
            // regions.

            static const auto pattern = std::regex {
                R"~~(R([OGW])F([RT])([-+GL]?)(?:_?[A-Z0-9_]{3})?)~~"
            };

            auto keywordPieces = std::smatch {};
            if (std::regex_match(this->node_.keyword, keywordPieces, pattern)) {
                this->identifyComponent(keywordPieces);
                this->identifyDirection(keywordPieces);
                this->identifyCumulative(keywordPieces);
                this->assignRegionIDs();
            }
        }

        double getValue(const RateWindow& iregFlow,
                        const double      sign,
                        const double      stepSize) const
        {
            const auto prim = this->useDirection_
                ? iregFlow.flow(this->component_, this->direction_)
                : iregFlow.flow(this->component_);

            const auto Sub = (this->subtract_ == Component::NumComponents)
                ? 0.0 : iregFlow.flow(this->subtract_);

            const auto val = sign * (prim - Sub);

            return this->isCumulative_
                ? stepSize * val
                : val;
        }

        void assignRegionIDs()
        {
            const auto& [r1, r2] =
                Opm::EclIO::splitSummaryNumber(this->node_.number);

            this->r1_ = r1 - 1;
            this->r2_ = r2 - 1;
        }

        void identifyComponent(const std::smatch& keywordPieces)
        {
            const auto main = keywordPieces[1].str();

            if (main == "O") {
                this->component_ = (keywordPieces[3].str() == "G")
                    ? Component::Vapoil
                    : Component::Oil;

                if (keywordPieces[3].str() == "L") {
                    // Free oil = "oil - vapoil"
                    this->subtract_ = Component::Vapoil;
                }
            }
            else if (main == "G") {
                this->component_ = (keywordPieces[3].str() == "L")
                    ? Component::Disgas
                    : Component::Gas;

                if (keywordPieces[3].str() == "G") {
                    // Free gas = "gas - disgas"
                    this->subtract_ = Component::Disgas;
                }
            }
            else if (main == "W") {
                this->component_ = Component::Water;
            }
        }

        void identifyDirection(const std::smatch& keywordPieces)
        {
            if (keywordPieces.length(3) == std::smatch::difference_type{0}) {
                return;
            }

            const auto dir = keywordPieces[3].str();

            this->useDirection_ = (dir == "+") || (dir == "-");

            if (dir == "-") {
                this->direction_ = Direction::Negative;
            }
        }

        void identifyCumulative(const std::smatch& keywordPieces)
        {
            assert (keywordPieces.length(2) != std::smatch::difference_type{0});

            const auto type = keywordPieces[2].str();

            this->isCumulative_ = type == "T";
        }
    };

    class GlobalProcessValue : public Base
    {
    public:
        explicit GlobalProcessValue(Opm::EclIO::SummaryNode node,
                                    const Opm::UnitSystem::measure m)
            : node_(std::move(node))
            , m_   (m)
        {}

        void update(const std::size_t    /* sim_step */,
                    const double         /* stepSize */,
                    const InputData&        input,
                    const SimulatorResults& simRes,
                    Opm::SummaryState&      st) const override
        {
            auto xPos = simRes.single.find(this->node_.keyword);
            if (xPos == simRes.single.end())
                return;

            const auto  val  = xPos->second;
            const auto& usys = input.es.getUnits();

            updateValue(this->node_, usys.from_si(this->m_, val), st);
        }

    private:
        Opm::EclIO::SummaryNode  node_;
        Opm::UnitSystem::measure m_;
    };

    class UserDefinedValue : public Base
    {
    public:
        void update(const std::size_t       /* sim_step */,
                    const double            /* stepSize */,
                    const InputData&        /* input */,
                    const SimulatorResults& /* simRes */,
                    Opm::SummaryState&      /* st */) const override
        {
            // No-op
        }
    };

    class Time : public Base
    {
    public:
        explicit Time(std::string saveKey)
            : saveKey_(std::move(saveKey))
        {}

        void update(const std::size_t       /* sim_step */,
                    const double               stepSize,
                    const InputData&           input,
                    const SimulatorResults& /* simRes */,
                    Opm::SummaryState&         st) const override
        {
            const auto& usys = input.es.getUnits();

            const auto m   = ::Opm::UnitSystem::measure::time;
            const auto val = st.get_elapsed() + stepSize;

            st.update(this->saveKey_, usys.from_si(m, val));
            st.update("TIME", usys.from_si(m, val));
        }

    private:
        std::string saveKey_;
    };

    class Day : public Base
    {
    public:
        explicit Day(std::string saveKey)
            : saveKey_(std::move(saveKey))
        {}

        void update(const std::size_t       /* sim_step */,
                    const double               stepSize,
                    const InputData&           input,
                    const SimulatorResults& /* simRes */,
                    Opm::SummaryState&         st) const override
        {
            auto sim_time = make_sim_time(input.sched, st, stepSize);
            st.update(this->saveKey_, sim_time.day());
        }

    private:
        std::string saveKey_;
    };

    class Month : public Base
    {
    public:
        explicit Month(std::string saveKey)
            : saveKey_(std::move(saveKey))
        {}

        void update(const std::size_t       /* sim_step */,
                    const double               stepSize,
                    const InputData&           input,
                    const SimulatorResults& /* simRes */,
                    Opm::SummaryState&         st) const override
        {
            auto sim_time = make_sim_time(input.sched, st, stepSize);
            st.update(this->saveKey_, sim_time.month());
        }

    private:
        std::string saveKey_;
    };

    class Year : public Base
    {
    public:
        explicit Year(std::string saveKey)
            : saveKey_(std::move(saveKey))
        {}

        void update(const std::size_t       /* sim_step */,
                    const double               stepSize,
                    const InputData&           input,
                    const SimulatorResults& /* simRes */,
                    Opm::SummaryState&         st) const override
        {
            auto sim_time = make_sim_time(input.sched, st, stepSize);
            st.update(this->saveKey_, sim_time.year());
        }

    private:
        std::string saveKey_;
    };

    class Years : public Base
    {
    public:
        explicit Years(std::string saveKey)
            : saveKey_(std::move(saveKey))
        {}

        void update(const std::size_t       /* sim_step */,
                    const double               stepSize,
                    const InputData&        /* input */,
                    const SimulatorResults& /* simRes */,
                    Opm::SummaryState&         st) const override
        {
            using namespace ::Opm::unit;

            const auto val = st.get_elapsed() + stepSize;

            st.update(this->saveKey_, convert::to(val, ecl_year));
        }

    private:
        std::string saveKey_;
    };

    class Factory
    {
    public:
        struct Descriptor
        {
            std::string uniquekey{};
            std::string unit{};
            std::unique_ptr<Base> evaluator{};
        };

        explicit Factory(const Opm::EclipseState& es,
                         const Opm::EclipseGrid&  grid,
                         const Opm::Schedule&     sched,
                         const Opm::SummaryState& st,
                         const Opm::UDQConfig&    udq)
            : es_(es), sched_(sched), grid_(grid), st_(st), udq_(udq)
        {}

        ~Factory() = default;

        Factory(const Factory&) = delete;
        Factory(Factory&&) = delete;
        Factory& operator=(const Factory&) = delete;
        Factory& operator=(Factory&&) = delete;

        Descriptor create(const Opm::EclIO::SummaryNode&);

    private:
        const Opm::EclipseState& es_;
        const Opm::Schedule&     sched_;
        const Opm::EclipseGrid&  grid_;
        const Opm::SummaryState& st_;
        const Opm::UDQConfig&    udq_;

        const Opm::EclIO::SummaryNode* node_{};

        Opm::UnitSystem::measure paramUnit_{Opm::UnitSystem::measure::_count};
        ofun paramFunction_{};

        Descriptor functionRelation();
        Descriptor blockValue();
        Descriptor aquiferValue();
        Descriptor regionValue();
        Descriptor interRegionValue();
        Descriptor globalProcessValue();
        Descriptor userDefinedValue();
        Descriptor unknownParameter();

        bool isBlockValue();
        bool isAquiferValue();
        bool isRegionValue();
        bool isInterRegionValue();
        bool isGlobalProcessValue();
        bool isFunctionRelation();
        bool isUserDefined();

        std::string functionUnitString() const;
        std::string directUnitString() const;
        std::string userDefinedUnit() const;
    };

    Factory::Descriptor Factory::create(const Opm::EclIO::SummaryNode& node)
    {
        this->node_ = &node;

        if (this->isUserDefined())
            return this->userDefinedValue();

        if (this->isBlockValue())
            return this->blockValue();

        if (this->isAquiferValue())
            return this->aquiferValue();

        if (this->isRegionValue())
            return this->regionValue();

        if (this->isInterRegionValue())
            return this->interRegionValue();

        if (this->isGlobalProcessValue())
            return this->globalProcessValue();

        if (this->isFunctionRelation())
            return this->functionRelation();

        return this->unknownParameter();
    }

    Factory::Descriptor Factory::functionRelation()
    {
        auto desc = this->unknownParameter();

        desc.unit = this->functionUnitString();
        desc.evaluator.reset(new FunctionRelation {
            *this->node_, std::move(this->paramFunction_)
        });

        return desc;
    }

    Factory::Descriptor Factory::blockValue()
    {
        auto desc = this->unknownParameter();

        desc.unit = this->directUnitString();
        desc.evaluator.reset(new BlockValue {
            *this->node_, this->paramUnit_
        });

        return desc;
    }

    Factory::Descriptor Factory::aquiferValue()
    {
        auto desc = this->unknownParameter();

        desc.unit = this->directUnitString();
        desc.evaluator.reset(new AquiferValue {
                *this->node_, this->paramUnit_
        });

        return desc;
    }

    Factory::Descriptor Factory::regionValue()
    {
        auto desc = this->unknownParameter();

        desc.unit = this->directUnitString();
        desc.evaluator.reset(new RegionValue {
            *this->node_, this->paramUnit_
        });

        return desc;
    }

    Factory::Descriptor Factory::interRegionValue()
    {
        auto desc = this->unknownParameter();

        desc.unit = this->directUnitString();
        desc.evaluator.reset(new InterRegionValue {
            *this->node_, this->paramUnit_
        });

        return desc;
    }

    Factory::Descriptor Factory::globalProcessValue()
    {
        auto desc = this->unknownParameter();

        desc.unit = this->directUnitString();
        desc.evaluator.reset(new GlobalProcessValue {
            *this->node_, this->paramUnit_
        });

        return desc;
    }

    Factory::Descriptor Factory::userDefinedValue()
    {
        auto desc = this->unknownParameter();

        desc.unit = this->userDefinedUnit();
        desc.evaluator.reset(new UserDefinedValue {});

        return desc;
    }

    Factory::Descriptor Factory::unknownParameter()
    {
        auto desc = Descriptor{};

        desc.uniquekey = this->node_->unique_key();

        return desc;
    }

    bool Factory::isBlockValue()
    {
        auto pos = block_units.find(this->node_->keyword);
        if (pos == block_units.end())
            return false;

        if (! this->grid_.cellActive(this->node_->number - 1))
            // 'node_' is a block value, but it is configured in a
            // deactivated cell.  Don't create an evaluation function.
            return false;

        // 'node_' represents a block value in an active cell.
        // Capture unit of measure and return true.
        this->paramUnit_ = pos->second;
        return true;
    }

    bool Factory::isAquiferValue()
    {
        auto pos = aquifer_units.find(this->node_->keyword);
        if (pos == aquifer_units.end()) return false;

        // if the aquifer does not exist, should we warn?
        if ( !this->es_.aquifer().hasAquifer(this->node_->number) ) return false;

        this->paramUnit_ = pos->second;
        return true;
    }

    bool Factory::isRegionValue()
    {
        auto keyword = this->node_->keyword;
        auto dash_pos = keyword.find("_");
        if (dash_pos != std::string::npos)
            keyword = keyword.substr(0, dash_pos);

        auto pos = region_units.find(keyword);
        if (pos == region_units.end())
            return false;

        // 'node_' represents a region value.  Capture unit
        // of measure and return true.
        this->paramUnit_ = pos->second;
        return true;
    }

    bool Factory::isInterRegionValue()
    {
        const auto end = std::min({
            // Infinity (std::string::npos) if no underscore
            this->node_->keyword.find('_'),

            // Don't look beyond end of keyword string.
            this->node_->keyword.size(),

            // Always at most 5 characters in the "real" keyword.
            std::string::size_type{5},
        });

        auto pos = interregion_units.find(this->node_->keyword.substr(0, end));
        if (pos == interregion_units.end()) {
            // Node_'s canonical form reduced keyword does not match any of
            // the supported inter-region flow summary vector keywords.
            return false;
        }

        // 'Node_' represents a supported inter-region summary vector.
        // Capture unit of measure and return true.
        this->paramUnit_ = pos->second;
        return true;
    }

    bool Factory::isGlobalProcessValue()
    {
        auto pos = single_values_units.find(this->node_->keyword);
        if (pos == single_values_units.end())
            return false;

        // 'node_' represents a single value (i.e., global process)
        // value.  Capture unit of measure and return true.
        this->paramUnit_ = pos->second;
        return true;
    }

    bool Factory::isFunctionRelation()
    {
        const auto normKw = (this->node_->category == Opm::EclIO::SummaryNode::Category::Region)
            ? Opm::EclIO::SummaryNode::normalise_region_keyword(this->node_->keyword)
            : Opm::EclIO::SummaryNode::normalise_keyword(this->node_->category, this->node_->keyword);

        auto pos = funs.find(normKw);
        if (pos != funs.end()) {
            // 'node_' represents a functional relation.
            // Capture evaluation function and return true.
            this->paramFunction_ = pos->second;
            return true;
        }

        if (normKw.length() <= std::string::size_type{4}) {
            return false;
        }

        const auto& tracers = this->es_.tracer();

        // Check for tracer names twice to allow for tracers starting with S or F
        auto istart = 4;
        auto tracer_name = normKw.substr(istart);
        auto trPos = std::find_if(tracers.begin(), tracers.end(),
                                  [&tracer_name](const auto& tracer)
                                  {
                                      return tracer.name == tracer_name;
                                  });

        if (trPos == tracers.end()) {
            if ((normKw[4] == 'F') || (normKw[4] == 'S'))
                istart = 5;
            else
                return false;
            tracer_name = normKw.substr(istart);
            trPos = std::find_if(tracers.begin(), tracers.end(),
                                    [&tracer_name](const auto& tracer)
                                    {
                                        return tracer.name == tracer_name;
                                    });

            if (trPos == tracers.end())
                return false;
        }

        auto tracer_tag = normKw.substr(0, istart);
        switch (trPos->phase) {
        case Opm::Phase::WATER:
            tracer_tag += "#W";
            break;

        case Opm::Phase::OIL:
            tracer_tag += "#O";
            break;

        case Opm::Phase::GAS:
            tracer_tag += "#G";
            break;

        default:
            return false;
        }

        pos = funs.find(tracer_tag);
        if (pos != funs.end()) {
            this->paramFunction_ = pos->second;
            return true;
        }

        return false;
    }

    bool Factory::isUserDefined()
    {
        return this->node_->is_user_defined();
    }

    std::string Factory::functionUnitString() const
    {
        const auto unit_string_tracer = this->es_.tracer()
            .get_unit_string(this->es_.getUnits(),
                             this->node_->keyword);

        if (! unit_string_tracer.empty()) {
            // Non-default unit for tracer amount.
            return unit_string_tracer;
        }

        const auto reg = Opm::out::RegionCache{};

        const fn_args args {
            {}, "", this->node_->keyword, 0.0, 0,
            this->node_->number, this->node_->fip_region,
            this->st_,
            {}, {}, {},
            reg, this->grid_, this->sched_,
            {}, {}, {}, this->es_.getUnits()
        };

        const auto prm = this->paramFunction_(args);

        return this->es_.getUnits().name(prm.unit);
    }

    std::string Factory::directUnitString() const
    {
        return this->es_.getUnits().name(this->paramUnit_);
    }

    std::string Factory::userDefinedUnit() const
    {
        const auto& kw = this->node_->keyword;

        return this->udq_.has_unit(kw)
            ?  this->udq_.unit(kw) : "";
    }
} // namespace Evaluator

void reportUnsupportedKeywords(std::vector<Opm::SummaryConfigNode> keywords)
{
    // Sort by location first, then keyword.
    auto loc_kw_ordering = [](const Opm::SummaryConfigNode& n1, const Opm::SummaryConfigNode& n2) {
        if (n1.location() == n2.location()) {
            return n1.keyword() < n2.keyword();
        }
        if (n1.location().filename == n2.location().filename) {
            return n1.location().lineno < n2.location().lineno;
        }
        return n1.location().filename < n2.location().filename;
    };
    std::sort(keywords.begin(), keywords.end(), loc_kw_ordering);

    // Reorder to remove duplicate { keyword, location } pairs, since
    // that will give duplicate and therefore useless warnings.
    auto same_kw_and_loc = [](const Opm::SummaryConfigNode& n1, const Opm::SummaryConfigNode& n2) {
        return (n1.keyword() == n2.keyword()) && (n1.location() == n2.location());
    };
    auto uend = std::unique(keywords.begin(), keywords.end(), same_kw_and_loc);

    for (auto node = keywords.begin(); node != uend; ++node) {
        const auto& location = node->location();
        Opm::OpmLog::warning(Opm::OpmInputError::format("Unhandled summary keyword {keyword}\n"
                                                        "In {file} line {line}", location));
    }
}

std::string makeWGName(std::string name)
{
    // Use default WGNAME if 'name' is empty or consists
    // exlusively of white-space (space and tab) characters.
    //
    // Use 'name' itself otherwise.

    const auto use_dflt = name.empty() ||
        (name.find_first_not_of(" \t") == std::string::npos);

    return use_dflt ? std::string(":+:+:+:+") : std::move(name);
}

class SummaryOutputParameters
{
public:
    using EvalPtr = std::unique_ptr<Evaluator::Base>;
    using SMSpecPrm = Opm::EclIO::OutputStream::
        SummarySpecification::Parameters;

    SummaryOutputParameters() = default;
    ~SummaryOutputParameters() = default;

    SummaryOutputParameters(const SummaryOutputParameters& rhs) = delete;
    SummaryOutputParameters(SummaryOutputParameters&& rhs) = default;

    SummaryOutputParameters&
    operator=(const SummaryOutputParameters& rhs) = delete;

    SummaryOutputParameters&
    operator=(SummaryOutputParameters&& rhs) = default;

    void makeParameter(std::string keyword,
                       std::string name,
                       const int   num,
                       std::string unit,
                       EvalPtr     evaluator)
    {
        this->smspec_.add(std::move(keyword), std::move(name),
                          std::max (num, 0), std::move(unit));

        this->evaluators_.push_back(std::move(evaluator));
    }

    const SMSpecPrm& summarySpecification() const
    {
        return this->smspec_;
    }

    const std::vector<EvalPtr>& getEvaluators() const
    {
        return this->evaluators_;
    }

private:
    SMSpecPrm smspec_{};
    std::vector<EvalPtr> evaluators_{};
};

class SMSpecStreamDeferredCreation
{
private:
    using Spec = ::Opm::EclIO::OutputStream::SummarySpecification;

public:
    using ResultSet = ::Opm::EclIO::OutputStream::ResultSet;
    using Formatted = ::Opm::EclIO::OutputStream::Formatted;

    explicit SMSpecStreamDeferredCreation(const Opm::InitConfig&          initcfg,
                                          const Opm::EclipseGrid&         grid,
                                          const std::time_t               start,
                                          const Opm::UnitSystem::UnitType utype);

    std::unique_ptr<Spec>
    createStream(const ResultSet& rset, const Formatted& fmt) const
    {
        return std::make_unique<Spec>(rset, fmt, this->uconv(),
                                      this->cartDims_, this->restart_,
                                      this->start_);
    }

private:
    Opm::UnitSystem::UnitType  utype_;
    std::array<int,3>          cartDims_;
    Spec::StartTime            start_;
    Spec::RestartSpecification restart_{};

    Spec::UnitConvention uconv() const;
};

SMSpecStreamDeferredCreation::
SMSpecStreamDeferredCreation(const Opm::InitConfig&          initcfg,
                             const Opm::EclipseGrid&         grid,
                             const std::time_t               start,
                             const Opm::UnitSystem::UnitType utype)
    : utype_   (utype)
    , cartDims_(grid.getNXYZ())
    , start_   (Opm::TimeService::from_time_t(start))
{
    if (initcfg.restartRequested()) {
        this->restart_.root = initcfg.getRestartRootNameInput();
        this->restart_.step = initcfg.getRestartStep();
    }
}

SMSpecStreamDeferredCreation::Spec::UnitConvention
SMSpecStreamDeferredCreation::uconv() const
{
    using UType = ::Opm::UnitSystem::UnitType;

    if (this->utype_ == UType::UNIT_TYPE_METRIC)
        return Spec::UnitConvention::Metric;

    if (this->utype_ == UType::UNIT_TYPE_FIELD)
        return Spec::UnitConvention::Field;

    if (this->utype_ == UType::UNIT_TYPE_LAB)
        return Spec::UnitConvention::Lab;

    if (this->utype_ == UType::UNIT_TYPE_PVT_M)
        return Spec::UnitConvention::Pvt_M;

    throw std::invalid_argument {
        "Unsupported Unit Convention (" +
        std::to_string(static_cast<int>(this->utype_)) + ')'
    };
}

std::unique_ptr<SMSpecStreamDeferredCreation>
makeDeferredSMSpecCreation(const Opm::EclipseState& es,
                           const Opm::EclipseGrid&  grid,
                           const Opm::Schedule&     sched)
{
    return std::make_unique<SMSpecStreamDeferredCreation>
        (es.cfg().init(), grid, sched.posixStartTime(),
         es.getUnits().getType());
}

std::string makeUpperCase(std::string input)
{
    for (auto& c : input) {
        const auto u = std::toupper(static_cast<unsigned char>(c));
        c = static_cast<std::string::value_type>(u);
    }

    return input;
}

Opm::EclIO::OutputStream::ResultSet
makeResultSet(const Opm::IOConfig& iocfg, const std::string& basenm)
{
    const auto& base = basenm.empty()
        ? makeUpperCase(iocfg.getBaseName())
        : basenm;

    return { iocfg.getOutputDir(), base };
}

void validateElapsedTime(const double             secs_elapsed,
                         const Opm::EclipseState& es,
                         const Opm::SummaryState& st)
{
    if (! (secs_elapsed < st.get_elapsed())) {
        return;
    }

    const auto& usys    = es.getUnits();
    const auto  elapsed = usys.from_si(measure::time, secs_elapsed);
    const auto  prev_el = usys.from_si(measure::time, st.get_elapsed());
    const auto  unt     = '[' + std::string{ usys.name(measure::time) } + ']';

    throw std::invalid_argument {
        fmt::format("Elapsed time ({} {}) "
                    "must not precede previous elapsed time ({} {}). "
                    "Incorrect restart time?", elapsed, unt, prev_el, unt)
    };
}

} // Anonymous namespace

class Opm::out::Summary::SummaryImplementation
{
public:
    explicit SummaryImplementation(SummaryConfig&      sumcfg,
                                   const EclipseState& es,
                                   const EclipseGrid&  grid,
                                   const Schedule&     sched,
                                   const std::string&  basename,
                                   const bool          writeEsmry);

    SummaryImplementation(const SummaryImplementation& rhs) = delete;
    SummaryImplementation(SummaryImplementation&& rhs) = default;
    SummaryImplementation& operator=(const SummaryImplementation& rhs) = delete;
    SummaryImplementation& operator=(SummaryImplementation&& rhs) = default;

    void eval(const int                              sim_step,
              const double                           secs_elapsed,
              const data::Wells&                     well_solution,
              const data::WellBlockAveragePressures& wbp,
              const data::GroupAndNetworkValues&     grp_nwrk_solution,
              GlobalProcessParameters                single_values,
              const std::optional<Inplace>&          initial_inplace,
              const Opm::Inplace&                    inplace,
              const RegionParameters&                region_values,
              const BlockValues&                     block_values,
              const data::Aquifers&                  aquifer_values,
              const InterRegFlowValues&              interreg_flows,
              SummaryState&                          st) const;

    void internal_store(const SummaryState& st,
                        const int           report_step,
                        const int           ministep_id,
                        const bool          isSubstep);

    void write(const bool is_final_summary);

private:
    struct MiniStep
    {
        int id{0};
        int seq{-1};
        bool isSubstep{false};
        std::vector<float> params{};
    };

    using EvalPtr = SummaryOutputParameters::EvalPtr;

    std::reference_wrapper<const Opm::EclipseGrid> grid_;
    std::reference_wrapper<const Opm::EclipseState> es_;
    std::reference_wrapper<const Opm::Schedule> sched_;
    Opm::out::RegionCache regCache_{};

    std::unique_ptr<SMSpecStreamDeferredCreation> deferredSMSpec_;

    Opm::EclIO::OutputStream::ResultSet rset_;
    Opm::EclIO::OutputStream::Formatted fmt_;
    Opm::EclIO::OutputStream::Unified   unif_;

    int prevCreate_{-1};
    int prevReportStepID_{-1};
    std::vector<MiniStep>::size_type numUnwritten_{0};

    SummaryOutputParameters                  outputParameters_{};
    std::unordered_map<std::string, EvalPtr> extra_parameters{};
    std::vector<std::string> valueKeys_{};
    std::vector<std::string> valueUnits_{};
    std::vector<MiniStep>    unwritten_{};

    std::unique_ptr<Opm::EclIO::OutputStream::SummarySpecification> smspec_{};
    std::unique_ptr<Opm::EclIO::EclOutput> stream_{};

    std::unique_ptr<Opm::EclIO::ExtSmryOutput> esmry_;

    void configureTimeVector(const EclipseState& es, const std::string& kw);
    void configureTimeVectors(const EclipseState& es, const SummaryConfig& sumcfg);

    void configureSummaryInput(const SummaryConfig& sumcfg,
                               Evaluator::Factory&  evaluatorFactory);

    void configureRequiredRestartParameters(const SummaryConfig& sumcfg,
                                            const AquiferConfig& aqConfig,
                                            const Schedule&      sched,
                                            Evaluator::Factory&  evaluatorFactory);

    void configureUDQ(const EclipseState& es,
                      const Schedule&     sched,
                      Evaluator::Factory& evaluatorFactory,
                      SummaryConfig&      summary_config);

    MiniStep& getNextMiniStep(const int  report_step,
                              const int  ministep_id,
                              const bool isSubstep);

    const MiniStep& lastUnwritten() const;

    void write(const MiniStep& ms);

    void createSMSpecIfNecessary();
    void createSmryStreamIfNecessary(const int report_step);
};

Opm::out::Summary::SummaryImplementation::
SummaryImplementation(SummaryConfig&      sumcfg,
                      const EclipseState& es,
                      const EclipseGrid&  grid,
                      const Schedule&     sched,
                      const std::string&  basename,
                      const bool          writeEsmry)
    : grid_          (std::cref(grid))
    , es_            (std::cref(es))
    , sched_         (std::cref(sched))
    , deferredSMSpec_(makeDeferredSMSpecCreation(es, grid, sched))
    , rset_          (makeResultSet(es.cfg().io(), basename))
    , fmt_           { es.cfg().io().getFMTOUT() }
    , unif_          { es.cfg().io().getUNIFOUT() }
{
    const auto st = SummaryState {
        TimeService::from_time_t(sched.getStartTime()),
        es.runspec().udqParams().undefinedValue()
    };

    Evaluator::Factory evaluatorFactory {
        es, grid, sched, st, sched.getUDQConfig(sched.size() - 1)
    };

    this->configureTimeVectors(es, sumcfg);
    this->configureSummaryInput(sumcfg, evaluatorFactory);
    this->configureRequiredRestartParameters(sumcfg, es.aquifer(),
                                             sched, evaluatorFactory);

    this->configureUDQ(es, sched, evaluatorFactory, sumcfg);

    this->regCache_.buildCache(sumcfg.fip_regions(),
                               es.globalFieldProps(),
                               grid, sched);

    const auto esmryFileName = EclIO::OutputStream::
        outputFileName(this->rset_, "ESMRY");

    if (std::filesystem::exists(esmryFileName)) {
        std::filesystem::remove(esmryFileName);
    }

    if (writeEsmry && !es.cfg().io().getFMTOUT()) {
        this->esmry_ = std::make_unique<Opm::EclIO::ExtSmryOutput>
            (this->valueKeys_, this->valueUnits_, es, sched.posixStartTime());
    }

    if (writeEsmry && es.cfg().io().getFMTOUT()) {
        OpmLog::warning("ESMRY only supported for unformatted output. Request ignored.");
    }
}

void Opm::out::Summary::SummaryImplementation::
internal_store(const SummaryState& st,
               const int           report_step,
               const int           ministep_id,
               const bool          isSubstep)
{
    auto& ms = this->getNextMiniStep(report_step, ministep_id, isSubstep);

    const auto nParam = this->valueKeys_.size();

    for (auto i = decltype(nParam){0}; i < nParam; ++i) {
        if (! st.has(this->valueKeys_[i]))
            // Parameter not yet evaluated (e.g., well/group not
            // yet active).  Nothing to do here.
            continue;

        ms.params[i] = st.get(this->valueKeys_[i]);
    }
}

void
Opm::out::Summary::SummaryImplementation::
eval(const int                              sim_step,
     const double                           secs_elapsed,
     const data::Wells&                     well_solution,
     const data::WellBlockAveragePressures& wbp,
     const data::GroupAndNetworkValues&     grp_nwrk_solution,
     GlobalProcessParameters                single_values,
     const std::optional<Inplace>&          initial_inplace,
     const Opm::Inplace&                    inplace,
     const RegionParameters&                region_values,
     const BlockValues&                     block_values,
     const data::Aquifers&                  aquifer_values,
     const InterRegFlowValues&              interreg_flows,
     Opm::SummaryState&                     st) const
{
    validateElapsedTime(secs_elapsed, this->es_, st);

    const auto duration = secs_elapsed - st.get_elapsed();

    single_values["TIMESTEP"] = duration;
    st.update("TIMESTEP", this->es_.get().getUnits().from_si(Opm::UnitSystem::measure::time, duration));

    const Evaluator::InputData input {
        this->es_, this->sched_, this->grid_, this->regCache_, initial_inplace
    };

    const Evaluator::SimulatorResults simRes {
        well_solution, wbp, grp_nwrk_solution, single_values, inplace,
        region_values, block_values, aquifer_values, interreg_flows
    };

    for (auto& evalPtr : this->outputParameters_.getEvaluators()) {
        evalPtr->update(sim_step, duration, input, simRes, st);
    }

    for (auto& [_, evalPtr] : this->extra_parameters) {
        (void)_;
        evalPtr->update(sim_step, duration, input, simRes, st);
    }

    st.update_elapsed(duration);
}

void Opm::out::Summary::SummaryImplementation::write(const bool is_final_summary)
{
    const auto zero = std::vector<MiniStep>::size_type{0};
    if (this->numUnwritten_ == zero) {
        // No unwritten data.  Nothing to do so return early.
        return;
    }

    this->createSMSpecIfNecessary();

    if (this->prevReportStepID_ < this->lastUnwritten().seq) {
        this->smspec_->write(this->outputParameters_.summarySpecification());
    }

    for (auto i = 0*this->numUnwritten_; i < this->numUnwritten_; ++i) {
        this->write(this->unwritten_[i]);
    }

    // Eagerly output last set of parameters to permanent storage.
    this->stream_->flushStream();

    if (this->esmry_ != nullptr) {
        for (auto i = 0*this->numUnwritten_; i < this->numUnwritten_; ++i) {
            this->esmry_->write(this->unwritten_[i].params,
                                this->unwritten_[i].seq,
                                is_final_summary);
        }
    }

    // Reset "unwritten" counter to reflect the fact that we've
    // output all stored ministeps.
    this->numUnwritten_ = zero;
}

void Opm::out::Summary::SummaryImplementation::write(const MiniStep& ms)
{
    this->createSmryStreamIfNecessary(ms.seq);

    if (this->prevReportStepID_ < ms.seq) {
        // XXX: Should probably write SEQHDR = 0 here since
        ///     we do not know the actual encoding needed.
        this->stream_->write("SEQHDR", std::vector<int>{ ms.seq });
        this->prevReportStepID_ = ms.seq;
    }

    this->stream_->write("MINISTEP", std::vector<int>{ ms.id });
    this->stream_->write("PARAMS"  , ms.params);
}

void
Opm::out::Summary::SummaryImplementation::
configureTimeVector(const EclipseState& es, const std::string& kw)
{
    const auto dfltwgname = makeWGName("");
    const auto dfltnum    = 0;

    this->valueKeys_.push_back(kw);

    if (kw == "TIME") {
        const auto* unit_string =
            es.getUnits().name(UnitSystem::measure::time);

        this->valueUnits_.push_back(unit_string);

        this->outputParameters_
            .makeParameter(kw, dfltwgname, dfltnum, unit_string,
                           std::make_unique<Evaluator::Time>(kw));
    }

    else if (kw == "DAY") {
        this->valueUnits_.push_back("");

        this->outputParameters_
            .makeParameter(kw, dfltwgname, dfltnum, "",
                           std::make_unique<Evaluator::Day>(kw));
    }

    else if ((kw == "MONTH") || (kw == "MNTH")) {
        this->valueUnits_.push_back("");

        this->outputParameters_
            .makeParameter(kw, dfltwgname, dfltnum, "",
                           std::make_unique<Evaluator::Month>(kw));
    }

    else if (kw == "YEAR") {
        this->valueUnits_.push_back("");

        this->outputParameters_
            .makeParameter(kw, dfltwgname, dfltnum, "",
                           std::make_unique<Evaluator::Year>(kw));
    }

    else if (kw == "YEARS") {
        this->valueUnits_.push_back("");

        this->outputParameters_
            .makeParameter(kw, dfltwgname, dfltnum, kw,
                           std::make_unique<Evaluator::Years>(kw));
    }
}

void
Opm::out::Summary::SummaryImplementation::
configureTimeVectors(const EclipseState&  es,
                     const SummaryConfig& sumcfg)
{
    // TIME and YEARS are always available.
    for (const auto* kw : { "TIME", "YEARS", }) {
        this->configureTimeVector(es, kw);
    }

    // DAY, MONTH, and YEAR only output if specifically requested.
    for (const auto* kw : { "DAY", "MONTH", "YEAR", }) {
        if (sumcfg.hasKeyword(kw)) {
            this->configureTimeVector(es, kw);
        }
    }
}

void
Opm::out::Summary::SummaryImplementation::
configureSummaryInput(const SummaryConfig& sumcfg,
                      Evaluator::Factory&  evaluatorFactory)
{
    auto unsuppkw = std::vector<SummaryConfigNode>{};
    for (const auto& node : sumcfg) {
        auto prmDescr = evaluatorFactory.create(node);

        if (! prmDescr.evaluator) {
            // No known evaluation function/type for this keyword
            unsuppkw.push_back(node);
            continue;
        }

        // This keyword has a known evaluation method.

        this->valueKeys_.push_back(std::move(prmDescr.uniquekey));
        this->valueUnits_.push_back(prmDescr.unit);

        this->outputParameters_
            .makeParameter(node.keyword(),
                           makeWGName(node.namedEntity()),
                           node.number(),
                           std::move(prmDescr.unit),
                           std::move(prmDescr.evaluator));
    }

    if (! unsuppkw.empty()) {
        reportUnsupportedKeywords(std::move(unsuppkw));
    }
}

// These nodes are added to the summary evaluation list because they are
// requested by the UDQ system.  In the case of well and group variables the
// code will add nodes for every well/group in the model--irrespective of
// what has been requested in the UDQ code.

namespace {

    template <typename... ExtraArgs>
    Opm::EclIO::SummaryNode
    make_node(const Opm::SummaryConfigNode& node,
              ExtraArgs&&...                extraArgs)
    {
        return {
            node.keyword(), node.category(), node.type(),
            std::forward<ExtraArgs>(extraArgs)...
        };
    }

    Opm::EclIO::SummaryNode translate_node(const Opm::SummaryConfigNode& node)
    {
        using Cat = Opm::SummaryConfigNode::Category;

        switch (node.category()) {
        case Cat::Field:
        case Cat::Miscellaneous:
            return make_node(node);

        case Cat::Group:
        case Cat::Node:
        case Cat::Well:
            return make_node(node, node.namedEntity());

        case Cat::Connection:
        case Cat::Completion:
        case Cat::Segment:
            return make_node(node, node.namedEntity(), node.number());

        case Cat::Block:
        case Cat::Aquifer:
            // No named entity in these categories
            return make_node(node, std::string {}, node.number());

        case Cat::Region:
            // No named entity in this category
            return make_node(node, std::string {}, node.number(), node.fip_region());
        }

        throw std::logic_error {
            fmt::format("Keyword category '{}' (e.g., summary "
                        "keyword {}) is not supported in ACTIONX",
                        node.category(), node.keyword())
        };
    }

    std::vector<Opm::EclIO::SummaryNode>
    requisite_udq_and_action_summary_nodes(const Opm::EclipseState& es,
                                           const Opm::Schedule&     sched,
                                           Opm::SummaryConfig&      smcfg)
    {
        auto nodes = std::vector<Opm::EclIO::SummaryNode>{};

        auto summary_keys = std::unordered_set<std::string>{};

        for (const auto& unique_udqs : sched.unique<Opm::UDQConfig>()) {
            unique_udqs.second.required_summary(summary_keys);
        }

        for (const auto& action : sched.back().actions.get()) {
            action.required_summary(summary_keys);
        }

        // Individual month names--typically used in ACTIONX conditions
        // involving time--are handled elsewhere (Opm::Action::Context
        // constructor) so exclude those from processing here.
        auto extraKeys = std::vector<std::string>{};
        extraKeys.reserve(summary_keys.size());

        std::copy_if(summary_keys.begin(), summary_keys.end(),
                     std::back_inserter(extraKeys),
                     [](const std::string& key)
                     { return ! Opm::TimeService::valid_month(key); });

        const auto newNodes = smcfg
            .registerRequisiteUDQorActionSummaryKeys(extraKeys, es, sched);

        std::transform(newNodes.begin(), newNodes.end(),
                       std::back_inserter(nodes),
                       [](const auto& newNode)
                       {
                           return translate_node(newNode);
                       });

        return nodes;
    }
}

void
Opm::out::Summary::SummaryImplementation::
configureUDQ(const EclipseState& es,
             const Schedule&     sched,
             Evaluator::Factory& evaluatorFactory,
             SummaryConfig&      summary_config)
{
    const auto time_vectors = std::unordered_set<std::string> {
        "TIME", "DAY", "MONTH", "YEAR", "YEARS", "MNTH",
    };

    auto has_evaluator = [this](const auto& key)
    {
        return std::find(this->valueKeys_.begin(),
                         this->valueKeys_.end(), key)
            != this->valueKeys_.end();
    };

    for (const auto& node : requisite_udq_and_action_summary_nodes(es, sched, summary_config)) {
        // Time related vectors are special cased in the valueKeys_ vector
        // and must be checked explicitly.
        if ((time_vectors.find(node.keyword) != time_vectors.end()) &&
            ! has_evaluator(node.keyword))
        {
            this->configureTimeVector(es, node.keyword);

            continue;
        }

        if (has_evaluator(node.unique_key())) {
            // Handler already registered in the summary evaluator in some
            // other way--e.g., the required restart vectors.

            continue;
        }

        auto descr = evaluatorFactory.create(node);

        if (descr.evaluator == nullptr) {
            if (node.is_user_defined()) {
                continue;
            }

            throw std::logic_error {
                fmt::format("Evaluation function for summary "
                            "vector '{}' ({}/{}) not found",
                            node.keyword, node.category, node.type)
            };
        }

        this->extra_parameters.emplace(descr.uniquekey, std::move(descr.evaluator));
    }
}

void
Opm::out::Summary::SummaryImplementation::
configureRequiredRestartParameters(const SummaryConfig& sumcfg,
                                   const AquiferConfig& aqConfig,
                                   const Schedule&      sched,
                                   Evaluator::Factory&  evaluatorFactory)
{
    auto makeEvaluator = [&sumcfg, &evaluatorFactory, this]
        (const Opm::EclIO::SummaryNode& node) -> void
    {
        if (sumcfg.hasSummaryKey(node.unique_key()))
            // Handler already exists.  Don't add second evaluation.
            return;

        auto descriptor = evaluatorFactory.create(node);
        if (descriptor.evaluator == nullptr)
            throw std::logic_error {
                fmt::format("Evaluation function for:{} not found", node.keyword)
            };

        this->extra_parameters
            .emplace(node.unique_key(), std::move(descriptor.evaluator));
    };

    for (const auto& node : requiredRestartVectors(sched)) {
        makeEvaluator(node);
    }

    for (const auto& node : requiredSegmentVectors(sched)) {
        makeEvaluator(node);
    }

    if (aqConfig.hasAnalyticalAquifer()) {
        const auto aquiferIDs = analyticAquiferIDs(aqConfig);

        for (const auto& node : requiredAquiferVectors(aquiferIDs)) {
            makeEvaluator(node);
        }
    }

    if (aqConfig.hasNumericalAquifer()) {
        const auto aquiferIDs = numericAquiferIDs(aqConfig);

        for (const auto& node : requiredNumericAquiferVectors(aquiferIDs)) {
            makeEvaluator(node);
        }
    }
}

Opm::out::Summary::SummaryImplementation::MiniStep&
Opm::out::Summary::SummaryImplementation::
getNextMiniStep(const int  report_step,
                const int  ministep_id,
                const bool isSubstep)
{
    if (this->numUnwritten_ == this->unwritten_.size()) {
        this->unwritten_.emplace_back();
    }

    assert ((this->numUnwritten_ < this->unwritten_.size()) &&
            "Internal inconsistency in 'unwritten' counter");

    auto& ms = this->unwritten_[this->numUnwritten_++];

    ms.id  = ministep_id;
    ms.seq = report_step;
    ms.isSubstep = isSubstep;

    ms.params.resize(this->valueKeys_.size(), 0.0f);

    std::fill(ms.params.begin(), ms.params.end(), 0.0f);

    return ms;
}

const Opm::out::Summary::SummaryImplementation::MiniStep&
Opm::out::Summary::SummaryImplementation::lastUnwritten() const
{
    assert (this->numUnwritten_ <= this->unwritten_.size());
    assert (this->numUnwritten_ >  decltype(this->numUnwritten_){0});

    return this->unwritten_[this->numUnwritten_ - 1];
}

void Opm::out::Summary::SummaryImplementation::createSMSpecIfNecessary()
{
    if (this->deferredSMSpec_) {
        // We need an SMSPEC file and none exists.  Create it and release
        // the resources captured to make the deferred creation call.
        this->smspec_ = this->deferredSMSpec_
            ->createStream(this->rset_, this->fmt_);

        this->deferredSMSpec_.reset();
    }
}

void
Opm::out::Summary::SummaryImplementation::
createSmryStreamIfNecessary(const int report_step)
{
    // Create stream if unset or if non-unified (separate) and new step.

    assert ((this->prevCreate_ <= report_step) &&
            "Inconsistent Report Step Sequence Detected");

    const auto do_create = ! this->stream_
        || (! this->unif_.set && (this->prevCreate_ < report_step));

    if (do_create) {
        this->stream_ = Opm::EclIO::OutputStream::
            createSummaryFile(this->rset_, report_step,
                              this->fmt_, this->unif_);

        this->prevCreate_ = report_step;
    }
}

// ===========================================================================
// Public Interface Below Separator
// ===========================================================================

namespace Opm::out {

Summary::Summary(SummaryConfig&       sumcfg,
                 const EclipseState&  es,
                 const EclipseGrid&   grid,
                 const Schedule&      sched,
                 const std::string&   basename,
                 const bool           writeEsmry)
    : pImpl_ { std::make_unique<SummaryImplementation>(sumcfg, es, grid, sched, basename, writeEsmry) }
{}

void Summary::eval(SummaryState&                          st,
                   const int                              report_step,
                   const double                           secs_elapsed,
                   const data::Wells&                     well_solution,
                   const data::WellBlockAveragePressures& wbp,
                   const data::GroupAndNetworkValues&     grp_nwrk_solution,
                   const GlobalProcessParameters&         single_values,
                   const std::optional<Inplace>&          initial_inplace,
                   const Inplace&                         inplace,
                   const RegionParameters&                region_values,
                   const BlockValues&                     block_values,
                   const Opm::data::Aquifers&             aquifer_values,
                   const InterRegFlowValues&              interreg_flows) const
{
    // Report_step is the one-based sequence number of the containing report.
    // Report_step = 0 for the initial condition, before simulation starts.
    // We typically don't get reports_step = 0 here.  When outputting
    // separate summary files 'report_step' is the number that gets
    // incorporated into the filename extension.
    //
    // Sim_step is the timestep which has been effective in the simulator,
    // and as such is the value necessary to use when looking up active
    // wells, groups, connections &c in the Schedule object.
    const auto sim_step = std::max(0, report_step - 1);

    auto process_values = single_values;

    this->pImpl_->eval(sim_step, secs_elapsed,
                       well_solution, wbp, grp_nwrk_solution,
                       std::move(process_values),
                       initial_inplace, inplace,
                       region_values, block_values,
                       aquifer_values, interreg_flows, st);
}

void Summary::add_timestep(const SummaryState& st,
                           const int           report_step,
                           const int           ministep_id,
                           const bool          isSubstep)
{
    this->pImpl_->internal_store(st, report_step, ministep_id, isSubstep);
}

void Summary::write(const bool is_final_summary) const
{
    this->pImpl_->write(is_final_summary);
}

Summary::~Summary() {}

} // namespace Opm::out
