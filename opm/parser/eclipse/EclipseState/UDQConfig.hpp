/*
 Copyright 2018 Statoil ASA.

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

#ifndef OPM_UDQ_CONFIG_HPP
#define OPM_UDQ_CONFIG_HPP


namespace Opm {

    class Deck;

    class UDQConfig
    {
    public:
        explicit UDQConfig(const Deck& deck);
        UDQConfig();

        bool   reseedRNG() const noexcept;
        int    randomSeed() const noexcept;
        double range() const noexcept;
        double undefinedValue() const noexcept;
        double cmpEpsilon() const noexcept;

    private:
        bool reseed_rng;
        int random_seed;
        double value_range;
        double undefined_value;
        double cmp_eps;
    };
}

#endif
