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

    DeckItemConstPtr item0 = record0->getItem("TABLEROW");
    BOOST_CHECK_EQUAL(10U , item0->size());
    BOOST_CHECK_EQUAL(1U , record0->size());

    DeckItemConstPtr item1 = record1->getItem("TABLEROW");
    BOOST_CHECK_EQUAL(10U , item1->size());
    BOOST_CHECK_EQUAL(1U , record1->size());

    DeckItemConstPtr item2 = record2->getItem("TABLEROW");
    BOOST_CHECK_EQUAL(1U , record2->size());
    BOOST_CHECK_EQUAL(0U , item2->size());

    DeckItemConstPtr item3 = record3->getItem("TABLEROW");
    BOOST_CHECK_EQUAL(10U , item3->size());
    BOOST_CHECK_EQUAL(1U , record3->size());

    DeckItemConstPtr item4 = record4->getItem("TABLEROW");
    BOOST_CHECK_EQUAL(10U , item4->size());
    BOOST_CHECK_EQUAL(1U , record4->size());

}


BOOST_AUTO_TEST_CASE( parse_PVTG_OK ) {
    ParserPtr parser2(new Parser());
    ParserPtr parser(new Parser(false));
    ParserKeywordPtr tabdimsKeyword( new ParserKeyword("TABDIMS" , 1));
    ParserKeywordPtr pvtgKeyword( new ParserKeyword("PVTG" , "TABDIMS" , "NTPVT" , INTERNALIZE , true));
    {
        ParserIntItemConstPtr item(new ParserIntItem(std::string("NTSFUN")));
        tabdimsKeyword->addItem(item);
    }

    {
        ParserIntItemConstPtr item(new ParserIntItem(std::string("NTPVT")));
        tabdimsKeyword->addItem(item);
    }

    {
        ParserIntItemConstPtr item(new ParserIntItem(std::string("NSSFUN")));
        tabdimsKeyword->addItem(item);
    }

    {
        ParserIntItemConstPtr item(new ParserIntItem(std::string("NPPVT")));
        tabdimsKeyword->addItem(item);
    }

    {
        ParserIntItemConstPtr item(new ParserIntItem(std::string("NTFIP")));
        tabdimsKeyword->addItem(item);
    }

    {
        ParserIntItemConstPtr item(new ParserIntItem(std::string("NRPVT")));
        tabdimsKeyword->addItem(item);
    }

    {
        ParserDoubleItemConstPtr item(new ParserDoubleItem(std::string("TABLEROW") , ALL));
        pvtgKeyword->addItem(item);
    }

    parser->addKeyword( tabdimsKeyword );
    parser->addKeyword( pvtgKeyword );

    check_parser( parser );
    check_parser( parser2 );
}
