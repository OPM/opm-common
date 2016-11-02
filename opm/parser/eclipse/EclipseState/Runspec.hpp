/*
  Copyright 2016  Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the terms
  of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  OPM is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OPM_RUNSPEC_HPP
#define OPM_RUNSPEC_HPP

#include <bitset>
#include <iosfwd>
#include <string>

namespace Opm {

class Deck;

enum class Phase {
    OIL   = 0,
    GAS   = 1,
    WATER = 2,
};

Phase get_phase( const std::string& );
std::ostream& operator<<( std::ostream&, const Phase& );

class Phases {
    public:
        Phases() noexcept = default;
        Phases( bool oil, bool gas, bool wat ) noexcept;

        bool active( Phase ) const noexcept;
        size_t size() const noexcept;

    private:
        std::bitset< 3 > bits;
};

class Runspec {
    public:
        explicit Runspec( const Deck& );
        explicit Runspec( const Phases& ) noexcept;

        const Phases& phases() const noexcept;

    private:
        Phases active_phases;
};

}

#endif // OPM_RUNSPEC_HPP

