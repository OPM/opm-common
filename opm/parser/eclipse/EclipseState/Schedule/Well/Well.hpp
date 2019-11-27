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

#include <string>

#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellConnections.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/WellSegments.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/ProductionControls.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/InjectionControls.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellFoamProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellBrineProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellTracerProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellPolymerProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellEconProductionLimits.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>

#include <opm/common/utility/ActiveGridCells.hpp>

namespace Opm {

class DeckRecord;
class EclipseGrid;
class DeckKeyword;
struct WellInjectionProperties;
class WellProductionProperties;
class UDQActive;
class UDQConfig;


class Well {
public:

    enum class Status {
        OPEN = 1,
        STOP = 2,
        SHUT = 3,
        AUTO = 4
    };
    static std::string Status2String(Status enumValue);
    static Status StatusFromString(const std::string& stringValue);


    enum class InjectorType {
        WATER = 1,
        GAS = 2,
        OIL = 3,
        MULTI = 4
    };
    static const std::string InjectorType2String( InjectorType enumValue );
    static InjectorType InjectorTypeFromString( const std::string& stringValue );


    /*
      The elements in this enum are used as bitmasks to keep track
      of which controls are present, i.e. the 2^n structure must
      be intact.
    */
    enum class InjectorCMode : int{
        RATE =  1 ,
        RESV =  2 ,
        BHP  =  4 ,
        THP  =  8 ,
        GRUP = 16 ,
        CMODE_UNDEFINED = 512
    };
    static const std::string InjectorCMode2String( InjectorCMode enumValue );
    static InjectorCMode InjectorCModeFromString( const std::string& stringValue );


    /*
      The items BHP, THP and GRUP only apply in prediction mode:
      WCONPROD. The elements in this enum are used as bitmasks to
      keep track of which controls are present, i.e. the 2^n
      structure must be intact.The NONE item is only used in WHISTCTL
      to cancel its effect.

      The properties are initialized with the CMODE_UNDEFINED
      value, but the undefined value is never assigned apart from
      that; and it is not part of the string conversion routines.
    */
    enum class ProducerCMode : int {
        NONE =     0,
        ORAT =     1,
        WRAT =     2,
        GRAT =     4,
        LRAT =     8,
        CRAT =    16,
        RESV =    32,
        BHP  =    64,
        THP  =   128,
        GRUP =   256,
        CMODE_UNDEFINED = 1024
    };
    static const std::string ProducerCMode2String( ProducerCMode enumValue );
    static ProducerCMode ProducerCModeFromString( const std::string& stringValue );



    enum class WELTARGCMode {
        ORAT =  1,
        WRAT =  2,
        GRAT =  3,
        LRAT =  4,
        CRAT =  5,   // Not supported
        RESV =  6,
        BHP  =  7,
        THP  =  8,
        VFP  =  9,
        LIFT = 10,   // Not supported
        GUID = 11
    };

    static WELTARGCMode WELTARGCModeFromString(const std::string& stringValue);


    enum class GuideRateTarget {
        OIL = 0,
        WAT = 1,
        GAS = 2,
        LIQ = 3,
        COMB = 4,
        WGA = 5,
        CVAL = 6,
        RAT = 7,
        RES = 8,
        UNDEFINED = 9
    };
    static const std::string GuideRateTarget2String( GuideRateTarget enumValue );
    static GuideRateTarget GuideRateTargetFromString( const std::string& stringValue );



    struct WellGuideRate {
        bool available;
        double guide_rate;
        GuideRateTarget guide_phase;
        double scale_factor;

        bool operator==(const WellGuideRate& data) const {
            return available == data.available &&
                   guide_rate == data.guide_rate &&
                   guide_phase == data.guide_phase &&
                   scale_factor == data.scale_factor;
        }
    };


    struct InjectionControls {
    public:
        InjectionControls(int controls_arg) :
            controls(controls_arg)
        {}

        double bhp_limit;
        double thp_limit;


        InjectorType injector_type;
        InjectorCMode cmode;
        double surface_rate;
        double reservoir_rate;
        double temperature;
        int    vfp_table_number;
        bool   prediction_mode;

        bool hasControl(InjectorCMode cmode_arg) const {
            return (this->controls & static_cast<int>(cmode_arg)) != 0;
        }

    private:
        int controls;
    };



    struct WellInjectionProperties {
        std::string name;
        UDAValue  surfaceInjectionRate;
        UDAValue  reservoirInjectionRate;
        UDAValue  BHPTarget;
        UDAValue  THPTarget;

        double  bhp_hist_limit;
        double  thp_hist_limit = 0;

        double  temperature;
        double  BHPH;
        double  THPH;
        int     VFPTableNumber;
        bool    predictionMode;
        int     injectionControls;
        Well::InjectorType injectorType;
        InjectorCMode controlMode;

        bool operator==(const WellInjectionProperties& other) const;
        bool operator!=(const WellInjectionProperties& other) const;

        WellInjectionProperties();
        WellInjectionProperties(const std::string& wname);
        WellInjectionProperties(const std::string& wname,
                                const UDAValue& surfaceInjRate,
                                const UDAValue& reservoirInjRate,
                                const UDAValue& BHP,
                                const UDAValue& THP,
                                double bhp_hist,
                                double thp_hist,
                                double temp,
                                double bhph,
                                double thph,
                                int vfpTableNum,
                                bool predMode,
                                int injControls,
                                Well::InjectorType injType,
                                InjectorCMode ctrlMode);

        void handleWELTARG(WELTARGCMode cmode, double newValue, double SIFactorP);
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

        void resetDefaultHistoricalBHPLimit();
        void resetBHPLimit();
        void setBHPLimit(const double limit);
        InjectionControls controls(const UnitSystem& unit_system, const SummaryState& st, double udq_default) const;
        bool updateUDQActive(const UDQConfig& udq_config, UDQActive& active) const;
    };

    struct ProductionControls {
    public:
        ProductionControls(int controls_arg) :
            controls(controls_arg)
        {
        }

        ProducerCMode cmode;
        double oil_rate;
        double water_rate;
        double gas_rate;
        double liquid_rate;
        double resv_rate;
        double bhp_history;
        double thp_history;
        double bhp_limit;
        double thp_limit;
        double alq_value;
        int    vfp_table_number;
        bool   prediction_mode;

        bool hasControl(ProducerCMode cmode_arg) const {
            return (this->controls & static_cast<int>(cmode_arg)) != 0;
        }

    private:
        int controls;
    };


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

        // BHP and THP limit
        double  bhp_hist_limit;
        double  thp_hist_limit = 0;

        // historical BHP and THP under historical mode
        double  BHPH        = 0.0;
        double  THPH        = 0.0;
        int     VFPTableNumber = 0;
        double  ALQValue    = 0.0;
        bool    predictionMode = false;
        ProducerCMode controlMode = ProducerCMode::CMODE_UNDEFINED;
        ProducerCMode whistctl_cmode = ProducerCMode::CMODE_UNDEFINED;

        bool operator==(const WellProductionProperties& other) const;
        bool operator!=(const WellProductionProperties& other) const;

        WellProductionProperties();
        WellProductionProperties(const std::string& name_arg);
        WellProductionProperties(const std::string& wname,
                                 const UDAValue& oilRate,
                                 const UDAValue& waterRate,
                                 const UDAValue& gasRate,
                                 const UDAValue& liquidRate,
                                 const UDAValue& resvRate,
                                 const UDAValue& BHP,
                                 const UDAValue& THP,
                                 double bhp_hist,
                                 double thp_hist,
                                 double bhph,
                                 double thph,
                                 int vfpTableNum,
                                 double alqValue,
                                 bool predMode,
                                 ProducerCMode ctrlMode,
                                 ProducerCMode whistctlMode,
                                 int prodCtrls);

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
        void handleWCONPROD( const std::string& well, const DeckRecord& record);
        void handleWCONHIST( const DeckRecord& record);
        void handleWELTARG( WELTARGCMode cmode, double newValue, double SiFactorP);
        void resetDefaultBHPLimit();
        void clearControls();
        ProductionControls controls(const SummaryState& st, double udq_default) const;
        bool updateUDQActive(const UDQConfig& udq_config, UDQActive& active) const;

        int getNumProductionControls() const;
        void setBHPLimit(const double limit);

    private:
        int m_productionControls = 0;
        void init_rates( const DeckRecord& record );

        void init_history(const DeckRecord& record);

        WellProductionProperties(const DeckRecord& record);

        double getBHPLimit() const;
    };


    Well();
    Well(const std::string& wname,
          const std::string& gname,
          std::size_t init_step,
          std::size_t insert_index,
          int headI,
          int headJ,
          double ref_depth,
          Phase phase,
          ProducerCMode whistctl_cmode,
          Connection::Order ordering,
          const UnitSystem& unit_system,
          double udq_undefined);

    Well(const std::string& wname,
         const std::string& gname,
         std::size_t init_step,
         std::size_t insert_index,
         int headI,
         int headJ,
         double ref_depth,
         const Phase& phase_arg,
         Connection::Order ordering,
         const UnitSystem& unit_system,
         double udq_undefined,
         Status status,
         double drainageRadius,
         bool allowCrossFlow,
         bool automaticShutIn,
         bool isProducer,
         const WellGuideRate& guideRate,
         double efficiencyFactor,
         double solventFraction,
         bool prediction_mode,
         std::shared_ptr<const WellEconProductionLimits> econLimits,
         std::shared_ptr<const WellFoamProperties> foamProperties,
         std::shared_ptr<const WellPolymerProperties> polymerProperties,
         std::shared_ptr<const WellTracerProperties> tracerProperties,
         std::shared_ptr<WellConnections> connections,
         std::shared_ptr<const WellProductionProperties> production,
         std::shared_ptr<const WellInjectionProperties> injection,
         std::shared_ptr<const WellSegments> segments);

    bool isMultiSegment() const;
    bool isAvailableForGroupControl() const;
    double getGuideRate() const;
    GuideRateTarget getGuideRatePhase() const;
    double getGuideRateScalingFactor() const;

    bool hasBeenDefined(size_t timeStep) const;
    std::size_t firstTimeStep() const;
    bool predictionMode() const;
    bool canOpen() const;
    bool isProducer() const;
    bool isInjector() const;
    InjectorType injectorType() const;
    size_t seqIndex() const;
    bool getAutomaticShutIn() const;
    bool getAllowCrossFlow() const;
    const std::string& name() const;
    int getHeadI() const;
    int getHeadJ() const;
    double getRefDepth() const;
    double getDrainageRadius() const;
    double getEfficiencyFactor() const;
    Connection::Order getWellConnectionOrdering() const;
    const WellProductionProperties& getProductionProperties() const;
    const WellInjectionProperties& getInjectionProperties() const;
    const WellEconProductionLimits& getEconLimits() const;
    const WellFoamProperties& getFoamProperties() const;
    const WellPolymerProperties& getPolymerProperties() const;
    const WellBrineProperties& getBrineProperties() const;
    const WellTracerProperties& getTracerProperties() const;
    const WellConnections& getConnections() const;
    const WellSegments& getSegments() const;
    double getSolventFraction() const;
    Status getStatus() const;
    const std::string& groupName() const;
    Phase getPreferredPhase() const;
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

    bool updatePrediction(bool prediction_mode);
    bool updateAutoShutin(bool auto_shutin);
    bool updateCrossFlow(bool allow_cross_flow);
    bool updateHead(int I, int J);
    bool updateRefDepth(double ref_dpeth);
    bool updateDrainageRadius(double drainage_radius);
    bool updateConnections(const std::shared_ptr<WellConnections> connections);
    bool updateStatus(Status status);
    bool updateGroup(const std::string& group);
    bool updateProducer(bool is_producer);
    bool updateWellGuideRate(bool available, double guide_rate, GuideRateTarget guide_phase, double scale_factor);
    bool updateWellGuideRate(double guide_rate);
    bool updateEfficiencyFactor(double efficiency_factor);
    bool updateSolventFraction(double solvent_fraction);
    bool updateTracer(std::shared_ptr<WellTracerProperties> tracer_properties);
    bool updateFoamProperties(std::shared_ptr<WellFoamProperties> foam_properties);
    bool updatePolymerProperties(std::shared_ptr<WellPolymerProperties> polymer_properties);
    bool updateBrineProperties(std::shared_ptr<WellBrineProperties> brine_properties);
    bool updateEconLimits(std::shared_ptr<WellEconProductionLimits> econ_limits);
    bool updateProduction(std::shared_ptr<WellProductionProperties> production);
    bool updateInjection(std::shared_ptr<WellInjectionProperties> injection);
    bool updateWSEGSICD(const std::vector<std::pair<int, SpiralICD> >& sicd_pairs);
    bool updateWSEGVALV(const std::vector<std::pair<int, Valve> >& valve_pairs);

    bool handleWELSEGS(const DeckKeyword& keyword);
    bool handleCOMPSEGS(const DeckKeyword& keyword, const EclipseGrid& grid, const ParseContext& parseContext, ErrorGuard& errors);
    bool handleWELOPEN(const DeckRecord& record, Connection::State status, bool action_mode);
    bool handleCOMPLUMP(const DeckRecord& record);
    bool handleWPIMULT(const DeckRecord& record);

    void filterConnections(const ActiveGridCells& grid);
    void switchToInjector();
    void switchToProducer();
    ProductionControls productionControls(const SummaryState& st) const;
    InjectionControls injectionControls(const SummaryState& st) const;
    int vfp_table_number() const;
    double alq_value() const;
    double temperature() const;
    const UnitSystem& units() const;
    double udqUndefined() const;
    bool hasSegments() const;
    const WellGuideRate& wellGuideRate() const;

    bool operator==(const Well& data) const;

private:
    std::string wname;
    std::string group_name;
    std::size_t init_step;
    std::size_t insert_index;
    int headI;
    int headJ;
    double ref_depth;
    Phase phase;
    Connection::Order ordering;
    UnitSystem unit_system;
    double udq_undefined;

    Status status;
    double drainage_radius;
    bool allow_cross_flow;
    bool automatic_shutin;
    bool producer;
    WellGuideRate guide_rate;
    double efficiency_factor;
    double solvent_fraction;
    bool prediction_mode = true;

    std::shared_ptr<const WellEconProductionLimits> econ_limits;
    std::shared_ptr<const WellFoamProperties> foam_properties;
    std::shared_ptr<const WellPolymerProperties> polymer_properties;
    std::shared_ptr<const WellBrineProperties> brine_properties;
    std::shared_ptr<const WellTracerProperties> tracer_properties;
    std::shared_ptr<WellConnections> connections; // The WellConnections object can not be const because of the filterConnections method - would be beneficial to rewrite to enable const
    std::shared_ptr<const WellProductionProperties> production;
    std::shared_ptr<const WellInjectionProperties> injection;
    std::shared_ptr<const WellSegments> segments;
};

std::ostream& operator<<( std::ostream&, const Well::WellInjectionProperties& );
std::ostream& operator<<( std::ostream&, const WellProductionProperties& );


}

#endif
