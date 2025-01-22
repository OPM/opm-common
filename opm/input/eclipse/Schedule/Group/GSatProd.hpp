/*
  Copyright 2024 Equinor ASA.

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

#ifndef GSATPROD_H
#define GSATPROD_H

#include <map>
#include <string>
#include <array>

namespace Opm {

    class SummaryState;

    class GSatProd {
    public:
        struct GSatProdGroup {
            enum Rate { Oil, Gas, Water, Resv, GLift };
            std::array<double, 5> rate{};
            bool operator==(const GSatProdGroup& data) const {
                return rate == data.rate;
            }

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(rate);
            }
        };

        static GSatProd serializationTestObject();

        bool has(const std::string& name) const;
        const GSatProdGroup& get(const std::string& name) const;
        void assign(const std::string& name, const double oil_rate,
                   const double gas_rate, const double water_rate,
                   const double resv_rate, const double glift_rate);
        std::size_t size() const;

        bool operator==(const GSatProd& data) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(groups_);
        }

    private:
        std::map<std::string, GSatProdGroup> groups_;
    };

}

#endif
