#define BOOST_TEST_MODULE ParserIntegrationTests
#include <math.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckDoubleItem.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>

#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

using namespace Opm;


void check_parser(ParserPtr parser) {

    boost::filesystem::path pvtgFile("testdata/integration_tests/PVTG/PVTG.txt");
    DeckPtr deck = parser->parse(pvtgFile.string());
    DeckKeywordConstPtr kw1 = deck->getKeyword("PVTG" , 0);
    BOOST_CHECK_EQUAL(5U , kw1->size());

    DeckRecordConstPtr record0 = kw1->getRecord(0);
    DeckRecordConstPtr record1 = kw1->getRecord(1);
    DeckRecordConstPtr record2 = kw1->getRecord(2);
    DeckRecordConstPtr record3 = kw1->getRecord(3);
    DeckRecordConstPtr record4 = kw1->getRecord(4);

    DeckItemConstPtr item0_0 = record0->getItem("GAS_PRESSURE");
    DeckItemConstPtr item0_1 = record0->getItem("DATA");
    BOOST_CHECK_EQUAL(1U , item0_0->size());
    BOOST_CHECK_EQUAL(9U , item0_1->size());
    BOOST_CHECK_EQUAL(2U , record0->size());

    DeckItemConstPtr item1_0 = record1->getItem("GAS_PRESSURE");
    DeckItemConstPtr item1_1 = record1->getItem("DATA");
    BOOST_CHECK_EQUAL(1U , item1_0->size());
    BOOST_CHECK_EQUAL(9U , item1_1->size());
    BOOST_CHECK_EQUAL(2U , record1->size());

    DeckItemConstPtr item2_0 = record2->getItem("GAS_PRESSURE");
    DeckItemConstPtr item2_1 = record2->getItem("DATA");
    BOOST_CHECK_EQUAL(1U , item2_0->size());
    BOOST_CHECK_EQUAL(0U , item2_1->size());
    BOOST_CHECK_EQUAL(2U , record2->size());


    DeckItemConstPtr item3_0 = record3->getItem("GAS_PRESSURE");
    DeckItemConstPtr item3_1 = record3->getItem("DATA");
    BOOST_CHECK_EQUAL(1U , item3_0->size());
    BOOST_CHECK_EQUAL(9U , item3_1->size());
    BOOST_CHECK_EQUAL(2U , record3->size());


    DeckItemConstPtr item4_0 = record4->getItem("GAS_PRESSURE");
    DeckItemConstPtr item4_1 = record4->getItem("DATA");
    BOOST_CHECK_EQUAL(1U , item4_0->size());
    BOOST_CHECK_EQUAL(9U , item4_1->size());
    BOOST_CHECK_EQUAL(2U , record4->size());




}


BOOST_AUTO_TEST_CASE( parse_PVTG_OK ) {
    ParserPtr parser(new Parser());
    check_parser( parser );
}
