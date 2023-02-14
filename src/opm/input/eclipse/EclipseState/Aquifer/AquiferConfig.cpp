/*
  Copyright (C) 2020 Equinor

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


#include <opm/input/eclipse/Deck/DeckSection.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/AquiferConfig.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <algorithm>
#include <cstddef>

namespace Opm {

AquiferConfig::AquiferConfig(const TableManager& tables,
                             const EclipseGrid& grid,
                             const Deck& deck,
                             const FieldPropsManager& field_props)
    : aquifetp(tables, deck)
    , aquiferct(tables, deck)
    , aquiferflux(AquiferFlux::aqufluxFromKeywords(SOLUTIONSection(deck).getKeywordList("AQUFLUX")) )
    , numerical_aquifers(deck, grid, field_props)
{}

AquiferConfig::AquiferConfig(const Aquifetp& fetp,
                             const AquiferCT& ct,
                             const AquFluxs& aqufluxs,
                             const Aquancon& conn)
    : aquifetp(fetp)
    , aquiferct(ct)
    , aquiferflux(aqufluxs)
    , aqconn(conn)
{}

void AquiferConfig::load_connections(const Deck& deck, const EclipseGrid& grid) {
    this->aqconn = Aquancon(grid, deck);
}

void AquiferConfig::pruneDeactivatedAquiferConnections(const std::vector<std::size_t>& deactivated_cells)
{
    if (deactivated_cells.empty())
        return;

    this->aqconn.pruneDeactivatedAquiferConnections(deactivated_cells);
}

void AquiferConfig::loadFromRestart(const RestartIO::RstAquifer& aquifers,
                                    const TableManager&          tables)
{
    this->aquifetp.loadFromRestart(aquifers, tables);
    this->aquiferct.loadFromRestart(aquifers, tables);
    this->aqconn.loadFromRestart(aquifers);
    // TODO: loadFromrestart for AQUFLUX;
}

AquiferConfig AquiferConfig::serializationTestObject()
{
    AquiferConfig result;
    result.aquifetp = Aquifetp::serializationTestObject();
    result.aquiferct = AquiferCT::serializationTestObject();
    result.aqconn = Aquancon::serializationTestObject();
    result.numerical_aquifers = NumericalAquifers::serializationTestObject();
    // TODO: serializationTestObject for AQUFLUX

    return result;
}

bool AquiferConfig::active() const {
    return this->hasAnalyticalAquifer() ||
           this->hasNumericalAquifer() ||
           !this->aquiferflux.empty() ;
}

bool AquiferConfig::operator==(const AquiferConfig& other) const {
    return this->aquifetp == other.aquifetp &&
           this->aquiferct == other.aquiferct &&
           this->aquiferflux == other.aquiferflux &&
           this->aqconn == other.aqconn &&
           this->numerical_aquifers == other.numerical_aquifers;
}

const AquiferCT& AquiferConfig::ct() const {
    return this->aquiferct;
}

const Aquifetp& AquiferConfig::fetp() const {
    return this->aquifetp;
}

const Aquancon& AquiferConfig::connections() const {
    return this->aqconn;
}

const AquiferConfig::AquFluxs& AquiferConfig::aquflux() const {
    return this->aquiferflux;
}

bool AquiferConfig::hasAquifer(const int aquID) const {
    return this->hasAnalyticalAquifer(aquID) ||
           numerical_aquifers.hasAquifer(aquID);
}

bool AquiferConfig::hasAnalyticalAquifer(const int aquID) const {
    return aquifetp.hasAquifer(aquID) ||
           aquiferct.hasAquifer(aquID) ||
           aquiferflux.count(aquID) > 0;
}

bool AquiferConfig::hasNumericalAquifer() const {
    return this->numerical_aquifers.size() > std::size_t{0};
}

const NumericalAquifers& AquiferConfig::numericalAquifers() const {
    return this->numerical_aquifers;
}

NumericalAquifers& AquiferConfig::mutableNumericalAquifers() const {
    return this->numerical_aquifers;
}

bool AquiferConfig::hasAnalyticalAquifer() const {
    return (this->aquiferct.size() > std::size_t{0})
        || (this->aquifetp.size() > std::size_t{0})
        || !this->aquiferflux.empty();
}

void AquiferConfig::appendAqufluxSchedule(const std::vector<int>& ids) {
    for (const auto& id : ids) {
        if (this->aquiferflux.find(id) == this->aquiferflux.end()) {
            // we create an inactvie dummy aquflux aquifers
            this->aquiferflux.insert({id, AquiferFlux{id}});
        }
    }
}

} // end of namespace Opm

std::vector<int> Opm::analyticAquiferIDs(const AquiferConfig& cfg)
{
    auto aquiferIDs = std::vector<int>{};

    if (! cfg.hasAnalyticalAquifer())
        return aquiferIDs;

    for (const auto& aquifer : cfg.ct())
        aquiferIDs.push_back(aquifer.aquiferID);

    for (const auto& aquifer : cfg.fetp())
        aquiferIDs.push_back(aquifer.aquiferID);

    for ([[maybe_unused]] const auto& [id, aqu] : cfg.aquflux())
        aquiferIDs.push_back(id);

    std::sort(aquiferIDs.begin(), aquiferIDs.end());

    return aquiferIDs;
}

std::vector<int> Opm::numericAquiferIDs(const AquiferConfig& cfg)
{
    auto aquiferIDs = std::vector<int>{};

    if (! cfg.hasNumericalAquifer())
        return aquiferIDs;

    const auto& aqunum = cfg.numericalAquifers();

    for (const auto& aq : aqunum.aquifers())
        aquiferIDs.push_back(static_cast<int>(aq.first));

    std::sort(aquiferIDs.begin(), aquiferIDs.end());

    return aquiferIDs;
}
