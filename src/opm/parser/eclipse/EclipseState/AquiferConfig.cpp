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


#include <opm/parser/eclipse/EclipseState/AquiferConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
// #include <opm/parser/eclipse/EclipseState/Grid/FieldPropsManager.hpp>

namespace Opm {

AquiferConfig::AquiferConfig(const TableManager& tables, const EclipseGrid& grid,
                             const FieldPropsManager& field_props, const Deck& deck):
    aquifetp(deck),
    aquiferct(tables, deck),
    numerical_aquifers(deck, grid, field_props),
    aqconn(grid,deck)
{}

AquiferConfig::AquiferConfig(const Aquifetp& fetp, const AquiferCT& ct, const Aquancon& conn) :
    aquifetp(fetp),
    aquiferct(ct),
    aqconn(conn)
{}

AquiferConfig AquiferConfig::serializeObject()
{
    AquiferConfig result;
    result.aquifetp = Aquifetp::serializeObject();
    result.aquiferct = AquiferCT::serializeObject();
    result.aqconn = Aquancon::serializeObject();

    return result;
}

bool AquiferConfig::active() const {
    return this->aqconn.active();
}

bool AquiferConfig::operator==(const AquiferConfig& other) {
    return this->aquifetp == other.aquifetp &&
           this->aquiferct == other.aquiferct &&
           this->aqconn == other.aqconn;
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

bool AquiferConfig::hasAquifer(const int aquID) const {
    return aquifetp.hasAquifer(aquID) || aquiferct.hasAquifer(aquID);
}

bool AquiferConfig::hasNumericalAquifer() const {
    return !this->numerical_aquifers.empty();
}

void AquiferConfig::updatePoreVolume(std::vector<double>& pore_volume) const {
    if ( !this->hasNumericalAquifer() ) return;

    this->numerical_aquifers.updatePoreVolume(pore_volume);
}

const NumericalAquifers& AquiferConfig::numericalAquifers() const {
    return this->numerical_aquifers;
}

}
