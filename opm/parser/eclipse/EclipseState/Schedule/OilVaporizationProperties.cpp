#include <opm/parser/eclipse/EclipseState/Schedule/OilVaporizationProperties.hpp>

namespace Opm {


    OilVaporizationProperties::OilVaporizationProperties(){

    }

    double OilVaporizationProperties::getMaxDRVDT() const{
        if (m_type == Opm::OilVaporizationEnum::DRVDT){
            return m_maxDRVDT;
        }else{
            throw std::logic_error("Only valid if type is DRVDT");
        }
    }

    double OilVaporizationProperties::getMaxDRSDT() const{
        if (m_type == Opm::OilVaporizationEnum::DRSDT){
            return m_maxDRSDT;
        }else{
            throw std::logic_error("Only valid if type is DRSDT");
        }
    }

    bool OilVaporizationProperties::getOption() const{
        if (m_type == Opm::OilVaporizationEnum::DRSDT){
            return m_maxDRSDT_allCells;
        }else{
            throw std::logic_error("Only valid if type is DRSDT");
        }
    }

    Opm::OilVaporizationEnum OilVaporizationProperties::getType() const{
        return m_type;
    }

    double OilVaporizationProperties::getVap1() const{
        if (m_type == Opm::OilVaporizationEnum::VAPPARS){
            return m_vap1;
        }else{
            throw std::logic_error("Only valid if type is VAPPARS");
        }
    }

    double OilVaporizationProperties::getVap2() const{
        if (m_type == Opm::OilVaporizationEnum::VAPPARS){
            return m_vap2;
        }else{
            throw std::logic_error("Only valid if type is VAPPARS");
        }
    }

    OilVaporizationPropertiesPtr OilVaporizationProperties::createOilVaporizationPropertiesDRSDT(double maximum, std::string option){
        auto ovp = OilVaporizationPropertiesPtr(new OilVaporizationProperties());
        ovp->m_type = Opm::OilVaporizationEnum::DRSDT;
        ovp->m_maxDRSDT = maximum;
        if (option == "ALL"){
            ovp->m_maxDRSDT_allCells = true;
        }else if (option == "FREE") {
            ovp->m_maxDRSDT_allCells = false;
        }else{
            throw std::invalid_argument("Only ALL or FREE is allowed as option string");
        }
        return ovp;
    }

    OilVaporizationPropertiesPtr OilVaporizationProperties::createOilVaporizationPropertiesDRVDT(double maximum){
        auto ovp = OilVaporizationPropertiesPtr(new OilVaporizationProperties());
        ovp->m_type = Opm::OilVaporizationEnum::DRVDT;
        ovp->m_maxDRVDT = maximum;
        return ovp;

    }

    OilVaporizationPropertiesPtr OilVaporizationProperties::createOilVaporizationPropertiesVAPPARS(double vap1, double vap2){
        auto ovp = OilVaporizationPropertiesPtr(new OilVaporizationProperties());
        ovp->m_type = Opm::OilVaporizationEnum::VAPPARS;
        ovp->m_vap1 = vap1;
        ovp->m_vap2 = vap2;
        return ovp;

    }

}
