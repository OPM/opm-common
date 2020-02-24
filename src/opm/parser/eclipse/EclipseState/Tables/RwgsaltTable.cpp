/*
  Copyright (C) 2020 by TNO

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
#include <opm/parser/eclipse/EclipseState/Tables/RwgsaltTable.hpp>

namespace Opm {

        static const size_t numEntries = 2;
        RwgsaltTable::RwgsaltTable()
        {
        }

        RwgsaltTable::RwgsaltTable(double refPressValue,
                                     double refSaltConValue,
                                     const std::vector<double>& tableValues)
            : m_pRefValues(refPressValue)
            , m_saltConsRefValues(refSaltConValue)
            , m_tableValues(tableValues)
        {
        }

        void RwgsaltTable::init(const Opm::DeckRecord& record0, const Opm::DeckRecord& record1)
        {

            m_pRefValues = record0.getItem("P_REF").getSIDoubleData()[0];
            m_saltConsRefValues = record0.getItem("SALT_CONCENTRATION_REF").getSIDoubleData()[0];
            m_tableValues = record1.getItem("DATA").getSIDoubleData();
        }

        size_t RwgsaltTable::size() const
        {
            return m_tableValues.size()/numEntries;
        }

        const std::vector<double>& RwgsaltTable::getTableValues() const
        {
            return m_tableValues;
        }

        double RwgsaltTable::getReferencePressureValue() const
        {
            return m_pRefValues;
        }

        double RwgsaltTable::getReferenceSaltConcentrationValue() const
        {
            return m_saltConsRefValues;
        }

        std::vector<double> RwgsaltTable::getSaltConcentrationColumn() const
        {
            size_t tableindex = 0;
            std::vector<double> saltCons(this->size());
            for(size_t i=0; i<this->size(); ++i){
                saltCons[i] = m_tableValues[tableindex];
                tableindex = tableindex+numEntries;
            }
            return saltCons;

        }

        std::vector<double> RwgsaltTable::getVaporizedWaterGasRatioColumn() const
        {
            size_t tableindex = 1;
            std::vector<double> vaporizedwatergasratio(this->size());
            for(size_t i=0; i<this->size(); ++i){
                vaporizedwatergasratio[i] = m_tableValues[tableindex];
                tableindex = tableindex+numEntries;
            }
            return formationvolumefactor;

        }       

        bool RwgsaltTable::operator==(const RwgsaltTable& data) const
        {
            return m_pRefValues == data.m_pRefValues &&
                   m_saltConsRefValues == data.m_saltConsRefValues &&
                   m_tableValues == data.m_tableValues;
        }

}

