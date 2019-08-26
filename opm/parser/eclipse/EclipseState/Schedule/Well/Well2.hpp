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
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellConnections.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/WellSegments.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/ProductionControls.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/InjectionControls.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellFoamProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellTracerProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellPolymerProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellEconProductionLimits.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>


namespace Opm {

class DeckRecord;
class EclipseGrid;
class DeckKeyword;
struct WellInjectionProperties;
class WellProductionProperties;
class UDQActive;
class UDQConfig;


struct WellGuideRate {
    bool available;
    double guide_rate;
    GuideRate::GuideRatePhaseEnum guide_phase;
    double scale_factor;
};


class Well2 {
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
        UDAValue  BHPLimit;
        UDAValue  THPLimit;
        double  temperature;
        double  BHPH;
        double  THPH;
        int     VFPTableNumber;
        bool    predictionMode;
        int     injectionControls;
        Well2::InjectorType injectorType;
        InjectorCMode controlMode;

        bool operator==(const WellInjectionProperties& other) const;
        bool operator!=(const WellInjectionProperties& other) const;

        WellInjectionProperties(const std::string& wname);
        void handleWELTARG(WellTarget::ControlModeEnum cmode, double newValue, double siFactorG, double siFactorL, double siFactorP);
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

        void setBHPLimit(const double limit);
        InjectionControls controls(const UnitSystem& unit_system, const SummaryState& st, double udq_default) const;
        bool updateUDQActive(const UDQConfig& udq_config, UDQActive& active) const;
    };


    Well2(const std::string& wname,
          const std::string& gname,
          std::size_t init_step,
          std::size_t insert_index,
          int headI,
          int headJ,
          double ref_depth,
          Phase phase,
          WellProducer::ControlModeEnum whistctl_cmode,
          WellCompletion::CompletionOrderEnum ordering,
          const UnitSystem& unit_system,
          double udq_undefined);

    bool isMultiSegment() const;
    bool isAvailableForGroupControl() const;
    double getGuideRate() const;
    GuideRate::GuideRatePhaseEnum getGuideRatePhase() const;
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
    WellCompletion::CompletionOrderEnum getWellConnectionOrdering() const;
    const WellProductionProperties& getProductionProperties() const;
    const WellInjectionProperties& getInjectionProperties() const;
    const WellEconProductionLimits& getEconLimits() const;
    const WellFoamProperties& getFoamProperties() const;
    const WellPolymerProperties& getPolymerProperties() const;
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
    bool updateWellGuideRate(bool available, double guide_rate, GuideRate::GuideRatePhaseEnum guide_phase, double scale_factor);
    bool updateWellGuideRate(double guide_rate);
    bool updateEfficiencyFactor(double efficiency_factor);
    bool updateSolventFraction(double solvent_fraction);
    bool updateTracer(std::shared_ptr<WellTracerProperties> tracer_properties);
    bool updateFoamProperties(std::shared_ptr<WellFoamProperties> foam_properties);
    bool updatePolymerProperties(std::shared_ptr<WellPolymerProperties> polymer_properties);
    bool updateEconLimits(std::shared_ptr<WellEconProductionLimits> econ_limits);
    bool updateProduction(std::shared_ptr<WellProductionProperties> production);
    bool updateInjection(std::shared_ptr<WellInjectionProperties> injection);

    bool handleWELSEGS(const DeckKeyword& keyword);
    bool handleCOMPSEGS(const DeckKeyword& keyword, const EclipseGrid& grid, const ParseContext& parseContext, ErrorGuard& errors);
    bool handleWELOPEN(const DeckRecord& record, WellCompletion::StateEnum status);
    bool handleCOMPLUMP(const DeckRecord& record);
    bool handleWPIMULT(const DeckRecord& record);

    void filterConnections(const EclipseGrid& grid);
    void switchToInjector();
    void switchToProducer();
    ProductionControls productionControls(const SummaryState& st) const;
    InjectionControls injectionControls(const SummaryState& st) const;
    int vfp_table_number() const;
    double alq_value() const;
    double temperature() const;
private:
    std::string wname;
    std::string group_name;
    std::size_t init_step;
    std::size_t insert_index;
    int headI;
    int headJ;
    double ref_depth;
    Phase phase;
    WellCompletion::CompletionOrderEnum ordering;
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
    std::shared_ptr<const WellTracerProperties> tracer_properties;
    std::shared_ptr<WellConnections> connections; // The WellConnections object can not be const because of the filterConnections method - would be beneficial to rewrite to enable const
    std::shared_ptr<const WellProductionProperties> production;
    std::shared_ptr<const WellInjectionProperties> injection;
    std::shared_ptr<const WellSegments> segments;
};

std::ostream& operator<<( std::ostream&, const Well2::WellInjectionProperties& );


}

#endif
