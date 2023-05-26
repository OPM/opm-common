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
 * \brief This is a program to test the polynomial spline interpolation.
 *
 * It just prints some function to stdout. You can look at the result
 * using the following commands:
 *
----------- snip -----------
./test_spline > spline.csv
gnuplot
gnuplot> plot "spline.csv" using 1:2 w l ti "Curve", \
              "spline.csv" using 1:3 w l ti "Derivative", \
              "spline.csv" using 1:4 w p ti "Monotonical"
----------- snap -----------
*/
#include "config.h"

#define BOOST_TEST_MODULE Spline
#include <boost/test/unit_test.hpp>

#include <opm/material/common/Spline.hpp>

#include <array>
#include <iostream>

template <class Spline, class Array>
void testCommon(const Spline& sp,
                const Array& x,
                const Array& y)
{
    static double eps = 1e-10;
    static double epsFD = 1e-7;
    size_t n = sp.numSamples();
    for (size_t i = 0; i < n; ++i) {
        // sure that we hit all sampling points
        double y0 = (i>0)?sp.eval(x[i]-eps):y[0];
        double y1 = sp.eval(x[i]);
        double y2 = (i<n-1)?sp.eval(x[i]+eps):y[n-1];
        BOOST_CHECK_MESSAGE(std::abs(y0 - y[i]) <= 100*eps && std::abs(y2 - y[i]) <= 100*eps,
                            "Spline seems to be discontinuous at sampling point " << i);
        BOOST_CHECK_MESSAGE(std::abs(y1 - y[i]) <= eps,
                            "Spline does not capture sampling point " << i);
        // make sure the derivative is continuous (assuming that the
        // second derivative is smaller than 1000)
        double d1 = sp.evalDerivative(x[i]);
        double d0 = (i>0)?sp.evalDerivative(x[i]-eps):d1;
        double d2 = (i<n-1)?sp.evalDerivative(x[i]+eps):d1;
        BOOST_CHECK_MESSAGE(std::abs(d1 - d0) <= 1000*eps && std::abs(d2 - d0) <= 1000*eps,
                            "Spline seems to exhibit a discontinuous derivative at sampling point " << i);
    }
    // make sure the derivatives are consistent with the curve
    size_t np = 3*n;
    for (size_t i = 0; i < np; ++i) {
        double xMin = sp.xAt(0);
        double xMax = sp.xAt(sp.numSamples() - 1);
        double xval = xMin + (xMax - xMin)*i/np;
        // first derivative
        double y1 = sp.eval(xval+epsFD);
        double y0 = sp.eval(xval);
        double mFD = (y1 - y0)/epsFD;
        double m = sp.evalDerivative(xval);
        BOOST_CHECK_MESSAGE(std::abs(mFD - m) <= 1000*epsFD,
                            "Derivative of spline seems to be inconsistent with curve"
                            " (" << mFD << " - " << m << " = " << mFD - m);
        // second derivative
        y1 = sp.evalDerivative(xval+epsFD);
        y0 = sp.evalDerivative(xval);
        mFD = (y1 - y0)/epsFD;
        m = sp.evalSecondDerivative(xval);
        BOOST_CHECK_MESSAGE(std::abs(mFD - m) <= 1000*epsFD,
                            "Second derivative of spline seems to be inconsistent with curve"
                            " (" << mFD << " - " << m << " = " << mFD - m);
        // Third derivative
        y1 = sp.evalSecondDerivative(xval+epsFD);
        y0 = sp.evalSecondDerivative(xval);
        mFD = (y1 - y0)/epsFD;
        m = sp.evalThirdDerivative(xval);
        BOOST_CHECK_MESSAGE(std::abs(mFD - m) <= 1000*epsFD,
                            "Third derivative of spline seems to be inconsistent with curve"
                            " (" << mFD << " - " << m << " = " << mFD - m);
    }
}

template <class Spline, class Array>
void testFull(const Spline& sp,
              const Array& x,
              const Array& y,
              double m0,
              double m1)
{
    // test the common properties of splines
    testCommon(sp, x, y);
    static double eps = 1e-5;
    size_t n = sp.numSamples();
    // make sure the derivative at both end points is correct
    double d0 = sp.evalDerivative(x[0]);
    double d1 = sp.evalDerivative(x[n-1]);
    BOOST_CHECK_MESSAGE(std::abs(d0 - m0) <= eps,
                        "Invalid derivative at beginning of interval: is " << d0 <<
                            " ought to be " << m0);
    BOOST_CHECK_MESSAGE(std::abs(d1 - m1) <= eps,
                        "Invalid derivative at end of interval: is " << d1 <<
                        " ought to be " << m1);
}

template <class Spline, class Array>
void testNatural(const Spline& sp,
                 const Array& x,
                 const Array& y)
{
    // test the common properties of splines
    testCommon(sp, x, y);
    static double eps = 1e-5;
    size_t n = sp.numSamples();
    // make sure the second derivatives at both end points are 0
    double d0 = sp.evalDerivative(x[0]);
    double d1 = sp.evalDerivative(x[0] + eps);
    double d2 = sp.evalDerivative(x[n-1] - eps);
    double d3 = sp.evalDerivative(x[n-1]);
    BOOST_CHECK_MESSAGE(std::abs(d1 - d0)/eps <= 1000*eps,
                        "Invalid second derivative at beginning of interval: is " <<
                        (d1 - d0)/eps << " ought to be 0");
    BOOST_CHECK_MESSAGE(std::abs(d3 - d2)/eps <= 1000*eps,
                        "Invalid second derivative at end of interval: is " <<
                        (d3 - d2)/eps << " ought to be 0");
}

template <class Spline, class Array>
void testMonotonic(const Spline& sp,
                   const Array& x,
                   const Array& y)
{
    // test the common properties of splines
    testCommon(sp, x, y);
    size_t n = sp.numSamples();
    for (size_t i = 0; i < n - 1; ++ i) {
        // make sure that the spline is monotonic for each interval
        // between sampling points
        BOOST_CHECK_MESSAGE(sp.monotonic(x[i], x[i + 1]),
                            "Spline says it is not monotonic in interval " <<
                            i << " where it should be");
        // test the intersection methods
        double d = (y[i] + y[i+1])/2;
        double interX = sp.template intersectInterval<double>(x[i], x[i+1],
                                                              /*a=*/0, /*b=*/0, /*c=*/0, d);
        double interY = sp.eval(interX);
        BOOST_CHECK_MESSAGE(std::abs(interY - d) <= 1e-5,
                            "Spline::intersectInterval() seems to be broken: " <<
                            sp.eval(interX) << " - " << d << " = " << sp.eval(interX) - d);
    }
    // make sure the spline says to be monotonic on the (extrapolated)
    // left and right sides
    BOOST_CHECK_MESSAGE(sp.monotonic(x[0] - 1.0, (x[0] + x[1])/2, /*extrapolate=*/true),
                       "Spline says it is not monotonic on left side where it should be");
    BOOST_CHECK_MESSAGE(sp.monotonic((x[n - 2]+ x[n - 1])/2, x[n-1] + 1.0, /*extrapolate=*/true),
                       "Spline says it is not monotonic on right side where it should be");
    for (size_t i = 0; i < n - 2; ++ i) {
        // make sure that the spline says that it is non-monotonic for
        // if extrema are within the queried interval
        BOOST_CHECK_MESSAGE(!sp.monotonic((x[i] + x[i + 1])/2, (x[i + 1] + x[i + 2])/2),
                            "Spline says it is monotonic in interval " <<
                            i << " where it should not be");
    }
}

struct Fixture {
    Fixture()
    {
        for (int i = 0; i < 5; ++i) {
            xVec.push_back(x[i]);
            yVec.push_back(y[i]);
            pointVec.push_back(points[i]);
        }
    }

    std::array<double,5> x{0.0, 5.0, 7.5, 8.75, 9.375};
    std::array<double,5> y{10.0, 0.0, 10.0, 0.0, 10.0};
    double points[5][2] =
        {
            {x[0], y[0]},
            {x[1], y[1]},
            {x[2], y[2]},
            {x[3], y[3]},
            {x[4], y[4]},
        };

    std::vector<double> xVec;
    std::vector<double> yVec;
    static constexpr double m0 = 10;
    static constexpr double m1 = -10;
    std::vector<double*> pointVec;
};

BOOST_FIXTURE_TEST_SUITE(Generic, Fixture)

BOOST_AUTO_TEST_CASE(TwoPointSeparate)
{
    Opm::Spline<double> sp(x[0], x[1], y[0], y[1], m0, m1);
    sp.set(x[0],x[1],y[0],y[1], m0, m1);
    testFull(sp, x, y, m0, m1);
}

BOOST_AUTO_TEST_CASE(TwoPointArray)
{
    Opm::Spline<double> sp(2, x.data(), y.data(), m0, m1);
    sp.setXYArrays(2, x.data(), y.data(), m0, m1);
    testFull(sp, x, y, m0, m1);
}

BOOST_AUTO_TEST_CASE(TwoPoint2DArray)
{
    Opm::Spline<double> sp(static_cast<size_t>(2), points, m0, m1);
    sp.setArrayOfPoints(2, points, m0, m1);
    testFull(sp, x, y, m0, m1);
}

BOOST_AUTO_TEST_CASE(FullSplineArray)
{
    Opm::Spline<double> sp(5, x.data(), y.data(), m0, m1);
    sp.setXYArrays(5, x.data(), y.data(), m0, m1);
    testFull(sp, x, y, m0, m1);
}

BOOST_AUTO_TEST_CASE(FullSplineVector)
{
    Opm::Spline<double> sp(xVec, yVec, m0, m1);
    sp.setXYContainers(xVec, yVec, m0, m1);
    testFull(sp, x, y, m0, m1);
}

BOOST_AUTO_TEST_CASE(FullSpline2DArray)
{
    Opm::Spline<double> sp;
    sp.setArrayOfPoints(5, points,m0, m1);
    testFull(sp, x, y, m0, m1);
}

BOOST_AUTO_TEST_CASE(FullSplinePointVector)
{
    Opm::Spline<double> sp;
    sp.setContainerOfPoints(pointVec, m0, m1);
    testFull(sp, x, y, m0, m1);
}

BOOST_AUTO_TEST_CASE(FullSplineInitList)
{
    std::initializer_list<const std::pair<double, double> > pointsInitList =
        {
            {x[0], y[0]},
            {x[1], y[1]},
            {x[2], y[2]},
            {x[3], y[3]},
            {x[4], y[4]},
        };

    Opm::Spline<double> sp;
    sp.setContainerOfTuples(pointsInitList, m0, m1);
    testFull(sp, x, y, m0, m1);
}

BOOST_AUTO_TEST_CASE(NaturalSplineArray)
{
    Opm::Spline<double> sp(5, x.data(), y.data());
    sp.setXYArrays(5, x.data(), y.data());
    testNatural(sp, x, y);
}

BOOST_AUTO_TEST_CASE(NaturalSplineVector)
{
    Opm::Spline<double> sp(xVec, yVec);
    sp.setXYContainers(xVec, yVec);
    testNatural(sp, x, y);
}

BOOST_AUTO_TEST_CASE(NaturalSpline2DArray)
{
    Opm::Spline<double> sp;
    sp.setArrayOfPoints(5, points);
    testNatural(sp, x, y);
}

BOOST_AUTO_TEST_CASE(NaturalSplinePointsVector)
{
    Opm::Spline<double> sp;
    sp.setContainerOfPoints(pointVec);
    testNatural(sp, x, y);
}

BOOST_AUTO_TEST_CASE(NaturalSplineInitList)
{
    std::initializer_list<const std::pair<double, double> > pointsInitList =
        {
            {x[0], y[0]},
            {x[1], y[1]},
            {x[2], y[2]},
            {x[3], y[3]},
            {x[4], y[4]},
        };
    Opm::Spline<double> sp;
    sp.setContainerOfTuples(pointsInitList);
    testNatural(sp, x, y);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(Monotonic)
{
    static constexpr int numSamples = 5;
    static constexpr int n = numSamples - 1;
    std::array<double, numSamples> x{0.0, 5.0, 7.5, 8.75, 10.0 };
    std::array<double, numSamples> y{10.0, 0.0, 10.0, 0.0, 10.0 };
    static constexpr double m1 = 10;
    static constexpr double m2 = -10;
    Opm::Spline<double> spFull(x, y, m1, m2);
    Opm::Spline<double> spNatural(x, y);
    Opm::Spline<double> spPeriodic(x, y, /*type=*/Opm::Spline<double>::Periodic);
    Opm::Spline<double> spMonotonic(x, y, /*type=*/Opm::Spline<double>::Monotonic);
    testMonotonic(spMonotonic, x, y);

    spFull.printCSV(x[0] - 1.00001,
                    x[n] + 1.00001,
                    1000, std::cout);
    std::cout << "\n";
    spNatural.printCSV(x[0] - 1.00001,
                       x[n] + 1.00001,
                       1000, std::cout);
    std::cout << "\n";
    spPeriodic.printCSV(x[0] - 1.00001,
                        x[n] + 1.00001,
                        1000, std::cout);
    std::cout << "\n";
    spMonotonic.printCSV(x[0] - 1.00001,
                         x[n] + 1.00001,
                         1000, std::cout);
    std::cout << "\n";
}
