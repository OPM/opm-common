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

namespace Opm {

    class SummaryState;

    class GSatProd {
    public:
        struct GSatProdGroup {
            double oil_rate;
            double gas_rate;
            double water_rate;
            double resv_rate;
            double glift_rate;

            bool operator==(const GSatProdGroup& data) const {
                return oil_rate == data.oil_rate &&
                       gas_rate == data.gas_rate &&
                       water_rate == data.water_rate &&
                       resv_rate == data.resv_rate &&
                       glift_rate == data.glift_rate;
            }

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(oil_rate);
                serializer(gas_rate);
                serializer(water_rate);
                serializer(resv_rate);
                serializer(glift_rate);
            }
        };

        static GSatProd serializationTestObject();

        bool has(const std::string& name) const;
        const GSatProdGroup& get(const std::string& name) const;
        void add(const std::string& name, const double& oil_rate,
                 const double& gas_rate, const double& water_rate,
                 const double& resv_rate, const double& glift_rate);
        size_t size() const;

        bool operator==(const GSatProd& data) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(groups);
        }

    private:
        std::map<std::string, GSatProdGroup> groups;
    };

}

#endif
