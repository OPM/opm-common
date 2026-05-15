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
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <opm/io/eclipse/SummaryNode.hpp>

#include <initializer_list>
#include <memory>
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

BOOST_AUTO_TEST_CASE(CGMITL)
{
    BOOST_CHECK_EQUAL(Opm::parseKeywordType("CGMITL"),
                      Opm::SummaryConfigNode::Type::Total);
}

BOOST_AUTO_TEST_SUITE_END()     // Total

BOOST_AUTO_TEST_SUITE_END()     // Type

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Category)

BOOST_AUTO_TEST_CASE(LWOPR_is_Well)
{
    using Cat = Opm::SummaryConfigNode::Category;
    BOOST_CHECK(Opm::parseKeywordCategory("LWOPR") == Cat::Well);
}

BOOST_AUTO_TEST_CASE(LCOFR_is_Connection)
{
    using Cat = Opm::SummaryConfigNode::Category;
    BOOST_CHECK(Opm::parseKeywordCategory("LCOFR") == Cat::Connection);
}

BOOST_AUTO_TEST_CASE(LBPR_is_Block)
{
    using Cat = Opm::SummaryConfigNode::Category;
    BOOST_CHECK(Opm::parseKeywordCategory("LBPR") == Cat::Block);
}

BOOST_AUTO_TEST_CASE(WOPR_is_Well)
{
    using Cat = Opm::SummaryConfigNode::Category;
    BOOST_CHECK(Opm::parseKeywordCategory("WOPR") == Cat::Well);
}

BOOST_AUTO_TEST_CASE(unknown_L_is_Misc)
{
    using Cat = Opm::SummaryConfigNode::Category;
    BOOST_CHECK(Opm::parseKeywordCategory("LXYZ") == Cat::Miscellaneous);
}

BOOST_AUTO_TEST_SUITE_END()     // Category

BOOST_AUTO_TEST_SUITE_END()     // ParseKeywords

// =====================================================================

BOOST_AUTO_TEST_SUITE(LGR)

BOOST_AUTO_TEST_CASE(lgr_field_roundtrip)
{
    using Node = Opm::SummaryConfigNode;
    using Cat  = Opm::SummaryConfigNode::Category;

    // Build a node and attach LGR name
    auto node = Node("LWOPR", Cat::Well, Opm::KeywordLocation{});
    node.lgr_name("LGR1");

    BOOST_CHECK(node.lgr_name().has_value());
    BOOST_CHECK_EQUAL(*node.lgr_name(), "LGR1");

    // Conversion to EclIO::SummaryNode preserves lgr name
    Opm::EclIO::SummaryNode sn = node;
    BOOST_CHECK(sn.lgr.has_value());
    BOOST_CHECK_EQUAL(sn.lgr->name, "LGR1");

    // Non-LGR node has empty optional
    auto global = Node("WOPR", Cat::Well, Opm::KeywordLocation{});
    BOOST_CHECK(!global.lgr_name().has_value());
    Opm::EclIO::SummaryNode sn2 = global;
    BOOST_CHECK(!sn2.lgr.has_value());

    // LB* node — only lgr_name propagates to EclIO::SummaryNode (no ijk field)
    auto blk = Node("LBPR", Cat::Block, Opm::KeywordLocation{});
    blk.lgr_name("LGR1");
    Opm::EclIO::SummaryNode sn3 = blk;
    BOOST_CHECK(sn3.lgr.has_value());
    BOOST_CHECK_EQUAL(sn3.lgr->name, "LGR1");
    // ijk is always {} in the IO layer; NUMS (number_) carries cell identity
    BOOST_CHECK_EQUAL(sn3.lgr->ijk[0], 0);
    BOOST_CHECK_EQUAL(sn3.lgr->ijk[1], 0);
    BOOST_CHECK_EQUAL(sn3.lgr->ijk[2], 0);
}

BOOST_AUTO_TEST_CASE(lgr_dedup_distinct_lgrs)
{
    // Regression: two nodes with same keyword+well but different LGRs
    // must NOT compare equal (silent dedup drop prevention)
    using Node = Opm::SummaryConfigNode;
    using Cat  = Opm::SummaryConfigNode::Category;

    auto n1 = Node("LWOPR", Cat::Well, Opm::KeywordLocation{});
    n1.namedEntity("PROD1");
    n1.lgr_name("LGR1");

    auto n2 = Node("LWOPR", Cat::Well, Opm::KeywordLocation{});
    n2.namedEntity("PROD1");
    n2.lgr_name("LGR2");

    BOOST_CHECK(n1 != n2);
    BOOST_CHECK(n1 < n2 || n2 < n1); // strict weak ordering holds

    // Same LGR — must be equal
    auto n3 = Node("LWOPR", Cat::Well, Opm::KeywordLocation{});
    n3.namedEntity("PROD1");
    n3.lgr_name("LGR1");

    BOOST_CHECK(n1 == n3);
    BOOST_CHECK(!(n1 < n3) && !(n3 < n1));
}

BOOST_AUTO_TEST_CASE(lgr_no_lgr_sorts_before_lgr)
{
    // non-LGR node sorts before LGR node with same keyword+entity
    using Node = Opm::SummaryConfigNode;
    using Cat  = Opm::SummaryConfigNode::Category;

    auto global = Node("WOPR", Cat::Well, Opm::KeywordLocation{});
    global.namedEntity("PROD1");

    auto lgr = Node("WOPR", Cat::Well, Opm::KeywordLocation{});
    lgr.namedEntity("PROD1");
    lgr.lgr_name("LGR1");

    BOOST_CHECK(global < lgr);
    BOOST_CHECK(!(lgr < global));
}

BOOST_AUTO_TEST_CASE(lgr_operator_equal_block_and_connection)
{
    using Node = Opm::SummaryConfigNode;
    using Cat  = Opm::SummaryConfigNode::Category;

    // Block: equal iff same number and same LGR
    {
        auto n1 = Node("LBPR", Cat::Block, Opm::KeywordLocation{});
        n1.number(42);
        n1.lgr_name("LGR1");

        auto n2 = Node("LBPR", Cat::Block, Opm::KeywordLocation{});
        n2.number(42);
        n2.lgr_name("LGR1");

        BOOST_CHECK(n1 == n2);
        BOOST_CHECK(!(n1 < n2) && !(n2 < n1));

        auto n3 = Node("LBPR", Cat::Block, Opm::KeywordLocation{});
        n3.number(42);
        n3.lgr_name("LGR2");

        BOOST_CHECK(n1 != n3);
        BOOST_CHECK(n1 < n3 || n3 < n1);
    }

    // Connection: equal iff same entity, number, and LGR
    {
        auto n1 = Node("LCOFR", Cat::Connection, Opm::KeywordLocation{});
        n1.namedEntity("PROD1");
        n1.number(3);
        n1.lgr_name("LGR1");

        auto n2 = Node("LCOFR", Cat::Connection, Opm::KeywordLocation{});
        n2.namedEntity("PROD1");
        n2.number(3);
        n2.lgr_name("LGR1");

        BOOST_CHECK(n1 == n2);
        BOOST_CHECK(!(n1 < n2) && !(n2 < n1));

        auto n3 = Node("LCOFR", Cat::Connection, Opm::KeywordLocation{});
        n3.namedEntity("PROD1");
        n3.number(3);
        n3.lgr_name("LGR2");

        BOOST_CHECK(n1 != n3);
        BOOST_CHECK(n1 < n3 || n3 < n1);
    }
}

BOOST_AUTO_TEST_CASE(lgr_operator_less_lgr_before_entity)
{
    // LGR identity sorts before named entity and numeric ID.
    // A node in LGR1 with a later-sorting entity sorts before a node in LGR2.
    using Node = Opm::SummaryConfigNode;
    using Cat  = Opm::SummaryConfigNode::Category;

    // Well: LGR1+PROD_Z sorts before LGR2+PROD_A
    {
        auto n1 = Node("LWOPR", Cat::Well, Opm::KeywordLocation{});
        n1.namedEntity("PROD_Z");
        n1.lgr_name("LGR1");

        auto n2 = Node("LWOPR", Cat::Well, Opm::KeywordLocation{});
        n2.namedEntity("PROD_A");
        n2.lgr_name("LGR2");

        BOOST_CHECK(n1 < n2);
        BOOST_CHECK(!(n2 < n1));
    }

    // Block: LGR1+number=99 sorts before LGR2+number=1
    {
        auto n1 = Node("LBPR", Cat::Block, Opm::KeywordLocation{});
        n1.number(99);
        n1.lgr_name("LGR1");

        auto n2 = Node("LBPR", Cat::Block, Opm::KeywordLocation{});
        n2.number(1);
        n2.lgr_name("LGR2");

        BOOST_CHECK(n1 < n2);
        BOOST_CHECK(!(n2 < n1));
    }

    // Connection: LGR1+PROD_Z+conn=99 sorts before LGR2+PROD_A+conn=1
    {
        auto n1 = Node("LCOFR", Cat::Connection, Opm::KeywordLocation{});
        n1.namedEntity("PROD_Z");
        n1.number(99);
        n1.lgr_name("LGR1");

        auto n2 = Node("LCOFR", Cat::Connection, Opm::KeywordLocation{});
        n2.namedEntity("PROD_A");
        n2.number(1);
        n2.lgr_name("LGR2");

        BOOST_CHECK(n1 < n2);
        BOOST_CHECK(!(n2 < n1));
    }
}

BOOST_AUTO_TEST_CASE(lgr_operator_less_null_before_set)
{
    // Null LGR sorts before any named LGR — verified for Block and Connection
    // (Well covered by lgr_no_lgr_sorts_before_lgr)
    using Node = Opm::SummaryConfigNode;
    using Cat  = Opm::SummaryConfigNode::Category;

    // Block
    {
        auto global = Node("LBPR", Cat::Block, Opm::KeywordLocation{});
        global.number(1);

        auto lgr = Node("LBPR", Cat::Block, Opm::KeywordLocation{});
        lgr.number(1);
        lgr.lgr_name("LGR1");

        BOOST_CHECK(global < lgr);
        BOOST_CHECK(!(lgr < global));
    }

    // Connection
    {
        auto global = Node("LCOFR", Cat::Connection, Opm::KeywordLocation{});
        global.namedEntity("PROD1");
        global.number(3);

        auto lgr = Node("LCOFR", Cat::Connection, Opm::KeywordLocation{});
        lgr.namedEntity("PROD1");
        lgr.number(3);
        lgr.lgr_name("LGR1");

        BOOST_CHECK(global < lgr);
        BOOST_CHECK(!(lgr < global));
    }
}

BOOST_AUTO_TEST_SUITE_END() // LGR

// =====================================================================

BOOST_AUTO_TEST_SUITE(NoSumLgr)

BOOST_AUTO_TEST_CASE(default_is_false)
{
    // A default-constructed SummaryConfig has noSumLgr_ == false
    Opm::SummaryConfig sc;
    BOOST_CHECK(!sc.noSumLgr());
}

BOOST_AUTO_TEST_CASE(serialization_test_object_sets_flag)
{
    // serializationTestObject() sets noSumLgr_ = true to exercise serialization
    const auto sc = Opm::SummaryConfig::serializationTestObject();
    BOOST_CHECK(sc.noSumLgr());
}

BOOST_AUTO_TEST_CASE(deck_sets_flag)
{
    const std::string deck_str = R"(
RUNSPEC
DIMENS
 1 1 1 /
START
 1 JAN 2020 /
GRID
DXV
 1*100.0 /
DYV
 1*100.0 /
DZV
 1*10.0 /
DEPTHZ
 4*2000.0 /
PORO
 1*0.3 /
PROPS
SOLUTION
SUMMARY
NOSUMLGR
SCHEDULE
END
)";
    const auto deck  = Opm::Parser{}.parseString(deck_str);
    const auto es    = Opm::EclipseState { deck };
    const auto sched = Opm::Schedule { deck, es, std::make_shared<Opm::Python>() };
    const auto cfg   = Opm::SummaryConfig { deck, sched, es.fieldProps(), es.aquifer() };
    BOOST_CHECK(cfg.noSumLgr());
}

BOOST_AUTO_TEST_SUITE_END() // NoSumLgr

// =====================================================================
// Deck-parsing integration tests using the two LGR .DATA files.
// Exercises keywordLW / keywordLC / keywordLB + handleKW L-prefix dispatch
// added in PR01. Tests that SummaryConfig correctly populates lgr_ on nodes
// produced from real deck keywords.
// =====================================================================

BOOST_AUTO_TEST_SUITE(LGR_SummaryConfig_Deck)

namespace {

Opm::SummaryConfig makeSummaryConfig(const std::string& path)
{
    const auto deck  = Opm::Parser{}.parseFile(path);
    const auto es    = Opm::EclipseState { deck };
    const auto sched = Opm::Schedule { deck, es, std::make_shared<Opm::Python>() };
    // Build a gridDims callback wrapping getInputGrid() so that
    // keywordLB/keywordLC can resolve linearised Cartesian indices via LGR
    // geometry.  For gridID=="" (global grid), return the global GridDims.
    // For an LGR name, return that LGR's GridDims so getGlobalIndex(i,j,k)
    // gives i + Nx*(j + Ny*k) relative to the LGR — not an active cell index.
    const auto& grid = es.getInputGrid();
    auto gridDims = [&grid](const std::string& gridID) -> Opm::GridDims
    {
        if (gridID.empty()) {
            return Opm::GridDims(grid.getNX(), grid.getNY(), grid.getNZ());
        }
        const auto& lgr = grid.getLGRCell(gridID);
        return Opm::GridDims(lgr.getNX(), lgr.getNY(), lgr.getNZ());
    };
    const auto parseCtx = Opm::ParseContext{};
    auto errors = Opm::ErrorGuard{};
    return Opm::SummaryConfig { deck, sched, es.fieldProps(), es.aquifer(),
                                parseCtx, errors,
                                std::move(gridDims) };
}

} // anonymous namespace

// --- 1LGR deck: INJ in LGR1, PROD in global grid ---

BOOST_AUTO_TEST_CASE(lgr_1lgr_lw_node_populated)
{
    // LWBHP 'LGR1' 'INJ' / — should produce one Well node with lgr_.name=="LGR1"
    const auto cfg  = makeSummaryConfig("LGR-WELL-3X3-1LGR.DATA");
    const auto hits = cfg.keywords("LWBHP");

    BOOST_REQUIRE_EQUAL(hits.size(), 1U);
    BOOST_REQUIRE(hits[0].lgr_name().has_value());
    BOOST_CHECK_EQUAL(*hits[0].lgr_name(), "LGR1");
    BOOST_CHECK_EQUAL(hits[0].namedEntity(), "INJ");
    BOOST_CHECK(hits[0].category() == Opm::SummaryConfigNode::Category::Well);
}

BOOST_AUTO_TEST_CASE(lgr_1lgr_lc_node_populated)
{
    // LCGFR 'LGR1' 'INJ' 2 2 1 / — should produce one Connection node
    const auto cfg  = makeSummaryConfig("LGR-WELL-3X3-1LGR.DATA");
    const auto hits = cfg.keywords("LCGFR");

    BOOST_REQUIRE_EQUAL(hits.size(), 1U);
    BOOST_REQUIRE(hits[0].lgr_name().has_value());
    BOOST_CHECK_EQUAL(*hits[0].lgr_name(), "LGR1");
    BOOST_CHECK_EQUAL(hits[0].namedEntity(), "INJ");
    BOOST_CHECK(hits[0].category() == Opm::SummaryConfigNode::Category::Connection);
}

BOOST_AUTO_TEST_CASE(lgr_1lgr_lb_node_populated)
{
    // LBPR 'LGR1' 2 2 1 / — should produce one Block node
    const auto cfg  = makeSummaryConfig("LGR-WELL-3X3-1LGR.DATA");
    const auto hits = cfg.keywords("LBPR");

    // 1LGR deck requests LBPR for LGR1; at minimum one node must match
    BOOST_CHECK(!hits.empty());
    const auto it = std::ranges::find_if(hits,
        [](const Opm::SummaryConfigNode& n) {
            return n.lgr_name().has_value()
                && *n.lgr_name() == "LGR1";
        });
    BOOST_CHECK(it != hits.end());
    BOOST_CHECK(it->category() == Opm::SummaryConfigNode::Category::Block);
}

BOOST_AUTO_TEST_CASE(lgr_1lgr_global_well_has_no_lgr)
{
    // PROD is on the global grid — its WOPR node must have no lgr (regression)
    const auto cfg  = makeSummaryConfig("LGR-WELL-3X3-1LGR.DATA");
    const auto hits = cfg.keywords("WOPR");

    const auto it = std::ranges::find_if(hits,
        [](const Opm::SummaryConfigNode& n) {
            return n.namedEntity() == "PROD";
        });
    BOOST_REQUIRE(it != hits.end());
    BOOST_CHECK(!it->lgr_name().has_value());
}

// --- 2LGR deck: INJ in LGR1, PROD in LGR2 — Fix4 dedup stress ---

BOOST_AUTO_TEST_CASE(lgr_2lgr_lw_two_nodes_not_deduped)
{
    // LWBHP has 'LGR1'/'INJ' AND 'LGR2'/'PROD' — must produce 2 distinct nodes
    const auto cfg  = makeSummaryConfig("LGR-WELL-3X3-2LGR.DATA");
    const auto hits = cfg.keywords("LWBHP");

    BOOST_REQUIRE_EQUAL(hits.size(), 2U);
    BOOST_CHECK(hits[0] != hits[1]);

    // One for each LGR
    const bool has_lgr1 = std::ranges::any_of(hits,
        [](const Opm::SummaryConfigNode& n) {
            return n.lgr_name().has_value() && *n.lgr_name() == "LGR1"
                && n.namedEntity() == "INJ";
        });
    const bool has_lgr2 = std::ranges::any_of(hits,
        [](const Opm::SummaryConfigNode& n) {
            return n.lgr_name().has_value() && *n.lgr_name() == "LGR2"
                && n.namedEntity() == "PROD";
        });
    BOOST_CHECK(has_lgr1);
    BOOST_CHECK(has_lgr2);
}

BOOST_AUTO_TEST_CASE(lgr_2lgr_lb_dedup_by_lgr_name)
{
    // The 2LGR deck requests LBPR at 3 cells per LGR (1,1,1), (2,2,1), (3,3,1).
    // The callback in makeSummaryConfig resolves each to a distinct active index,
    // so all 3 nodes per LGR survive deduplication.
    // Regression: nodes with same number_ but different LGR must not be collapsed.
    const auto cfg  = makeSummaryConfig("LGR-WELL-3X3-2LGR.DATA");
    const auto hits = cfg.keywords("LBPR");

    // 3 distinct cells per LGR — all must survive
    const auto lgr1_count = std::ranges::count_if(hits,
        [](const Opm::SummaryConfigNode& n) {
            return n.lgr_name().has_value() && *n.lgr_name() == "LGR1";
        });
    const auto lgr2_count = std::ranges::count_if(hits,
        [](const Opm::SummaryConfigNode& n) {
            return n.lgr_name().has_value() && *n.lgr_name() == "LGR2";
        });
    BOOST_CHECK_EQUAL(lgr1_count, 3);
    BOOST_CHECK_EQUAL(lgr2_count, 3);

    // Regression: a node in LGR1 and LGR2 with the same number_ must be distinct.
    const bool lgr1_lgr2_distinct = std::ranges::any_of(hits,
        [](const Opm::SummaryConfigNode& n) {
            return n.lgr_name().has_value() && *n.lgr_name() == "LGR1";
        }) &&
        std::ranges::any_of(hits,
        [](const Opm::SummaryConfigNode& n) {
            return n.lgr_name().has_value() && *n.lgr_name() == "LGR2";
        });
    BOOST_CHECK(lgr1_lgr2_distinct);
}

BOOST_AUTO_TEST_CASE(lgr_lb_callback_resolves_distinct_numbers)
{
    // Verify that a non-trivial gridDims callback assigns distinct number_ values
    // per cell so that all nodes survive deduplication.
    // Uses a mock callback — no real LGR geometry needed.

    const std::string deck_str = R"(
RUNSPEC
DIMENS
 3 3 1 /
WELLDIMS
 2 1 1 2 /
START
 1 JAN 2020 /
GRID
DXV
 3*100.0 /
DYV
 3*100.0 /
DZV
 1*10.0 /
DEPTHZ
 16*2000.0 /
PORO
 9*0.3 /
PROPS
SOLUTION
SUMMARY
LBPR
 'LGR1'  1 1 1 /
 'LGR1'  2 2 1 /
 'LGR1'  3 3 1 /
/
SCHEDULE
END
)";

    // Mock: LGR1 is treated as a 3x3x1 grid.  The callback returns GridDims
    // for that grid so getGlobalIndex(i,j,k) = i + 3*j + 9*k (0-based)
    // gives a linearised Cartesian index relative to the LGR.
    auto mockGridDims = [](const std::string& /*gridID*/) -> Opm::GridDims
    {
        return Opm::GridDims(3, 3, 1);
    };
    // Expected 1-based Cartesian indices:
    //   (1,1,1) → 0-based (0,0,0) → 0 + 1 = 1
    //   (2,2,1) → 0-based (1,1,0) → 1+3  + 1 = 5
    //   (3,3,1) → 0-based (2,2,0) → 2+6  + 1 = 9

    const auto deck    = Opm::Parser{}.parseString(deck_str);
    const auto es      = Opm::EclipseState { deck };
    const auto sched   = Opm::Schedule { deck, es, std::make_shared<Opm::Python>() };
    const auto parseCtx = Opm::ParseContext{};
    auto errors = Opm::ErrorGuard{};

    const auto cfg = Opm::SummaryConfig {
        deck, sched, es.fieldProps(), es.aquifer(),
        parseCtx, errors, mockGridDims
    };

    const auto hits = cfg.keywords("LBPR");
    BOOST_REQUIRE_EQUAL(hits.size(), 3u);

    std::vector<int> numbers;
    for (const auto& n : hits) numbers.push_back(n.number());
    std::ranges::sort(numbers);
    BOOST_CHECK_EQUAL(numbers[0], 1);   // cell (1,1,1) → Cartesian 0 → 1-based 1
    BOOST_CHECK_EQUAL(numbers[1], 5);   // cell (2,2,1) → Cartesian 4 → 1-based 5
    BOOST_CHECK_EQUAL(numbers[2], 9);   // cell (3,3,1) → Cartesian 8 → 1-based 9
}

BOOST_AUTO_TEST_SUITE_END() // LGR_SummaryConfig_Deck

// =====================================================================
// Inline schema tests — parse LW*/LC*/LB* from a minimal string deck.
// These catch schema bugs (wrong "size", missing items, bad regex) without
// needing .DATA files on disk. Any keyword-recognition or record-format
// regression shows up here before Jenkins.
// =====================================================================

BOOST_AUTO_TEST_SUITE(LGR_Schema_Inline)

namespace {

// Parse a minimal complete deck from an inline string and return SummaryConfig.
// The RUNSPEC/GRID/PROPS/SCHEDULE sections are minimal stubs so EclipseState
// can be constructed without needing any external files.
Opm::SummaryConfig parseSummarySection(const std::string& summary_body)
{
    const std::string deck_str = R"(
RUNSPEC
TITLE
 LGR_SCHEMA_TEST /
DIMENS
 3 3 1 /
WELLDIMS
 4 1 1 4 /
START
 1 JAN 2020 /
GRID
DXV
 3*100.0 /
DYV
 3*100.0 /
DZV
 1*10.0 /
DEPTHZ
 16*2000.0 /
PORO
 9*0.3 /
PROPS
SOLUTION
SUMMARY
)" + summary_body + R"(
SCHEDULE
WELSPECL
 'WELL1'  'G'  'LGR1'  2 2  2000.0  'OIL' /
 'WELL2'  'G'  'LGR2'  3 3  2000.0  'OIL' /
 'INJ'    'G'  'LGR1'  1 1  2000.0  'WAT' /
 'PROD'   'G'  'LGR2'  3 3  2000.0  'OIL' /
/
DATES
 1 FEB 2020 /
/
END
)";

    const auto deck  = Opm::Parser{}.parseString(deck_str);
    const auto es    = Opm::EclipseState { deck };
    const auto sched = Opm::Schedule { deck, es, std::make_shared<Opm::Python>() };
    return Opm::SummaryConfig { deck, sched, es.fieldProps(), es.aquifer() };
}

} // anonymous namespace

BOOST_AUTO_TEST_CASE(lw_single_record_produces_node)
{
    // Verify that LWBHP with one LGR/well record produces exactly one node.
    // Catches: "size":1 regression (would throw before reaching keywordLW),
    // AND the lgr_well_tag matching (would silently produce zero nodes).
    const auto cfg = parseSummarySection(
        "LWBHP\n"
        " 'LGR1'  'WELL1' /\n"
        "/\n"
    );

    const auto nodes = cfg.keywords("LWBHP");
    BOOST_REQUIRE_EQUAL(nodes.size(), 1u);
    BOOST_CHECK_EQUAL(nodes[0].namedEntity(), "WELL1");
    BOOST_REQUIRE(nodes[0].lgr_name().has_value());
    BOOST_CHECK_EQUAL(*nodes[0].lgr_name(), "LGR1");
}

BOOST_AUTO_TEST_CASE(lw_multi_record_produces_two_nodes)
{
    // Verify that LWBHP with two records (LGR1/WELL1, LGR2/WELL2) produces two nodes.
    // Catches: "size":1 bug — schema must be table format (no "size" field).
    const auto cfg = parseSummarySection(
        "LWBHP\n"
        " 'LGR1'  'WELL1' /\n"
        " 'LGR2'  'WELL2' /\n"
        "/\n"
    );

    const auto nodes = cfg.keywords("LWBHP");
    BOOST_REQUIRE_EQUAL(nodes.size(), 2u);
    BOOST_REQUIRE(nodes[0].lgr_name().has_value());
    BOOST_CHECK_EQUAL(*nodes[0].lgr_name(), "LGR1");
    BOOST_REQUIRE(nodes[1].lgr_name().has_value());
    BOOST_CHECK_EQUAL(*nodes[1].lgr_name(), "LGR2");
}

BOOST_AUTO_TEST_CASE(lc_multi_record_produces_nodes)
{
    // Verify that LCOFR with multiple records (LGR + well + I J K) produces nodes.
    const auto cfg = parseSummarySection(
        "LCOFR\n"
        " 'LGR1'  'WELL1'  1 1 1 /\n"
        " 'LGR1'  'WELL1'  2 1 1 /\n"
        "/\n"
    );

    const auto nodes = cfg.keywords("LCOFR");
    // The stub deck has no LGR geometry, so the gridDims callback returns
    // GridDims{} (NX=0) — cell index resolution is skipped.
    // Both records share number_=INT_MIN and are deduped to 1.
    BOOST_CHECK_EQUAL(nodes.size(), 1u);
    for (const auto& node : nodes) {
        BOOST_CHECK(node.lgr_name().has_value());
        BOOST_CHECK_EQUAL(*node.lgr_name(), "LGR1");
    }
}

BOOST_AUTO_TEST_CASE(lb_multi_record_produces_nodes)
{
    // Verify that LBPR with multiple records (LGR + I J K) produces nodes.
    // keywordLB has no well lookup — tests pure schema parsing.
    const auto cfg = parseSummarySection(
        "LBPR\n"
        " 'LGR1'  1 1 1 /\n"
        " 'LGR1'  2 2 1 /\n"
        "/\n"
    );

    const auto nodes = cfg.keywords("LBPR");
    // The stub deck has no LGR geometry, so the gridDims callback returns
    // GridDims{} (NX=0) — cell index resolution is skipped.
    // Both records share number_=INT_MIN and are deduped to 1.
    BOOST_REQUIRE_EQUAL(nodes.size(), 1u);
    BOOST_REQUIRE(nodes[0].lgr_name().has_value());
    BOOST_CHECK_EQUAL(*nodes[0].lgr_name(), "LGR1");
}

BOOST_AUTO_TEST_CASE(lw_regex_matches_lwstat)
{
    // Verify the LW.+ regex covers LWSTAT and produces a node.
    const auto cfg = parseSummarySection(
        "LWSTAT\n"
        " 'LGR1'  'INJ' /\n"
        "/\n"
    );

    const auto nodes = cfg.keywords("LWSTAT");
    BOOST_REQUIRE_EQUAL(nodes.size(), 1u);
    BOOST_REQUIRE(nodes[0].lgr_name().has_value());
    BOOST_CHECK_EQUAL(*nodes[0].lgr_name(), "LGR1");
}

BOOST_AUTO_TEST_SUITE_END() // LGR_Schema_Inline
