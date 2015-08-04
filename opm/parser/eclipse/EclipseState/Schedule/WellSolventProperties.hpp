/*
  Copyright 2014 Statoil ASA.
  Copyright 2015 IRIS AS.

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

#ifndef WELLSOLVENTPROPERTIES_HPP_HEADER_INCLUDED
#define WELLSOLVENTPROPERTIES_HPP_HEADER_INCLUDED


namespace Opm {

    struct WellSolventProperties {
        double m_solventConcentration;

        bool operator==(const WellSolventProperties& other) const;
        bool operator!=(const WellSolventProperties& other) const;
        WellSolventProperties();
    };
}

#endif
