/*
  Copyright 2019 SINTEF Digital, Mathematics and Cybernetics.

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

#ifndef OPM_FOAMCONFIG_HPP
#define OPM_FOAMCONFIG_HPP

#include <cstddef>
#include <vector>

namespace Opm {

    class DeckKeyword;
    class DeckRecord;

    class FoamRecord {
        public:
            explicit FoamRecord( const DeckRecord& );

            double referenceSurfactantConcentration() const;
            double exponent() const;
            double minimumSurfactantConcentration() const;

        private:
            double reference_surfactant_concentration_;
            double exponent_;
            double minimum_surfactant_concentration_;
    };

    class FoamConfig {
        public:
            using const_iterator = std::vector< FoamRecord >::const_iterator;

            FoamConfig() = default;
            explicit FoamConfig( const DeckKeyword& );

            const FoamRecord& getRecord( std::size_t id ) const;

            std::size_t size() const;
            bool empty() const;

            const_iterator begin() const;
            const_iterator end() const;

        private:
            std::vector< FoamRecord > records;
    };

}

#endif // OPM_FOAMCONFIG_HPP
