/*
  Copyright (C) 2019 by Norce

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

#include <vector>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SaltSolubilityTable.hpp>

namespace Opm {
        SaltSolubilityTable SaltSolubilityTable::serializeObject()
        {
            SaltSolubilityTable result;
            result.m_tableValues = {1.0, 2.0, 3.0};

            return result;
        }

        void SaltSolubilityTable::init(const Opm::DeckRecord& record )
        {
            m_tableValues = record.getItem("SALT_SOLUBILITY").getSIDoubleData();
        }

        const std::vector<double>& SaltSolubilityTable::getSaltSolubilityColumn() const
        {
            return m_tableValues;
        }

        bool SaltSolubilityTable::operator==(const SaltSolubilityTable& data) const
        {
            return m_tableValues == data.m_tableValues;
        }


}

