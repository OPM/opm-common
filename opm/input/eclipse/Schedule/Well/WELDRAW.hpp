/*
  Copyright (C) 2026 Equinor ASA

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

#ifndef WELDRAW_HPP_HEADER_INCLUDED
#define WELDRAW_HPP_HEADER_INCLUDED

#include <opm/input/eclipse/Deck/UDAValue.hpp>

#include <string>

namespace Opm {
    class DeckRecord;
    enum class Phase;
    class SummaryState;
} // namespace Opm

namespace Opm {

    /// Maximum allowable drawdown for production wells (keyword WELDRAW).
    ///
    /// At each timestep the drawdown limit is converted into a maximum
    /// production rate for the selected phase,
    ///
    ///    Qmax = Dmax * sum_j (Tw_j * M_j)
    ///
    /// with the sum running over the well's open connections.
    class WELDRAW
    {
    public:
        /// Phase whose production rate is limited by the drawdown limit.
        enum class TargetPhase : unsigned char { LIQ, GAS };

        /// Whether the limit applies to the PI-weighted average drawdown
        /// (AVG) or to the maximum drawdown over the well's connection
        /// grid blocks (MAX).
        enum class Mode : unsigned char { AVG, MAX };

        static WELDRAW serializationTestObject();

        /// Assign drawdown limit properties from a WELDRAW record.
        ///
        /// \param[in] record Single WELDRAW keyword record.
        /// \param[in] preferred_phase The well's preferred phase (WELSPECS
        /// item 6), used to infer the target phase when item 3 is defaulted.
        void update(const DeckRecord& record, Phase preferred_phase);

        /// Whether a drawdown limit is currently in effect.
        bool active() const
        {
            return this->m_active;
        }

        /// Maximum allowable drawdown in SI units (Pascal).
        ///
        /// \param[in] well_name Name of the well owning this limit, for
        /// UDA evaluation.
        /// \param[in] st Summary vectors, for UDA evaluation.
        /// \param[in] udq_undefined Value of undefined UDQs.
        double maxDrawdown(const std::string& well_name,
                           const SummaryState& st,
                           double udq_undefined) const;

        TargetPhase targetPhase() const
        {
            return this->m_phase;
        }

        Mode mode() const
        {
            return this->m_mode;
        }

        /// Whether the drawdown limit should be included when calculating
        /// the well's production potential (item 4).
        bool useInPotentials() const
        {
            return this->m_use_in_potentials;
        }

        bool operator==(const WELDRAW& other) const;
        bool operator!=(const WELDRAW& other) const
        {
            return !(*this == other);
        }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_max_drawdown);
            serializer(m_phase);
            serializer(m_mode);
            serializer(m_use_in_potentials);
            serializer(m_active);
        }

    private:
        UDAValue m_max_drawdown{0.0};
        TargetPhase m_phase{TargetPhase::LIQ};
        Mode m_mode{Mode::AVG};
        bool m_use_in_potentials{false};
        bool m_active{false};
    };

} // namespace Opm

#endif // WELDRAW_HPP_HEADER_INCLUDED
