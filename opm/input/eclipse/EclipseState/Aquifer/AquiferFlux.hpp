/*
  Copyright (C) 2023 Equinor

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

#ifndef OPM_AQUIFERFLUX_HPP
#define OPM_AQUIFERFLUX_HPP

#include <optional>
#include <unordered_map>
#include <vector>

namespace Opm {
    class DeckKeyword;
    class DeckRecord;
}

namespace  Opm {
    struct AquiferFlux {
        explicit AquiferFlux(const DeckRecord& record);
        int id;
        double flux;
        double salt_concentration;
        std::optional<double> temperature;
        std::optional<double> datum_pressure;
        // to work with ScheduleState::map_member
        int name() const;

        static std::unordered_map<int, AquiferFlux> aqufluxFromKeywords(const std::vector<const DeckKeyword*>& keywords);
    };
} // end of namespace Opm

#endif //OPM_AQUIFERFLUX_HPP
