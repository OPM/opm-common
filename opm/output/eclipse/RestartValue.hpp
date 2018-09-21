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

#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/output/data/Solution.hpp>
#include <opm/output/data/Wells.hpp>

namespace Opm {


    class RestartKey {
    public:

        std::string key;
        UnitSystem::measure dim;
        bool required;

        RestartKey( const std::string& _key, UnitSystem::measure _dim)
            : key(_key),
              dim(_dim),
              required(true)
        {}


        RestartKey( const std::string& _key, UnitSystem::measure _dim, bool _required)
            : key(_key),
              dim(_dim),
              required(_required)
        {}

    };


    /*
      A simple class used to communicate values between the simulator and the
      RestartIO function.
    */


    class RestartValue {
    public:
        using ExtraVector = std::vector<std::pair<RestartKey, std::vector<double>>>;
        data::Solution solution;
        data::Wells wells;
        ExtraVector extra;

        RestartValue(data::Solution sol, data::Wells wells_arg);

        bool hasExtra(const std::string& key) const;
        void addExtra(const std::string& key, UnitSystem::measure dimension, std::vector<double> data);
        void addExtra(const std::string& key, std::vector<double> data);
        const std::vector<double>& getExtra(const std::string& key) const;

        void convertFromSI(const UnitSystem& units);
        void convertToSI(const UnitSystem& units);
    };

}


#endif
