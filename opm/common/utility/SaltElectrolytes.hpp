/*
  Copyright 2026 NORCE.

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
#ifndef OPM_SALTELECTROLYTES_HPP
#define OPM_SALTELECTROLYTES_HPP

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/SaltArray.hpp>

#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/components/CaIon.hpp>
#include <opm/material/components/ClIon.hpp>
#include <opm/material/components/KIon.hpp>
#include <opm/material/components/MgIon.hpp>
#include <opm/material/components/NaIon.hpp>
#include <opm/material/components/SO4Ion.hpp>

#include <fmt/format.h>

#include <stdexcept>
#include <utility>
#include <vector>

namespace Opm
{

/*!
 * Speciates a SaltArray of dissolved ions into the electrolytes (NaCl, CaCl2, ...)
 * needed by the Laliberte (viscosity) and Laliberte & Cooper (density) correlations
 * for multicomponent-salt brine.
 *
 * @tparam Scalar Value type used for molar masses
 */
template <class Scalar>
class SaltElectrolytes
{
public:
    /// Supported electrolytes for Laliberte viscosity and Laliberte-Cooper density
    enum class Electrolyte
    {
        CaCl2, KCl, K2SO4, MgCl2, MgSO4, NaCl, Na2SO4
    };

    /*!
     * Molar mass of electrolyte
     *
     * @param e Electrolyte
     * @return Molar mass [kg/mol]
     */
    static Scalar molarMass(const Electrolyte e)
    {
        switch (e) {
        case Electrolyte::CaCl2:
            return CaIon<Scalar>::molarMass() + 2.0 * ClIon<Scalar>::molarMass();
        case Electrolyte::KCl:
            return KIon<Scalar>::molarMass() + ClIon<Scalar>::molarMass();
        case Electrolyte::K2SO4:
            return 2.0 * KIon<Scalar>::molarMass() + SO4Ion<Scalar>::molarMass();
        case Electrolyte::MgCl2:
            return MgIon<Scalar>::molarMass() + 2.0 * ClIon<Scalar>::molarMass();
        case Electrolyte::MgSO4:
            return MgIon<Scalar>::molarMass() + SO4Ion<Scalar>::molarMass();
        case Electrolyte::NaCl:
            return NaIon<Scalar>::molarMass() + ClIon<Scalar>::molarMass();
        case Electrolyte::Na2SO4:
            return 2.0 * NaIon<Scalar>::molarMass() + SO4Ion<Scalar>::molarMass();
        default:
            throw std::runtime_error("Unknown Electrolyte!");
        }
    }

    /*!
     * Generate electrolytes from salt ions
     *
     * @param salinity Array of salt ions
     * @return Vector of electrolytes and their mass fractions
     */
    template <class Evaluation>
    static std::vector<std::pair<Electrolyte, Evaluation> >
    fromSaltComponents(const SaltArray<Evaluation, SaltMassFraction>& salinity)
    {
        auto electrolytes = fromIonMolality_(salinity.template convert_to<SaltMolality>());

        // Convert salt electrolytes from molality to mass fraction
        molalToMassFrac_(electrolytes);

        return electrolytes;
    }

private:
    static constexpr Scalar molalTolerance = 1e-6; ///< tolerance for molality calculations

    /*!
     * Generate electrolytes (in molality) from salt ion molalities
     *
     * @param ionMolality Molality of each salt ion
     * @return Vector of electrolytes and their molality [mol electrolyte/kg water]
     */
    template <class Evaluation>
    static std::vector<std::pair<Electrolyte, Evaluation> >
    fromIonMolality_(const SaltArray<Evaluation, SaltMolality>& ionMolality)
    {
        // Generate (valid) electrolytes from salt components
        auto molalSalinity = ionMolality;
        auto [cations, anions] = molalSalinity.cations_and_anions();
        std::vector<std::pair<Electrolyte, Evaluation> > electrolytes;
        for (const auto& anionIdx : anions) {
            auto& molalAnion = molalSalinity[anionIdx];
            for (std::size_t i = 0; i < cations.size() && molalAnion >= molalTolerance; ++i) {
                const auto& cationIdx = cations[i];
                auto& molalCation = molalSalinity[cationIdx];
                if (molalCation < molalTolerance) {
                    continue;
                }

                // Generate salt electrolyte (in molal), and subtract used cation and anion
                // molalities. Note: Electrolytes are made from cation and anion pairs in
                // decreasing ion strength
                electrolytes.push_back(
                    electrolyteAndRemainingMolal_(cationIdx,
                                                  molalCation,
                                                  anionIdx,
                                                  molalAnion));
            }
        }

        // Warn if there is any leftover molality after generating electrolytes
        if (molalSalinity.sum() > molalTolerance) {
            OpmLog::debug(
                fmt::format(
                    fmt::runtime("Sum molality of salt components (={}) > tolerance (={}) "
                        "after conversion to electrolytes, and will be ignored in the "
                        "brine property calculations!"),
                    static_cast<double>(scalarValue(molalSalinity.sum())),
                    static_cast<double>(molalTolerance))
                );
        }

        return electrolytes;
    }

    /*!
     * @brief Calculate electrolyte from cation and anion, and adjust molality of cation and
     * anion after generating the electrolyte
     *
     * The electrolyte is made from the dissociation (or chemical equilibrium) equation
     * A_pB_q = pA+ + qB-. Hence, to make 1 molal of A_pB_q (electrolyte), p molal of A+ (cation)
     * and q molal of B- (anion) are needed. In this function we make as many molal electrolyte
     * as the maximum amount of cations or anions allow, based on the dissociation equation.
     *
     * @param cationIdx Salt index of cation
     * @param cation Molality of cation [mol cation/kg water]
     * @param anionIdx Salt index of anion
     * @param anion Molality of anion [mol anion/kg water]
     * @return Electrolyte and its molality [mol electrolyte/kg water]
     */
    template <class Evaluation>
    static std::pair<Electrolyte, Evaluation>
    electrolyteAndRemainingMolal_(const SaltIndex cationIdx,
                                  Evaluation& cation,
                                  const SaltIndex anionIdx,
                                  Evaluation& anion)
    {
        // NOTE: All comments describe unit molal conversion between electrolyte and ions
        Evaluation electrolyteMolal;
        std::pair<Electrolyte, Evaluation> electrolyte;
        if (cationIdx == SaltIndex::MG && anionIdx == SaltIndex::SO4) {
            // 1m Mg+ + 1m SO4-- = 1m MgSO4
            electrolyteMolal = min(cation, anion);
            cation -= electrolyteMolal;
            anion -= electrolyteMolal;
            electrolyte = {Electrolyte::MgSO4, electrolyteMolal};
        } else if (cationIdx == SaltIndex::NA && anionIdx == SaltIndex::SO4) {
            // 2m Na+ + 1m SO4-- = 1m Na2SO4
            electrolyteMolal = min(cation / 2.0, anion);
            cation -= electrolyteMolal * 2.0;
            anion -= electrolyteMolal;
            electrolyte = {Electrolyte::Na2SO4, electrolyteMolal};
        } else if (cationIdx == SaltIndex::K && anionIdx == SaltIndex::SO4) {
            // 2m K+ + 1m SO4-- = 1m K2SO4
            electrolyteMolal = min(cation / 2.0, anion);
            cation -= electrolyteMolal * 2.0;
            anion -= electrolyteMolal;
            electrolyte = {Electrolyte::K2SO4, electrolyteMolal};
        } else if (cationIdx == SaltIndex::MG && anionIdx == SaltIndex::CL) {
            // 1m Mg+ + 2m Cl- = 1m MgCl2
            electrolyteMolal = min(cation, anion / 2.0);
            cation -= electrolyteMolal;
            anion -= electrolyteMolal * 2.0;
            electrolyte = {Electrolyte::MgCl2, electrolyteMolal};
        } else if (cationIdx == SaltIndex::CA && anionIdx == SaltIndex::CL) {
            // 1m Ca+ + 2m Cl- = 1m CaCl2
            electrolyteMolal = min(cation, anion / 2.0);
            cation -= electrolyteMolal;
            anion -= electrolyteMolal * 2.0;
            electrolyte = {Electrolyte::CaCl2, electrolyteMolal};
        } else if (cationIdx == SaltIndex::NA && anionIdx == SaltIndex::CL) {
            // 1m Na+ + 1m Cl- = 1m NaCl
            electrolyteMolal = min(cation, anion);
            cation -= electrolyteMolal;
            anion -= electrolyteMolal;
            electrolyte = {Electrolyte::NaCl, electrolyteMolal};
        } else if (cationIdx == SaltIndex::K && anionIdx == SaltIndex::CL) {
            // 1m K+ + 1m Cl- = 1m KCl
            electrolyteMolal = min(cation, anion);
            cation -= electrolyteMolal;
            anion -= electrolyteMolal;
            electrolyte = {Electrolyte::KCl, electrolyteMolal};
        } else {
            throw std::runtime_error("Unknown cation and/or anion SaltIndex!");
        }

        return electrolyte;
    }

    /*!
     * Convert electrolyte molality to mass fraction
     *
     * @param electrolyte Vector of electrolytes and their molality
     */
    template <class Evaluation>
    static void
    molalToMassFrac_(std::vector<std::pair<Electrolyte, Evaluation> >& electrolyte)
    {
        Scalar sum = 1.0;
        for (auto& salt : electrolyte) {
            auto mMsalt = molarMass(salt.first);
            salt.second *= mMsalt;
            sum += decay<Scalar>(salt.second);
        }
        for (auto& salt : electrolyte) {
            salt.second /= sum;
        }
    }
};

} // namespace Opm

#endif // OPM_SALTELECTROLYTES_HPP
