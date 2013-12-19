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


/**
   The unit sets emplyed in ECLIPSE, in particular the FIELD units,
   are quite inconsistent. Ideally one should choose units for a set
   of base quantities like Mass,Time and Length and then derive the
   units for e.g. pressure and flowrate in a consisten manner. However
   that is not the case; for instance in the metric system we have:

      [Length] = meters
      [time] = days
      [mass] = kg

   This should give:

      [Pressure] = [mass] / ([length] * [time]^2) = kg / (m * days * days)

   Instead pressure is given in Bars. When it comes to FIELD units the
   number of such examples is long.
*/

      


namespace Opm {

    namespace Metric {
        const double Pressure          = 100000;   
        const double Length            = 1.0;      
        const double Time              = 86400;
        const double Mass              = 1.0;
        const double Permeability      = 9.869233e-10;
        const double DissolvedGasRaito = 1.0;
        const double FlowVolume        = 1.0;
        const double Density           = 1.0;
        const double Viscosity         = 0.001;               // cP
    }


     namespace Field {
         const double Pressure          = 6894.76;
         const double Length            = 0.3048;
         const double Time              = 86400;
         const double Mass              = 0.45359237;
         const double Permeability      = 9.869233e-10;
         const double DissolvedGasRaito = 178.1076;            // Mscf / stb
         const double FlowVolume        = 158.987294;          // STB
         const double Density           = 16.01846;            // lb/ft^3
         const double Viscosity         = 0.001;               // cP
    }

}


#endif
