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


#include <opm/io/eclipse/rst/well.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Units/Dimension.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Units/Units.hpp>
#include <opm/input/eclipse/Schedule/Well/WDFAC.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>


#include <string>
#include <vector>
#include <cmath>
#include <iostream>
#include <cassert>
#include <numeric>

namespace Opm {

    WDFAC WDFAC::serializationTestObject()
    {
        WDFAC result;
        result.m_a = 1.23;
        result.m_b = 0.456;
        result.m_c = 0.457;
        result.m_d = 0.458;
        result.m_total_cf = 1.0;
        result.m_type = WDFACTYPE::NONE;

        return result;
    }

    bool WDFAC::operator==(const WDFAC& other) const {
        return (m_a == other.m_a)
            && (m_b == other.m_b)
            && (m_c == other.m_c)
            && (m_d == other.m_d)
            && (m_total_cf == other.m_total_cf)
            && (m_type == other.m_type);
    }

    void WDFAC::updateWDFAC(const DeckRecord& record) {
        m_d = record.getItem<ParserKeywords::WDFAC::DFACTOR>().getSIDouble(0);
        m_type = WDFACTYPE::DFACTOR;
    }

    void WDFAC::updateWDFACCOR(const DeckRecord& record) {
        m_a = record.getItem<ParserKeywords::WDFACCOR::A>().getSIDouble(0);
        m_b = record.getItem<ParserKeywords::WDFACCOR::B>().getSIDouble(0);
        m_c = record.getItem<ParserKeywords::WDFACCOR::C>().getSIDouble(0);
        m_type = WDFACTYPE::DAKEMODEL;
    }

    void WDFAC::updateWDFACType(const WellConnections& connections) {

        const auto non_trivial_dfactor =
            std::any_of(connections.begin(), connections.end(),
                [](const auto& conn) { return conn.dFactor() != 0.0; });

        // non-trivial dfactors detected use connection D factors
        if (non_trivial_dfactor) {
            m_type = WDFACTYPE::CON_DFACTOR;
            updateTotalCF(connections);
        }
    }

    void WDFAC::updateTotalCF(const WellConnections& connections) {
        m_total_cf = std::accumulate(connections.begin(), connections.end(), 0.0,
                [](const double tot_cf, const auto& conn) { return tot_cf + conn.CF(); });
    }

    double WDFAC::getDFactor(const Connection& connection, double mu, double rho, double phi) const {

        switch (m_type)
        {
        case WDFACTYPE::NONE:
            return 0.0;
        case WDFACTYPE::DFACTOR: {
            if (m_total_cf < 0.0) {
                throw std::invalid_argument { "Total connection factor is not set" };
            }
            return  m_d * m_total_cf / connection.CF();
        }
        case WDFACTYPE::CON_DFACTOR: {
            double d =  connection.dFactor();
            // If a negative d factor is set in COMPDAT individual connection d factors should be used directly.
            if (d < 0)
                return -d;
            // If a positive d factor is set in COMPDAT the connection d factors is treated like a well d factor.
            // and thus scaled with the connection index
            if (m_total_cf < 0.0) {
                throw std::invalid_argument { "Total connection factor is not set" };
            }

            return d * m_total_cf / connection.CF();
        }
        case WDFACTYPE::DAKEMODEL:
        {   
            double Kh = connection.Kh();
            double Ke = connection.Ke();
            double h = Kh / Ke;
            double rw = connection.rw();
            const auto k_md = unit::convert::to(Ke, prefix::milli*unit::darcy);
            double beta = m_a * (std::pow(k_md, m_b) * std::pow(phi, m_c));
            double specific_gravity = rho / 1.225; // divide by density of air at standard conditions. 
            return beta * specific_gravity * Ke / (h * mu * rw );
        }
        default:
            break;
        }
        return 0.0;
    }

    bool WDFAC::useDFactor() const {
        return m_type != WDFACTYPE::NONE;
    }

    bool WDFAC::operator!=(const WDFAC& other) const {
        return !(*this == other);
    }
}
