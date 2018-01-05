/*
  Copyright (C) 2017 TNO

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

#include <opm/parser/eclipse/EclipseState/AquiferCT.hpp>

namespace Opm {

    AquiferCT::AquiferCT(const EclipseState& eclState, const Deck& deck)
    { 
      if (!deck.hasKeyword("AQUCT")){
                std::cout<<("The Carter-Tracy aquifer parameters must be specified in the deck through the AQUCT keyword")<<std::endl;
                }
                const auto& aquctKeyword = deck.getKeyword("AQUCT");
                // Resize the parameter vector container based on row entries in aquct
                // We do the same for aquifers too because number of aquifers is assumed to be for each entry in aquct
                m_aquct.resize(aquctKeyword.size());
                for (size_t aquctRecordIdx = 0; aquctRecordIdx < aquctKeyword.size(); ++ aquctRecordIdx) 
                {
                    const auto& aquctRecord = aquctKeyword.getRecord(aquctRecordIdx);

                    m_aquct.at(aquctRecordIdx).aquiferID = aquctRecord.getItem("AQUIFER_ID").template get<int>(0);
                    m_aquct.at(aquctRecordIdx).h = aquctRecord.getItem("THICKNESS_AQ").getSIDouble(0);
                    m_aquct.at(aquctRecordIdx).phi_aq = aquctRecord.getItem("PORO_AQ").getSIDouble(0);
                    m_aquct.at(aquctRecordIdx).d0 = aquctRecord.getItem("DAT_DEPTH").getSIDouble(0);
                    m_aquct.at(aquctRecordIdx).C_t = aquctRecord.getItem("C_T").getSIDouble(0);
                    m_aquct.at(aquctRecordIdx).r_o = aquctRecord.getItem("RAD").getSIDouble(0);
                    m_aquct.at(aquctRecordIdx).k_a = aquctRecord.getItem("PERM_AQ").getSIDouble(0);
                    m_aquct.at(aquctRecordIdx).theta = aquctRecord.getItem("INFLUENCE_ANGLE").getSIDouble(0);
                    m_aquct.at(aquctRecordIdx).c1 = 0.008527; // We are using SI
                    m_aquct.at(aquctRecordIdx).c2 = 6.283;
                    m_aquct.at(aquctRecordIdx).inftableID = aquctRecord.getItem("TABLE_NUM_INFLUENCE_FN").template get<int>(0);
                    m_aquct.at(aquctRecordIdx).pvttableID = aquctRecord.getItem("TABLE_NUM_WATER_PRESS").template get<int>(0);

                    // Get the correct influence table values
                    if (m_aquct.at(aquctRecordIdx).inftableID > 1)
                    {
                        const auto& aqutabTable = eclState.getTableManager().getAqutabTables().getTable(m_aquct.at(aquctRecordIdx).inftableID - 2);
                        const auto& aqutab_tdColumn = aqutabTable.getColumn(0);
                        const auto& aqutab_piColumn = aqutabTable.getColumn(1);
                        m_aquct.at(aquctRecordIdx).td = aqutab_tdColumn.vectorCopy();
                        m_aquct.at(aquctRecordIdx).pi = aqutab_piColumn.vectorCopy();
                    }
                    else
                    {
                        set_default_tables(m_aquct.at(aquctRecordIdx).td,m_aquct.at(aquctRecordIdx).pi);
                    }
                }

      if (!deck.hasKeyword("AQUANCON")){
                std::cout<<("The Carter-Tracy aquifer connections must be specified in the deck with the AQUANCON keyword")<<std::endl;
                }

                const auto& aquanconKeyword = deck.getKeyword("AQUANCON");
                // Resize the parameter vector container based on row entries in aquancon
                m_aquancon.resize(aquanconKeyword.size());
                //For now assuming AQUANCON keyword defines connection only for one aquifer
                for (size_t aquanconRecordIdx = 0; aquanconRecordIdx < aquanconKeyword.size(); ++ aquanconRecordIdx) 
                {
                    const auto& aquanconRecord = aquanconKeyword.getRecord(aquanconRecordIdx);

                    m_aquancon.at(aquanconRecordIdx).aquiferID = aquanconRecord.getItem("AQUIFER_ID").template get<int>(0);
                    m_aquancon.at(aquanconRecordIdx).i1 = aquanconRecord.getItem("I1").template get<int>(0);
                    m_aquancon.at(aquanconRecordIdx).i2 = aquanconRecord.getItem("I2").template get<int>(0);
                    m_aquancon.at(aquanconRecordIdx).j1 = aquanconRecord.getItem("J1").template get<int>(0);
                    m_aquancon.at(aquanconRecordIdx).j2 = aquanconRecord.getItem("J2").template get<int>(0);
                    m_aquancon.at(aquanconRecordIdx).k1 = aquanconRecord.getItem("K1").template get<int>(0);
                    m_aquancon.at(aquanconRecordIdx).k2 = aquanconRecord.getItem("K2").template get<int>(0);
                    m_aquancon.at(aquanconRecordIdx).influx_coeff = aquanconRecord.getItem("INFLUX_COEFF").getSIDouble(0);
                    m_aquancon.at(aquanconRecordIdx).influx_mult = aquanconRecord.getItem("INFLUX_MULT").getSIDouble(0);
                    m_aquancon.at(aquanconRecordIdx).face = aquanconRecord.getItem("FACE").getTrimmedString(0);

                }                                          
    }

    const std::vector<AquiferCT::AQUCT_data>& AquiferCT::getAquifers() const

    {
        return m_aquct;
    }

    const std::vector<AquiferCT::AQUANCON_data>& AquiferCT::getAquancon() const

    {
        return m_aquancon;
    }

    const int AquiferCT::getAqInflTabID(size_t aquiferIndex)

    {
        return m_aquct.at(aquiferIndex).inftableID;
    }

    const int AquiferCT::getAqPvtTabID(size_t aquiferIndex)

    {
        return m_aquct.at(aquiferIndex).pvttableID;
    }

    const double AquiferCT::getAqInfluxCoeff(size_t aquanconRecord)

    {
        return m_aquancon.at(aquanconRecord).influx_coeff;
    }

    const double AquiferCT::getAqInfluxMult(size_t aquanconRecord)

    {
        return m_aquancon.at(aquanconRecord).influx_mult;
    }

}