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

#include <opm/output/data/Aquifer.hpp>
#include <opm/output/data/Groups.hpp>
#include <opm/output/data/Solution.hpp>
#include <opm/output/data/Wells.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <string>
#include <utility>
#include <vector>

namespace Opm {

    class RestartKey {
    public:
        std::string key{};
        UnitSystem::measure dim{UnitSystem::measure::_count};
        bool required = false;

        RestartKey() = default;

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

        bool operator==(const RestartKey& key2) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(key);
            serializer(dim);
            serializer(required);
        }

        static RestartKey serializationTestObject();
    };

    /*
      A simple class used to communicate values between the simulator and
      the RestartIO functions.
    */
    class RestartValue {
    public:
        using ExtraVector = std::vector<std::pair<RestartKey, std::vector<double>>>;

        data::Solution solution{};
        data::Wells wells{};
        data::GroupAndNetworkValues grp_nwrk{};
        data::Aquifers aquifer{};
        ExtraVector extra{};

        RestartValue(data::Solution sol,
                     data::Wells wells_arg,
                     data::GroupAndNetworkValues grpn_nwrk_arg,
                     data::Aquifers aquifer_arg);

        // Overloaded constructor to handle grid containing LGR
        RestartValue(data::Solution sol,
                     data::Wells wells_arg,
                     data::GroupAndNetworkValues grpn_nwrk_arg,
                     data::Aquifers aquifer_arg,
                     int lgr_grid);

        RestartValue() = default;

        bool hasExtra(const std::string& key) const;
        void addExtra(const std::string& key, UnitSystem::measure dimension, std::vector<double> data);
        void addExtra(const std::string& key, UnitSystem::measure dimension, std::vector<float> data);
        void addExtra(const std::string& key, std::vector<double> data);
        void addExtra(const std::string& key, std::vector<float> data);
        const std::vector<double>& getExtra(const std::string& key) const;
        void filter_wells_for_lgr(data::Wells& allwells, int lgr_grid);
        void convertFromSI(const UnitSystem& units);
        void convertToSI(const UnitSystem& units);

        bool operator==(const RestartValue& val2) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
          serializer(solution);
          serializer(wells);
          serializer(grp_nwrk);
          serializer(aquifer);
          serializer(extra);
        }

        static RestartValue serializationTestObject();
    };

}

#endif // RESTART_VALUE_HPP
