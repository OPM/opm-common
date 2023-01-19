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
 */

#include <config.h>

#define BOOST_TEST_MODULE EVALUATION_FORMAT_TESTS
#include <boost/test/unit_test.hpp>

#include <opm/material/densead/EvaluationFormat.hpp>

BOOST_AUTO_TEST_CASE(TestEvaluation2)
{
    Opm::DenseAd::Evaluation<double,2> ev2;
    ev2.setValue(1.0);
    ev2.setDerivative(0, 2.0);
    ev2.setDerivative(1, 3.0);

    const std::string s2 = fmt::format("{}", ev2);
    const std::string f2 = fmt::format("{:.2f}", ev2);
    const std::string e2 = fmt::format("{:.1e}", ev2);

    BOOST_CHECK_EQUAL(s2, "v: 1 / d: [2, 3]");
    BOOST_CHECK_EQUAL(f2, "v: 1.00 / d: [2.00, 3.00]");
    BOOST_CHECK_EQUAL(e2, "v: 1.0e+00 / d: [2.0e+00, 3.0e+00]");
}

BOOST_AUTO_TEST_CASE(TestEvaluation3)
{
    Opm::DenseAd::Evaluation<double,3> ev3;
    ev3.setValue(1.0);
    ev3.setDerivative(0, 2.0);
    ev3.setDerivative(1, 3.0);
    ev3.setDerivative(2, 4.0);

    const std::string s3 = fmt::format("{}", ev3);
    const std::string f3 = fmt::format("{:.2f}", ev3);
    const std::string e3 = fmt::format("{:.1e}", ev3);

    BOOST_CHECK_EQUAL(s3, "v: 1 / d: [2, 3, 4]");
    BOOST_CHECK_EQUAL(f3, "v: 1.00 / d: [2.00, 3.00, 4.00]");
    BOOST_CHECK_EQUAL(e3, "v: 1.0e+00 / d: [2.0e+00, 3.0e+00, 4.0e+00]");
}
