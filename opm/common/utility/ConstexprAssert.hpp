/*
   A compilation of the following posts:
   http://stackoverflow.com/questions/18648069/g-doesnt-compile-constexpr-function-with-assert-in-it
   http://ericniebler.com/2014/09/27/assert-and-constexpr-in-cxx11/

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

#ifndef OPM_CONSTEXPR_ASSERT_HPP
#define OPM_CONSTEXPR_ASSERT_HPP

#include <cassert>
#include <utility>

namespace {
template<class Assert>
void constexpr_assert_failed(Assert&& a) noexcept { std::forward<Assert>(a)(); }
}

// When evaluated at compile time emits a compilation error if condition is not true.
// Invokes the standard assert at run time.
#define constexpr_assert(cond) \
    ((void)((cond) ? 0 : (constexpr_assert_failed([](){ assert(!#cond);}), 0)))

#endif
