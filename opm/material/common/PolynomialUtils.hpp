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
 * \brief Provides free functions to invert polynomials of degree 1, 2 and 3.
 */
#ifndef OPM_POLYNOMIAL_UTILS_HH
#define OPM_POLYNOMIAL_UTILS_HH

#include <cmath>
#include <algorithm>

#include <opm/material/common/MathToolbox.hpp>

namespace Opm {
/*!
 * \ingroup Math
 * \brief Invert a linear polynomial analytically
 *
 * The polynomial is defined as
 * \f[ p(x) = a\; x + b \f]
 *
 * This method Returns the number of solutions which are in the real
 * numbers, i.e. 1 except if the slope of the line is 0.
 *
 * \param sol Container into which the solutions are written
 * \param a The coefficient for the linear term
 * \param b The coefficient for the constant term
 */
template <class Scalar, class SolContainer>
unsigned invertLinearPolynomial(SolContainer& sol,
                                Scalar a,
                                Scalar b)
{
    if (std::abs(scalarValue(a)) < 1e-30)
        return 0;

    sol[0] = -b/a;
    return 1;
}

/*!
 * \ingroup Math
 * \brief Invert a quadratic polynomial analytically
 *
 * The polynomial is defined as
 * \f[ p(x) = a\; x^2 + + b\;x + c \f]
 *
 * This method teturns the number of solutions which are in the real
 * numbers. The "sol" argument contains the real roots of the parabola
 * in order with the smallest root first.
 *
 * \param sol Container into which the solutions are written
 * \param a The coefficient for the quadratic term
 * \param b The coefficient for the linear term
 * \param c The coefficient for the constant term
 */
template <class Scalar, class SolContainer>
unsigned invertQuadraticPolynomial(SolContainer& sol,
                                   Scalar a,
                                   Scalar b,
                                   Scalar c)
{
    // check for a line
    if (std::abs(scalarValue(a)) < 1e-30)
        return invertLinearPolynomial(sol, b, c);

    // discriminant
    Scalar Delta = b*b - 4*a*c;
    if (Delta < 0)
        return 0; // no real roots

    Delta = sqrt(Delta);
    sol[0] = (- b + Delta)/(2*a);
    sol[1] = (- b - Delta)/(2*a);

    // sort the result
    if (sol[0] > sol[1])
        std::swap(sol[0], sol[1]);
    return 2; // two real roots
}

//! \cond SKIP_THIS
template <class Scalar, class SolContainer>
void invertCubicPolynomialPostProcess_(SolContainer& sol,
                                       int numSol,
                                       Scalar a,
                                       Scalar b,
                                       Scalar c,
                                       Scalar d)
{
    // do one Newton iteration on the analytic solution if the
    // precision is increased
    for (int i = 0; i < numSol; ++i) {
        Scalar x = sol[i];
        Scalar fOld = d + x*(c + x*(b + x*a));

        Scalar fPrime = c + x*(2*b + x*3*a);
        if (std::abs(scalarValue(fPrime)) < 1e-30)
            continue;
        x -= fOld/fPrime;

        Scalar fNew = d + x*(c + x*(b + x*a));
        if (std::abs(scalarValue(fNew)) < std::abs(scalarValue(fOld)))
            sol[i] = x;
    }
}
//! \endcond

/*!
 * \ingroup Math
 * \brief Invert a cubic polynomial analytically
 *
 * The polynomial is defined as
 * \f[ p(x) = a\; x^3 + + b\;x^3 + c\;x + d \f]
 *
 * This method teturns the number of solutions which are in the real
 * numbers. The "sol" argument contains the real roots of the cubic
 * polynomial in order with the smallest root first.
 *
 * \param sol Container into which the solutions are written
 * \param a The coefficient for the cubic term
 * \param b The coefficient for the quadratic term
 * \param c The coefficient for the linear term
 * \param d The coefficient for the constant term
 */
template <class Scalar, class SolContainer>
unsigned invertCubicPolynomial(SolContainer* sol,
                               Scalar a,
                               Scalar b,
                               Scalar c,
                               Scalar d)
{
    // reduces to a quadratic polynomial
    if (std::abs(scalarValue(a)) < 1e-30)
        return invertQuadraticPolynomial(sol, b, c, d);

    // normalize the polynomial
    b /= a;
    c /= a;
    d /= a;
    a = 1;

    // get rid of the quadratic term by subsituting x = t - b/3
    Scalar p = c - b*b/3;
    Scalar q = d + (2*b*b*b - 9*b*c)/27;

    if (std::abs(scalarValue(p)) > 1e-30 && std::abs(scalarValue(q)) > 1e-30) {
        // At this point
        //
        // t^3 + p*t + q = 0
        //
        // with p != 0 and q != 0 holds. Introducing the variables u and v
        // with the properties
        //
        //   u + v = t       and       3*u*v + p = 0
        //
        // leads to
        //
        // u^3 + v^3 + q = 0 .
        //
        // multiplying both sides with u^3 and taking advantage of the
        // fact that u*v = -p/3 leads to
        //
        // u^6 + q*u^3 - p^3/27 = 0
        //
        // Now, substituting u^3 = w yields
        //
        // w^2 + q*w - p^3/27 = 0
        //
        // This is a quadratic equation with the solutions
        //
        // w = -q/2 + sqrt(q^2/4 + p^3/27) and
        // w = -q/2 - sqrt(q^2/4 + p^3/27)
        //
        // Since w is equivalent to u^3 it is sufficient to only look at
        // one of the two cases. Then, there are still 2 cases: positive
        // and negative discriminant.
        Scalar wDisc = q*q/4 + p*p*p/27;
        if (wDisc >= 0) { // the positive discriminant case:
            // calculate the cube root of - q/2 + sqrt(q^2/4 + p^3/27)
            Scalar u = - q/2 + sqrt(wDisc);
            if (u < 0) u = - pow(-u, 1.0/3);
            else u = pow(u, 1.0/3);

            // at this point, u != 0 since p^3 = 0 is necessary in order
            // for u = 0 to hold, so
            sol[0] = u - p/(3*u) - b/3;
            // does not produce a division by zero. the remaining two
            // roots of u are rotated by +- 2/3*pi in the complex plane
            // and thus not considered here
            invertCubicPolynomialPostProcess_(sol, 1, a, b, c, d);
            return 1;
        }
        else { // the negative discriminant case:
            Scalar uCubedRe = - q/2;
            Scalar uCubedIm = sqrt(-wDisc);
            // calculate the cube root of - q/2 + sqrt(q^2/4 + p^3/27)
            Scalar uAbs  = pow(sqrt(uCubedRe*uCubedRe + uCubedIm*uCubedIm), 1.0/3);
            Scalar phi = atan2(uCubedIm, uCubedRe)/3;

            // calculate the length and the angle of the primitive root

            // with the definitions from above it follows that
            //
            // x = u - p/(3*u) - b/3
            //
            // where x and u are complex numbers. Rewritten in polar form
            // this is equivalent to
            //
            // x = |u|*e^(i*phi) - p*e^(-i*phi)/(3*|u|) - b/3 .
            //
            // Factoring out the e^ terms and subtracting the additional
            // terms, yields
            //
            // x = (e^(i*phi) + e^(-i*phi))*(|u| - p/(3*|u|)) - y - b/3
            //
            // with
            //
            // y = - |u|*e^(-i*phi) + p*e^(i*phi)/(3*|u|) .
            //
            // The crucial observation is the fact that y is the conjugate
            // of - x + b/3. This means that after taking advantage of the
            // relation
            //
            // e^(i*phi) + e^(-i*phi) = 2*cos(phi)
            //
            // the equation
            //
            // x = 2*cos(phi)*(|u| - p / (3*|u|)) - conj(x) - 2*b/3
            //
            // holds. Since |u|, p, b and cos(phi) are real numbers, it
            // follows that Im(x) = - Im(x) and thus Im(x) = 0. This
            // implies
            //
            // Re(x) = x = cos(phi)*(|u| - p / (3*|u|)) - b/3 .
            //
            // Considering the fact that u is a cubic root, we have three
            // values for phi which differ by 2/3*pi. This allows to
            // calculate the three real roots of the polynomial:
            for (int i = 0; i < 3; ++i) {
                sol[i] = cos(phi)*(uAbs - p/(3*uAbs)) - b/3;
                phi += 2*M_PI/3;
            }

            // post process the obtained solution to increase numerical
            // precision
            invertCubicPolynomialPostProcess_(sol, 3, a, b, c, d);

            // sort the result
            std::sort(sol, sol + 3);

            return 3;
        }
    }
    // Handle some (pretty unlikely) special cases required to avoid
    // divisions by zero in the code above...
    else if (std::abs(scalarValue(p)) < 1e-30 && std::abs(scalarValue(q)) < 1e-30) {
        // t^3 = 0, i.e. triple root at t = 0
        sol[0] = sol[1] = sol[2] = 0.0 - b/3;
        return 3;
    }
    else if (std::abs(scalarValue(p)) < 1e-30 && std::abs(scalarValue(q)) > 1e-30) {
        // t^3 + q = 0,
        //
        // i. e. single real root at t=curt(q)
        Scalar t;
        if (-q > 0) t = pow(-q, 1./3);
        else t = - pow(q, 1./3);
        sol[0] = t - b/3;

        return 1;
    }

    assert(std::abs(scalarValue(p)) > 1e-30 && std::abs(scalarValue(q)) > 1e-30);

    // t^3 + p*t = 0 = t*(t^2 + p),
    //
    // i. e. roots at t = 0, t^2 + p = 0
    if (p > 0) {
        sol[0] = 0.0 - b/3;
        return 1; // only a single real root at t=0
    }

    // two additional real roots at t = sqrt(-p) and t = -sqrt(-p)
    sol[0] = -sqrt(-p) - b/3;;
    sol[1] = 0.0 - b/3;
    sol[2] = sqrt(-p) - b/3;

    return 3;
}

/*!
 * \ingroup Math
 * \brief Invert a cubic polynomial analytically
 *
 * The polynomial is defined as
 * \f[ p(x) = a\; x^3 + + b\;x^3 + c\;x + d \f]
 *
 * This method returns the number of solutions which are in the real
 * numbers. The "sol" argument contains the real roots of the cubic
 * polynomial in order with the smallest root first.
 *
 * \param sol Container into which the solutions are written
 * \param a The coefficient for the cubic term
 * \param b The coefficient for the quadratic term
 * \param c The coefficient for the linear term
 * \param d The coefficient for the constant term
 */
template <class Scalar, class SolContainer>
unsigned cubicRoots(SolContainer* sol,
                             Scalar a,
                             Scalar b,
                             Scalar c,
                             Scalar d)
{
    // reduces to a quadratic polynomial
    if (std::abs(scalarValue(a)) < 1e-30)
        return invertQuadraticPolynomial(sol, b, c, d);

    // We need to reduce the cubic equation to its "depressed cubic" form (however strange that sounds)
    // Depressed cubic form: t^3 + p*t + q, where x = t - b/3*a is the transform we use when we have
    // roots for t. p and q are defined below.
    // Formula for p and q:
    Scalar p = (3.0 * a * c - b * b) / (3.0 * a * a);
    Scalar q = (2.0 * b * b * b - 9.0 * a * b * c + 27.0 * d * a * a) / (27.0 * a * a * a);

    // Check if we have three or one real root by looking at the discriminant, and solve accordingly with
    // correct formula
    Scalar discr = 4.0 * p * p * p + 27.0 * q * q;
    if (discr < 0.0) {
        // Find three real roots of a depressed cubic, using the trigonometric method
        // Help calculation
        Scalar theta = (1.0 / 3.0) * acos( ((3.0 * q) / (2.0 * p)) * sqrt(-3.0 / p) );

        // Calculate the three roots
        sol[0] = 2.0 * sqrt(-p / 3.0) * cos( theta ) - b / (3.0 * a);
        sol[1] = 2.0 * sqrt(-p / 3.0) * cos( theta - ((2.0 * M_PI) / 3.0) ) - b / (3.0 * a);
        sol[2] = 2.0 * sqrt(-p / 3.0) * cos( theta - ((4.0 * M_PI) / 3.0) ) - b / (3.0 * a);

        // Sort in ascending order
        std::sort(sol, sol + 3);

        // Return confirmation of three roots

        return 3;
    }
    else if (discr > 0.0) {

        // Find one real root of a depressed cubic using hyperbolic method. Different solutions depending on
        // sign of p
        Scalar t = 0;
        if (p < 0) {
            // Help calculation
            Scalar theta = (1.0 / 3.0) * acosh( ((-3.0 * abs(q)) / (2.0 * p)) * sqrt(-3.0 / p) );

            // Root
            t = ( (-2.0 * abs(q)) / q ) * sqrt(-p / 3.0) * cosh(theta);
        }
        else if (p > 0) {
            // Help calculation
            Scalar theta = (1.0 / 3.0) * asinh( ((3.0 * q) / (2.0 * p)) * sqrt(3.0 / p) );
            // Root
            t = -2.0 * sqrt(p / 3.0) * sinh(theta);

        }
        else {
            throw std::runtime_error(" p = 0 in cubic root solver!");
        }

        // Transform t to output solution
        sol[0] = t - b / (3.0 * a);
        return 1;

    }
    else {
        // The discriminant, 4*p^3 + 27*q^2 = 0, thus we have simple (real) roots
        // If p = 0 then also q = 0, and t = 0 is a triple root
        if (p == 0) {
            sol[0] = sol[1] = sol[2] = 0.0 - b / (3.0 * a);
        }
        // If p != 0, the we have a simple root and a double root
        else {
            sol[0] = (3.0 * q / p) - b / (3.0 * a);
            sol[1] = sol[2] = (-3.0 * q) / (2.0 * p) - b / (3.0 * a);
            std::sort(sol, sol + 3);
        }
        return 3;
    }
}
} // end Opm

#endif
