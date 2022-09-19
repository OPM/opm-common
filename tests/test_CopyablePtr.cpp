/*
  Copyright 2022 Equinor ASA.

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

#define BOOST_TEST_MODULE CopyablePtrTest
#include <boost/test/unit_test.hpp>

#include <opm/utility/CopyablePtr.hpp>

#include <memory>

namespace {
template <class T>
class A {
    using CopyablePtr = Opm::Utility::CopyablePtr<T>;
public:
    A() : aptr_{} {}
    void assign(T value) {
        aptr_ = std::make_unique<T>(value);
    }
    const CopyablePtr& get_aptr() const {return aptr_;}
private:
    CopyablePtr aptr_;
};

struct B {
    double a;
    int b;
    int getb() {return b;}
};
} // namespace

BOOST_AUTO_TEST_SUITE ()

BOOST_AUTO_TEST_CASE (copyable)
{
    A<B> a1 {};
    a1.assign(B{1.1,2});
    BOOST_CHECK_MESSAGE(a1.get_aptr().get()->a == 1.1, "Able to assign new value to pointer");
    A<B> a2 = a1;
    BOOST_CHECK_MESSAGE(a2.get_aptr().get()->a == 1.1, "Able to copy construct a new pointer");
    a1.assign(B{1.3,3});
    BOOST_CHECK_MESSAGE(a1.get_aptr().get()->a == 1.3, "Able to access new value to pointer");
    BOOST_CHECK_MESSAGE(a2.get_aptr().get()->b == 2, "Copied value not affected by parent modification");
    BOOST_CHECK_MESSAGE(a1.get_aptr().get()->getb() == 3, "Member access operator works");
    BOOST_CHECK_MESSAGE(a1.get_aptr(), "Boolean context operator works");
}

BOOST_AUTO_TEST_SUITE_END()
