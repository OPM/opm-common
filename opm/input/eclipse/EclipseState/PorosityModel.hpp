/*
  Copyright 2026 TNO

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

#ifndef OPM_POROSITY_MODEL_HPP
#define OPM_POROSITY_MODEL_HPP

namespace Opm {

class Deck;

/// Porosity model of a simulation run, as selected in the RUNSPEC section.
///
/// A run resolves either one continuum per cell (single porosity, the
/// default) or two: the rock matrix and the fracture system.  DUALPORO
/// selects the dual-porosity model, in which fluid moves between fracture
/// cells and between each matrix cell and its fracture twin, and DUALPERM
/// the dual-permeability model, which adds flow between matrix cells.  In
/// the dual-continuum models the fracture permeabilities are scaled by the
/// fracture porosity unless NODPPM switches the scaling off.
///
/// This is the selection the EGRID file header records to distinguish
/// single-porosity, dual-porosity and dual-permeability grids.
class PorosityModel
{
public:
    /// Which continua the run resolves.
    enum class Type {
        /// One continuum per cell.
        SinglePorosity,

        /// Matrix and fracture continua, with flow between fracture cells
        /// and between each matrix cell and its fracture twin.
        DualPorosity,

        /// Dual porosity with flow between matrix cells as well.
        DualPermeability,
    };

    /// Single porosity.
    PorosityModel() = default;

    /// Porosity model selected by the RUNSPEC keywords of a deck.
    ///
    /// \param[in] deck Input deck.  DUALPERM takes precedence over DUALPORO.
    explicit PorosityModel(const Deck& deck);

    /// Selected model.
    Type type() const noexcept;

    /// Whether the run resolves both a matrix and a fracture continuum,
    /// i.e., whether the model is dual porosity or dual permeability.
    bool dualContinuum() const noexcept;

    /// Whether the run resolves flow between matrix cells, i.e., whether
    /// the model is dual permeability.
    bool dualPermeability() const noexcept;

    /// Whether the fracture permeabilities are scaled by the fracture
    /// porosity.  Active in the dual-continuum models unless NODPPM is
    /// specified, never in single porosity.  Every consumer of the scaling
    /// rule, well connection factors and cell transmissibilities alike,
    /// must take the answer from here rather than derive its own.
    bool fracturePermeabilityScalingActive() const noexcept;

    static PorosityModel serializationTestObject();

    bool operator==(const PorosityModel& other) const;

    template <class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->m_type);
        serializer(this->m_scale_fracture_perm);
    }

private:
    Type m_type{Type::SinglePorosity};

    /// Fracture permeability scaling as requested by the deck.  Only
    /// meaningful in the dual-continuum models; NODPPM turns it off.
    bool m_scale_fracture_perm{true};
};

} // namespace Opm

#endif // OPM_POROSITY_MODEL_HPP
