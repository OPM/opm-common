/*
   Copyright 2020 Statoil ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#include "config.h"

#define BOOST_TEST_MODULE OpmInputError_format

#include <boost/test/unit_test.hpp>

#include <exception>

#include <opm/common/utility/OpmInputError.hpp>

const Opm::KeywordLocation location { "MXUNSUPP", "FILENAME.DAT", 42 } ;
const std::string error_string { "Error encountered" } ;

BOOST_AUTO_TEST_CASE(simple) {
    const std::string expected { "MXUNSUPP@FILENAME.DAT:42" } ;

    const std::string format_string { "{keyword}@{file}:{line}" } ;
    const std::string formatted { Opm::OpmInputError::format(format_string, location) } ;

    BOOST_CHECK_EQUAL(formatted, expected);
}

BOOST_AUTO_TEST_CASE(named) {
    const std::string expected { "MXUNSUPP@FILENAME.DAT:42: Error encountered" } ;

    const std::string format_string { "{keyword}@{file}:{line}: {error}" } ;
    const std::string formatted { Opm::OpmInputError::format(format_string, location, fmt::arg("error", error_string)) } ;

    BOOST_CHECK_EQUAL(formatted, expected);
}

BOOST_AUTO_TEST_CASE(positional) {
    const std::string expected { "MXUNSUPP@FILENAME.DAT:42: Error encountered" } ;

    const std::string format_string { "{keyword}@{file}:{line}: {}" } ;
    const std::string formatted { Opm::OpmInputError::format(format_string, location, error_string) } ;

    BOOST_CHECK_EQUAL(formatted, expected);
}

BOOST_AUTO_TEST_CASE(exception) {
    const std::string expected { R"(Problem parsing keyword MXUNSUPP
In FILENAME.DAT line 42.
Internal error message: Runtime Error)" };

    const std::string formatted { Opm::OpmInputError::formatException(location, std::runtime_error("Runtime Error")) } ;

    BOOST_CHECK_EQUAL(formatted, expected);
}

BOOST_AUTO_TEST_CASE(exception_reason) {
    const std::string expected { R"(Problem parsing keyword MXUNSUPP
In FILENAME.DAT line 42.
Reason: Runtime Error)" };

    const std::string formatted { Opm::OpmInputError::formatException(location, std::runtime_error("Runtime Error"), "Reason") } ;

    BOOST_CHECK_EQUAL(formatted, expected);
}

BOOST_AUTO_TEST_CASE(exception_init) {
    const std::string expected { R"(Problem parsing keyword MXUNSUPP
In FILENAME.DAT line 42.
Reason: Runtime Error)" };

    const std::string formatted { Opm::OpmInputError(location, std::runtime_error("Runtime Error"), "Reason").what() } ;

    BOOST_CHECK_EQUAL(formatted, expected);
}

BOOST_AUTO_TEST_CASE(exception_nest) {
    const std::string expected { R"(Problem parsing keyword MXUNSUPP
In FILENAME.DAT line 42.
Internal error message: Runtime Error)" };

    try {
        try {
            throw std::runtime_error("Runtime Error");
        } catch (const std::exception& e) {
            std::throw_with_nested(Opm::OpmInputError(location, e));
        }
    } catch (const Opm::OpmInputError& opm_error) {
        BOOST_CHECK_EQUAL(opm_error.what(), expected);
    }
}
