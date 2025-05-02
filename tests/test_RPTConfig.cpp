// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:

/*
  Copyright 2025 Equinor

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

#define BOOST_TEST_MODULE RPT_Config

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Schedule/RPTConfig.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/R.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

BOOST_AUTO_TEST_SUITE(Basic_Operations)

namespace {
    Opm::DeckKeyword getRptSched(const std::string& input)
    {
        return Opm::Parser{}.parseString(input)
            .get<Opm::ParserKeywords::RPTSCHED>()
            .back();
    }
} // Anonymous keyword

BOOST_AUTO_TEST_SUITE(Accepts_All_Mnemonics)

BOOST_AUTO_TEST_CASE(No_Mnemonics)
{
    const auto cfg = Opm::RPTConfig { getRptSched(R"(SCHEDULE
RPTSCHED
/
END
)") };

    BOOST_CHECK_EQUAL(cfg.size(), std::size_t{0});
    BOOST_CHECK_MESSAGE(! cfg.contains("No such mnemonic"),
                        R"("No such mnemonic" must not exist)");
    BOOST_CHECK_THROW(cfg.at("HELLO"), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(Selected_Mnemonics)
{
    const auto cfg = Opm::RPTConfig { getRptSched(R"(SCHEDULE
RPTSCHED
  WELLS=2 WELSPECS KRO PRESSURE=42 FIPRESV FIP=2
/
END
)") };

    BOOST_CHECK_EQUAL(cfg.size(), std::size_t{6});
    BOOST_CHECK_MESSAGE(! cfg.contains("No such mnemonic"),
                        R"("No such mnemonic" must not exist)");
    BOOST_CHECK_THROW(cfg.at("HELLO"), std::out_of_range);

    BOOST_CHECK_MESSAGE(cfg.contains("WELLS"), R"(Mnemonic "WELLS" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("WELSPECS"), R"(Mnemonic "WELSPECS" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("KRO"), R"(Mnemonic "KRO" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("PRESSURE"), R"(Mnemonic "PRESSURE" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("FIPRESV"), R"(Mnemonic "FIPRESV" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("FIP"), R"(Mnemonic "FIP" must exist)");

    BOOST_CHECK_EQUAL(cfg.at("WELLS"), 2u);
    BOOST_CHECK_EQUAL(cfg.at("WELSPECS"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("KRO"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("PRESSURE"), 42u);
    BOOST_CHECK_EQUAL(cfg.at("FIPRESV"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("FIP"), 2u);
}

BOOST_AUTO_TEST_CASE(Unknown_Mnemonics)
{
    const auto cfg = Opm::RPTConfig { getRptSched(R"(SCHEDULE
RPTSCHED
  ACUTE=17 CAPTIONS
/
END
)") };

    BOOST_CHECK_EQUAL(cfg.size(), std::size_t{2});

    BOOST_CHECK_MESSAGE(cfg.contains("ACUTE"), R"(Mnemonic "ACUTE" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("CAPTIONS"), R"(Mnemonic "CAPTIONS" must exist)");

    BOOST_CHECK_EQUAL(cfg.at("ACUTE"), 17u);
    BOOST_CHECK_EQUAL(cfg.at("CAPTIONS"), 1u);
}

BOOST_AUTO_TEST_CASE(With_Extra_Spaces)
{
    const auto cfg = Opm::RPTConfig { getRptSched(R"(SCHEDULE
RPTSCHED
  WELLS = 2
/
END
)") };

    BOOST_CHECK_EQUAL(cfg.size(), std::size_t{1});

    BOOST_CHECK_MESSAGE(cfg.contains("WELLS"), R"(Mnemonic "WELLS" must exist)");

    BOOST_CHECK_EQUAL(cfg.at("WELLS"), 2u);
}

BOOST_AUTO_TEST_SUITE_END()     // Accepts_All_Mnemonics

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Mnemonic_Validity_Checking)

BOOST_AUTO_TEST_SUITE(No_Existing_Mnemonics)

namespace {
    Opm::RPTConfig makeConfigObject(const std::string& input)
    {
        const auto* prev = static_cast<const Opm::RPTConfig*>(nullptr);
        const auto parseContext = Opm::ParseContext{};
        auto errors = Opm::ErrorGuard{};

        return Opm::RPTConfig {
            getRptSched(input), prev, parseContext, errors
        };
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(No_Mnemonics)
{
    const auto cfg = makeConfigObject(R"(SCHEDULE
RPTSCHED
/
END
)");

    BOOST_CHECK_EQUAL(cfg.size(), std::size_t{0});
    BOOST_CHECK_MESSAGE(! cfg.contains("No such mnemonic"),
                        R"("No such mnemonic" must not exist)");
    BOOST_CHECK_THROW(cfg.at("HELLO"), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(Selected_Mnemonics)
{
    const auto cfg = makeConfigObject(R"(SCHEDULE
RPTSCHED
  WELLS=2 WELSPECS KRO PRESSURE=42 FIPRESV FIP=2
/
END
)");

    BOOST_CHECK_EQUAL(cfg.size(), std::size_t{6});
    BOOST_CHECK_MESSAGE(! cfg.contains("No such mnemonic"),
                        R"("No such mnemonic" must not exist)");
    BOOST_CHECK_THROW(cfg.at("HELLO"), std::out_of_range);

    BOOST_CHECK_MESSAGE(cfg.contains("WELLS"), R"(Mnemonic "WELLS" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("WELSPECS"), R"(Mnemonic "WELSPECS" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("KRO"), R"(Mnemonic "KRO" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("PRESSURE"), R"(Mnemonic "PRESSURE" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("FIPRESV"), R"(Mnemonic "FIPRESV" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("FIP"), R"(Mnemonic "FIP" must exist)");

    BOOST_CHECK_EQUAL(cfg.at("WELLS"), 2u);
    BOOST_CHECK_EQUAL(cfg.at("WELSPECS"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("KRO"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("PRESSURE"), 42u);
    BOOST_CHECK_EQUAL(cfg.at("FIPRESV"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("FIP"), 2u);
}

BOOST_AUTO_TEST_CASE(Unknown_Mnemonics)
{
    const auto cfg = makeConfigObject(R"(SCHEDULE
RPTSCHED
  ACUTE=17 CAPTIONS
/
END
)");

    BOOST_CHECK_EQUAL(cfg.size(), std::size_t{0});

    BOOST_CHECK_MESSAGE(! cfg.contains("ACUTE"), R"(Mnemonic "ACUTE" must NOT exist)");
    BOOST_CHECK_MESSAGE(! cfg.contains("CAPTIONS"), R"(Mnemonic "CAPTIONS" must NOT exist)");
}

BOOST_AUTO_TEST_CASE(With_Extra_Spaces)
{
    const auto cfg = makeConfigObject(R"(SCHEDULE
RPTSCHED
  WELLS = 2
/
END
)");

    BOOST_CHECK_EQUAL(cfg.size(), std::size_t{1});

    BOOST_CHECK_MESSAGE(cfg.contains("WELLS"), R"(Mnemonic "WELLS" must exist)");

    BOOST_CHECK_EQUAL(cfg.at("WELLS"), 2u);
}

BOOST_AUTO_TEST_CASE(Integer_Controls)
{
    const auto cfg = makeConfigObject(R"(SCHEDULE
RPTSCHED
 0 0 0 0 0 0 2 2 2 0 1 1 0 1 1 0 0 /
END
)");

    BOOST_CHECK_EQUAL(cfg.size(), std::size_t{17});

    BOOST_CHECK_MESSAGE(cfg.contains("PRES"), R"(Mnemonic "PRES" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("SOIL"), R"(Mnemonic "SOIL" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("SWAT"), R"(Mnemonic "SWAT" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("SGAS"), R"(Mnemonic "SGAS" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("RS"), R"(Mnemonic "RS" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("RV"), R"(Mnemonic "RV" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("RESTART"), R"(Mnemonic "RESTART" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("FIP"), R"(Mnemonic "FIP" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("WELLS"), R"(Mnemonic "WELLS" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("VFPPROD"), R"(Mnemonic "VFPPROD" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("SUMMARY"), R"(Mnemonic "SUMMARY" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("CPU"), R"(Mnemonic "CPU" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("AQUCT"), R"(Mnemonic "AQUCT" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("WELSPECS"), R"(Mnemonic "WELSPECS" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("NEWTON"), R"(Mnemonic "NEWTON" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("POILD"), R"(Mnemonic "POILD" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("PWAT"), R"(Mnemonic "PWAT" must exist)");

    BOOST_CHECK_EQUAL(cfg.at("PRES"), 0u);
    BOOST_CHECK_EQUAL(cfg.at("SOIL"), 0u);
    BOOST_CHECK_EQUAL(cfg.at("SWAT"), 0u);
    BOOST_CHECK_EQUAL(cfg.at("SGAS"), 0u);
    BOOST_CHECK_EQUAL(cfg.at("RS"), 0u);
    BOOST_CHECK_EQUAL(cfg.at("RV"), 0u);
    BOOST_CHECK_EQUAL(cfg.at("RESTART"), 2u);
    BOOST_CHECK_EQUAL(cfg.at("FIP"), 2u);
    BOOST_CHECK_EQUAL(cfg.at("WELLS"), 2u);
    BOOST_CHECK_EQUAL(cfg.at("VFPPROD"), 0u);
    BOOST_CHECK_EQUAL(cfg.at("SUMMARY"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("CPU"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("AQUCT"), 0u);
    BOOST_CHECK_EQUAL(cfg.at("WELSPECS"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("NEWTON"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("POILD"), 0u);
    BOOST_CHECK_EQUAL(cfg.at("PWAT"), 0u);
}

BOOST_AUTO_TEST_CASE(Nothing_Mnemonic_Clears_List)
{
    const auto cfg = makeConfigObject(R"(SCHEDULE
RPTSCHED
  WELLS=2 WELSPECS KRO PRESSURE=42 FIPRESV FIP=2
  NOTHING -- Clears mnemonic list
/
END
)");

    BOOST_CHECK_EQUAL(cfg.size(), std::size_t{0});

    BOOST_CHECK_MESSAGE(! cfg.contains("WELLS"), R"(Mnemonic "WELLS" must NOT exist)");
    BOOST_CHECK_MESSAGE(! cfg.contains("WELSPECS"), R"(Mnemonic "WELSPECS" must NOT exist)");
    BOOST_CHECK_MESSAGE(! cfg.contains("KRO"), R"(Mnemonic "KRO" must NOT exist)");
    BOOST_CHECK_MESSAGE(! cfg.contains("PRESSURE"), R"(Mnemonic "PRESSURE" must NOT exist)");
    BOOST_CHECK_MESSAGE(! cfg.contains("FIPRESV"), R"(Mnemonic "FIPRESV" must NOT exist)");
    BOOST_CHECK_MESSAGE(! cfg.contains("FIP"), R"(Mnemonic "FIP" must NOT exist)");
}

BOOST_AUTO_TEST_SUITE_END()     // No_Existing_Mnemonics

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(With_Existing_Mnemonics)

namespace {
    Opm::RPTConfig makeConfigObject(const std::string& input)
    {
        const auto prev = Opm::RPTConfig { getRptSched(R"(SCHEDULE
RPTSCHED
  WELLS FIP=2 /
)") };
        const auto parseContext = Opm::ParseContext{};
        auto errors = Opm::ErrorGuard{};

        return Opm::RPTConfig {
            getRptSched(input), &prev, parseContext, errors
        };
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(No_Mnemonics)
{
    const auto cfg = makeConfigObject(R"(RPTSCHED
/
END
)");

    BOOST_CHECK_EQUAL(cfg.size(), std::size_t{2});

    BOOST_CHECK_MESSAGE(cfg.contains("WELLS"), R"(Mnemonic "WELLS" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("FIP"), R"(Mnemonic "FIP" must exist)");

    BOOST_CHECK_EQUAL(cfg.at("WELLS"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("FIP"), 2u);
}

BOOST_AUTO_TEST_CASE(Selected_Mnemonics)
{
    const auto cfg = makeConfigObject(R"(RPTSCHED
  WELLS=2 WELSPECS KRO PRESSURE=42 FIPRESV
/
END
)");

    BOOST_CHECK_EQUAL(cfg.size(), std::size_t{6});
    BOOST_CHECK_MESSAGE(! cfg.contains("No such mnemonic"),
                        R"("No such mnemonic" must not exist)");
    BOOST_CHECK_THROW(cfg.at("HELLO"), std::out_of_range);

    BOOST_CHECK_MESSAGE(cfg.contains("WELLS"), R"(Mnemonic "WELLS" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("WELSPECS"), R"(Mnemonic "WELSPECS" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("KRO"), R"(Mnemonic "KRO" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("PRESSURE"), R"(Mnemonic "PRESSURE" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("FIPRESV"), R"(Mnemonic "FIPRESV" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("FIP"), R"(Mnemonic "FIP" must exist)");

    BOOST_CHECK_EQUAL(cfg.at("WELLS"), 2u);
    BOOST_CHECK_EQUAL(cfg.at("WELSPECS"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("KRO"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("PRESSURE"), 42u);
    BOOST_CHECK_EQUAL(cfg.at("FIPRESV"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("FIP"), 2u);
}

BOOST_AUTO_TEST_CASE(Unknown_Mnemonics)
{
    const auto cfg = makeConfigObject(R"(RPTSCHED
  ACUTE=17 CAPTIONS
/
END
)");

    BOOST_CHECK_EQUAL(cfg.size(), std::size_t{2});

    BOOST_CHECK_MESSAGE(! cfg.contains("ACUTE"), R"(Mnemonic "ACUTE" must NOT exist)");
    BOOST_CHECK_MESSAGE(! cfg.contains("CAPTIONS"), R"(Mnemonic "CAPTIONS" must NOT exist)");
}

BOOST_AUTO_TEST_CASE(With_Extra_Spaces)
{
    const auto cfg = makeConfigObject(R"(RPTSCHED
  FIP = 3
/
END
)");

    BOOST_CHECK_EQUAL(cfg.size(), std::size_t{2});

    BOOST_CHECK_MESSAGE(cfg.contains("WELLS"), R"(Mnemonic "WELLS" must exist)");
    BOOST_CHECK_MESSAGE(cfg.contains("FIP"), R"(Mnemonic "FIP" must exist)");

    BOOST_CHECK_EQUAL(cfg.at("WELLS"), 1u);
    BOOST_CHECK_EQUAL(cfg.at("FIP"), 3u);
}

BOOST_AUTO_TEST_SUITE_END()     // With_Existing_Mnemonics

BOOST_AUTO_TEST_SUITE_END()     // Mnemonic_Validity_Checking

BOOST_AUTO_TEST_SUITE_END()     // Basic_Operations
