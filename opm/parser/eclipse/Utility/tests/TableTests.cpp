/*
  Copyright (C) 2013 by Andreas Lauser

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
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

// generic table classes
#include <opm/parser/eclipse/Utility/SingleRecordTable.hpp>
#include <opm/parser/eclipse/Utility/MultiRecordTable.hpp>
#include <opm/parser/eclipse/Utility/FullTable.hpp>

// keyword specific table classes
#include <opm/parser/eclipse/Utility/PvtoTable.hpp>
#include <opm/parser/eclipse/Utility/SwofTable.hpp>
#include <opm/parser/eclipse/Utility/SgofTable.hpp>

#define BOOST_TEST_MODULE SingleRecordTableTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/filesystem.hpp>

#include <stdexcept>
#include <iostream>

BOOST_AUTO_TEST_CASE(CreateSingleRecordTable) {
    const char *deckData =
        "TABDIMS\n"
        " 2 /\n"
        "\n"
        "SWOF\n"
        " 1 2 3 4\n"
        " 5 6 7 8 /\n"
        " 9 10 11 12 /\n";

    Opm::ParserPtr parser(new Opm::Parser);
    Opm::DeckConstPtr deck(parser->parseString(deckData));

    std::vector<std::string> tooFewColumnNames{"A", "B", "C"};
    std::vector<std::string> justRightColumnNames{"A", "B", "C", "D"};
    std::vector<std::string> tooManyColumnNames{"A", "B", "C", "D", "E"};

    BOOST_CHECK_EQUAL(Opm::SingleRecordTable::numTables(deck->getKeyword("SWOF")), 2);
    Opm::SingleRecordTable tmpTable;
    BOOST_CHECK_THROW(tmpTable.init(deck->getKeyword("SWOF"),
                                    tooFewColumnNames,
                                    /*recordIdx=*/0,
                                    /*firstEntryOffset=*/0),
                      std::runtime_error);
    BOOST_CHECK_THROW(tmpTable.init(deck->getKeyword("SWOF"),
                                    tooManyColumnNames,
                                    /*recordIdx=*/0,
                                    /*firstEntryOffset=*/0),
                      std::runtime_error);

    tmpTable.init(deck->getKeyword("SWOF"),
                  justRightColumnNames,
                  /*recordIdx=*/0,
                  /*firstEntryOffset=*/0);
}

BOOST_AUTO_TEST_CASE(CreateMultiTable) {
    const char *deckData =
        "TABDIMS\n"
        "1 2 /\n"
        "\n"
        "PVTO\n"
        " 1 2 3 4"
        "   5 6 7/\n"
        " 8 9 10 11 /\n"
        "/\n"
        "12 13 14 15\n"
        "   16 17 18/\n"
        "19 20 21 22/\n"
        "/\n";

    Opm::ParserPtr parser(new Opm::Parser);
    Opm::DeckConstPtr deck(parser->parseString(deckData));

    std::vector<std::string> tooFewColumnNames{"A", "B", "C"};
    std::vector<std::string> justRightColumnNames{"A", "B", "C", "D"};
    std::vector<std::string> tooManyColumnNames{"A", "B", "C", "D", "E"};

    BOOST_CHECK_EQUAL(Opm::MultiRecordTable::numTables(deck->getKeyword("PVTO")), 2);
    // this mistake can't be detected as the MultiRecordTable takes
    // the first $N items as the column names...
    /*
    BOOST_CHECK_THROW(Opm::MultiRecordTable(deck->getKeyword("PVTO"),
                                                  tooFewColumnNames,
                                                  0),
                      std::runtime_error);
    */
    Opm::MultiRecordTable mrt;
    BOOST_CHECK_THROW(mrt.init(deck->getKeyword("PVTO"),
                               tooManyColumnNames,
                               /*tableIdx=*/0,
                               /*firstEntryOffset=*/0),
                      std::runtime_error);

    BOOST_CHECK_NO_THROW(mrt.init(deck->getKeyword("PVTO"),
                                  justRightColumnNames,
                                  /*recordIdx=*/0,
                                  /*firstEntryOffset=*/0));
}

BOOST_AUTO_TEST_CASE(SwofTable_Tests) {
    const char *deckData =
        "TABDIMS\n"
        "2 /\n"
        "\n"
        "SWOF\n"
        " 1 2 3 4\n"
        " 5 6 7 8/\n"
        "  9 10 11 12\n"
        " 13 14 15 16\n"
        " 17 18 19 20/\n";

    Opm::ParserPtr parser(new Opm::Parser);
    Opm::DeckConstPtr deck(parser->parseString(deckData));
    Opm::DeckKeywordConstPtr swofKeyword = deck->getKeyword("SWOF");

    BOOST_CHECK_EQUAL(Opm::SwofTable::numTables(swofKeyword), 2);

    Opm::SwofTable swof1Table;
    Opm::SwofTable swof2Table;

    swof1Table.init(deck->getKeyword("SWOF"), /*tableIdx=*/0);
    swof2Table.init(deck->getKeyword("SWOF"), /*tableIdx=*/1);

    BOOST_CHECK_EQUAL(swof1Table.numRows(), 2);
    BOOST_CHECK_EQUAL(swof2Table.numRows(), 3);

    BOOST_CHECK_EQUAL(swof1Table.numColumns(), 4);
    BOOST_CHECK_EQUAL(swof2Table.numColumns(), 4);

    BOOST_CHECK_EQUAL(swof1Table.getSwColumn().front(), 1.0);
    BOOST_CHECK_EQUAL(swof1Table.getSwColumn().back(), 5.0);

    BOOST_CHECK_EQUAL(swof1Table.getKrwColumn().front(), 2.0);
    BOOST_CHECK_EQUAL(swof1Table.getKrwColumn().back(), 6.0);

    BOOST_CHECK_EQUAL(swof1Table.getKrowColumn().front(), 3.0);
    BOOST_CHECK_EQUAL(swof1Table.getKrowColumn().back(), 7.0);

    BOOST_CHECK_EQUAL(swof1Table.getPcowColumn().front(), 4.0e5);
    BOOST_CHECK_EQUAL(swof1Table.getPcowColumn().back(), 8.0e5);

    // for the second table, we only check the first column and trust
    // that everything else is fine...
    BOOST_CHECK_EQUAL(swof2Table.getSwColumn().front(), 9.0);
    BOOST_CHECK_EQUAL(swof2Table.getSwColumn().back(), 17.0);
}

BOOST_AUTO_TEST_CASE(SgofTable_Tests) {
    const char *deckData =
        "TABDIMS\n"
        "2 /\n"
        "\n"
        "SGOF\n"
        " 1 2 3 4\n"
        " 5 6 7 8/\n"
        "  9 10 11 12\n"
        " 13 14 15 16\n"
        " 17 18 19 20/\n";

    Opm::ParserPtr parser(new Opm::Parser);
    Opm::DeckConstPtr deck(parser->parseString(deckData));
    Opm::DeckKeywordConstPtr sgofKeyword = deck->getKeyword("SGOF");

    BOOST_CHECK_EQUAL(Opm::SgofTable::numTables(sgofKeyword), 2);

    Opm::SgofTable sgof1Table;
    Opm::SgofTable sgof2Table;

    sgof1Table.init(deck->getKeyword("SGOF"), /*tableIdx=*/0);
    sgof2Table.init(deck->getKeyword("SGOF"), /*tableIdx=*/1);

    BOOST_CHECK_EQUAL(sgof1Table.numRows(), 2);
    BOOST_CHECK_EQUAL(sgof2Table.numRows(), 3);

    BOOST_CHECK_EQUAL(sgof1Table.numColumns(), 4);
    BOOST_CHECK_EQUAL(sgof2Table.numColumns(), 4);

    BOOST_CHECK_EQUAL(sgof1Table.getSgColumn().front(), 1.0);
    BOOST_CHECK_EQUAL(sgof1Table.getSgColumn().back(), 5.0);

    BOOST_CHECK_EQUAL(sgof1Table.getKrgColumn().front(), 2.0);
    BOOST_CHECK_EQUAL(sgof1Table.getKrgColumn().back(), 6.0);

    BOOST_CHECK_EQUAL(sgof1Table.getKrogColumn().front(), 3.0);
    BOOST_CHECK_EQUAL(sgof1Table.getKrogColumn().back(), 7.0);

    BOOST_CHECK_EQUAL(sgof1Table.getPcogColumn().front(), 4.0e5);
    BOOST_CHECK_EQUAL(sgof1Table.getPcogColumn().back(), 8.0e5);

    // for the second table, we only check the first column and trust
    // that everything else is fine...
    BOOST_CHECK_EQUAL(sgof2Table.getSgColumn().front(), 9.0);
    BOOST_CHECK_EQUAL(sgof2Table.getSgColumn().back(), 17.0);
}

BOOST_AUTO_TEST_CASE(PvtoTable_Tests) {
    const char *deckData =
        "TABDIMS\n"
        "1 2 /\n"
        "\n"
        "PVTO\n"
        " 1 2 3 4"
        "   5 6 7/\n"
        " 8 9 10 11 /\n"
        "/\n"
        "12 13 14 15\n"
        "   16 17 18/\n"
        "19 20 21 22/\n"
        "23 24 25 26/\n"
        "/\n";

    Opm::ParserPtr parser(new Opm::Parser);
    Opm::DeckConstPtr deck(parser->parseString(deckData));
    Opm::DeckKeywordConstPtr pvtoKeyword = deck->getKeyword("PVTO");

    BOOST_CHECK_EQUAL(Opm::PvtoTable::numTables(pvtoKeyword), 2);

    Opm::PvtoTable pvto1Table;
    Opm::PvtoTable pvto2Table;

    pvto1Table.init(deck->getKeyword("PVTO"), /*tableIdx=*/0);
    pvto2Table.init(deck->getKeyword("PVTO"), /*tableIdx=*/1);

    const auto pvto1OuterTable = pvto1Table.getOuterTable();
    const auto pvto2OuterTable = pvto2Table.getOuterTable();

    BOOST_CHECK_EQUAL(pvto1OuterTable->numRows(), 2);
    BOOST_CHECK_EQUAL(pvto2OuterTable->numRows(), 3);

    BOOST_CHECK_EQUAL(pvto1OuterTable->numColumns(), 4);
    BOOST_CHECK_EQUAL(pvto2OuterTable->numColumns(), 4);

    BOOST_CHECK_EQUAL(pvto1OuterTable->getGasSolubilityColumn().front(), 1.0);
    BOOST_CHECK_EQUAL(pvto1OuterTable->getGasSolubilityColumn().back(), 8.0);

    BOOST_CHECK_EQUAL(pvto1OuterTable->getPressureColumn().front(), 2.0e5);
    BOOST_CHECK_EQUAL(pvto1OuterTable->getPressureColumn().back(), 9.0e5);

    BOOST_CHECK_EQUAL(pvto1OuterTable->getOilFormationFactorColumn().front(), 3.0);
    BOOST_CHECK_EQUAL(pvto1OuterTable->getOilFormationFactorColumn().back(), 10.0);

    BOOST_CHECK_EQUAL(pvto1OuterTable->getOilViscosityColumn().front(), 4.0e-3);
    BOOST_CHECK_EQUAL(pvto1OuterTable->getOilViscosityColumn().back(), 11.0e-3);

    // for the second table, we only check the first column and trust
    // that everything else is fine...
    BOOST_CHECK_EQUAL(pvto2OuterTable->getGasSolubilityColumn().front(), 12.0);
    BOOST_CHECK_EQUAL(pvto2OuterTable->getGasSolubilityColumn().back(), 23.0);
}
