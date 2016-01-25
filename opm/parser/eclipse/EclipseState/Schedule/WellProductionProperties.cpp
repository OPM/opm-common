#include <opm/parser/eclipse/EclipseState/Schedule/WellProductionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>

#include <string>
#include <vector>

namespace Opm {
    WellProductionProperties::
    WellProductionProperties()
    {
        init( );
        predictionMode = true;
    }

    WellProductionProperties::
    WellProductionProperties(DeckRecordConstPtr record)
    {
        init( );
        WaterRate = record->getItem("WRAT")->getSIDouble(0);
        OilRate   = record->getItem("ORAT")->getSIDouble(0);
        GasRate   = record->getItem("GRAT")->getSIDouble(0);
    }


    WellProductionProperties WellProductionProperties::history(double BHPLimit , DeckRecordConstPtr record, bool addGroupProductionControl)
    {
        // Modes supported in WCONHIST just from {O,W,G}RAT values
        //
        // Note: The default value of observed {O,W,G}RAT is zero
        // (numerically) whence the following control modes are
        // unconditionally supported.
        WellProductionProperties p(record);
        p.predictionMode = false;

        for (auto const& modeKey : {"ORAT", "WRAT", "GRAT", "LRAT", "RESV", "GRUP"}) {
            auto cmode = WellProducer::ControlModeFromString( modeKey );
            p.addProductionControl( cmode );
        }

        /*
          We do not update the BHPLIMIT based on the BHP value given
          in WCONHIST, that is purely a historical value; instead we
          copy the old value of the BHP limit from the previous
          timestep.

          To actually set the BHPLIMIT in historical mode you must
          use the WELTARG keyword.
        */
        p.BHPLimit = BHPLimit;
        {
            const auto cmodeItem = record->getItem("CMODE");
            if (!cmodeItem->defaultApplied(0)) {
                const WellProducer::ControlModeEnum cmode = WellProducer::ControlModeFromString( cmodeItem->getString(0) );

                if (p.hasProductionControl( cmode ))
                    p.controlMode = cmode;
                else
                    throw std::invalid_argument("Setting CMODE to unspecified control");
            }
        }
        return p;
    }



    WellProductionProperties WellProductionProperties::prediction(DeckRecordConstPtr record, bool addGroupProductionControl)
    {
        WellProductionProperties p(record);
        p.predictionMode = true;

        p.LiquidRate     = record->getItem("LRAT"     )->getSIDouble(0);
        p.ResVRate       = record->getItem("RESV"     )->getSIDouble(0);
        p.BHPLimit       = record->getItem("BHP"      )->getSIDouble(0);
        p.THPLimit       = record->getItem("THP"      )->getSIDouble(0);
        p.ALQValue       = record->getItem("ALQ"      )->getRawDouble(0); //NOTE: Unit of ALQ is never touched
        p.VFPTableNumber = record->getItem("VFP_TABLE")->getInt(0);

        for (auto const& modeKey : {"ORAT", "WRAT", "GRAT", "LRAT","RESV", "BHP" , "THP"}) {
             if (!record->getItem(modeKey)->defaultApplied(0)) {
                 auto cmode = WellProducer::ControlModeFromString( modeKey );
                 p.addProductionControl( cmode );
            }
        }

        if (addGroupProductionControl) {
            p.addProductionControl(WellProducer::GRUP);
        }


        {
            const auto cmodeItem = record->getItem("CMODE");
            if (cmodeItem->hasValue(0)) {
                const WellProducer::ControlModeEnum cmode = WellProducer::ControlModeFromString( cmodeItem->getString(0) );

                if (p.hasProductionControl( cmode ))
                    p.controlMode = cmode;
                else
                    throw std::invalid_argument("Setting CMODE to unspecified control");
            }
        }
        return p;
    }


    void WellProductionProperties::init() {
        OilRate = 0.0;
        WaterRate = 0.0;
        GasRate = 0.0;
        LiquidRate = 0.0;
        ResVRate = 0.0;
        THPLimit = 0.0;
        BHPLimit = 0.0;
        VFPTableNumber = 0;
        ALQValue = 0.0;
        controlMode = WellProducer::CMODE_UNDEFINED;

        m_productionControls = 0;
    }

    bool WellProductionProperties::operator==(const WellProductionProperties& other) const {
        if ((OilRate == other.OilRate) &&
            (WaterRate == other.WaterRate) &&
            (GasRate == other.GasRate) &&
            (LiquidRate == other.LiquidRate) &&
            (ResVRate == other.ResVRate) &&
            (BHPLimit == other.BHPLimit) &&
            (THPLimit == other.THPLimit) &&
            (VFPTableNumber == other.VFPTableNumber) &&
            (controlMode == other.controlMode) &&
            (m_productionControls == other.m_productionControls))
            return true;
        else
            return false;
    }


    bool WellProductionProperties::operator!=(const WellProductionProperties& other) const {
        return !(*this == other);
    }


} // namespace Opm
