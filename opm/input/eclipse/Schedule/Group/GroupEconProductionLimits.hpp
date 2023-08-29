/*
  Copyright 2023 Equinor ASA.

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

#ifndef GROUP_ECON_PRODUCTION_LIMITS_H
#define GROUP_ECON_PRODUCTION_LIMITS_H

#include <map>
#include <string>
#include <optional>

#include <opm/input/eclipse/Deck/UDAValue.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/common/utility/Serializer.hpp>
#include <opm/common/utility/MemPacker.hpp>

namespace Opm {

class DeckRecord;

class GroupEconProductionLimits {
public:
    enum class EconWorkover {
        NONE = 0,
        CON  = 1, // CON
        CONP = 2, // +CON
        WELL = 3,
        PLUG = 4,
        ALL  = 5
    };

    class GEconGroup {
    public:
        GEconGroup() = default;
        GEconGroup(const DeckRecord &record, const int report_step);
        bool endRun() const;
        UDAValue minOilRate() const;
        UDAValue minGasRate() const;
        UDAValue maxWaterCut() const;
        UDAValue maxGasOilRatio() const;
        UDAValue maxWaterGasRatio() const;
        int maxOpenWells() const;
        bool operator==(const GEconGroup& other) const;
        int reportStep() const;
        template<class Serializer> void serializeOp(Serializer& serializer);
        static GEconGroup serializationTestObject();
        EconWorkover workover() const;

    private:
        UDAValue m_min_oil_rate;
        UDAValue m_min_gas_rate;
        UDAValue m_max_water_cut;
        UDAValue m_max_gas_oil_ratio;
        UDAValue m_max_water_gas_ratio;
        EconWorkover m_workover;
        bool m_end_run;
        int m_max_open_wells;
        int m_report_step;  // Used to get UDQ undefined value
    };

    class GEconGroupProp {
        /* Same as GEconGroup but with UDA values realized at given report step*/
    public:
        GEconGroupProp(const double min_oil_rate,
                       const double min_gas_rate,
                       const double max_water_cut,
                       const double max_gas_oil_ratio,
                       const double max_water_gas_ratio,
                       EconWorkover workover,
                       bool end_run,
                       int max_open_wells);
        bool endRun() const;
        std::optional<double> minOilRate() const;
        std::optional<double> minGasRate() const;
        std::optional<double> maxWaterCut() const;
        std::optional<double> maxGasOilRatio() const;
        std::optional<double> maxWaterGasRatio() const;
        int maxOpenWells() const;
        EconWorkover workover() const;

    private:
        std::optional<double> m_min_oil_rate;
        std::optional<double> m_min_gas_rate;
        std::optional<double> m_max_water_cut;
        std::optional<double> m_max_gas_oil_ratio;
        std::optional<double> m_max_water_gas_ratio;
        EconWorkover m_workover;
        bool m_end_run;
        int m_max_open_wells;
    };

    GroupEconProductionLimits() = default;
    //explicit GroupEconProductionLimits(const RestartIO::RstWell& rstWell);

    void add_group(const int report_step, const std::string &group_name, const DeckRecord &record);
    static EconWorkover econWorkoverFromString(const std::string& string_value);
    const GEconGroup& get_group(const std::string& gname) const;
    GEconGroupProp get_group_prop(
        const Schedule &schedule, const SummaryState &st, const std::string& gname) const;
    bool has_group(const std::string& gname) const;
    bool operator==(const GroupEconProductionLimits& other) const;
    bool operator!=(const GroupEconProductionLimits& other) const;
    template<class Serializer>
    void serializeOp(Serializer& serializer) const
    {
        serializer(m_groups);
    }
    static GroupEconProductionLimits serializationTestObject();
    size_t size() const;

private:
    std::map<std::string, GEconGroup> m_groups;
};

} // namespace Opm


#endif
