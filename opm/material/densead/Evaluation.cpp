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

#include <config.h>
#include <opm/material/densead/Evaluation.hpp>

#if HAVE_QUAD
#include <opm/material/common/quad.hpp>
#endif

#include <ostream>
#include <opm/common/utility/gpuDecorators.hpp>

namespace Opm {
namespace DenseAd {

template <class ValueT, int numDerivs, unsigned staticSize>
OPM_HOST_DEVICE void printEvaluation(std::ostream& os,
                     const Evaluation<ValueT,numDerivs,staticSize>& eval,
                     bool withDer)
{
  // print value
  os << "v: " << eval.value();

  if (withDer) {
      os << " / d:";

      // print derivatives
      for (int varIdx = 0; varIdx < eval.size(); ++varIdx) {
        os << " " << eval.derivative(varIdx);
      }
  }
}

#define INSTANCE_IMPL(T,size1,size2) \
    template void printEvaluation(std::ostream&, \
                                  const Evaluation<T,size1,size2>&, \
                                  bool);

#if HAVE_QUAD
#define INSTANCE(size) \
    INSTANCE_IMPL(double,size,0u) \
    INSTANCE_IMPL(quad,size,0u) \
    INSTANCE_IMPL(float,size,0u)
#else
#define INSTANCE(size) \
    INSTANCE_IMPL(double,size,0u) \
    INSTANCE_IMPL(float,size,0u)
#endif

#if HAVE_QUAD
#define INSTANCE_DYN(size) \
    INSTANCE_IMPL(double,-1,size) \
    INSTANCE_IMPL(quad,-1,size) \
    INSTANCE_IMPL(float,-1,size)
#else
#define INSTANCE_DYN(size) \
    INSTANCE_IMPL(double,-1,size) \
    INSTANCE_IMPL(float,-1,size)
#endif

INSTANCE(1)
INSTANCE(2)
INSTANCE(3)
INSTANCE(4)
INSTANCE(5)
INSTANCE(6)
INSTANCE(7)
INSTANCE(8)
INSTANCE(9)
INSTANCE(10)
INSTANCE(12)
INSTANCE(24)

INSTANCE_DYN(4u)
INSTANCE_DYN(5u)
INSTANCE_DYN(6u)
INSTANCE_DYN(7u)
INSTANCE_DYN(8u)
INSTANCE_DYN(9u)
INSTANCE_DYN(10u)
INSTANCE_DYN(11u)
}
}
