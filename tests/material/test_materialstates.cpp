// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2025 NORCE AS

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
#include "config.h"

#define BOOST_TEST_MODULE MaterialStates
#include <boost/test/unit_test.hpp>

#include <boost/mpl/list.hpp>

#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/materialstates/MaterialStateTPSA.hpp>

#include <utility>


// Scalar and evaluation types to check
using EvalTypes = boost::mpl::list<float,
                                   double,
                                   Opm::DenseAd::Evaluation<float, 7>,
                                   Opm::DenseAd::Evaluation<double, 7>>;

BOOST_AUTO_TEST_CASE_TEMPLATE(TpsaMaterialState, Eval, EvalTypes)
{
    using MaterialState = Opm::MaterialStateTPSA<Eval>;

    // Instantiate Material state
    MaterialState ms;

    // Check if copyable
    [[maybe_unused]] MaterialState tmpMs(ms);

    // Valgrind checkDefined interface
    ms.checkDefined();

    // Check methods
    while (false) {
        std::ignore = ms.displacement(/*dirIdx=*/0);
        std::ignore = ms.rotation(/*dirIdx=*/0);
        std::ignore = ms.solidPressure();
    }
}
