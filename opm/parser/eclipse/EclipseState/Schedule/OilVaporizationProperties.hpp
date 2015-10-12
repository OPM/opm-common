#ifndef DRSDT_HPP
#define DRSDT_HPP
#include <iostream>
#include <string>
#include <memory>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>

namespace Opm
{
     /*
     * The OilVaporizationProperties class
     * This classe is used to store the values from {VAPPARS, DRSDT, DRVDT} the behavior of the keywords are mutal exclusive.
     * Any one of the three keywords {VAPPARS, DRSDT, DRVDT} will cancel previous settings of the other keywords.
     * Ask for type first and the ask for the correct values for this type, asking for values not valid for the current type will throw a logic exception.
     */
    class OilVaporizationProperties
    {
    public:


        static std::shared_ptr<OilVaporizationProperties> createOilVaporizationPropertiesDRSDT(double maxDRSDT, std::string option);
        static std::shared_ptr<OilVaporizationProperties> createOilVaporizationPropertiesDRVDT(double maxDRVDT);
        static std::shared_ptr<OilVaporizationProperties> createOilVaporizationPropertiesVAPPARS(double vap1, double vap2);
        Opm::OilVaporizationEnum getType() const;
        double getVap1() const;
        double getVap2() const;
        double getMaxDRSDT() const;
        double getMaxDRVDT() const;
        bool getOption() const;

    private:
        OilVaporizationProperties();
        Opm::OilVaporizationEnum m_type;
        double m_vap1;
        double m_vap2;
        double m_maxDRSDT;
        double m_maxDRVDT;
        bool m_maxDRSDT_allCells;
    };
    typedef std::shared_ptr<OilVaporizationProperties> OilVaporizationPropertiesPtr;
    typedef std::shared_ptr<const OilVaporizationProperties> OilVaporizationPropertiesConstPtr;
}
#endif // DRSDT_H
