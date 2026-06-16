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

#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/A.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <cstddef>

namespace Opm {

AquiferConfig::AquiferConfig(const TableManager& tables,
                             const EclipseGrid& grid,
                             const Deck& deck,
                             const FieldPropsManager& field_props)
    : aquifetp(tables, deck)
    , aquiferct(tables, deck)
    , aquiferflux(SOLUTIONSection(deck).getKeywordList("AQUFLUX"))
    , numerical_aquifers(deck, grid, field_props)
{
    this->loadAquiferTracers(deck);
}

AquiferConfig::AquiferConfig(const Aquifetp& fetp,
                             const AquiferCT& ct,
                             const AquiferFlux& aqufluxs,
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
    this->aquiferflux.loadFromRestart(aquifers);
    this->aqconn.loadFromRestart(aquifers);
}

AquiferConfig AquiferConfig::serializationTestObject()
{
    AquiferConfig result;
    result.aquifetp = Aquifetp::serializationTestObject();
    result.aquiferct = AquiferCT::serializationTestObject();
    result.aqconn = Aquancon::serializationTestObject();
    result.aquiferflux = AquiferFlux::serializationTestObject();
    result.numerical_aquifers = NumericalAquifers::serializationTestObject();
    result.aquifer_tracers_ = { AquiferTracerConcentration::serializationTestObject() };

    return result;
}

bool AquiferConfig::active() const {
    return this->hasAnalyticalAquifer() ||
           this->hasNumericalAquifer();
}

bool AquiferConfig::operator==(const AquiferConfig& other) const {
    return this->aquifetp == other.aquifetp &&
           this->aquiferct == other.aquiferct &&
           this->aquiferflux == other.aquiferflux &&
           this->aqconn == other.aqconn &&
           this->numerical_aquifers == other.numerical_aquifers &&
           this->aquifer_tracers_ == other.aquifer_tracers_;
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

const AquiferFlux& AquiferConfig::aquflux() const {
    return this->aquiferflux;
}

bool AquiferConfig::hasAquifer(const int aquID) const {
    return this->hasAnalyticalAquifer(aquID) ||
           numerical_aquifers.hasAquifer(aquID);
}

bool AquiferConfig::hasAnalyticalAquifer(const int aquID) const {
    return aquifetp.hasAquifer(aquID) ||
           aquiferct.hasAquifer(aquID) ||
           aquiferflux.hasAquifer(aquID);
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
        || (this->aquiferflux.size() > std::size_t{0});
}

void AquiferConfig::appendAqufluxSchedule(const std::unordered_set<int>& ids) {
    this->aquiferflux.appendAqufluxSchedule(ids);
}

void AquiferConfig::loadAquiferTracers(const Deck& deck)
{
    using AQANTRC = ParserKeywords::AQANTRC;

    for (const auto* keyword : SOLUTIONSection(deck).getKeywordList("AQANTRC")) {
        for (const auto& record : *keyword) {
            const int aquifer_id =
                record.getItem<AQANTRC::AQUIFER_ID>().get<int>(0);

            if (!this->hasAquifer(aquifer_id)) {
                throw OpmInputError {
                    fmt::format("AQANTRC references unknown aquifer ID {}",
                                aquifer_id),
                    keyword->location(),
                };
            }

            this->aquifer_tracers_.push_back(
                AquiferTracerConcentration {
                    aquifer_id,
                    record.getItem<AQANTRC::TRACER>().getTrimmedString(0),
                    record.getItem<AQANTRC::VALUE>().get<double>(0),
                });
        }
    }
}

const std::vector<AquiferTracerConcentration>&
AquiferConfig::aquiferTracers() const
{
    return this->aquifer_tracers_;
}

} // end of namespace Opm

std::vector<int> Opm::analyticAquiferIDs(const AquiferConfig& cfg)
{
    auto aquiferIDs = std::vector<int>{};

    if (! cfg.hasAnalyticalAquifer())
        return aquiferIDs;

    std::ranges::transform(cfg.ct(), std::back_inserter(aquiferIDs),
                           [](const auto& aquifer) { return aquifer.aquiferID; });

    std::ranges::transform(cfg.fetp(), std::back_inserter(aquiferIDs),
                           [](const auto& aquifer) { return aquifer.aquiferID; });

    std::ranges::transform(cfg.aquflux(), std::back_inserter(aquiferIDs),
                           [](const auto& aquifer) { return aquifer.second.id; });

    std::ranges::sort(aquiferIDs);

    return aquiferIDs;
}

std::vector<int> Opm::numericAquiferIDs(const AquiferConfig& cfg)
{
    auto aquiferIDs = std::vector<int>{};

    if (! cfg.hasNumericalAquifer())
        return aquiferIDs;

    const auto& aqunum = cfg.numericalAquifers();

    std::ranges::transform(aqunum.aquifers(), std::back_inserter(aquiferIDs),
                           [](const auto& aquifer)
                           { return static_cast<int>(aquifer.first); });

    std::ranges::sort(aquiferIDs);

    return aquiferIDs;
}
