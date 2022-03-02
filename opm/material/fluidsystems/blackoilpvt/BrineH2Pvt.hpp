// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
This file is part of the Open Porous Media project (OPM).

OPM is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

OPM is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OPM.  If not, see <http://www.gnu.org/licenses/>.

Consult the COPYING file in the top-level source directory of this
module for the precise wording of the license and the list of
copyright holders.
*/
/*!
* \file
* \copydoc Opm::BrineH2Pvt
*/
#ifndef OPM_BRINE_H2_PVT_HPP
#define OPM_BRINE_H2_PVT_HPP

#include <opm/material/binarycoefficients/Brine_H2.hpp>
#include <opm/material/components/SimpleHuDuanH2O.hpp>
#include <opm/material/components/Brine.hpp>
#include <opm/material/components/H2.hpp>

#include <vector>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#endif

namespace Opm {
/*!
* \brief This class represents the Pressure-Volume-Temperature relations of the liquid phase for a H2-Brine system
*/
template <class Scalar>
class BrineH2Pvt
{
    static const bool extrapolate = true;
public:
    typedef SimpleHuDuanH2O<Scalar> H2O;
    typedef ::Opm::Brine<Scalar, H2O> Brine;
    typedef ::Opm::H2<Scalar> H2;

    // The binary coefficients for brine and H2 used by this fluid system
    typedef BinaryCoeff::Brine_H2<Scalar, H2O, H2> BinaryCoeffBrineH2;

    explicit BrineH2Pvt() = default;
    BrineH2Pvt( const std::vector<Scalar>& brineReferenceDensity,
                const std::vector<Scalar>& h2ReferenceDensity,
                const std::vector<Scalar>& salinity)
        : brineReferenceDensity_(brineReferenceDensity),
          h2ReferenceDensity_(h2ReferenceDensity),
          salinity_(salinity)
    {
    }
#if HAVE_ECL_INPUT
    /*!
    * \brief Initialize the parameters for Brine-H2 system using an ECL deck.
    *
    */
    void initFromState(const EclipseState& eclState, const Schedule&)
    {
        // Error message for DENSITY keyword
        if (!eclState.getTableManager().getDensityTable().empty()) {
            std::cerr << "WARNING: H2STORE is enabled but DENSITY is in the deck. \n" <<
                         "The surface density is computed based on H2-BRINE PVT at standard conditions (STCOND)" <<
                         " and DENSITY is ignored " << std::endl;
        }
        // Error message for entering PVDO/PVTO in deck
        if (eclState.getTableManager().hasTables("PVDO") || !eclState.getTableManager().getPvtoTables().empty()) {
            std::cerr << "WARNING: H2STORE is enabled but PVDO or PVTO is in the deck. \n" <<
                         "BRINE PVT properties are computed based on the Li et al. (2018) and PVDO/PVTO input" <<
                         " is ignored. " << std::endl;
        }

        // We only supported single pvt region for the H2-brine module
        size_t numRegions = 1;
        setNumRegions(numRegions);
        size_t regionIdx = 0;

        // Currently we only support constant salinity
        const Scalar molality = eclState.getTableManager().salinity(); // mol/kg
        const Scalar MmNaCl = 58e-3; // molar mass of NaCl [kg/mol]
        Brine::salinity = 1 / ( 1 + 1 / (molality*MmNaCl)); // convert to mass fraction
        salinity_[regionIdx] = molality;  // molality used in functions

        // set the surface conditions using the STCOND keyword
        Scalar T_ref = eclState.getTableManager().stCond().temperature;
        Scalar P_ref = eclState.getTableManager().stCond().pressure;

        brineReferenceDensity_[regionIdx] = Brine::liquidDensity(T_ref, P_ref, extrapolate);
        h2ReferenceDensity_[regionIdx] = H2::gasDensity(T_ref, P_ref);
    }
#endif

    void setNumRegions(size_t numRegions)
    {
        brineReferenceDensity_.resize(numRegions);
        h2ReferenceDensity_.resize(numRegions);
        salinity_.resize(numRegions);
    }

    /*!
    * \brief Initialize the reference densities of all fluids for a given PVT region
    */
    void setReferenceDensities(unsigned regionIdx,
                               Scalar rhoRefBrine,
                               Scalar rhoRefH2,
                               Scalar /*rhoRefWater*/)
    {
        brineReferenceDensity_[regionIdx] = rhoRefBrine;
        h2ReferenceDensity_[regionIdx] = rhoRefH2;
    }

    /*!
    * \brief Finish initializing the oil phase PVT properties.
    */
    void initEnd()
    {
    }

    /*!
    * \brief Return the number of PVT regions which are considered by this PVT-object.
    */
    unsigned numRegions() const
    { return brineReferenceDensity_.size(); }

    /*!
    * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
    */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned regionIdx,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation& Rs) const
    {
        // NOT IMPLEMENTED YET!
    }

    /*!
    * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
    */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& /*Rs*/) const
    {
        //TODO: The viscosity does not yet depend on the composition
        return saturatedViscosity(regionIdx, temperature, pressure);
    }

    /*!
    * \brief Returns the dynamic viscosity [Pa s] of oil saturated gas at given pressure.
    */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned /*regionIdx*/,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure) const
    {
        return Brine::liquidViscosity(temperature, pressure);
    }

    /*!
    * \brief Returns the formation volume factor [-] of the fluid phase.
    */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& Rs) const
    {
        return density_(regionIdx, temperature, pressure, Rs)/brineReferenceDensity_[regionIdx];
    }

    /*!
    * \brief Returns the formation volume factor [-] of brine saturated with CO2 at a given pressure.
    */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure) const
    {
        Evaluation rsSat = rsSat_(regionIdx, temperature, pressure);
        return density_(regionIdx, temperature, pressure, rsSat)/brineReferenceDensity_[regionIdx];
    }

    /*!
    * \brief Returns the saturation pressure of the brine phase [Pa] depending on its mass fraction of the gas component
    *
    * \param Rs
    */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned /*regionIdx*/,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& /*Rs*/) const
    {
        throw std::runtime_error("Requested the saturation pressure for the brine-co2 pvt module. Not yet implemented.");
    }

    /*!
    * \brief Returns the gas dissoluiton factor \f$R_s\f$ [m^3/m^3] of the liquid phase.
    */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure,
                                             const Evaluation& /*oilSaturation*/,
                                             const Evaluation& /*maxOilSaturation*/) const
    {
        //TODO support VAPPARS
        return rsSat_(regionIdx, temperature, pressure);
    }

    /*!
    * \brief Returns thegas dissoluiton factor  \f$R_s\f$ [m^3/m^3] of the liquid phase.
    */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure) const
    {
        return rsSat_(regionIdx, temperature, pressure);
    }

    const Scalar oilReferenceDensity(unsigned regionIdx) const
    { return brineReferenceDensity_[regionIdx]; }

    const Scalar gasReferenceDensity(unsigned regionIdx) const
    { return h2ReferenceDensity_[regionIdx]; }

    const Scalar salinity(unsigned regionIdx) const
    { return salinity_[regionIdx]; }

    bool operator==(const BrineH2Pvt<Scalar>& data) const
    {
        return h2ReferenceDensity_ == data.h2ReferenceDensity_ &&
                brineReferenceDensity_ == data.brineReferenceDensity_;
    }

    /*!
    * \brief Diffusion coefficient of H2 in water
    */
    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned /*compIdx*/) const
    {
        // NOT IMPLEMENTED YET!
    }

private:
    std::vector<Scalar> brineReferenceDensity_;
    std::vector<Scalar> h2ReferenceDensity_;
    std::vector<Scalar> salinity_;

    /*!
    * \brief Calculate density of aqueous solution (H2O-NaCl/brine and H2).
    * 
    * \param temperature temperature [K]
    * \param pressure pressure [Pa]
    * \param Rs gas dissolution factor [-]
    */
    template <class LhsEval>
    LhsEval density_(unsigned regionIdx,
                     const LhsEval& temperature,
                     const LhsEval& pressure,
                     const LhsEval& Rs) const
    {
        // convert Rs to mole fraction (via mass fraction)
        LhsEval xlH2 = convertXoGToxoG_(convertRsToXoG_(Rs,regionIdx));

        // calculate the density of solution
        LhsEval result = liquidDensity_(temperature,
                                        pressure,
                                        xlH2);

        Valgrind::CheckDefined(result);
        return result;
    }

    /*!
    * \brief Calculated the density of the aqueous solution where contributions of salinity and dissolved H2 is taken
    * into account.
    * 
    * \param T temperature [K]
    * \param pl liquid pressure [Pa]
    * \param xlH2 mole fraction H2 [-]
    */
    template <class LhsEval>
    LhsEval liquidDensity_(const LhsEval& T,
                           const LhsEval& pl,
                           const LhsEval& xlH2) const
    {
        // check input variables
        Valgrind::CheckDefined(T);
        Valgrind::CheckDefined(pl);
        Valgrind::CheckDefined(xlH2);

        // calculate individual contribution to density
        const LhsEval& rho_brine = Brine::liquidDensity(T, pl, extrapolate);
        const LhsEval& rho_pure = H2O::liquidDensity(T, pl, extrapolate);
        const LhsEval& rho_lH2 = liquidDensityWaterH2_(T, pl, xlH2);
        const LhsEval& contribH2 = rho_lH2 - rho_pure;

        return rho_brine + contribH2;
    }

    /*!
    * \brief Density of aqueous solution with dissolved H2. Formula from Li et al. (2018) and Garica, Lawrence Berkeley
    * National Laboratory, 2001.
    * 
    * \param temperature [K]
    * \param pl liquid pressure [Pa]
    * \param xlH2 mole fraction [-]
    */
    template <class LhsEval>
    LhsEval liquidDensityWaterH2_(const LhsEval& temperature,
                                          const LhsEval& pl,
                                          const LhsEval& xlH2) const
    {
        // molar masses
        Scalar M_H2 = H2::molarMass();
        Scalar M_H2O = H2O::molarMass();

        // density of pure water
        const LhsEval& rho_pure = H2O::liquidDensity(temperature, pl, extrapolate);

        // (apparent) molar volume of H2, Eq. (14) in Li et al. (2018)
        const LhsEval& A1 = 51.1904 - 0.208062*temperature + 3.4427e-4*temperature*temperature;
        const LhsEval& A2 = -0.022;
        const LhsEval& V_phi = A1 + A2 * pl;

        // density of solution, Eq. (19) in Garcia (2001)
        const LhsEval xlH2O = 1.0 - xlH2;
        const LhsEval& M_T = M_H2O * xlH2O + M_H2 * xlH2;
        const LhsEval& rho_aq = 1 / (xlH2 * V_phi/M_T + M_H2O * xlH2O / (rho_pure * M_T));

        return rho_aq;

    }

    /*!
    * \brief Convert a gas dissolution factor to the the corresponding mass fraction of the gas component in the oil
    * phase.
    * 
    * \param Rs gass dissolution factor [-]
    * \param regionIdx region index
    */
    template <class LhsEval>
    LhsEval convertRsToXoG_(const LhsEval& Rs, unsigned regionIdx) const
    {
        Scalar rho_oRef = brineReferenceDensity_[regionIdx];
        Scalar rho_gRef = h2ReferenceDensity_[regionIdx];

        const LhsEval& rho_oG = Rs*rho_gRef;
        return rho_oG/(rho_oRef + rho_oG);
    }

    /*!
    * \brief Convert a gas mass fraction in the oil phase the corresponding mole fraction.
    *
    * \param XoG mass fraction [-]
    */
    template <class LhsEval>
    LhsEval convertXoGToxoG_(const LhsEval& XoG) const
    {
        Scalar M_H2 = H2::molarMass();
        Scalar M_Brine = Brine::molarMass();
        return XoG*M_Brine / (M_H2*(1 - XoG) + XoG*M_Brine);
    }

    /*!
    * \brief Convert the mass fraction of the gas component in the oil phase to the corresponding gas dissolution
    * factor.
    * 
    * \param XoG mass fraction [-]
    * \param regionIdx region index
    */
    template <class LhsEval>
    LhsEval convertXoGToRs(const LhsEval& XoG, unsigned regionIdx) const
    {
        Scalar rho_oRef = brineReferenceDensity_[regionIdx];
        Scalar rho_gRef = h2ReferenceDensity_[regionIdx];

        return XoG/(1.0 - XoG)*(rho_oRef/rho_gRef);
    }

    /*!
    * \brief Saturated gas dissolution factor, Rs.
    *
    * \param regionIdx region index
    * \param temperature [K]
    * \param pressure pressure [Pa]
    */
    template <class LhsEval>
    LhsEval rsSat_(unsigned regionIdx,
                   const LhsEval& temperature,
                   const LhsEval& pressure) const
    {
        // calulate the equilibrium composition for the given temperature and pressure
        LhsEval xlH2;
        BinaryCoeffBrineH2::calculateMoleFractions(temperature,
                                                   pressure,
                                                   salinity_[regionIdx],
                                                   xlH2);
        
        // normalize the phase compositions
        xlH2 = max(0.0, min(1.0, xlH2));

        return convertXoGToRs(convertxoGToXoG(xlH2), regionIdx);
    }
};  // end class BrineH2Pvt
}  // end namespace Opm
#endif
