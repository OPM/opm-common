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

#include <opm/input/eclipse/EclipseState/Aquifer/AquiferFlux.hpp>
#include <include/opm/input/eclipse/Parser/ParserKeywords/A.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

namespace Opm {
    SingleAquiferFlux::SingleAquiferFlux(const DeckRecord& record)
    : id (record.getItem<ParserKeywords::AQUFLUX::AQUIFER_ID>().get<int>(0))
    , flux(record.getItem<ParserKeywords::AQUFLUX::FLUX>().getSIDouble(0))
    , salt_concentration(record.getItem<ParserKeywords::AQUFLUX::SC_0>().getSIDouble(0))
    , active(true)
    {
        if (record.getItem<ParserKeywords::AQUFLUX::TEMP>().hasValue(0)) {
            this->temperature = record.getItem<ParserKeywords::AQUFLUX::TEMP>().getSIDouble(0);
        }

        if (record.getItem<ParserKeywords::AQUFLUX::PRESSURE>().hasValue(0)) {
            this->datum_pressure = record.getItem<ParserKeywords::AQUFLUX::PRESSURE>().getSIDouble(0);
        }
    }

    SingleAquiferFlux::SingleAquiferFlux(const int aquifer_id)
    : id (aquifer_id)
    , active(false)
    {
    }

    SingleAquiferFlux::SingleAquiferFlux(const int id_arg, const double flux_arg, const double sal,
                                         const bool active_arg, double temp, double pres)
       : id(id_arg)
       , flux(flux_arg)
       , salt_concentration(sal)
       , active(active_arg)
       , temperature(temp)
       , datum_pressure(pres)
    {}

    bool SingleAquiferFlux::operator==(const SingleAquiferFlux& other) const {
       return this->id == other.id &&
              this->flux == other.flux &&
              this->salt_concentration == other.salt_concentration &&
              this->active == other.active &&
              this->temperature == other.temperature &&
              this->datum_pressure == other.datum_pressure;
    }

    SingleAquiferFlux SingleAquiferFlux::serializationTestObject() {
        SingleAquiferFlux result(1, 5., 3.0, true, 8.0, 10.0);
        return result;
    }

    void AquiferFlux::appendAqufluxSchedule(const std::unordered_set<int>& ids) {
        for (const auto& id : ids) {
            if (this->m_aquifers.count(id) == 0) {
                // we create an inactvie dummy aquflux aquifers,
                // while essentially, we only need the list of the ids
                this->m_aquifers.insert({id, SingleAquiferFlux{id}});
            }
        }
    }

    AquiferFlux::AquiferFlux(const std::vector<const DeckKeyword*>& keywords) {
        for (const auto* keyword : keywords) {
            for (const auto& record : *keyword) {
                SingleAquiferFlux aquifer(record);
                this->m_aquifers.insert_or_assign(aquifer.id, aquifer);
            }
        }
    }

    AquiferFlux::AquiferFlux(const std::vector<SingleAquiferFlux>& aquifers)
    {
        for (const auto& aquifer : aquifers) {
            this->m_aquifers.emplace(aquifer.id, aquifer);
        }
    }

    bool AquiferFlux::operator==(const AquiferFlux& other) const {
        return this->m_aquifers == other.m_aquifers;
    }

    bool AquiferFlux::hasAquifer(const int id) const {
        return this->m_aquifers.count(id) > 0;
    }

    size_t AquiferFlux::size() const {
        return this->m_aquifers.size();
    }

    AquiferFlux::AquFluxs::const_iterator AquiferFlux::begin() const {
        return this->m_aquifers.begin();
    }

    AquiferFlux::AquFluxs::const_iterator AquiferFlux::end() const {
        return this->m_aquifers.end();
    }

    void AquiferFlux::loadFromRestart(const RestartIO::RstAquifer&) {
        // Constant flux aquifer objects loaded from restart file added to
        // Schedule only.
    }

    AquiferFlux AquiferFlux::serializationTestObject() {
        AquiferFlux result;
        auto single_aquifer = SingleAquiferFlux::serializationTestObject();
        result.m_aquifers.insert({single_aquifer.id, single_aquifer});
        return result;
    }
} // end of namespace Opm
