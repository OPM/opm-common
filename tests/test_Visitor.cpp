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

#define BOOST_TEST_MODULE VISITOR_TESTS
#include <boost/test/unit_test.hpp>

#include <opm/common/utility/Visitor.hpp>

#include <stdexcept>
#include <variant>

namespace {

struct TestA {
  void testThrow()
  {
      throw std::runtime_error("A");
  }

  char returnData()
  {
      return 'A';
  }
};

struct TestB {
  void testThrow()
  {
      throw std::range_error("B");
  }

  char returnData()
  {
      return 'B';
  }
};

}

using Variant = std::variant<std::monostate, TestA, TestB>;

// Test overload set visitor on a simple list of classes
BOOST_AUTO_TEST_CASE(VariantReturn)
{
    Variant v{};
    char result = '\0';
    Opm::MonoThrowHandler<std::logic_error> mh{"Mono state"};
    auto rD = [&result](auto& param)
              {
                  result = param.returnData();
              };
    BOOST_CHECK_THROW(std::visit(Opm::VisitorOverloadSet{mh, rD}, v), std::logic_error);
    BOOST_CHECK(result == '\0');
    v = TestA{};
    BOOST_CHECK_NO_THROW(std::visit(Opm::VisitorOverloadSet{mh, rD}, v));
    BOOST_CHECK(result == 'A');
    v = TestB{};
    BOOST_CHECK_NO_THROW(std::visit(Opm::VisitorOverloadSet{mh, rD}, v));
    BOOST_CHECK(result == 'B');
}

// Test that overload set visitor throws expected exceptions
BOOST_AUTO_TEST_CASE(VariantThrow)
{
    Variant v{};
    Opm::MonoThrowHandler<std::logic_error> mh{"Mono state"};
    auto rD = [](auto& param)
              {
                  param.testThrow();
              };
    BOOST_CHECK_THROW(std::visit(Opm::VisitorOverloadSet{mh, rD}, v), std::logic_error);
    v = TestA{};
    BOOST_CHECK_THROW(std::visit(Opm::VisitorOverloadSet{mh, rD}, v), std::runtime_error);
    v = TestB{};
    BOOST_CHECK_THROW(std::visit(Opm::VisitorOverloadSet{mh, rD}, v), std::range_error);
}
