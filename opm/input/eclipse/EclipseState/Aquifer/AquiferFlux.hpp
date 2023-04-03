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
#include <unordered_set>
#include <vector>

namespace Opm {
    class DeckKeyword;
    class DeckRecord;
}

namespace Opm { namespace RestartIO {
    class RstAquifer;
}} // Opm::RestartIO

namespace  Opm {
    struct SingleAquiferFlux
    {
        SingleAquiferFlux() = default;
        explicit SingleAquiferFlux(const DeckRecord& record);

        // using id to create an inactive dummy aquifer
        explicit SingleAquiferFlux(int id);
        SingleAquiferFlux(int id, double flux, double sal, bool active_, double temp, double pres);

        int id {0};
        double flux {0.};
        double salt_concentration {0.};
        bool active {false};
        std::optional<double> temperature;
        std::optional<double> datum_pressure;

        bool operator==(const SingleAquiferFlux& other) const;

        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->id);
            serializer(this->flux);
            serializer(this->salt_concentration);
            serializer(this->active);
            serializer(this->temperature);
            serializer(this->datum_pressure);
        }

        static SingleAquiferFlux serializationTestObject();
    };

    class AquiferFlux
    {
    public:
        using AquFluxs = std::unordered_map<int, SingleAquiferFlux>;

        AquiferFlux() = default;
        explicit AquiferFlux(const std::vector<const DeckKeyword*>& keywords);

        // Primarily for unit testing purposes.
        explicit AquiferFlux(const std::vector<SingleAquiferFlux>& aquifers);

        void appendAqufluxSchedule(const std::unordered_set<int>& ids);

        bool hasAquifer(int id) const;

        bool operator==(const AquiferFlux& other) const;

        size_t size() const;

        AquFluxs::const_iterator begin() const;
        AquFluxs::const_iterator end() const;

        void loadFromRestart(const RestartIO::RstAquifer& rst);

        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->m_aquifers);
        }

        static AquiferFlux serializationTestObject();

    private:
        AquFluxs m_aquifers{};
    };
} // end of namespace Opm

#endif //OPM_AQUIFERFLUX_HPP
