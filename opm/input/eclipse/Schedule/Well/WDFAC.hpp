/*
  Copyright 2023 Equinor.

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

#ifndef WDFAC_HPP_HEADER_INCLUDED
#define WDFAC_HPP_HEADER_INCLUDED

namespace Opm {
    class DeckRecord;
    class WellConnections;
} // namespace Opm

namespace Opm { namespace RestartIO {
    struct RstWell;
}} // namespace Opm::RestartIO

namespace Opm {

    enum class WDFACTYPE {
        NONE = 1,
        DFACTOR = 2,
        DAKEMODEL = 3,
        CON_DFACTOR = 4
    };

    class WDFAC
    {
    public:
        static WDFAC serializationTestObject();

        double getDFactor(const Connection& connection, double mu, double rho, double phi) const;
        void updateWDFAC(const DeckRecord& record);
        //void updateWDFAC(const RestartIO::RstWell& rst_well);
        void updateWDFACCOR(const DeckRecord& record);
        //void updateWDFACOR(const RestartIO::RstWell& rst_well);

        void updateWDFACType(const WellConnections& connections);
        bool useDFactor() const;

        bool operator==(const WDFAC& other) const;
        bool operator!=(const WDFAC& other) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_a);
            serializer(m_b);
            serializer(m_c);
            serializer(m_d);
            serializer(m_total_cf);
            serializer(m_type);
        }

    private:
        double m_a{0.0};
        double m_b{0.0};
        double m_c{0.0};
        double m_d{0.0};
        double m_total_cf{-1.0};
        WDFACTYPE m_type = WDFACTYPE::NONE;
    };

} // namespace Opm

#endif  // WDFAC_HPP_HEADER_INCLUDED
