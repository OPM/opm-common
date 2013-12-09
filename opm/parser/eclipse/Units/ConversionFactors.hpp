/*
  Copyright 2013 Statoil ASA.

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

#ifndef CONVERSION_FACTORS_HPP
#define CONVERSION_FACTORS_HPP


namespace Opm {

    namespace Metric {
        const double Pressure     = 100000;   
        const double Length       = 1.0;      
        const double Time         = 86400;
        const double Mass         = 1.0;
        const double Permeability = 9.869233e-10;
    }

     namespace Field {
        const double Pressure     = 6894.76;
        const double Length       = 0.3048;
        const double Time         = 86400;
        const double Mass         = 0.45359237;
        const double Permeability = 9.869233e-10;
    }

}


#endif
