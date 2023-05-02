/*
  Copyright 2019 Equinor ASA.

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


#ifndef WELL2_HPP
#define WELL2_HPP

#include <cstddef>
#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <opm/input/eclipse/Deck/UDAValue.hpp>
#include <opm/input/eclipse/EclipseState/Phase.hpp>
#include <opm/input/eclipse/Schedule/ScheduleTypes.hpp>
#include <opm/input/eclipse/Schedule/VFPProdTable.hpp>
#include <opm/input/eclipse/Schedule/Well/PAvg.hpp>
#include <opm/input/eclipse/Schedule/Well/PAvgCalculator.hpp>
#include <opm/input/eclipse/Schedule/Well/WellEnums.hpp>
#include <opm/input/eclipse/Schedule/Well/WellInjectionControls.hpp>
#include <opm/input/eclipse/Schedule/Well/WellProductionControls.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

namespace Opm {

class ActiveGridCells;
class AutoICD;
class DeckKeyword;
class DeckRecord;
class ErrorGuard;
class EclipseGrid;
class ParseContext;
class ScheduleGrid;
class SICD;
class SummaryState;
class UDQActive;
class UDQConfig;
class Valve;
class TracerConfig;
class WellConnections;
struct WellBrineProperties;
class WellEconProductionLimits;
struct WellFoamProperties;
struct WellMICPProperties;
struct WellPolymerProperties;
class WellSegments;
class WellTracerProperties;
class WVFPEXP;

namespace RestartIO {
struct RstWell;
}

class Well {
public:
    using Status = WellStatus;

    /*
      The elements in this enum are used as bitmasks to keep track
      of which controls are present, i.e. the 2^n structure must
      be intact.
    */
    using InjectorCMode = WellInjectorCMode;

    /*
      The properties are initialized with the CMODE_UNDEFINED
      value, but the undefined value is never assigned apart from
      that; and it is not part of the string conversion routines.
    */
    using ProducerCMode = WellProducerCMode;

    using WELTARGCMode = WellWELTARGCMode;

    using GuideRateTarget = WellGuideRateTarget;

    using GasInflowEquation = WellGasInflowEquation;

    struct WellGuideRate {
        bool available;
        double guide_rate;
        GuideRateTarget guide_phase;
        double scale_factor;

        static WellGuideRate serializationTestObject()
        {
            WellGuideRate result;
            result.available = true;
            result.guide_rate = 1.0;
            result.guide_phase = GuideRateTarget::COMB;
            result.scale_factor = 2.0;

            return result;
        }

        bool operator==(const WellGuideRate& data) const {
            return available == data.available &&
                   guide_rate == data.guide_rate &&
                   guide_phase == data.guide_phase &&
                   scale_factor == data.scale_factor;
        }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(available);
            serializer(guide_rate);
            serializer(guide_phase);
            serializer(scale_factor);
        }
    };

    using InjectionControls = WellInjectionControls;

    struct WellInjectionProperties {
        std::string name;
        UDAValue  surfaceInjectionRate;
        UDAValue  reservoirInjectionRate;
        UDAValue  BHPTarget;
        UDAValue  THPTarget;

        double  bhp_hist_limit = 0.0;
        double  thp_hist_limit = 0.0;

        double  BHPH;
        double  THPH;
        int     VFPTableNumber;
        bool    predictionMode;
        int     injectionControls;
        InjectorType injectorType;
        InjectorCMode controlMode;

        double rsRvInj;

        bool operator==(const WellInjectionProperties& other) const;
        bool operator!=(const WellInjectionProperties& other) const;

        WellInjectionProperties();
        WellInjectionProperties(const UnitSystem& units, const std::string& wname);

        static WellInjectionProperties serializationTestObject();

        void handleWELTARG(WELTARGCMode cmode, const UDAValue& new_arg, double SIFactorP);
        void handleWCONINJE(const DeckRecord& record, bool availableForGroupControl, const std::string& well_name);
        void handleWCONINJH(const DeckRecord& record, bool is_producer, const std::string& well_name);
        bool hasInjectionControl(InjectorCMode controlModeArg) const {
            if (injectionControls & static_cast<int>(controlModeArg))
                return true;
            else
                return false;
        }

        void dropInjectionControl(InjectorCMode controlModeArg) {
            auto int_arg = static_cast<int>(controlModeArg);
            if ((injectionControls & int_arg) != 0)
                injectionControls -= int_arg;
        }

        void addInjectionControl(InjectorCMode controlModeArg) {
            auto int_arg = static_cast<int>(controlModeArg);
            if ((injectionControls & int_arg) == 0)
                injectionControls += int_arg;
        }

        void clearControls();

        void resetDefaultHistoricalBHPLimit();
        void resetBHPLimit();
        void setBHPLimit(const double limit);
        InjectionControls controls(const UnitSystem& unit_system, const SummaryState& st, double udq_default) const;
        bool updateUDQActive(const UDQConfig& udq_config, UDQActive& active) const;
        bool updateUDQActive(const UDQConfig& udq_config, const WELTARGCMode cmode, UDQActive& active) const;
        void update_uda(const UDQConfig& udq_config, UDQActive& udq_active, UDAControl control, const UDAValue& value);
        void handleWTMULT(Well::WELTARGCMode cmode, double factor);

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(name);
            serializer(surfaceInjectionRate);
            serializer(reservoirInjectionRate);
            serializer(BHPTarget);
            serializer(THPTarget);
            serializer(bhp_hist_limit);
            serializer(thp_hist_limit);
            serializer(BHPH);
            serializer(THPH);
            serializer(VFPTableNumber);
            serializer(predictionMode);
            serializer(injectionControls);
            serializer(injectorType);
            serializer(controlMode);
            serializer(rsRvInj);
        }
    };

    using ProductionControls = WellProductionControls;

    class WellProductionProperties {
    public:
        // the rates serve as limits under prediction mode
        // while they are observed rates under historical mode
        std::string name;
        UDAValue  OilRate;
        UDAValue  WaterRate;
        UDAValue  GasRate;
        UDAValue  LiquidRate;
        UDAValue  ResVRate;
        UDAValue  BHPTarget;
        UDAValue  THPTarget;
        UDAValue  ALQValue;

        // BHP and THP limit
        double  bhp_hist_limit = 0.0;
        double  thp_hist_limit = 0.0;

        // historical BHP and THP under historical mode
        double  BHPH        = 0.0;
        double  THPH        = 0.0;
        int     VFPTableNumber = 0;
        bool    predictionMode = false;
        ProducerCMode controlMode = ProducerCMode::CMODE_UNDEFINED;
        ProducerCMode whistctl_cmode = ProducerCMode::CMODE_UNDEFINED;

        bool operator==(const WellProductionProperties& other) const;
        bool operator!=(const WellProductionProperties& other) const;

        WellProductionProperties();
        WellProductionProperties(const UnitSystem& units, const std::string& name_arg);

        static WellProductionProperties serializationTestObject();

        bool hasProductionControl(ProducerCMode controlModeArg) const {
            return (m_productionControls & static_cast<int>(controlModeArg)) != 0;
        }

        void dropProductionControl(ProducerCMode controlModeArg) {
            if (hasProductionControl(controlModeArg))
                m_productionControls -= static_cast<int>(controlModeArg);
        }

        void addProductionControl(ProducerCMode controlModeArg) {
            if (! hasProductionControl(controlModeArg))
                m_productionControls += static_cast<int>(controlModeArg);
        }

        // this is used to check whether the specified control mode is an effective history matching production mode
        static bool effectiveHistoryProductionControl(ProducerCMode cmode);
        void handleWCONPROD( const std::optional<VFPProdTable::ALQ_TYPE>& alq_type, const UnitSystem& unit_system, const std::string& well, const DeckRecord& record);
        void handleWCONHIST( const std::optional<VFPProdTable::ALQ_TYPE>& alq_type, const UnitSystem& unit_system, const DeckRecord& record);
        void handleWELTARG( WELTARGCMode cmode, const UDAValue& new_arg, double SiFactorP);
        void resetDefaultBHPLimit();
        void clearControls();
        ProductionControls controls(const SummaryState& st, double udq_default) const;
        bool updateUDQActive(const UDQConfig& udq_config, UDQActive& active) const;
        bool updateUDQActive(const UDQConfig& udq_config, const WELTARGCMode cmode, UDQActive& active) const;
        void update_uda(const UDQConfig& udq_config, UDQActive& udq_active, UDAControl control, const UDAValue& value);

        void setBHPLimit(const double limit);
        int productionControls() const { return this->m_productionControls; }
        void handleWTMULT(Well::WELTARGCMode cmode, double factor);

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(name);
            serializer(OilRate);
            serializer(WaterRate);
            serializer(GasRate);
            serializer(LiquidRate);
            serializer(ResVRate);
            serializer(BHPTarget);
            serializer(THPTarget);
            serializer(ALQValue);
            serializer(bhp_hist_limit);
            serializer(thp_hist_limit);
            serializer(BHPH);
            serializer(THPH);
            serializer(VFPTableNumber);
            serializer(predictionMode);
            serializer(controlMode);
            serializer(whistctl_cmode);
            serializer(m_productionControls);
        }

    private:
        int m_productionControls = 0;
        void init_rates( const DeckRecord& record );

        void init_history(const DeckRecord& record);
        void init_vfp(const std::optional<VFPProdTable::ALQ_TYPE>& alq_type, const UnitSystem& unit_system, const DeckRecord& record);

        WellProductionProperties(const DeckRecord& record);

        double getBHPLimit() const;
    };

    static int eclipseControlMode(const Well::InjectorCMode imode,
                                  const InjectorType        itype);

    static int eclipseControlMode(const Well::ProducerCMode pmode);

    static int eclipseControlMode(const Well&         well,
                                  const SummaryState& st);


    Well() = default;
    Well(const std::string& wname,
         const std::string& gname,
         std::size_t init_step,
         std::size_t insert_index,
         int headI,
         int headJ,
         const std::optional<double>& ref_depth,
         const WellType& wtype_arg,
         ProducerCMode whistctl_cmode,
         Connection::Order ordering,
         const UnitSystem& unit_system,
         double udq_undefined,
         double dr,
         bool allow_xflow,
         bool auto_shutin,
         int pvt_table,
         GasInflowEquation inflow_eq);

    Well(const RestartIO::RstWell& rst_well,
         int report_step,
         const TracerConfig& tracer_config,
         const UnitSystem& unit_system,
         double udq_undefined);

    static Well serializationTestObject();

    bool isMultiSegment() const;
    bool isAvailableForGroupControl() const;
    double getGuideRate() const;
    GuideRateTarget getGuideRatePhase() const;
    GuideRateTarget getRawGuideRatePhase() const;
    double getGuideRateScalingFactor() const;

    bool hasBeenDefined(std::size_t timeStep) const;
    std::size_t firstTimeStep() const;
    const WellType& wellType() const;
    bool predictionMode() const;
    bool isProducer() const;
    bool isInjector() const;
    InjectorCMode injection_cmode() const;
    ProducerCMode production_cmode() const;
    InjectorType injectorType() const;
    std::size_t seqIndex() const;
    bool getAutomaticShutIn() const;
    bool getAllowCrossFlow() const;
    const std::string& name() const;
    const std::vector<std::string>& wListNames() const;
    int getHeadI() const;
    int getHeadJ() const;
    double getWPaveRefDepth() const;
    bool hasRefDepth() const;
    double getRefDepth() const;
    double getDrainageRadius() const;
    double getEfficiencyFactor() const;
    double getSolventFraction() const;
    Status getStatus() const;
    const std::string& groupName() const;
    Phase getPreferredPhase() const;

    bool hasConnections() const;
    const std::vector<const Connection *> getConnections(int completion) const;
    const WellConnections& getConnections() const;
    const WellSegments& getSegments() const;
    int maxSegmentID() const;
    int maxBranchID() const;

    const WellProductionProperties& getProductionProperties() const;
    const WellInjectionProperties& getInjectionProperties() const;
    const WellEconProductionLimits& getEconLimits() const;
    const WellFoamProperties& getFoamProperties() const;
    const WellPolymerProperties& getPolymerProperties() const;
    const WellMICPProperties& getMICPProperties() const;
    const WellBrineProperties& getBrineProperties() const;
    const WellTracerProperties& getTracerProperties() const;
    const WVFPEXP& getWVFPEXP() const;
    /* The rate of a given phase under the following assumptions:
     * * Returns zero if production is requested for an injector (and vice
     *   versa)
     * * If this is an injector and something else than the
     *   requested phase is injected, returns 0, i.e.
     *   water_injector.injection_rate( gas ) == 0
     * * Mixed injection is not supported and always returns 0.
     */
    double production_rate( const SummaryState& st, Phase phase) const;
    double injection_rate( const SummaryState& st,  Phase phase) const;
    static bool wellNameInWellNamePattern(const std::string& wellName, const std::string& wellNamePattern);

    /*
      The getCompletions() function will return a map:

      {
        1 : [Connection, Connection],
        2 : [Connection, Connection, Connecton],
        3 : [Connection],
        4 : [Connection]
      }

      The integer ID's correspond to the COMPLETION id given by the COMPLUMP
      keyword.
    */
    std::map<int, std::vector<Connection>> getCompletions() const;
    /*
      For hasCompletion(int completion) and getConnections(int completion) the
      completion argument is an integer ID used to denote a collection of
      connections. The integer ID is assigned with the COMPLUMP keyword.
     */
    bool hasCompletion(int completion) const;
    bool updatePrediction(bool prediction_mode);
    bool updateAutoShutin(bool auto_shutin);
    bool updateCrossFlow(bool allow_cross_flow);
    bool updatePVTTable(int pvt_table);
    bool updateHead(int I, int J);
    void updateRefDepth();
    bool updateRefDepth(const std::optional<double>& ref_dpeth);
    bool updateDrainageRadius(double drainage_radius);
    void updateSegments(std::shared_ptr<WellSegments> segments_arg);
    bool updateConnections(std::shared_ptr<WellConnections> connections, bool force);
    bool updateConnections(std::shared_ptr<WellConnections> connections, const ScheduleGrid& grid);
    bool updateStatus(Status status);
    bool updateGroup(const std::string& group);
    bool updateWellGuideRate(bool available, double guide_rate, GuideRateTarget guide_phase, double scale_factor);
    bool updateWellGuideRate(double guide_rate);
    bool updateEfficiencyFactor(double efficiency_factor);
    bool updateSolventFraction(double solvent_fraction);
    bool updateTracer(std::shared_ptr<WellTracerProperties> tracer_properties);
    bool updateFoamProperties(std::shared_ptr<WellFoamProperties> foam_properties);
    bool updatePolymerProperties(std::shared_ptr<WellPolymerProperties> polymer_properties);
    bool updateMICPProperties(std::shared_ptr<WellMICPProperties> micp_properties);
    bool updateBrineProperties(std::shared_ptr<WellBrineProperties> brine_properties);
    bool updateEconLimits(std::shared_ptr<WellEconProductionLimits> econ_limits);
    bool updateProduction(std::shared_ptr<WellProductionProperties> production);
    bool updateInjection(std::shared_ptr<WellInjectionProperties> injection);
    bool updateWellProductivityIndex();
    bool updateWSEGSICD(const std::vector<std::pair<int, SICD> >& sicd_pairs);
    bool updateWSEGVALV(const std::vector<std::pair<int, Valve> >& valve_pairs);
    bool updateWSEGAICD(const std::vector<std::pair<int, AutoICD> >& aicd_pairs, const KeywordLocation& location);
    bool updateWPAVE(const PAvg& pavg);
    void updateWPaveRefDepth(double ref_depth);
    bool updateWVFPEXP(std::shared_ptr<WVFPEXP> wvfpexp);

    bool handleWELSEGS(const DeckKeyword& keyword);
    bool handleCOMPSEGS(const DeckKeyword& keyword, const ScheduleGrid& grid, const ParseContext& parseContext, ErrorGuard& errors);
    bool handleWELOPENConnections(const DeckRecord& record, Connection::State status);
    bool handleCOMPLUMP(const DeckRecord& record);
    bool handleWPIMULT(const DeckRecord& record);
    bool applyGlobalWPIMULT(double scale_factor);

    void filterConnections(const ActiveGridCells& grid);
    ProductionControls productionControls(const SummaryState& st) const;
    InjectionControls injectionControls(const SummaryState& st) const;
    int vfp_table_number() const;
    int pvt_table_number() const;
    int fip_region_number() const;
    GasInflowEquation gas_inflow_equation() const;
    bool segmented_density_calculation() const { return true; }
    double alq_value() const;
    double temperature() const;
    void setWellTemperature(const double temp);
    bool hasInjected( ) const;
    bool hasProduced( ) const;
    bool updateHasInjected( );
    bool updateHasProduced();
    bool cmp_structure(const Well& other) const;
    bool operator==(const Well& data) const;
    bool hasSameConnectionsPointers(const Well& other) const;
    void setInsertIndex(std::size_t index);
    double convertDeckPI(double deckPI) const;
    void applyWellProdIndexScaling(const double       scalingFactor,
                                   std::vector<bool>& scalingApplicable);
    const PAvg& pavg() const;
    PAvgCalculator pavg_calculator(const EclipseGrid& grid, const std::vector<double>& porv) const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(wname);
        serializer(group_name);
        serializer(init_step);
        serializer(insert_index);
        serializer(headI);
        serializer(headJ);
        serializer(ref_depth);
        serializer(wpave_ref_depth);
        serializer(unit_system);
        serializer(udq_undefined);
        serializer(status);
        serializer(drainage_radius);
        serializer(allow_cross_flow);
        serializer(automatic_shutin);
        serializer(pvt_table);
        serializer(gas_inflow);
        serializer(wtype);
        serializer(guide_rate);
        serializer(efficiency_factor);
        serializer(solvent_fraction);
        serializer(has_produced);
        serializer(has_injected);
        serializer(prediction_mode);
        serializer(econ_limits);
        serializer(foam_properties);
        serializer(polymer_properties);
        serializer(micp_properties);
        serializer(brine_properties);
        serializer(tracer_properties);
        serializer(connections);
        serializer(production);
        serializer(injection);
        serializer(segments);
        serializer(wvfpexp);
        serializer(m_pavg);
        serializer(well_temperature);
    }

private:
    void switchToInjector();
    void switchToProducer();

    GuideRateTarget preferredPhaseAsGuideRatePhase() const;

    std::string wname;
    std::string group_name;
    std::size_t init_step;
    std::size_t insert_index;
    int headI;
    int headJ;
    std::optional<double> ref_depth;
    std::optional<double> wpave_ref_depth;
    double drainage_radius;
    bool allow_cross_flow;
    bool automatic_shutin;
    int pvt_table;
    GasInflowEquation gas_inflow = GasInflowEquation::STD;  // Will NOT be loaded/assigned from restart file
    UnitSystem unit_system;
    double udq_undefined;
    WellType wtype;
    WellGuideRate guide_rate;
    double efficiency_factor;
    double solvent_fraction;
    bool has_produced = false;
    bool has_injected = false;
    bool prediction_mode = true;

    std::shared_ptr<WellEconProductionLimits> econ_limits;
    std::shared_ptr<WellFoamProperties> foam_properties;
    std::shared_ptr<WellPolymerProperties> polymer_properties;
    std::shared_ptr<WellMICPProperties> micp_properties;
    std::shared_ptr<WellBrineProperties> brine_properties;
    std::shared_ptr<WellTracerProperties> tracer_properties;
    std::shared_ptr<WellConnections> connections; // The WellConnections object cannot be const because of WELPI and the filterConnections method
    std::shared_ptr<WellProductionProperties> production;
    std::shared_ptr<WellInjectionProperties> injection;
    std::shared_ptr<WellSegments> segments;
    std::shared_ptr<WVFPEXP> wvfpexp;
    Status status;
    PAvg m_pavg;
    double well_temperature;
};

std::ostream& operator<<( std::ostream&, const Well::WellInjectionProperties& );
std::ostream& operator<<( std::ostream&, const Well::WellProductionProperties& );

}
#endif
