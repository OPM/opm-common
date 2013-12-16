#define BOOST_TEST_MODULE ParserIntegrationTests
#include <math.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckDoubleItem.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>

#include <opm/parser/eclipse/Utility/SimpleTable.hpp>

using namespace Opm;


void check_parser(ParserPtr parser) {
    boost::filesystem::path pvtgFile("testdata/integration_tests/SWOF/SWOF.txt");
    DeckPtr deck = parser->parse(pvtgFile.string());
    DeckKeywordConstPtr kw1 = deck->getKeyword("SWOF" , 0);
    BOOST_CHECK_EQUAL(1U , kw1->size());

    DeckRecordConstPtr record0 = kw1->getRecord(0);
    BOOST_CHECK_EQUAL(1U , record0->size());

    DeckItemConstPtr item0 = record0->getItem(0);
    BOOST_CHECK_EQUAL(11U * 4, item0->size());
}

void check_SimpleTable(ParserPtr parser) {
    boost::filesystem::path pvtgFile("testdata/integration_tests/SWOF/SWOF.txt");
    DeckPtr deck = parser->parse(pvtgFile.string());
    DeckKeywordConstPtr kw1 = deck->getKeyword("SWOF" , 0);

    std::vector<std::string> columnNames{"SW", "KRW", "KROW", "PCOW"};
    Opm::SimpleTable table(kw1, columnNames);
}

BOOST_AUTO_TEST_CASE( parse_SWOF_OK ) {
    ParserPtr parser2(new Parser(/*addDefault=*/true));
    ParserPtr parser(new Parser(/*addDefault=*/false));

    ParserKeywordPtr tabdimsKeyword( new ParserKeyword("TABDIMS" , 1));
    tabdimsKeyword->addItem(Opm::ParserItemPtr(new ParserIntItem("NTSFUN")));
    tabdimsKeyword->addItem(Opm::ParserItemPtr(new ParserIntItem("NTPVT")));
    tabdimsKeyword->addItem(Opm::ParserItemPtr(new ParserIntItem("NSSFUN")));
    tabdimsKeyword->addItem(Opm::ParserItemPtr(new ParserIntItem("NPPVT")));
    tabdimsKeyword->addItem(Opm::ParserItemPtr(new ParserIntItem("NTFIP")));
    tabdimsKeyword->addItem(Opm::ParserItemPtr(new ParserIntItem("NRPVT")));

    ParserKeywordPtr swofKeyword( new ParserKeyword("SWOF" , "TABDIMS" , "NTSFUN" , INTERNALIZE , true));
    swofKeyword->addItem(Opm::ParserItemPtr(new ParserDoubleItem("TABLEROW", ALL)));

    parser->addKeyword( tabdimsKeyword );
    parser->addKeyword( swofKeyword );

    check_parser( parser );
    check_parser( parser2 );

    check_SimpleTable(parser);
}
