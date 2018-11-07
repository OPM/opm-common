/*
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
#ifndef DRSDT_HPP
#define DRSDT_HPP

#include <string>
#include <memory>
#include <vector>

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>

namespace Opm
{
     /*
     * The OilVaporizationProperties class
     * This classe is used to store the values from {VAPPARS, DRSDT, DRVDT}
     * The VAPPARS and {DRSDT, DRVDT} are mutal exclusive and will cancel previous settings of the other keywords.
     * Ask for type first and the ask for the correct values for this type, asking for values not valid for the current type will throw a logic exception.
     */
    class OilVaporizationProperties {
    public:
        explicit OilVaporizationProperties(const size_t numPvtReginIdx);
        static void updateDRSDT(Opm::OilVaporizationProperties& ovp, const std::vector<double>& maxDRSDT, const std::vector<std::string>& option);
        static void updateDRVDT(Opm::OilVaporizationProperties& ovp, const std::vector<double>& maxDRVDT);
        static void updateVAPPARS(Opm::OilVaporizationProperties& ovp, const std::vector<double>& vap1, const std::vector<double>& vap2);

        Opm::OilVaporizationEnum getType() const;
        double getVap1(const size_t pvtRegionIdx) const;
        double getVap2(const size_t pvtRegionIdx) const;
        double getMaxDRSDT(const size_t pvtRegionIdx) const;
        double getMaxDRVDT(const size_t pvtRegionIdx) const;
        bool getOption(const size_t pvtRegionIdx) const;
        bool drsdtActive() const;
        bool drvdtActive() const;
        bool defined() const;
        size_t numPvtRegions() const {return m_maxDRSDT.size();}

        /*
         * if either argument was default constructed == will always be false
         * and != will always be true
         */
        bool operator==( const OilVaporizationProperties& ) const;
        bool operator!=( const OilVaporizationProperties& ) const;

    private:
        Opm::OilVaporizationEnum m_type = OilVaporizationEnum::UNDEF;
        std::vector<double> m_vap1;
        std::vector<double> m_vap2;
        std::vector<double> m_maxDRSDT;
        std::vector<bool> m_maxDRSDT_allCells;
        std::vector<double> m_maxDRVDT;
    };
}
#endif // DRSDT_H
