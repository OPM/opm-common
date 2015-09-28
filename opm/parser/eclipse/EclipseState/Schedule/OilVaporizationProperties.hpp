#ifndef DRSDT_HPP
#define DRSDT_HPP
#include <iostream>
#include <string>
#include <memory>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>

namespace Opm
{
    class OilVaporizationProperties
    {
    public:
        OilVaporizationProperties();
        OilVaporizationProperties(Opm::OilVaporizationEnum type, double maximum, std::string option);
        OilVaporizationProperties(Opm::OilVaporizationEnum type, double maximum);
        OilVaporizationProperties(Opm::OilVaporizationEnum type, double vap1, double vap2);
        Opm::OilVaporizationEnum getType() const;
        double getVap1() const;
        double getVap2() const;
        double getMaximum() const;
        std::string getOption() const;

    private:
        Opm::OilVaporizationEnum m_type;
        double m_maximum;
        double m_vap1;
        double m_vap2;
        std::string m_option;

    };
    typedef std::shared_ptr<OilVaporizationProperties> OilVaporizationPropertiesPtr;
    typedef std::shared_ptr<const OilVaporizationProperties> OilVaporizationPropertiesConstPtr;
}
#endif // DRSDT_H
