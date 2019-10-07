


#define BOOST_TEST_MODULE DeckValueTests

#include <vector>

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/A.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/B.hpp>

#include <opm/parser/eclipse/Deck/DeckValue.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(DeckValueTest) {

    const DeckValue value0;

    BOOST_CHECK(value0.is_default());
    BOOST_CHECK(!value0.is_compatible<int>());
    BOOST_CHECK(!value0.is_compatible<std::string>());
    BOOST_CHECK(!value0.is_compatible<double>());
    BOOST_CHECK_THROW( value0.get<int>(), std::invalid_argument);
    BOOST_CHECK_THROW( value0.get<std::string>(), std::invalid_argument);
    BOOST_CHECK_THROW( value0.get<double>(), std::invalid_argument);

    DeckValue value1(10);
    BOOST_CHECK(!value1.is_default());
    BOOST_CHECK(value1.is_compatible<int>());
    BOOST_CHECK(value1.is_compatible<double>());
    BOOST_CHECK(!value1.is_compatible<std::string>());
    BOOST_CHECK_EQUAL( value1.get<int>(), 10);
    BOOST_CHECK_EQUAL( value1.get<double>(), 10);

    DeckValue value2(10.0);
    BOOST_CHECK(value2.is_compatible<double>());
    BOOST_CHECK(!value2.is_compatible<int>());
    BOOST_CHECK(!value2.is_compatible<std::string>());
    BOOST_CHECK_EQUAL( value2.get<double>(), 10);
    BOOST_CHECK_THROW( value2.get<std::string>(), std::invalid_argument);
    BOOST_CHECK_THROW( value2.get<int>(), std::invalid_argument);

    DeckValue value3("FUBHP");
    BOOST_CHECK(!value3.is_compatible<double>());
    BOOST_CHECK(value3.is_compatible<std::string>());
    BOOST_CHECK_EQUAL( value3.get<std::string>(), std::string("FUBHP"));
    BOOST_CHECK_THROW( value3.get<double>(), std::invalid_argument);
    BOOST_CHECK_THROW( value3.get<int>(), std::invalid_argument);


}


BOOST_AUTO_TEST_CASE(DeckKeywordConstructor) {
    
    Parser parser;

    const ParserKeyword& big_model = parser.getKeyword("BIGMODEL");
    BOOST_CHECK_THROW( DeckKeyword( big_model, {{DeckValue("WORD_A")}} ), std::invalid_argument );

    const ParserKeyword& box = parser.getKeyword("BOX");
    std::vector< DeckValue > record1 = {DeckValue(1), DeckValue(2), DeckValue(3), DeckValue(4), DeckValue(5), DeckValue(6)}; 
    BOOST_CHECK_NO_THROW( DeckKeyword( box, {record1} ) );
    //BOOST_CHECK_THROW( DeckKeyword( box, {record1, record1} ) , std::invalid_argument);

    const ParserKeyword& addreg = parser.getKeyword("ADDREG");

    BOOST_CHECK_NO_THROW( DeckKeyword( addreg, {{ DeckValue("WORD_A") }} ) );
    BOOST_CHECK_THROW( DeckKeyword( addreg, {{DeckValue("WORD_A"), DeckValue(77), DeckValue(16.25), DeckValue("WORD_B")}} ) , std::invalid_argument);

    std::vector< DeckValue > record = {DeckValue("WORD_A"), DeckValue(16.25), DeckValue(77), DeckValue("WORD_B")};
    DeckKeyword deck_kw(addreg, {record});

    BOOST_CHECK_EQUAL( deck_kw.size(), 1 );

    const DeckRecord& deck_record = deck_kw.getRecord(0);
    BOOST_CHECK_EQUAL( deck_record.size(), 4 );

    const auto& array = deck_record.getItem( 0 );
    const auto& shift = deck_record.getItem( 1 );
    const auto& number = deck_record.getItem( 2 );
    const auto& name = deck_record.getItem( 3 );

    BOOST_CHECK_EQUAL( array.get<std::string>(0), "WORD_A" );
    BOOST_CHECK_EQUAL( shift.get<double>(0), 16.25 );
    BOOST_CHECK_EQUAL( number.get<int>(0), 77 );
    BOOST_CHECK_EQUAL( name.get<std::string>(0), "WORD_B" );

    //checking default values:
    record = {DeckValue("WORD_A"), DeckValue(), DeckValue(77)};
    DeckKeyword deck_kw1(addreg, {record});

    const DeckRecord& deck_record1 = deck_kw1.getRecord(0);
    const auto& shift1 = deck_record1.getItem( 1 );
    const auto& name1 = deck_record1.getItem( 3 );
    BOOST_CHECK_EQUAL( shift1.get<double>(0), 0 );
    BOOST_CHECK_EQUAL( name1.get<std::string>(0), "M" );

    //check that int can substitute double
    BOOST_CHECK_NO_THROW( DeckKeyword(addreg, {{DeckValue("WORD_A"), DeckValue(5), DeckValue(77)}}   ) );

    
}
