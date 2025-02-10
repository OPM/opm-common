/*
  Copyright 2025 Equinor ASA

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
#define BOOST_TEST_MODULE TestCriticalError

#include <boost/test/unit_test.hpp>
#include <opm/common/CriticalError.hpp>

BOOST_AUTO_TEST_CASE(TestCriticalError)
{
    try {
        OPM_TRY_THROW_AS_CRITICAL_ERROR(throw std::runtime_error("test"));
        BOOST_FAIL("Should have thrown");
    } catch (const Opm::CriticalError& outerException) {
        BOOST_CHECK(outerException.getInnerException() != nullptr);
        try {
            std::rethrow_exception(outerException.getInnerException());
        } catch (const std::runtime_error& innerException) {
            BOOST_CHECK_EQUAL(innerException.what(), "test");
        }
    }
}

BOOST_AUTO_TEST_CASE(TestCriticalErrorBeginEnd)
{
    try {
        OPM_BEGIN_TRY_CATCH_RETHROW_AS_CRITICAL_ERROR();
        throw std::runtime_error("test");
        BOOST_FAIL("Should have thrown");
        OPM_END_TRY_CATCH_RETHROW_AS_CRITICAL_ERROR();
    } catch (const Opm::CriticalError& outerException) {
        BOOST_CHECK(outerException.getInnerException() != nullptr);
        try {
            std::rethrow_exception(outerException.getInnerException());
        } catch (const std::runtime_error& innerException) {
            BOOST_CHECK_EQUAL(innerException.what(), "test");
        }
    }
}

BOOST_AUTO_TEST_CASE(TestCriticalErrorBeginEndPassCriticalError)
{
    // make sure we simply rethrow CriticalError without decorating it
    try { 
        OPM_BEGIN_TRY_CATCH_RETHROW_AS_CRITICAL_ERROR();
        throw Opm::CriticalError("test");
        BOOST_FAIL("Should have thrown");
        OPM_END_TRY_CATCH_RETHROW_AS_CRITICAL_ERROR();
    } catch (const Opm::CriticalError& e) {
        BOOST_CHECK_EQUAL(e.what(), "test");
    }
}

BOOST_AUTO_TEST_CASE(TestCriticalErrorMacroPassCriticalError) {
    // make sure we simply rethrow CriticalError without decorating it
    try {
        OPM_TRY_THROW_AS_CRITICAL_ERROR(throw Opm::CriticalError("test"));
        BOOST_FAIL("Should have thrown");
    } catch (const Opm::CriticalError& e) {
        BOOST_CHECK_EQUAL(e.what(), "test");
    }
}
