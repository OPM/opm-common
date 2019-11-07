/*
  Copyright 2019 Equinor ASA.

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

#ifndef GCONSUMP_H
#define GCONSUMP_H

#include <map>
#include <string>

#include <opm/parser/eclipse/Deck/UDAValue.hpp>

namespace Opm {

    class GConSump {
    public:
        GConSump() = default;

        struct GCONSUMPGroup {
            UDAValue consumption_rate;
            UDAValue import_rate;
            std::string network_node;
        };

        bool has(const std::string& name) const;
        const GCONSUMPGroup& get(const std::string& name) const;
        void add(const std::string& name, const UDAValue& consumption_rate, const UDAValue& import_rate, const std::string network_node);
        size_t size() const;

    private:
        std::map<std::string, GCONSUMPGroup> groups;
    };

}

#endif
