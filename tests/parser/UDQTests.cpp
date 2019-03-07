/*
  Copyright 2018 Statoil ASA.

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

#define BOOST_TEST_MODULE UDQTests
#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQInput.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQWellSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQExpression.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQContext.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQAssign.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQFunction.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQFunctionTable.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>

using namespace Opm;


Schedule make_schedule(const std::string& input) {
    Parser parser;

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    return Schedule(deck, grid , eclipseProperties, runspec);
}

BOOST_AUTO_TEST_CASE(UDQWellSetTest) {
    std::vector<std::string> wells = {"P1", "P2", "I1", "I2"};
    UDQWellSet ws("NAME", wells);
    UDQWellSet ws2("NAME", wells, 100.0);

    BOOST_CHECK_EQUAL(4, ws.size());
    ws.assign("P1", 1.0);

    const auto& value = ws["P1"];
    BOOST_CHECK_EQUAL(value.value(), 1.0);
    BOOST_CHECK_EQUAL(ws["P1"].value(), 1.0);

    BOOST_REQUIRE_THROW(ws.assign("NO_SUCH_WELL", 1.0), std::invalid_argument);

    ws.assign("*", 2.0);
    for (const auto& w : wells)
        BOOST_CHECK_EQUAL(ws[w].value(), 2.0);

    ws.assign(3.0);
    for (const auto& w : wells)
        BOOST_CHECK_EQUAL(ws[w].value(), 3.0);

    ws.assign("P*", 4.0);
    BOOST_CHECK_EQUAL(ws["P1"].value(), 4.0);
    BOOST_CHECK_EQUAL(ws["P2"].value(), 4.0);

    ws.assign("I2", 5.0);
    BOOST_CHECK_EQUAL(ws["I2"].value(), 5.0);


    for (const auto& w : wells)
        BOOST_CHECK_EQUAL(ws2[w].value(), 100.0);

    UDQSet us("NAME", wells.size());
    for (std::size_t i=0; i < wells.size(); i++)
        us.assign(i, 1.0 * i);

    UDQWellSet ws3("name", wells, us);
    for (std::size_t i=0; i < wells.size(); i++)
        BOOST_CHECK_EQUAL(ws3[wells[i]].value(), i*1.0);
}

BOOST_AUTO_TEST_CASE(KEYWORDS) {
    const std::string input = R"(
RUNSPEC

UDQDIMS
   10* 'N'/

UDQPARAM
  3* 0.25 /

)";
    Parser parser;

    auto deck = parser.parseString(input);
    auto runspec = Runspec(deck);
    auto udq_params = runspec.udqParams();

    BOOST_CHECK_EQUAL(0.25, udq_params.cmpEpsilon());

    // The reseed parameter is set to false, so the repeated calls to reseedRNG() should have
    // no effect.

    udq_params.reseedRNG(100);
    auto r1 = udq_params.true_rng()();
    udq_params.reseedRNG(100);
    auto r2 = udq_params.true_rng()();

    BOOST_CHECK( r1 != r2 );
}


BOOST_AUTO_TEST_CASE(ENUM_CONVERSION) {
    BOOST_CHECK_THROW(UDQ::varType("WWCT"), std::invalid_argument);
    BOOST_CHECK_THROW(UDQ::varType("XUCT"), std::invalid_argument);

    BOOST_CHECK(UDQ::varType("WUBHP") == UDQVarType::WELL_VAR);
    BOOST_CHECK(UDQ::varType("GUBHP") == UDQVarType::GROUP_VAR);
    BOOST_CHECK(UDQ::varType("CUBHP") == UDQVarType::CONNECTION_VAR);
    BOOST_CHECK(UDQ::varType("FUBHP") == UDQVarType::FIELD_VAR);
    BOOST_CHECK(UDQ::varType("RUBHP") == UDQVarType::REGION_VAR);
    BOOST_CHECK(UDQ::varType("AUBHP") == UDQVarType::AQUIFER_VAR);
    BOOST_CHECK(UDQ::varType("SUBHP") == UDQVarType::SEGMENT_VAR);
}



BOOST_AUTO_TEST_CASE(UDQ_KEWYORDS) {
    const std::string input = R"(
RUNSPEC

UDQDIMS
   10* 'Y'/

UDQPARAM
  3* 0.25 /

SCHEDULE

UDQ
  ASSIGN WUBHP 0.0 /
  UNITS  WUBHP 'BARSA' /
  DEFINE FUOPR  AVEG(WOPR) + 1/
  ASSIGN WUXUNIT 0.0 /
/

DATES
  10 'JAN' 2010 /
/

UDQ
  ASSIGN WUBHP 0.0 /
  DEFINE FUOPR  AVEG(WOPR) + 1/
  UNITS  WUBHP 'BARSA' /  -- Repeating the same unit multiple times is superfluous but OK
/
)";

    auto schedule = make_schedule(input);
    const auto& udq = schedule.getUDQConfig(0);
    BOOST_CHECK_EQUAL(1, udq.expressions().size());

    BOOST_CHECK_THROW( udq.unit("NO_SUCH_KEY"), std::invalid_argument );
    BOOST_CHECK_EQUAL( udq.unit("WUBHP"), "BARSA");
    BOOST_CHECK( udq.has_keyword("WUBHP") );
    BOOST_CHECK( !udq.has_keyword("NO_SUCH_KEY") );
    BOOST_CHECK( !udq.has_unit("WUXUNIT"));
    BOOST_CHECK( udq.has_unit("WUBHP"));

    Parser parser;
    auto deck = parser.parseString(input);
    auto udq_params1 = UDQParams(deck);
    BOOST_CHECK_EQUAL(0.25, udq_params1.cmpEpsilon());
    auto& sim_rng1 = udq_params1.sim_rng();
    auto& true_rng1 = udq_params1.true_rng();

    auto udq_params2 = UDQParams(deck);
    auto& sim_rng2 = udq_params2.sim_rng();
    auto& true_rng2 = udq_params2.true_rng();

    BOOST_CHECK( sim_rng1() == sim_rng2() );
    BOOST_CHECK( true_rng1() != true_rng2() );

    udq_params1.reseedRNG(100);
    udq_params2.reseedRNG(100);
    BOOST_CHECK( true_rng1() == true_rng2() );
}

BOOST_AUTO_TEST_CASE(UDQ_CHANGE_UNITS_ILLEGAL) {
  const std::string input = R"(
RUNSPEC

UDQDIMS
   10* 'Y'/

UDQPARAM
  3* 0.25 /

SCHEDULE

UDQ
  ASSIGN WUBHP 0.0 /
  UNITS  WUBHP 'BARSA' /
  DEFINE FUOPR  AVEG(WOPR) + 1/
/

DATES
  10 'JAN' 2010 /
/

UDQ
  ASSIGN WUBHP 0.0 /
  DEFINE FUOPR  AVEG(WOPR) + 1/
  UNITS  WUBHP 'HOURS' /  -- Changing unit runtime is *not* supported
/
)";

  BOOST_CHECK_THROW( make_schedule(input), std::invalid_argument);
}






BOOST_AUTO_TEST_CASE(UDQ_KEYWORD) {
    // Invalid action
    BOOST_REQUIRE_THROW( UDQ::actionType("INVALID_ACTION"), std::invalid_argument);

    // Invalid keyword
    BOOST_REQUIRE_THROW( UDQExpression(UDQAction::ASSIGN, "INVALID_KEYWORD", {}), std::invalid_argument);

    BOOST_CHECK_NO_THROW(UDQExpression(UDQAction::ASSIGN ,"WUBHP", {"1"}));
}


BOOST_AUTO_TEST_CASE(UDQ_DEFINE_DATA) {
    const std::string input = R"(
RUNSPEC

UDQDIMS
   10* 'Y'/

UDQPARAM
  3* 0.25 /

SCHEDULE

UDQ
DEFINE CUMW1 P12 10 12 1 (4.0 + 6*(4 - 2)) /
DEFINE WUMW1 WBHP 'P*1*' UMAX WBHP 'P*4*' /
/


)";
    const auto schedule = make_schedule(input);
    const auto& udq = schedule.getUDQConfig(0);
    const auto& records = udq.expressions();
    const auto& rec0 = records[0];
    const auto& rec1 = records[1];
    const std::vector<std::string> exp0 = {"P12", "10", "12", "1", "(", "4.0", "+", "6", "*", "(", "4", "-", "2", ")", ")"};
    const std::vector<std::string> exp1 = {"WBHP", "P*1*", "UMAX", "WBHP" , "P*4*"};
    BOOST_CHECK_EQUAL_COLLECTIONS(rec0.tokens().begin(), rec0.tokens().end(), exp0.begin(), exp0.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(rec1.tokens().begin(), rec1.tokens().end(), exp1.begin(), exp1.end());
}


BOOST_AUTO_TEST_CASE(UDQ_DEFINE_WITH_SLASH) {
    const std::string input = R"(
UDQ
 DEFINE WUWCT WWPR / ( WWPR + WOPR ) /
/


)";
    Parser parser;
    auto deck = parser.parseString(input);
    const auto& udq = deck.getKeyword("UDQ");
    const auto& record = udq.getRecord(0);
    const auto& data_item = record.getItem("DATA");
    const auto& data = data_item.getData<std::string>();
    std::vector<std::string> exp = {"WWPR", "/", "(", "WWPR", "+", "WOPR", ")"};
    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(),
                                  exp.begin(), exp.end());
}


BOOST_AUTO_TEST_CASE(UDQ_ASSIGN_DATA) {
    const std::string input = R"(
RUNSPEC

UDQDIMS
   10* 'Y'/

UDQPARAM
  3* 0.25 /

SCHEDULE

UDQ
ASSIGN WU1 P12 4.0 /
ASSIGN WU2 8.0 /
/


)";
    const auto schedule = make_schedule(input);
    const auto& udq = schedule.getUDQConfig(0);
    const auto& assignments = udq.assignments();
    const auto& ass0 = assignments[0];
    const auto& ass1 = assignments[1];


    BOOST_CHECK_EQUAL(ass0.keyword(), "WU1");
    BOOST_CHECK_EQUAL(ass1.keyword(), "WU2");

    BOOST_CHECK_EQUAL(ass0.value(), 4.0 );
    BOOST_CHECK_EQUAL(ass1.value(), 8.0 );

    std::vector<std::string> sel0 = {"P12"};
    std::vector<std::string> sel1 = {};
    BOOST_CHECK_EQUAL_COLLECTIONS(ass0.selector().begin(), ass0.selector().end(), sel0.begin(), sel0.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(ass1.selector().begin(), ass1.selector().end(), sel1.begin(), sel1.end());
}




BOOST_AUTO_TEST_CASE(UDQ_CONTEXT) {
    SummaryState st;
    UDQFunctionTable func_table;
    UDQContext ctx(func_table, st);
    BOOST_CHECK_EQUAL(ctx.get("JAN"), 1.0);

    BOOST_REQUIRE_THROW(ctx.get("NO_SUCH_KEY"), std::out_of_range);

    for (std::string& key : std::vector<std::string>({"ELAPSED", "MSUMLINS", "MSUMNEWT", "NEWTON", "TCPU", "TIME", "TIMESTEP"}))
        BOOST_CHECK_NO_THROW( ctx.get(key) );

    st.add("SUMMARY:KEY", 1.0);
    BOOST_CHECK_EQUAL(ctx.get("SUMMARY:KEY") , 1.0 );
}

BOOST_AUTO_TEST_CASE(UDQ_SET) {
    UDQSet s1("NAME", 5);

    for (const auto& v : s1) {
        BOOST_CHECK_EQUAL(false, v.defined());
        BOOST_REQUIRE_THROW( v.value(), std::invalid_argument);
    }
    BOOST_CHECK_EQUAL(s1.defined_size(), 0);

    s1.assign(1);
    for (const auto& v : s1) {
        BOOST_CHECK_EQUAL(true, v.defined());
        BOOST_CHECK_EQUAL( v.value(), 1.0);
    }
    BOOST_CHECK_EQUAL(s1.defined_size(), s1.size());

    s1.assign(0,0.0);
    {
        UDQSet s2("NAME", 6);
        BOOST_REQUIRE_THROW(s1 + s2, std::invalid_argument);
    }
    {
        UDQSet s2("NAME", 5);
        s2.assign(0, 25);
        auto s3 = s1 + s2;

        auto v0 = s3[0];
        BOOST_CHECK_EQUAL(v0.value(), 25);

        auto v4 = s3[4];
        BOOST_CHECK( !v4.defined() );
    }
    s1.assign(0,1.0);
    {
        UDQSet s2 = s1 + 1.0;
        UDQSet s3 = s2 * 2.0;
        UDQSet s4 = s1 - 1.0;
        for (const auto& v : s2) {
            BOOST_CHECK_EQUAL(true, v.defined());
            BOOST_CHECK_EQUAL( v.value(), 2.0);
        }

        for (const auto& v : s3) {
            BOOST_CHECK_EQUAL(true, v.defined());
            BOOST_CHECK_EQUAL( v.value(), 4.0);
        }

        for (const auto& v : s4) {
            BOOST_CHECK_EQUAL(true, v.defined());
            BOOST_CHECK_EQUAL( v.value(), 0);
        }
    }
}


BOOST_AUTO_TEST_CASE(UDQ_FUNCTION_TABLE) {
    UDQParams params;
    UDQFunctionTable udqft(params);
    BOOST_CHECK(udqft.has_function("SUM"));
    BOOST_CHECK(!udqft.has_function("NO_SUCH_FUNCTION"));
    UDQSet arg("NAME", 5);
    arg.assign(0,1);
    arg.assign(2,2);
    arg.assign(4,4);
    {
        const auto& func = dynamic_cast<const UDQScalarFunction&>(udqft.get("SUM"));
        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL(result.value(), 7);
    }
    {
        const auto& func = dynamic_cast<const UDQScalarFunction&>(udqft.get("NORM1"));
        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL(result.value(), 7);
    }
    {
        const auto& func = dynamic_cast<const UDQScalarFunction&>(udqft.get("NORM2"));
        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL(result.value(), std::sqrt(1 + 4+ 16));
    }
    {
        const auto& func = dynamic_cast<const UDQScalarFunction&>(udqft.get("NORMI"));
        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL(result.value(), 4);
    }
    {
        const auto& func = dynamic_cast<const UDQScalarFunction&>(udqft.get("MIN"));
        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL(result.value(), 1);
    }
    {
        const auto& func = dynamic_cast<const UDQScalarFunction&>(udqft.get("MAX"));
        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL(result.value(), 4);
    }
    {
        const auto& func = dynamic_cast<const UDQScalarFunction&>(udqft.get("AVEA"));
        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL(result.value(), 7.0/3);
    }
    {
        const auto& func = dynamic_cast<const UDQScalarFunction&>(udqft.get("AVEG"));
        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL(result.value(), std::exp((std::log(1) + std::log(2.0) + std::log(4))/3));
    }
    {
        const auto& func = dynamic_cast<const UDQScalarFunction&>(udqft.get("PROD"));
        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL(result.value(), 8.0);
    }
    {
        UDQSet arg2("NAME", 4);
        arg2.assign(0,1);
        arg2.assign(2,4);
        arg2.assign(3,4);
        const auto& func = dynamic_cast<const UDQScalarFunction&>(udqft.get("AVEH"));
        auto result = func.eval(arg2);
        BOOST_CHECK_EQUAL(result.value(), 2.0);
    }
}

BOOST_AUTO_TEST_CASE(CMP_FUNCTIONS) {
    UDQParams params;
    UDQFunctionTable udqft(params);
    UDQSet arg1("NAME", 5);
    UDQSet arg2("NAME", 5);
    UDQSet arg3("NAME", 3);
    arg1.assign(1,1);

    arg1.assign(0,1);
    arg1.assign(2,2);
    arg1.assign(4,4);

    arg2.assign(0, 0.9);
    arg2.assign(2, 2.5);
    arg2.assign(4, 4.0);

    BOOST_CHECK_THROW(UDQBinaryFunction::EQ(0.25, arg1, arg3), std::invalid_argument);


    {
        auto result = UDQBinaryFunction::EQ(0, arg1, arg2);

        BOOST_CHECK_EQUAL( result.defined_size(), 3 );
        BOOST_CHECK_EQUAL( result[0].value(), 0);
        BOOST_CHECK_EQUAL( result[2].value(), 0);
        BOOST_CHECK_EQUAL( result[4].value(), 1);

        result = UDQBinaryFunction::EQ(0.20, arg1, arg2);
        BOOST_CHECK_EQUAL( result[0].value(), 1);
        BOOST_CHECK_EQUAL( result[2].value(), 0);
        BOOST_CHECK_EQUAL( result[4].value(), 1);

        const auto& func = dynamic_cast<const UDQBinaryFunction&>(udqft.get("=="));
        result = func.eval(arg1, arg2);
        BOOST_CHECK_EQUAL( result[0].value(), 0);
        BOOST_CHECK_EQUAL( result[2].value(), 0);
        BOOST_CHECK_EQUAL( result[4].value(), 1);
    }
    {
        const auto& func = dynamic_cast<const UDQBinaryFunction&>(udqft.get("<"));
        auto result = func.eval(arg1, arg2);
        BOOST_CHECK_EQUAL( result.defined_size(), 3 );
        BOOST_CHECK_EQUAL( result[0].value(), 0);
        BOOST_CHECK_EQUAL( result[2].value(), 1);
        BOOST_CHECK_EQUAL( result[4].value(), 0);
    }
    {
        const auto& func = dynamic_cast<const UDQBinaryFunction&>(udqft.get(">"));
        auto result = func.eval(arg1, arg2);
        BOOST_CHECK_EQUAL( result.defined_size(), 3 );
        BOOST_CHECK_EQUAL( result[0].value(), 1);
        BOOST_CHECK_EQUAL( result[2].value(), 0);
        BOOST_CHECK_EQUAL( result[4].value(), 0);
    }
    {
        const auto& func = dynamic_cast<const UDQBinaryFunction&>(udqft.get("^"));
        UDQSet arg1("NAME", 4);
        UDQSet arg2("NAME", 4);

        for (std::size_t i=0; i < arg1.size(); i++) {
            arg1.assign(i, i + 1);
            arg2.assign(i, 2);
        }
        auto result = func.eval(arg1, arg2);
        for (std::size_t i=0; i < arg1.size(); i++)
            BOOST_CHECK_EQUAL( result[i].value(), (i+1)*(i+1));
    }
    {
        auto result = UDQBinaryFunction::GE(1.0, arg1, arg2);
        BOOST_CHECK_EQUAL( result[0].value(), 1);

        // This is bisarre - but due to the large epsilon 2 and 2.5 compare as
        // equal; and then we evaluate 2 >= 2.5 as TRUE!
        BOOST_CHECK_EQUAL( result[2].value(), 1);
        BOOST_CHECK_EQUAL( result[4].value(), 1);
    }
    {
        const auto& func = dynamic_cast<const UDQBinaryFunction&>(udqft.get("<="));
        auto result = func.eval(arg1, arg2);
        BOOST_CHECK_EQUAL( result[0].value(), 0);
        BOOST_CHECK_EQUAL( result[2].value(), 1);
        BOOST_CHECK_EQUAL( result[4].value(), 1);


    }
}

BOOST_AUTO_TEST_CASE(BAD_CAST) {
    UDQParams params;
    UDQFunctionTable udqft(params);

    BOOST_CHECK_THROW( dynamic_cast<const UDQUnaryElementalFunction&>(udqft.get("==")), std::bad_cast);
}


BOOST_AUTO_TEST_CASE(ELEMENTAL_UNARY_FUNCTIONS) {
    UDQParams params;
    UDQFunctionTable udqft(params);
    UDQSet arg("NAME", 5);
    arg.assign(0,1);
    arg.assign(2,2);
    arg.assign(4,4);

    {
        const auto& func = dynamic_cast<const UDQUnaryElementalFunction&>(udqft.get("ABS"));
        UDQSet arg2("NAME", 5);
        arg2.assign(0,1);
        arg2.assign(2,-2);
        arg2.assign(4,4);
        auto result = func.eval(arg2);
        BOOST_CHECK_EQUAL( result[0].value(), 1);
        BOOST_CHECK_EQUAL( result[2].value(), 2);
        BOOST_CHECK_EQUAL( result[4].value(), 4);
    }
    {
        const auto& func = dynamic_cast<const UDQUnaryElementalFunction&>(udqft.get("DEF"));
        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL( result[0].value(), 1);
        BOOST_CHECK_EQUAL( result[2].value(), 1);
        BOOST_CHECK_EQUAL( result[4].value(), 1);
    }
    {
        const auto& func = dynamic_cast<const UDQUnaryElementalFunction&>(udqft.get("UNDEF"));
        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL( result[1].value(), 1);
        BOOST_CHECK_EQUAL( result[3].value(), 1);
        BOOST_CHECK_EQUAL( result.defined_size(), 2);
    }
    {
        const auto& func = dynamic_cast<const UDQUnaryElementalFunction&>(udqft.get("EXP"));
        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL( result[0].value(), std::exp(1));
        BOOST_CHECK_EQUAL( result[2].value(), std::exp(2));
        BOOST_CHECK_EQUAL( result[4].value(), std::exp(4));
    }
    {
        const auto& func = dynamic_cast<const UDQUnaryElementalFunction&>(udqft.get("IDV"));
        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL( result[0].value(), 1);
        BOOST_CHECK_EQUAL( result[1].value(), 0);
        BOOST_CHECK_EQUAL( result[2].value(), 1);
        BOOST_CHECK_EQUAL( result[3].value(), 0);
        BOOST_CHECK_EQUAL( result[4].value(), 1);
    }
    {
        const auto& func = dynamic_cast<const UDQUnaryElementalFunction&>(udqft.get("LOG"));
        UDQSet arg("NAME", 3);
        arg.assign(0, 10);
        arg.assign(2,1000);

        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL( result[0].value(), 1);
        BOOST_CHECK( !result[1] );
        BOOST_CHECK_EQUAL( result[2].value(), 3);
    }
    {
        const auto& func = dynamic_cast<const UDQUnaryElementalFunction&>(udqft.get("NINT"));
        UDQSet arg("NAME", 3);
        arg.assign(0, 0.75);
        arg.assign(2, 1.25);

        auto result = func.eval(arg);
        BOOST_CHECK_EQUAL( result[0].value(), 1);
        BOOST_CHECK( !result[1] );
        BOOST_CHECK_EQUAL( result[2].value(), 1);
    }
    {
        const auto& func = dynamic_cast<const UDQUnaryElementalFunction&>(udqft.get("RANDN"));
        UDQSet arg("NAME", 3);
        arg.assign(0, -1.0);
        arg.assign(2, -1.0);

        auto result1 = func.eval(arg);
        auto result2 = func.eval(arg);
        BOOST_CHECK( result1[0].value() != -1.0);
        BOOST_CHECK( !result1[1] );
        BOOST_CHECK( result1[2].value() != -1.0);

        BOOST_CHECK( result1[0].value() != result2[0].value());
        BOOST_CHECK( result1[2].value() != result2[2].value());
    }
    {
        const auto& func = dynamic_cast<const UDQUnaryElementalFunction&>(udqft.get("SORTA"));
        auto result = func.eval(arg);

        BOOST_CHECK_EQUAL( result[0].value(), 0);
        BOOST_CHECK( !result[1] );
        BOOST_CHECK_EQUAL( result[2].value(), 1);
        BOOST_CHECK( !result[3] );
        BOOST_CHECK_EQUAL( result[4].value(), 2);
    }
    {
        const auto& func = dynamic_cast<const UDQUnaryElementalFunction&>(udqft.get("SORTD"));
        auto result = func.eval(arg);

        BOOST_CHECK_EQUAL( result[0].value(), 2);
        BOOST_CHECK( !result[1] );
        BOOST_CHECK_EQUAL( result[2].value(), 1);
        BOOST_CHECK( !result[3] );
        BOOST_CHECK_EQUAL( result[4].value(), 0);
    }
}


BOOST_AUTO_TEST_CASE(UNION_FUNCTIONS) {
    UDQParams params;
    UDQFunctionTable udqft(params);
    UDQSet arg1("NAME", 5);
    UDQSet arg2("NAME", 5);

    arg1.assign(0,1);
    arg1.assign(2,2);

    arg2.assign(0, 1.0);
    arg2.assign(3, 3 );

    const auto& func = dynamic_cast<const UDQBinaryFunction&>(udqft.get("UADD"));
    auto result = func.eval(arg1, arg2);
    BOOST_CHECK_EQUAL( 3, result.defined_size() );
    BOOST_CHECK_EQUAL( 2, result[0].value() );
    BOOST_CHECK_EQUAL( 2, result[2].value() );
    BOOST_CHECK_EQUAL( 3, result[3].value() );
}




BOOST_AUTO_TEST_CASE(FUNCTIONS_INVALID_ARGUMENT) {
    UDQSet arg("NAME",3);
    arg.assign(0, -1);
    BOOST_REQUIRE_THROW( UDQScalarFunction::AVEG(arg), std::invalid_argument);
    BOOST_REQUIRE_THROW( UDQUnaryElementalFunction::LOG(arg), std::invalid_argument);
    BOOST_REQUIRE_THROW( UDQUnaryElementalFunction::LN(arg), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(UDQ_SET_DIV) {
    UDQSet s("NAME", 5);
    s.assign(0,1);
    s.assign(2,2);
    s.assign(4,5);

    auto result = 10 / s;
    BOOST_CHECK_EQUAL( result.defined_size(), 3);
    BOOST_CHECK_EQUAL( result[0].value(), 10);
    BOOST_CHECK_EQUAL( result[2].value(), 5);
    BOOST_CHECK_EQUAL( result[4].value(), 2);
}




BOOST_AUTO_TEST_CASE(UDQASSIGN_TEST) {
    UDQAssign as1("WUPR", {}, 1.0);
    UDQAssign as2("WUPR", {"P*"}, 2.0);
    UDQAssign as3("WUPR", {"P1"}, 4.0);
    std::vector<std::string> ws1 = {"P1", "P2", "I1", "I2"};

    auto res1 = as1.eval_wells(ws1);
    BOOST_CHECK_EQUAL(res1.size(), 4);
    BOOST_CHECK_EQUAL(res1["P1"].value(), 1.0);
    BOOST_CHECK_EQUAL(res1["I2"].value(), 1.0);

    auto res2 = as2.eval_wells(ws1);
    BOOST_CHECK_EQUAL(res2["P1"].value(), 2.0);
    BOOST_CHECK_EQUAL(res2["P2"].value(), 2.0);
    BOOST_CHECK(!res2["I1"].defined());
    BOOST_CHECK(!res2["I2"].defined());

    auto res3 = as3.eval_wells(ws1);
    BOOST_CHECK_EQUAL(res3["P1"].value(), 4.0);
    BOOST_CHECK(!res3["P2"].defined());
    BOOST_CHECK(!res3["I1"].defined());
    BOOST_CHECK(!res3["I2"].defined());
}
