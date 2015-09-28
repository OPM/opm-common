#include "OilVaporizationProperties.hpp"

namespace Opm {
    OilVaporizationProperties::OilVaporizationProperties(Opm::OilVaporizationEnum type, double maximum, std::string option){
        m_type = type;
        m_maximum = maximum;
        m_option = option;
    }

    OilVaporizationProperties::OilVaporizationProperties(Opm::OilVaporizationEnum type, double maximum){
        m_type = type;
        m_maximum = maximum;
    }

    OilVaporizationProperties::OilVaporizationProperties(Opm::OilVaporizationEnum type, double vap1, double vap2){
        m_type = type;
        m_vap1 = vap1;
        m_vap2 = vap2;
    }



    OilVaporizationProperties::OilVaporizationProperties(){

    }

    double OilVaporizationProperties::getMaximum() const{
        return m_maximum;
    }

    std::string OilVaporizationProperties::getOption() const{
        return m_option;
    }

    Opm::OilVaporizationEnum OilVaporizationProperties::getType() const{
        return m_type;
    }

    double OilVaporizationProperties::getVap1() const{
        return m_vap1;
    }

    double OilVaporizationProperties::getVap2() const{
        return m_vap2;
    }
}
