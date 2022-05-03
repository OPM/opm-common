// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
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

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*!
 * \file
 * \copydoc Opm::LETCurvesParams
 */
#ifndef OPM_TWO_PHASE_LET_CURVES_PARAMS_HPP
#define OPM_TWO_PHASE_LET_CURVES_PARAMS_HPP

#include <opm/material/common/Valgrind.hpp>
#include <opm/material/common/EnsureFinalized.hpp>

#include <cassert>

namespace Opm {

/*!
 * \ingroup FluidMatrixInteractions
 *
 * \brief Specification of the material parameters for the
 *        LET constitutive relations.
 *
 *\see TwoPhaseLETCurves
 */
template <class TraitsT>
class TwoPhaseLETCurvesParams : public EnsureFinalized
{
    typedef typename TraitsT::Scalar Scalar;
public:

    typedef TraitsT Traits;

    static const int wIdx = 0; //wetting phase index for two phase let
    static const int nwIdx = 1; //non-wetting phase index for two phase let

    TwoPhaseLETCurvesParams()
    {
        Valgrind::SetUndefined(*this);
    }

    virtual ~TwoPhaseLETCurvesParams() {}


    /*!
     * \brief Calculate all dependent quantities once the independent
     *        quantities of the parameter object have been set.
     */
    void finalize()
    {
        EnsureFinalized :: finalize ();

        // printLETCoeffs();
    }

    /*!
     * \brief Returns the Smin_ parameter
     */
    Scalar Smin(const unsigned phaseIdx) const
    { EnsureFinalized::check(); if (phaseIdx<Traits::numPhases) return Smin_[phaseIdx]; return 0.0;}

    /*!
     * \brief Returns the dS_ parameter
     */
    Scalar dS(const unsigned phaseIdx) const
    { EnsureFinalized::check(); if (phaseIdx<Traits::numPhases) return dS_[phaseIdx]; return 0.0;}

    /*!
     * \brief Returns the Epc_ parameter
     */
    Scalar Sminpc() const
    { EnsureFinalized::check(); return Sminpc_; }

    /*!
     * \brief Returns the Epc_ parameter
     */
    Scalar dSpc() const
    { EnsureFinalized::check(); return dSpc_; }

    /*!
     * \brief Returns the L_ parameter
     */
    Scalar L(const unsigned phaseIdx) const
    { EnsureFinalized::check(); if (phaseIdx<Traits::numPhases) return L_[phaseIdx]; return 0.0;}

    /*!
     * \brief Returns the E_ parameter
     */
    Scalar E(const unsigned phaseIdx) const
    { EnsureFinalized::check(); if (phaseIdx<Traits::numPhases) return E_[phaseIdx]; return 0.0;}

    /*!
     * \brief Returns the T_ parameter
     */
    Scalar T(const unsigned phaseIdx) const
    { EnsureFinalized::check(); if (phaseIdx<Traits::numPhases) return T_[phaseIdx]; return 0.0;}

    /*!
     * \brief Returns the Krt_ parameter
     */
    Scalar Krt(const unsigned phaseIdx) const
    { EnsureFinalized::check(); if (phaseIdx<Traits::numPhases) return Krt_[phaseIdx]; return 0.0;}

    /*!
     * \brief Returns the Lpc_ parameter
     */
    Scalar Lpc() const
    { EnsureFinalized::check(); return Lpc_; }

    /*!
     * \brief Returns the Epc_ parameter
     */
    Scalar Epc() const
    { EnsureFinalized::check(); return Epc_; }

    /*!
     * \brief Returns the Tpc_ parameter
     */
    Scalar Tpc() const
    { EnsureFinalized::check(); return Tpc_; }

    /*!
     * \brief Returns the Pcir_ parameter
     */
    Scalar Pcir() const
    { EnsureFinalized::check(); return Pcir_; }

    /*!
     * \brief Returns the Pct_ parameter
     */
    Scalar Pct() const
    { EnsureFinalized::check(); return Pct_; }

    /*!
     * \brief Set the LET-related parameters for the relative
     *        permeability curve of the wetting phase.
     *        Dummy argument to align interface with class
     *        PiecewiseLinearTwoPhaseMaterialParams.
     */
    template <class Container>
    void setKrwSamples(const Container& letProp, const Container& )
    {
        setLETCoeffs(wIdx, letProp[2], letProp[3], letProp[4], letProp[5]);
        //Smin_[wIdx] = letProp[1];
        //dS_[nwIdx] = 1.0 - letProp[0];
        Smin_[wIdx] = letProp[0];
        dS_[wIdx] = letProp[1] - letProp[0];
    }

    /*!
     * \brief Set the LET-related parameters for the relative
     *        permeability curve of the non-wetting phase.
     *        Dummy argument to align interface with class
     *        PiecewiseLinearTwoPhaseMaterialParams.
     */
    template <class Container>
    void setKrnSamples(const Container& letProp, const Container& )
    {
        setLETCoeffs(nwIdx, letProp[2], letProp[3], letProp[4], letProp[5]);
        //Smin_[nwIdx] = letProp[1];
        //dS_[wIdx] = 1.0 - letProp[0];
        Smin_[nwIdx] = letProp[0];
        dS_[nwIdx] = letProp[1] - letProp[0];
    }


    /*!
     * \brief Set the LET-related parameters for the capillary
     *        pressure curve of the non-wetting phase.
     *        Dummy argument to align interface with class
     *        PiecewiseLinearTwoPhaseMaterialParams.
     */
    template <class Container>
    void setPcnwSamples(const Container& letProp, const Container& )
    {
        setLETPcCoeffs(letProp[2], letProp[3], letProp[4], letProp[5], letProp[6]);
        Sminpc_ = letProp[0];
        dSpc_ = 1.0 - letProp[0] - letProp[1];
    }

private:
    /*!
     * \brief Set the LET coefficients for phase relperm
     */

    void setLETCoeffs(unsigned phaseIdx, Scalar L, Scalar E, Scalar T, Scalar Krt)
    {
        if (phaseIdx < Traits::numPhases) {
          L_[phaseIdx]=L;
          E_[phaseIdx]=E;
          T_[phaseIdx]=T;
          Krt_[phaseIdx]=Krt;
        }
    }

    /*!
     * \brief Set the LET coefficients for cap. pressure
     */

    void setLETPcCoeffs(Scalar L, Scalar E, Scalar T, Scalar Pcir, Scalar Pct)
    {
        Lpc_=L;
        Epc_=E;
        Tpc_=T;
        Pcir_=Pcir;
        Pct_=Pct;
    }

    /*!
     * \brief Set the LET coefficients for cap. pressure
     */

    void printLETCoeffs()
    {
/*
        std::cout << "# LET parameters: "<< std::endl;
        for (int i=0; i<Traits::numPhases; ++i) {
            std::cout << "Kr[" << i;
            std::cout << "]:  Smin:" << Smin_[i];
            std::cout << " dS:" << dS_[i];
            std::cout << " L:" << L_[i];
            std::cout << " E:" << E_[i];
            std::cout << " T:" << T_[i];
            std::cout << " Krt:" << Krt_[i];
            std::cout << std::endl;
        }

        std::cout << "Pc: Smin:" << Sminpc_;
        std::cout << " dS:" << dSpc_;
        std::cout << " L:" << Lpc_;
        std::cout << " E:" << Epc_;
        std::cout << " T:" << Tpc_;
        std::cout << " Pcir:" << Pcir_;
        std::cout << " Pct:" << Pct_;
        std::cout << std::endl;

        std::cout << "================================="<< std::endl;
*/
    }

    Scalar Smin_[Traits::numPhases];
    Scalar dS_[Traits::numPhases];

    Scalar L_[Traits::numPhases];
    Scalar E_[Traits::numPhases];
    Scalar T_[Traits::numPhases];
    Scalar Krt_[Traits::numPhases];

    Scalar Sminpc_;
    Scalar dSpc_;
    Scalar Lpc_;
    Scalar Epc_;
    Scalar Tpc_;
    Scalar Pcir_;
    Scalar Pct_;
};
} // namespace Opm

#endif // OPM_TWO_PHASE_LET_CURVES_PARAMS_HPP
