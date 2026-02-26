/*
  Copyright 2026 Equinor

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

#define BOOST_TEST_MODULE SummaryConfigNode

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

#include <opm/io/eclipse/SummaryNode.hpp>

#include <initializer_list>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace Opm::EclIO {
    template <typename CharT, typename Traits>
    std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& os,
               const SummaryNode::Type            t)
    {
        switch (t) {
        case SummaryNode::Type::Rate:
            return os << "Rate";

        case SummaryNode::Type::Total:
            return os << "Total";

        case SummaryNode::Type::Ratio:
            return os << "Ratio";

        case SummaryNode::Type::Pressure:
            return os << "Pressure";

        case SummaryNode::Type::Count:
            return os << "Count";

        case SummaryNode::Type::Mode:
            return os << "Mode";

        case SummaryNode::Type::ProdIndex:
            return os << "ProdIndex";

        case SummaryNode::Type::Undefined:
            return os << "Undefined";
        }

        return os << "<Unknown ("
                  << static_cast<std::underlying_type_t<EclIO::SummaryNode::Type>>(t)
                  << ")>";
    }
} // namespace Opm::EclIO

BOOST_AUTO_TEST_SUITE(ParseKeywords)

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Type)

BOOST_AUTO_TEST_SUITE(Total)

BOOST_AUTO_TEST_CASE(BOPT)
{
    BOOST_CHECK_EQUAL(Opm::parseKeywordType("BOPT"),
                      Opm::SummaryConfigNode::Type::Total);
}

BOOST_AUTO_TEST_CASE(COPT)
{
    BOOST_CHECK_EQUAL(Opm::parseKeywordType("COPT"),
                      Opm::SummaryConfigNode::Type::Total);
}

BOOST_AUTO_TEST_CASE(FOPT)
{
    BOOST_CHECK_EQUAL(Opm::parseKeywordType("FOPT"),
                      Opm::SummaryConfigNode::Type::Total);
}

BOOST_AUTO_TEST_CASE(GOPT)
{
    BOOST_CHECK_EQUAL(Opm::parseKeywordType("GOPT"),
                      Opm::SummaryConfigNode::Type::Total);
}

BOOST_AUTO_TEST_CASE(ROPT)
{
    BOOST_CHECK_EQUAL(Opm::parseKeywordType("ROPT"),
                      Opm::SummaryConfigNode::Type::Total);
}

BOOST_AUTO_TEST_CASE(WOPT)
{
    BOOST_CHECK_EQUAL(Opm::parseKeywordType("WOPT"),
                      Opm::SummaryConfigNode::Type::Total);
}

BOOST_AUTO_TEST_SUITE_END()     // Total

BOOST_AUTO_TEST_SUITE_END()     // Type

BOOST_AUTO_TEST_SUITE_END()     // ParseKeywords
