/*
  Copyright (c) 2017 Statoil ASA
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
#ifndef RESTART_VALUE_HPP
#define RESTART_VALUE_HPP

#include <string>
#include <map>
#include <vector>

namespace Opm {


    /*
      Small convenience class used in the map passed to the
      RestartIO::load( ) function. A class instance can be constructed
      from a UnitSystem::measure value, which will default to a
      required key, but it can also be constructed from a
      two-parameter constructor, and then the required/not required
      can controlled explicitly:


         RestartIO::load(..., {{"PRESSURE" , UnitSystem::measure::pressure},
                               {"MAX_SWAT" , {UnitSystem::measure::identity, false}} )

     The RestartKey( ) for pressure is implicitly created from
     UnitSystem::measure::pressure and will be required, whereas the
     MAX_SWAT keyword is optional.

    */
    class RestartKey {
    public:

        UnitSystem::measure dim;
        bool required = true;

        explicit RestartKey( UnitSystem::measure _dim)
            : dim(_dim)
        {}


        RestartKey( UnitSystem::measure _dim, bool _required)
            : dim(_dim),
              required(_required)
        {}

    };


    /*
      A simple struct - the only purpose is to facilitate return by value from the
      RestartIO::load( ) function.
    */


    struct RestartValue {

        data::Solution solution;
        data::Wells wells;
        std::map<std::string,std::vector<double>> extra = {};


        RestartValue(data::Solution sol, data::Wells wells_arg, std::map<std::string, std::vector<double>> extra_arg) :
            solution(std::move(sol)),
            wells(std::move(wells_arg)),
            extra(std::move(extra_arg))
        {
        }


        RestartValue(data::Solution sol, data::Wells wells_arg) :
            solution(std::move(sol)),
            wells(std::move(wells_arg))
        {
        }

    };

}


#endif
