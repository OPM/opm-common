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
 *
 * \brief This is the unit test for the 2D tabulation classes.
 *
 * I.e., for the UniformTabulated2DFunction and UniformXTabulated2DFunction classes.
 */
#include "config.h"

#define BOOST_TEST_MODULE 2DTables
#include <boost/test/unit_test.hpp>

#include <opm/material/common/UniformXTabulated2DFunction.hpp>
#include <opm/material/common/UniformTabulated2DFunction.hpp>
#include <opm/material/common/IntervalTabulated2DFunction.hpp>

#include <memory>
#include <cmath>
#include <iostream>

template <class ScalarT>
struct Test
{
    typedef ScalarT  Scalar;

    static Scalar testFn1(Scalar x, Scalar /* y */)
    { return x; }

    static Scalar testFn2(Scalar /* x */, Scalar y)
    { return y; }

    static Scalar testFn3(Scalar x, Scalar y)
    { return x*y; }

    template <class Fn>
    Opm::UniformTabulated2DFunction<Scalar>
    createUniformTabulatedFunction(Fn& f)
    {
        Scalar xMin = -2.0;
        Scalar xMax = 3.0;
        unsigned m = 50;

        Scalar yMin = -1/2.0;
        Scalar yMax = 1/3.0;
        unsigned n = 40;

        Opm::UniformTabulated2DFunction<Scalar> tab(xMin, xMax, m, yMin, yMax, n);

        for (unsigned i = 0; i < m; ++i) {
            Scalar x = xMin + Scalar(i)/(m - 1) * (xMax - xMin);
            for (unsigned j = 0; j < n; ++j) {
                Scalar y = yMin + Scalar(j)/(n - 1) * (yMax - yMin);
                tab.setSamplePoint(i, j, f(x, y));
            }
        }

        return tab;
    }

    template <class Fn>
    Opm::UniformXTabulated2DFunction<Scalar>
    createUniformXTabulatedFunction(Fn& f)
    {
        Scalar xMin = -2.0;
        Scalar xMax = 3.0;
        unsigned m = 50;

        Scalar yMin = -1/2.0;
        Scalar yMax = 1/3.0;
        unsigned n = 40;
        Opm::UniformXTabulated2DFunction<Scalar> tab(Opm::UniformXTabulated2DFunction<Scalar>::InterpolationPolicy::Vertical);
        for (unsigned i = 0; i < m; ++i) {
            Scalar x = xMin + Scalar(i)/(m - 1) * (xMax - xMin);
            tab.appendXPos(x);
            for (unsigned j = 0; j < n; ++j) {
                Scalar y = yMin + Scalar(j)/(n -1) * (yMax - yMin);
                tab.appendSamplePoint(i, y, f(x, y));
            }
        }

        return tab;
    }


    template <class Fn>
    Opm::UniformXTabulated2DFunction<Scalar>
    createUniformXTabulatedFunction2(Fn& f)
    {
        Scalar xMin = -2.0;
        Scalar xMax = 3.0;
        Scalar m = 50;

        Scalar yMin = - 4.0;
        Scalar yMax = 5.0;

        Opm::UniformXTabulated2DFunction<Scalar> tab(Opm::UniformXTabulated2DFunction<Scalar>::InterpolationPolicy::Vertical);

        for (unsigned i = 0; i < m; ++i) {
            Scalar x = xMin + Scalar(i)/(m - 1) * (xMax - xMin);
            tab.appendXPos(x);

            Scalar n = i + 10;

            for (unsigned j = 0; j < n; ++j) {
                Scalar y = yMin + Scalar(j)/(n -1) * (yMax - yMin);
                tab.appendSamplePoint(i, y, f(x, y));
            }
        }

        return tab;
    }


    template <class Fn>
    Opm::IntervalTabulated2DFunction<Scalar>
    createIntervalTabulated2DFunction(Fn& f)
    {
        const Scalar xMin = -2.0;
        const Scalar xMax = 3.0;
        const unsigned m = 50;

        const Scalar yMin = -1/2.0;
        const Scalar yMax = 1/3.0;
        const unsigned n = 40;

        std::vector<Scalar> xSamples(m);
        std::vector<Scalar> ySamples(n);
        std::vector<std::vector<Scalar>> data(m, std::vector<Scalar>(n));

        for (unsigned i = 0; i < m; ++i) {
            xSamples[i] = xMin + Scalar(i)/(m - 1) * (xMax - xMin);

            for (unsigned j = 0; j < n; ++j) {
                ySamples[j] = yMin + Scalar(j)/(n -1) * (yMax - yMin);
                data[i][j] = f(xSamples[i], ySamples[j]);
            }
        }

        return Opm::IntervalTabulated2DFunction<Scalar>(xSamples, ySamples, data, true, true);
    }

    template <class Fn, class Table>
    bool compareTableWithAnalyticFn(const Table& table,
                                    Scalar xMin,
                                    Scalar xMax,
                                    unsigned numX,

                                    Scalar yMin,
                                    Scalar yMax,
                                    unsigned numY,

                                    Fn& f,
                                    Scalar tolerance = 1e-8)
    {
        // make sure that the tabulated function evaluates to the same thing as the analytic
        // one (modulo tolerance)
        for (unsigned i = 1; i <= numX; ++i) {
            Scalar x = xMin + Scalar(i)/numX*(xMax - xMin);

            for (unsigned j = 0; j < numY; ++j) {
                Scalar y = yMin + Scalar(j)/numY*(yMax - yMin);
                BOOST_CHECK_SMALL(table.eval(x, y, false) - f(x, y), tolerance);
            }
        }

        return true;
    }

    template <class Fn, class Table>
    bool compareTableWithAnalyticFn2(const Table& table,
                                     const Scalar xMin,
                                     const Scalar xMax,
                                     unsigned numX,
                                     const Scalar yMin,
                                     const Scalar yMax,
                                     unsigned numY,
                                     Fn& f,
                                     Scalar tolerance = 1e-8)
    {
        // make sure that the tabulated function evaluates to the same thing as the analytic
        // one (modulo tolerance)
        for (unsigned i = 1; i <= numX; ++i) {
            Scalar x = xMin + Scalar(i)/numX*(xMax - xMin);

            for (unsigned j = 0; j < numY; ++j) {
                Scalar y = yMin + Scalar(j)/numY*(yMax - yMin);
                BOOST_CHECK_SMALL(table.eval(x, y) - f(x, y), tolerance);
            }
        }

        return true;
    }

    template <class UniformTable, class UniformXTable, class Fn>
    void compareTables(const UniformTable& uTable,
                       const UniformXTable& uXTable,
                       Fn& f,
                       Scalar tolerance = 1e-8)
    {
        // make sure the uniform and the non-uniform tables exhibit the same dimensions
        BOOST_CHECK_SMALL(uTable.xMin() - uXTable.xMin(), tolerance);
        BOOST_CHECK_SMALL(uTable.xMax() - uXTable.xMax(), tolerance);
        BOOST_CHECK_EQUAL(uTable.numX(), uXTable.numX());
        for (unsigned i = 0; i < uTable.numX(); ++i) {
            BOOST_CHECK_SMALL(uTable.yMin() - uXTable.yMin(i), tolerance);
            BOOST_CHECK_SMALL(uTable.yMax() - uXTable.yMax(i), tolerance);
            BOOST_CHECK_EQUAL(uTable.numY(), uXTable.numY(i));
        }

        // make sure that the x and y values are identical
        for (unsigned i = 0; i < uTable.numX(); ++i) {
            BOOST_CHECK_SMALL(uTable.iToX(i) - uXTable.iToX(i), tolerance);
            for (unsigned j = 0; j < uTable.numY(); ++j) {
                BOOST_CHECK_SMALL(uTable.jToY(j) - uXTable.jToY(i,j), tolerance);
            }
        }

        // check that the applicable range is correct. Note that due to rounding errors it is
        // undefined whether the table applies to the boundary of the tabulated domain or not
        Scalar xMin = uTable.xMin();
        Scalar yMin = uTable.yMin();
        Scalar xMax = uTable.xMax();
        Scalar yMax = uTable.yMax();

        for (int yMod = -1; yMod < 2; yMod += 2) {
            for (int xMod = -1; xMod < 2; xMod += 2) {
                Scalar x = xMin + xMod * tolerance;
                Scalar y = yMin + yMod * tolerance;
                BOOST_CHECK_MESSAGE(uTable.applies(x,y) == (xMod > 0 && yMod > 0),
                                    "uTable.applies(" << x << "," << y << ")");
                BOOST_CHECK_MESSAGE(uTable.applies(x,y) == (xMod > 0 && yMod > 0),
                                    "uXTable.applies(" << x << "," << y << ")");
            }
        }

        for (int xMod = -1; xMod < 2; xMod += 2) {
            for (int yMod = -1; yMod < 2; yMod += 2) {
                Scalar x = xMax + xMod * tolerance;
                Scalar y = yMax + yMod * tolerance;
                BOOST_CHECK_MESSAGE(uTable.applies(x,y) == (xMod < 0 && yMod < 0),
                                    "uTable.applies(" << x <<  "," <<  y << ")");
                BOOST_CHECK_MESSAGE(uXTable.applies(x,y) == (xMod < 0 && yMod < 0),
                                    "uXTable.applies(" << x << "," << y << + ")");
            }
        }

        // make sure that the function values at the sampling points are identical and that
        // they correspond to the analytic function
        unsigned m2 = uTable.numX()*5;
        unsigned n2 = uTable.numY()*5;
        compareTableWithAnalyticFn(uTable,
                                   xMin, xMax, m2,
                                   yMin, yMax, n2,
                                   f,
                                   tolerance);

        compareTableWithAnalyticFn(uXTable,
                                   xMin, xMax, m2,
                                   yMin, yMax, n2,
                                   f,
                                   tolerance);
    }
};

using Types = std::tuple<float,double>;

BOOST_AUTO_TEST_CASE_TEMPLATE(UniformTabulatedFunction1, Scalar, Types)
{
    Test<Scalar> test;
    Scalar tolerance;
    if constexpr (std::is_same_v<Scalar,float>) {
        tolerance = 1e-5;
    } else {
        tolerance = 1e-11;
    }
    auto uniformTab = test.createUniformTabulatedFunction(test.testFn1);
    auto uniformXTab = test.createUniformXTabulatedFunction(test.testFn1);
    test.compareTables(uniformTab, uniformXTab, test.testFn1, tolerance);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(UniformTabulatedFunction2, Scalar, Types)
{
    Test<Scalar> test;
    Scalar tolerance;
    if constexpr (std::is_same_v<Scalar,float>) {
        tolerance = 1e-6;
    } else {
        tolerance = 1e-12;
    }
    auto uniformTab = test.createUniformTabulatedFunction(test.testFn2);
    auto uniformXTab = test.createUniformXTabulatedFunction(test.testFn2);
    test.compareTables(uniformTab, uniformXTab, test.testFn2, tolerance);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(UniformTabulatedFunction3, Scalar, Types)
{
    Test<Scalar> test;
    auto uniformTab = test.createUniformTabulatedFunction(test.testFn3);
    auto uniformXTab = test.createUniformXTabulatedFunction(test.testFn3);
    test.compareTables(uniformTab, uniformXTab, test.testFn3, 1e-2);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(UniformXTabulatedFunction2, Scalar, Types)
{
    Test<Scalar> test;
    auto uniformXTab = test.createUniformXTabulatedFunction2(test.testFn3);
    test.compareTableWithAnalyticFn(uniformXTab,
                                    -2.0, 3.0, 100,
                                    -4.0, 5.0, 100,
                                    test.testFn3,
                                    1e-2);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(IntervalTabulatedFunction1, Scalar, Types)
{
    Test<Scalar> test;
    const Scalar xMin = -4.0;
    const Scalar xMax = 8.0;
    const unsigned m = 250;

    const Scalar yMin = -2. / 2.0;
    const Scalar yMax = 3. / 3.0;
    const unsigned n = 170;

    // extrapolation and interpolation involved, the tolerance needs to be bigger
    Scalar tolerance;
    if constexpr (std::is_same_v<Scalar,float>) {
        tolerance = 1e-3;
    } else {
        tolerance = 1e-9;
    }

    auto xytab = test.createIntervalTabulated2DFunction(test.testFn1);
    test.compareTableWithAnalyticFn2(xytab, xMin, xMax, m,
                                     yMin, yMax, n, test.testFn1, tolerance);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(IntervalTabulatedFunction2, Scalar, Types)
{
    Test<Scalar> test;
    const Scalar xMin = -4.0;
    const Scalar xMax = 8.0;
    const unsigned m = 250;

    const Scalar yMin = -2. / 2.0;
    const Scalar yMax = 3. / 3.0;
    const unsigned n = 170;

    // extrapolation and interpolation involved, the tolerance needs to be bigger
    Scalar tolerance;
    if constexpr (std::is_same_v<Scalar,float>) {
        tolerance = 1e-3;
    } else {
        tolerance = 1e-9;
    }

    auto xytab = test.createIntervalTabulated2DFunction(test.testFn2);
    test.compareTableWithAnalyticFn2(xytab, xMin, xMax, m,
                                     yMin, yMax, n, test.testFn2, tolerance);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(IntervalTabulatedFunction3, Scalar, Types)
{
    Test<Scalar> test;
    const Scalar xMin = -4.0;
    const Scalar xMax = 8.0;
    const unsigned m = 250;

    const Scalar yMin = -2. / 2.0;
    const Scalar yMax = 3. / 3.0;
    const unsigned n = 170;

    // extrapolation and interpolation involved, the tolerance needs to be bigger
    Scalar tolerance;
    if constexpr (std::is_same_v<Scalar,float>) {
        tolerance = 1e-3;
    } else {
        tolerance = 1e-9;
    }

    auto xytab = test.createIntervalTabulated2DFunction(test.testFn3);
    test.compareTableWithAnalyticFn2(xytab, xMin, xMax, m,
                                     yMin, yMax, n, test.testFn3, tolerance);
}
