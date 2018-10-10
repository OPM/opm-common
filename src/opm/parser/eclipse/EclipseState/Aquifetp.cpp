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

#include <opm/parser/eclipse/EclipseState/Aquifetp.hpp>

namespace Opm {

    Aquifetp::Aquifetp(const Deck& deck)
    { 
        if (!deck.hasKeyword("AQUFETP"))
            return;

        const auto& aqufetpKeyword = deck.getKeyword("AQUFETP");

        for (auto& aqufetpRecord : aqufetpKeyword){
  
            Aquifetp::AQUFETP_data data;
     
            data.aquiferID = aqufetpRecord.getItem("AQUIFER_ID").template get<int>(0);
            data.d0 = aqufetpRecord.getItem("DAT_DEPTH").getSIDouble(0);
            data.C_t = aqufetpRecord.getItem("C_T").getSIDouble(0);
            data.pvttableID = aqufetpRecord.getItem("TABLE_NUM_WATER_PRESS").template get<int>(0);
	    data.V0 = aqufetpRecord.getItem("V0").getSIDouble(0);
            data.J = aqufetpRecord.getItem("PI").getSIDouble(0);

            if (aqufetpRecord.getItem("P0").hasValue(0) )
	    {
		double * raw_ptr = new double ( aqufetpRecord.getItem("P0").getSIDouble(0));
		data.p0.reset( raw_ptr );
	    }
	    m_aqufetp.push_back( std::move(data) );
            }     
    }

    const std::vector<Aquifetp::AQUFETP_data>& Aquifetp::getAquifers() const
    {
        return m_aqufetp;
    }

    int Aquifetp::getAqPvtTabID(size_t aquiferIndex)
    {
        return m_aqufetp.at(aquiferIndex).pvttableID;
    }

}
