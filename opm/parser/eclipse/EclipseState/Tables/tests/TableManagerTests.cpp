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

#define BOOST_TEST_MODULE SingleRecordTableTests

#include <opm/core/utility/platform_dependent/disable_warnings.h>
#include <boost/test/unit_test.hpp>
#include <opm/core/utility/platform_dependent/reenable_warnings.h>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

// generic table classes
#include <opm/parser/eclipse/EclipseState/Tables/SingleRecordTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/MultiRecordTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/FullTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>

// keyword specific table classes
#include <opm/parser/eclipse/EclipseState/Tables/PvtoTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SwofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlyadsTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/VFPProdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/VFPInjTable.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/filesystem.hpp>

#include <stdexcept>
#include <iostream>


std::shared_ptr<const Opm::Deck> createSingleRecordDeck() {
    const char *deckData =
        "TABDIMS\n"
        " 2 /\n"
        "\n"
        "SWOF\n"
        " 1 2 3 4\n"
        " 5 6 7 8 /\n"
        " 9 10 11 12 /\n";

    Opm::ParserPtr parser(new Opm::Parser);
    Opm::DeckConstPtr deck(parser->parseString(deckData, Opm::ParseMode()));
    return deck;
}



BOOST_AUTO_TEST_CASE( CreateTables ) {
    std::shared_ptr<const Opm::Deck> deck = createSingleRecordDeck();
    Opm::TableManager tables(*deck);
    auto tabdims = tables.getTabdims();
    BOOST_CHECK_EQUAL( tabdims->getNumSatTables() , 2 );
}

/*****************************************************************/

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
    Opm::DeckConstPtr deck(parser->parseString(deckData, Opm::ParseMode()));

    std::vector<std::string> tooFewColumnNames{"A", "B", "C"};
    std::vector<std::string> justRightColumnNames{"A", "B", "C", "D"};
    std::vector<std::string> tooManyColumnNames{"A", "B", "C", "D", "E"};

    BOOST_CHECK_EQUAL(Opm::SingleRecordTable::numTables(deck->getKeyword("SWOF")), 2);
    Opm::SingleRecordTable tmpTable;
    BOOST_CHECK_THROW(tmpTable.initFORUNITTESTONLY(deck->getKeyword("SWOF"),
                                                   tooFewColumnNames,
                                                   /*recordIdx=*/0,
                                                   /*firstEntryOffset=*/0),
                      std::runtime_error);
    BOOST_CHECK_THROW(tmpTable.initFORUNITTESTONLY(deck->getKeyword("SWOF"),
                                                   tooManyColumnNames,
                                                   /*recordIdx=*/0,
                                                   /*firstEntryOffset=*/0),
                      std::runtime_error);

    tmpTable.initFORUNITTESTONLY(deck->getKeyword("SWOF"),
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
    Opm::DeckConstPtr deck(parser->parseString(deckData, Opm::ParseMode()));

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
    BOOST_CHECK_THROW(mrt.initFORUNITTESTONLY(deck->getKeyword("PVTO"),
                                              tooManyColumnNames,
                                              /*tableIdx=*/0,
                                              /*firstEntryOffset=*/0),
                      std::runtime_error);

    BOOST_CHECK_NO_THROW(mrt.initFORUNITTESTONLY(deck->getKeyword("PVTO"),
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
    Opm::DeckConstPtr deck(parser->parseString(deckData, Opm::ParseMode()));
    Opm::DeckKeywordConstPtr swofKeyword = deck->getKeyword("SWOF");

    BOOST_CHECK_EQUAL(Opm::SwofTable::numTables(swofKeyword), 2);

    Opm::SwofTable swof1Table;
    Opm::SwofTable swof2Table;

    swof1Table.initFORUNITTESTONLY(deck->getKeyword("SWOF"), /*tableIdx=*/0);
    swof2Table.initFORUNITTESTONLY(deck->getKeyword("SWOF"), /*tableIdx=*/1);

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
    Opm::DeckConstPtr deck(parser->parseString(deckData, Opm::ParseMode()));
    Opm::DeckKeywordConstPtr sgofKeyword = deck->getKeyword("SGOF");

    BOOST_CHECK_EQUAL(Opm::SgofTable::numTables(sgofKeyword), 2);

    Opm::SgofTable sgof1Table;
    Opm::SgofTable sgof2Table;

    sgof1Table.initFORUNITTESTONLY(deck->getKeyword("SGOF"), /*tableIdx=*/0);
    sgof2Table.initFORUNITTESTONLY(deck->getKeyword("SGOF"), /*tableIdx=*/1);

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

BOOST_AUTO_TEST_CASE(PlyadsTable_Tests) {
    {
        const char *correctDeckData =
            "TABDIMS\n"
            "/\n"
            "PLYADS\n"
            "0.00    0.0 \n"
            "0.25    0.000010\n"
            "0.50    0.000018\n"
            "0.75    0.000023\n"
            "1.00    0.000027\n"
            "1.25    0.000030\n"
            "1.50    0.000030\n"
            "1.75    0.000030\n"
            "2.00    0.000030\n"
            "3.00    0.000030 /\n";
        Opm::ParserPtr parser(new Opm::Parser);
        Opm::DeckConstPtr deck(parser->parseString(correctDeckData, Opm::ParseMode()));
        Opm::DeckKeywordConstPtr plyadsKeyword = deck->getKeyword("PLYADS");

        BOOST_CHECK_EQUAL(Opm::PlyadsTable::numTables(plyadsKeyword), 1);

        Opm::PlyadsTable plyadsTable;
        plyadsTable.initFORUNITTESTONLY(plyadsKeyword, /*tableIdx=*/0);

        BOOST_CHECK_CLOSE(plyadsTable.getPolymerConcentrationColumn().front(), 0.0, 1e-6);
        BOOST_CHECK_CLOSE(plyadsTable.getPolymerConcentrationColumn().back(), 3.0, 1e-6);

        BOOST_CHECK_CLOSE(plyadsTable.getAdsorbedPolymerColumn().front(), 0.0, 1e-6);
        BOOST_CHECK_CLOSE(plyadsTable.getAdsorbedPolymerColumn().back(), 0.000030, 1e-6);
    }

    {
        // first column not strictly monotonic
        const char *incorrectDeckData =
            "TABDIMS\n"
            "/\n"
            "PLYADS\n"
            "0.00    0.0 \n"
            "0.00    0.000010\n"
            "0.50    0.000018\n"
            "0.75    0.000023\n"
            "1.00    0.000027\n"
            "1.25    0.000030\n"
            "1.50    0.000030\n"
            "1.75    0.000030\n"
            "2.00    0.000030\n"
            "3.00    0.000030 /\n";
        Opm::ParserPtr parser(new Opm::Parser);
        Opm::DeckConstPtr deck(parser->parseString(incorrectDeckData, Opm::ParseMode()));
        Opm::DeckKeywordConstPtr plyadsKeyword = deck->getKeyword("PLYADS");

        BOOST_CHECK_EQUAL(Opm::PlyadsTable::numTables(plyadsKeyword), 1);

        Opm::PlyadsTable plyadsTable;
        BOOST_CHECK_THROW(plyadsTable.initFORUNITTESTONLY(plyadsKeyword, /*tableIdx=*/0), std::invalid_argument);
    }

    {
        // second column not monotonic
        const char *incorrectDeckData =
            "TABDIMS\n"
            "/\n"
            "PLYADS\n"
            "0.00    0.0 \n"
            "0.25    0.000010\n"
            "0.50    0.000018\n"
            "0.75    0.000023\n"
            "1.00    0.000027\n"
            "1.25    0.000030\n"
            "1.50    0.000030\n"
            "1.75    0.000030\n"
            "2.00    0.000030\n"
            "3.00    0.000029 /\n";
        Opm::ParserPtr parser(new Opm::Parser);
        Opm::DeckConstPtr deck(parser->parseString(incorrectDeckData, Opm::ParseMode()));
        Opm::DeckKeywordConstPtr plyadsKeyword = deck->getKeyword("PLYADS");

        BOOST_CHECK_EQUAL(Opm::PlyadsTable::numTables(plyadsKeyword), 1);

        Opm::PlyadsTable plyadsTable;
        BOOST_CHECK_THROW(plyadsTable.initFORUNITTESTONLY(plyadsKeyword, /*tableIdx=*/0), std::invalid_argument);
    }
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
    Opm::DeckConstPtr deck(parser->parseString(deckData, Opm::ParseMode()));
    Opm::DeckKeywordConstPtr pvtoKeyword = deck->getKeyword("PVTO");

    BOOST_CHECK_EQUAL(Opm::PvtoTable::numTables(pvtoKeyword), 2);

    Opm::PvtoTable pvto1Table;
    Opm::PvtoTable pvto2Table;

    pvto1Table.initFORUNITTESTONLY(deck->getKeyword("PVTO"), /*tableIdx=*/0);
    pvto2Table.initFORUNITTESTONLY(deck->getKeyword("PVTO"), /*tableIdx=*/1);

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


/**
 * Tests "happy path" for a VFPPROD table, i.e., when everything goes well
 */
BOOST_AUTO_TEST_CASE(VFPProdTable_happy_Test) {
    const char *deckData = "\
VFPPROD \n\
-- Table Depth  Rate   WFR   GFR   TAB ALQ    UNITS  BODY    \n\
-- ----- ----- ----- ----- ----- ----- --- -------- -----    \n\
      5  32.9  'LIQ' 'WCT' 'GOR' 'THP' ' ' 'METRIC' 'BHP'  / \n\
-- Rate axis \n\
1 3 5 /      \n\
-- THP axis  \n\
7 11 /       \n\
-- WFR axis  \n\
13 17 /      \n\
-- GFR axis  \n\
19 23 /      \n\
-- ALQ axis  \n\
29 31 /      \n\
-- Table data with THP# WFR# GFR# ALQ# <values 1-num_rates> \n\
1 1 1 1 1.5 2.5 3.5 /    \n\
2 1 1 1 4.5 5.5 6.5 /    \n\
1 2 1 1 7.5 8.5 9.5 /    \n\
2 2 1 1 10.5 11.5 12.5 / \n\
1 1 2 1 13.5 14.5 15.5 / \n\
2 1 2 1 16.5 17.5 18.5 / \n\
1 2 2 1 19.5 20.5 21.5 / \n\
2 2 2 1 22.5 23.5 24.5 / \n\
1 1 1 2 25.5 26.5 27.5 / \n\
2 1 1 2 28.5 29.5 30.5 / \n\
1 2 1 2 31.5 32.5 33.5 / \n\
2 2 1 2 34.5 35.5 36.5 / \n\
1 1 2 2 37.5 38.5 39.5 / \n\
2 1 2 2 40.5 41.5 42.5 / \n\
1 2 2 2 43.5 44.5 45.5 / \n\
2 2 2 2 46.5 47.5 48.5 / \n";

    Opm::ParserPtr parser(new Opm::Parser);
    Opm::DeckConstPtr deck(parser->parseString(deckData, Opm::ParseMode()));
    Opm::DeckKeywordConstPtr vfpprodKeyword = deck->getKeyword("VFPPROD");
    std::shared_ptr<Opm::UnitSystem> units(Opm::UnitSystem::newMETRIC());

    BOOST_CHECK_EQUAL(deck->numKeywords("VFPPROD"), 1);

    Opm::VFPProdTable vfpprodTable;

    vfpprodTable.init(vfpprodKeyword, units);

    BOOST_CHECK_EQUAL(vfpprodTable.getTableNum(), 5);
    BOOST_CHECK_EQUAL(vfpprodTable.getDatumDepth(), 32.9);
    BOOST_CHECK_EQUAL(vfpprodTable.getFloType(), Opm::VFPProdTable::FLO_LIQ);
    BOOST_CHECK_EQUAL(vfpprodTable.getWFRType(), Opm::VFPProdTable::WFR_WCT);
    BOOST_CHECK_EQUAL(vfpprodTable.getGFRType(), Opm::VFPProdTable::GFR_GOR);
    BOOST_CHECK_EQUAL(vfpprodTable.getALQType(), Opm::VFPProdTable::ALQ_UNDEF);

    //Flo axis
    {
        const std::vector<double>& flo = vfpprodTable.getFloAxis();
        BOOST_REQUIRE_EQUAL(flo.size(), 3);

        //Unit of FLO is SM3/day, convert to SM3/second
        double conversion_factor = 1.0 / (60*60*24);
        BOOST_CHECK_EQUAL(flo[0], 1*conversion_factor);
        BOOST_CHECK_EQUAL(flo[1], 3*conversion_factor);
        BOOST_CHECK_EQUAL(flo[2], 5*conversion_factor);
    }

    //THP axis
    {
        const std::vector<double>& thp = vfpprodTable.getTHPAxis();
        BOOST_REQUIRE_EQUAL(thp.size(), 2);

        //Unit of THP is barsa => convert to pascal
        double conversion_factor = 100000.0;
        BOOST_CHECK_EQUAL(thp[0], 7*conversion_factor);
        BOOST_CHECK_EQUAL(thp[1], 11*conversion_factor);
    }

    //WFR axis
    {
        const std::vector<double>& wfr = vfpprodTable.getWFRAxis();
        BOOST_REQUIRE_EQUAL(wfr.size(), 2);

        //Unit of WFR is SM3/SM3
        BOOST_CHECK_EQUAL(wfr[0], 13);
        BOOST_CHECK_EQUAL(wfr[1], 17);
    }

    //GFR axis
    {
        const std::vector<double>& gfr = vfpprodTable.getGFRAxis();
        BOOST_REQUIRE_EQUAL(gfr.size(), 2);

        //Unit of GFR is SM3/SM3
        BOOST_CHECK_EQUAL(gfr[0], 19);
        BOOST_CHECK_EQUAL(gfr[1], 23);
    }

    //ALQ axis
    {
        const std::vector<double>& alq = vfpprodTable.getALQAxis();
        BOOST_REQUIRE_EQUAL(alq.size(), 2);

        //Unit of ALQ undefined
        BOOST_CHECK_EQUAL(alq[0], 29);
        BOOST_CHECK_EQUAL(alq[1], 31);
    }

    //The data itself
    {
        typedef Opm::VFPProdTable::array_type::size_type size_type;
        const Opm::VFPProdTable::array_type& data = vfpprodTable.getTable();
        const size_type* size = data.shape();

        BOOST_CHECK_EQUAL(size[0], 2);
        BOOST_CHECK_EQUAL(size[1], 2);
        BOOST_CHECK_EQUAL(size[2], 2);
        BOOST_CHECK_EQUAL(size[3], 2);
        BOOST_CHECK_EQUAL(size[4], 3);

        //Table given as BHP => barsa. Convert to pascal
        double conversion_factor = 100000.0;

        double index = 0.5;
        for (size_type a=0; a<size[3]; ++a) {
            for (size_type g=0; g<size[2]; ++g) {
                for (size_type w=0; w<size[1]; ++w) {
                    for (size_type t=0; t<size[0]; ++t) {
                        for (size_type f=0; f<size[4]; ++f) {
                            index += 1.0;
                            BOOST_CHECK_EQUAL(data[t][w][g][a][f], index*conversion_factor);
                        }
                    }
                }
            }
        }
    }
}



/**
 * Checks that the VFPPROD table will succeed with a minimal set of
 * specified values.
 */
BOOST_AUTO_TEST_CASE(VFPProdTable_minimal_Test) {
    const char *deckData = "\
VFPPROD \n\
-- Table Depth  Rate   WFR   GFR      \n\
-- ----- ----- ----- ----- -----      \n\
      5  32.9  'LIQ' 'WCT' 'GOR'    / \n\
-- Rate axis \n\
1 /          \n\
-- THP axis  \n\
7 /          \n\
-- WFR axis  \n\
13 /         \n\
-- GFR axis  \n\
19 /         \n\
-- ALQ axis  \n\
29 /         \n\
-- Table data with THP# WFR# GFR# ALQ# <values 1-num_rates> \n\
1 1 1 1 1.5 /    \n";

    Opm::ParserPtr parser(new Opm::Parser);
    Opm::DeckConstPtr deck(parser->parseString(deckData, Opm::ParseMode()));
    Opm::DeckKeywordConstPtr vfpprodKeyword = deck->getKeyword("VFPPROD");
    std::shared_ptr<Opm::UnitSystem> units(Opm::UnitSystem::newMETRIC());

    BOOST_CHECK_EQUAL(deck->numKeywords("VFPPROD"), 1);

    Opm::VFPProdTable vfpprodTable;

    vfpprodTable.init(vfpprodKeyword, units);

    BOOST_CHECK_EQUAL(vfpprodTable.getTableNum(), 5);
    BOOST_CHECK_EQUAL(vfpprodTable.getDatumDepth(), 32.9);
    BOOST_CHECK_EQUAL(vfpprodTable.getFloType(), Opm::VFPProdTable::FLO_LIQ);
    BOOST_CHECK_EQUAL(vfpprodTable.getWFRType(), Opm::VFPProdTable::WFR_WCT);
    BOOST_CHECK_EQUAL(vfpprodTable.getGFRType(), Opm::VFPProdTable::GFR_GOR);
    BOOST_CHECK_EQUAL(vfpprodTable.getALQType(), Opm::VFPProdTable::ALQ_UNDEF);

    //Flo axis
    {
        const std::vector<double>& flo = vfpprodTable.getFloAxis();
        BOOST_REQUIRE_EQUAL(flo.size(), 1);

        //Unit of FLO is SM3/day, convert to SM3/second
        double conversion_factor = 1.0 / (60*60*24);
        BOOST_CHECK_EQUAL(flo[0], 1*conversion_factor);
    }

    //THP axis
    {
        const std::vector<double>& thp = vfpprodTable.getTHPAxis();
        BOOST_REQUIRE_EQUAL(thp.size(), 1);

        //Unit of THP is barsa => convert to pascal
        double conversion_factor = 100000.0;
        BOOST_CHECK_EQUAL(thp[0], 7*conversion_factor);
    }

    //WFR axis
    {
        const std::vector<double>& wfr = vfpprodTable.getWFRAxis();
        BOOST_REQUIRE_EQUAL(wfr.size(), 1);

        //Unit of WFR is SM3/SM3
        BOOST_CHECK_EQUAL(wfr[0], 13);
    }

    //GFR axis
    {
        const std::vector<double>& gfr = vfpprodTable.getGFRAxis();
        BOOST_REQUIRE_EQUAL(gfr.size(), 1);

        //Unit of GFR is SM3/SM3
        BOOST_CHECK_EQUAL(gfr[0], 19);
    }

    //ALQ axis
    {
        const std::vector<double>& alq = vfpprodTable.getALQAxis();
        BOOST_REQUIRE_EQUAL(alq.size(), 1);

        //Unit of ALQ undefined
        BOOST_CHECK_EQUAL(alq[0], 29);
    }

    //The data itself
    {
        typedef Opm::VFPProdTable::array_type::size_type size_type;
        const Opm::VFPProdTable::array_type& data = vfpprodTable.getTable();
        const size_type* size = data.shape();

        //Table given as BHP => barsa. Convert to pascal
        double conversion_factor = 100000.0;

        BOOST_CHECK_EQUAL(size[0]*size[1]*size[2]*size[3]*size[4], 1);
        BOOST_CHECK_EQUAL(data[0][0][0][0][0], 1.5*conversion_factor);
    }
}







/**
 * Spot checks that the VFPPROD table will fail nicely when given invalid data
 */
BOOST_AUTO_TEST_CASE(VFPProdTable_sad_Test) {
    /**
     * Missing value in table
     */
    {
        const char *missing_values = "\
VFPPROD \n\
-- Table Depth  Rate   WFR   GFR      \n\
-- ----- ----- ----- ----- -----      \n\
      5  32.9  'LIQ' 'WCT' 'GOR'    / \n\
-- Rate axis \n\
1 2 /        \n\
-- THP axis  \n\
7 /          \n\
-- WFR axis  \n\
13 /         \n\
-- GFR axis  \n\
19 /         \n\
-- ALQ axis  \n\
29 /         \n\
-- Table data with THP# WFR# GFR# ALQ# <values 1-num_rates> \n\
-- Will fail, as rate axis requires two elements            \n\
1 1 1 1 1.5 /    \n";

        Opm::ParserPtr parser(new Opm::Parser);
        Opm::DeckConstPtr deck(parser->parseString(missing_values, Opm::ParseMode()));
        Opm::DeckKeywordConstPtr vfpprodKeyword = deck->getKeyword("VFPPROD");
        std::shared_ptr<Opm::UnitSystem> units(Opm::UnitSystem::newMETRIC());
        BOOST_CHECK_EQUAL(deck->numKeywords("VFPPROD"), 1);

        Opm::VFPProdTable vfpprodTable;


        BOOST_CHECK_THROW(vfpprodTable.init(vfpprodKeyword, units), std::invalid_argument);
    }



    /**
     * Missing value in table #2
     */
    {
        const char *missing_values = "\
VFPPROD \n\
-- Table Depth  Rate   WFR   GFR      \n\
-- ----- ----- ----- ----- -----      \n\
      5  32.9  'LIQ' 'WCT' 'GOR'    / \n\
-- Rate axis \n\
1 /          \n\
-- THP axis  \n\
7 9 /        \n\
-- WFR axis  \n\
13 /         \n\
-- GFR axis  \n\
19 /         \n\
-- ALQ axis  \n\
29 /         \n\
-- Table data with THP# WFR# GFR# ALQ# <values 1-num_rates> \n\
-- Will fail, as two entries are required                   \n\
1 1 1 1 1.5 /    \n";

        Opm::ParserPtr parser(new Opm::Parser);
        Opm::DeckConstPtr deck(parser->parseString(missing_values, Opm::ParseMode()));
        Opm::DeckKeywordConstPtr vfpprodKeyword = deck->getKeyword("VFPPROD");
        std::shared_ptr<Opm::UnitSystem> units(Opm::UnitSystem::newMETRIC());
        BOOST_CHECK_EQUAL(deck->numKeywords("VFPPROD"), 1);

        Opm::VFPProdTable vfpprodTable;


        BOOST_CHECK_THROW(vfpprodTable.init(vfpprodKeyword, units), std::invalid_argument);
    }


    /**
     * Missing items in header
     */
    {
        const char *missing_metadata = "\
VFPPROD \n\
-- Table Depth   \n\
-- ----- -----   \n\
      5  32.9  / \n\
-- Rate axis \n\
1 2 /        \n\
-- THP axis  \n\
7 /          \n\
-- WFR axis  \n\
13 /         \n\
-- GFR axis  \n\
19 /         \n\
-- ALQ axis  \n\
29 /         \n\
-- Table data with THP# WFR# GFR# ALQ# <values 1-num_rates> \n\
1 1 1 1 1.5 2.5 /    \n";

        Opm::ParserPtr parser(new Opm::Parser);
        Opm::DeckConstPtr deck(parser->parseString(missing_metadata, Opm::ParseMode()));
        Opm::DeckKeywordConstPtr vfpprodKeyword = deck->getKeyword("VFPPROD");
        std::shared_ptr<Opm::UnitSystem> units(Opm::UnitSystem::newMETRIC());
        BOOST_CHECK_EQUAL(deck->numKeywords("VFPPROD"), 1);

        Opm::VFPProdTable vfpprodTable;


        BOOST_CHECK_THROW(vfpprodTable.init(vfpprodKeyword, units), std::out_of_range);
    }



    /**
     * Wrong items in header
     */
    {
        const char *wrong_metadata = "\
VFPPROD \n\
-- Table Depth   \n\
-- ----- -----   \n\
      5  32.9  'WCT' 'LIC' 'GARBAGE'    / \n\
-- Rate axis \n\
1 2 /        \n\
-- THP axis  \n\
7 /          \n\
-- WFR axis  \n\
13 /         \n\
-- GFR axis  \n\
19 /         \n\
-- ALQ axis  \n\
29 /         \n\
-- Table data with THP# WFR# GFR# ALQ# <values 1-num_rates> \n\
1 1 1 1 1.5 2.5 /    \n";

        Opm::ParserPtr parser(new Opm::Parser);
        Opm::DeckConstPtr deck(parser->parseString(wrong_metadata, Opm::ParseMode()));
        Opm::DeckKeywordConstPtr vfpprodKeyword = deck->getKeyword("VFPPROD");
        std::shared_ptr<Opm::UnitSystem> units(Opm::UnitSystem::newMETRIC());
        BOOST_CHECK_EQUAL(deck->numKeywords("VFPPROD"), 1);

        Opm::VFPProdTable vfpprodTable;

        BOOST_CHECK_THROW(vfpprodTable.init(vfpprodKeyword, units), std::invalid_argument);
    }



    /**
     * Wrong axes in header
     */
    {
        const char *missing_axes = "\
VFPPROD \n\
-- Table Depth   \n\
-- ----- -----   \n\
      5  32.9  'LIC' 'WCT' 'OGR'    / \n\
-- Rate axis \n\
1 2 /        \n\
-- THP axis  \n\
7 /          \n\
-- WFR axis  \n\
13 /         \n\
-- GFR axis  \n\
19 /         \n\
-- ALQ axis  \n\
-- Missing!  \n\
-- Table data with THP# WFR# GFR# ALQ# <values 1-num_rates> \n\
1 1 1 1 1.5 2.5 /    \n";

        Opm::ParserPtr parser(new Opm::Parser);
        Opm::DeckConstPtr deck(parser->parseString(missing_axes, Opm::ParseMode()));
        Opm::DeckKeywordConstPtr vfpprodKeyword = deck->getKeyword("VFPPROD");
        std::shared_ptr<Opm::UnitSystem> units(Opm::UnitSystem::newMETRIC());
        BOOST_CHECK_EQUAL(deck->numKeywords("VFPPROD"), 1);

        Opm::VFPProdTable vfpprodTable;

        BOOST_CHECK_THROW(vfpprodTable.init(vfpprodKeyword, units), std::invalid_argument);
    }
}





/**
 * Tests "happy path" for a VFPPROD table, i.e., when everything goes well
 */
BOOST_AUTO_TEST_CASE(VFPInjTable_happy_Test) {
    const char *deckData = "\
VFPINJ \n\
-- Table Depth  Rate   TAB  UNITS  BODY    \n\
-- ----- ----- ----- ----- ------ -----    \n\
       5  32.9   WAT   THP METRIC   BHP /  \n\
-- Rate axis \n\
1 3 5 /      \n\
-- THP axis  \n\
7 11 /       \n\
-- Table data with THP# <values 1-num_rates> \n\
1 1.5 2.5 3.5 /    \n\
2 4.5 5.5 6.5 /    \n";

    Opm::ParserPtr parser(new Opm::Parser);
    Opm::DeckConstPtr deck(parser->parseString(deckData, Opm::ParseMode()));
    Opm::DeckKeywordConstPtr vfpprodKeyword = deck->getKeyword("VFPINJ");
    std::shared_ptr<Opm::UnitSystem> units(Opm::UnitSystem::newMETRIC());

    BOOST_CHECK_EQUAL(deck->numKeywords("VFPINJ"), 1);

    Opm::VFPInjTable vfpinjTable;

    vfpinjTable.init(vfpprodKeyword, units);

    BOOST_CHECK_EQUAL(vfpinjTable.getTableNum(), 5);
    BOOST_CHECK_EQUAL(vfpinjTable.getDatumDepth(), 32.9);
    BOOST_CHECK_EQUAL(vfpinjTable.getFloType(), Opm::VFPInjTable::FLO_WAT);

    //Flo axis
    {
        const std::vector<double>& flo = vfpinjTable.getFloAxis();
        BOOST_REQUIRE_EQUAL(flo.size(), 3);

        //Unit of FLO is SM3/day, convert to SM3/second
        double conversion_factor = 1.0 / (60*60*24);
        BOOST_CHECK_EQUAL(flo[0], 1*conversion_factor);
        BOOST_CHECK_EQUAL(flo[1], 3*conversion_factor);
        BOOST_CHECK_EQUAL(flo[2], 5*conversion_factor);
    }

    //THP axis
    {
        const std::vector<double>& thp = vfpinjTable.getTHPAxis();
        BOOST_REQUIRE_EQUAL(thp.size(), 2);

        //Unit of THP is barsa => convert to pascal
        double conversion_factor = 100000.0;
        BOOST_CHECK_EQUAL(thp[0], 7*conversion_factor);
        BOOST_CHECK_EQUAL(thp[1], 11*conversion_factor);
    }

    //The data itself
    {
        typedef Opm::VFPInjTable::array_type::size_type size_type;
        const Opm::VFPInjTable::array_type& data = vfpinjTable.getTable();
        const size_type* size = data.shape();

        BOOST_CHECK_EQUAL(size[0], 2);
        BOOST_CHECK_EQUAL(size[1], 3);

        //Table given as BHP => barsa. Convert to pascal
        double conversion_factor = 100000.0;

        double index = 0.5;
        for (size_type t=0; t<size[0]; ++t) {
            for (size_type f=0; f<size[1]; ++f) {
                index += 1.0;
                BOOST_CHECK_EQUAL(data[t][f], index*conversion_factor);
            }
        }
    }
}













/**
 * Spot checks that the VFPPROD table will fail nicely when given invalid data
 */
BOOST_AUTO_TEST_CASE(VFPInjTable_sad_Test) {
    /**
     * Missing value in table
     */
    {
        const char *missing_values = "\
VFPINJ \n\
-- Table Depth  Rate   TAB  UNITS  BODY    \n\
-- ----- ----- ----- ----- ------ -----    \n\
       5  32.9   WAT   THP METRIC   BHP /  \n\
-- Rate axis \n\
1 3 5 /      \n\
-- THP axis  \n\
7 11 /       \n\
-- Table data with THP# <values 1-num_rates> \n\
-- Will fail, as rate axis requires three elements  \n\
1 1.5 2.5 /    \n\
2 4.5 5.5 /    \n";

        Opm::ParserPtr parser(new Opm::Parser);
        Opm::DeckConstPtr deck(parser->parseString(missing_values, Opm::ParseMode()));
        Opm::DeckKeywordConstPtr vfpinjKeyword = deck->getKeyword("VFPINJ");
        std::shared_ptr<Opm::UnitSystem> units(Opm::UnitSystem::newMETRIC());
        BOOST_CHECK_EQUAL(deck->numKeywords("VFPINJ"), 1);

        Opm::VFPProdTable vfpprodTable;


        BOOST_CHECK_THROW(vfpprodTable.init(vfpinjKeyword, units), std::invalid_argument);
    }



    /**
     * Missing value in table #2
     */
    {
        const char *missing_values = "\
VFPINJ \n\
-- Table Depth  Rate   TAB  UNITS  BODY    \n\
-- ----- ----- ----- ----- ------ -----    \n\
       5  32.9   WAT   THP METRIC   BHP /  \n\
-- Rate axis \n\
1 3 5 /      \n\
-- THP axis  \n\
7 11 /       \n\
-- Table data with THP# <values 1-num_rates> \n\
-- Will fail, as two entries are required                   \n\
1 1.5 2.5 3.5 /    \n";

        Opm::ParserPtr parser(new Opm::Parser);
        Opm::DeckConstPtr deck(parser->parseString(missing_values, Opm::ParseMode()));
        Opm::DeckKeywordConstPtr vfpinjKeyword = deck->getKeyword("VFPINJ");
        std::shared_ptr<Opm::UnitSystem> units(Opm::UnitSystem::newMETRIC());
        BOOST_CHECK_EQUAL(deck->numKeywords("VFPINJ"), 1);

        Opm::VFPProdTable vfpprodTable;


        BOOST_CHECK_THROW(vfpprodTable.init(vfpinjKeyword, units), std::invalid_argument);
    }


    /**
     * Missing items in header
     */
    {
        const char *missing_metadata = "\
VFPINJ \n\
-- Table Depth      \n\
-- ----- -----      \n\
       5  32.9   /  \n\
-- Rate axis \n\
1 3 5 /      \n\
-- THP axis  \n\
7 11 /       \n\
-- Table data with THP# <values 1-num_rates> \n\
1 1.5 2.5 3.5 /    \n\
2 4.5 5.5 6.5 /    \n";

        Opm::ParserPtr parser(new Opm::Parser);
        Opm::DeckConstPtr deck(parser->parseString(missing_metadata, Opm::ParseMode()));
        Opm::DeckKeywordConstPtr vfpinjKeyword = deck->getKeyword("VFPINJ");
        std::shared_ptr<Opm::UnitSystem> units(Opm::UnitSystem::newMETRIC());
        BOOST_CHECK_EQUAL(deck->numKeywords("VFPINJ"), 1);

        Opm::VFPProdTable vfpprodTable;


        BOOST_CHECK_THROW(vfpprodTable.init(vfpinjKeyword, units), std::invalid_argument);
    }



    /**
     * Wrong items in header
     */
    {
        const char *wrong_metadata = "\
VFPINJ \n\
-- Table Depth  Rate   TAB  UNITS  BODY    \n\
-- ----- ----- ----- ----- ------ -----    \n\
       5  32.9   GOR   BHP    FOO  GAGA /  \n\
-- Rate axis \n\
1 3 5 /      \n\
-- THP axis  \n\
7 11 /       \n\
-- Table data with THP# <values 1-num_rates> \n\
1 1.5 2.5 3.5 /    \n\
2 4.5 5.5 6.5 /    \n";

        Opm::ParserPtr parser(new Opm::Parser);
        Opm::DeckConstPtr deck(parser->parseString(wrong_metadata, Opm::ParseMode()));
        Opm::DeckKeywordConstPtr vfpinjKeyword = deck->getKeyword("VFPINJ");
        std::shared_ptr<Opm::UnitSystem> units(Opm::UnitSystem::newMETRIC());
        BOOST_CHECK_EQUAL(deck->numKeywords("VFPINJ"), 1);

        Opm::VFPProdTable vfpprodTable;

        BOOST_CHECK_THROW(vfpprodTable.init(vfpinjKeyword, units), std::invalid_argument);
    }



    /**
     * Wrong axes in header
     */
    {
        const char *missing_axes = "\
VFPINJ \n\
-- Table Depth  Rate   TAB  UNITS  BODY    \n\
-- ----- ----- ----- ----- ------ -----    \n\
       5  32.9   WAT   THP METRIC   BHP /  \n\
-- Rate axis \n\
1 3 5 /      \n\
-- THP axis  \n\
-- Missing!  \n\
-- Table data with THP# <values 1-num_rates> \n\
1 1.5 2.5 3.5 /    \n\
2 4.5 5.5 6.5 /    \n";

        Opm::ParserPtr parser(new Opm::Parser);
        Opm::DeckConstPtr deck(parser->parseString(missing_axes, Opm::ParseMode()));
        Opm::DeckKeywordConstPtr vfpinjKeyword = deck->getKeyword("VFPINJ");
        std::shared_ptr<Opm::UnitSystem> units(Opm::UnitSystem::newMETRIC());
        BOOST_CHECK_EQUAL(deck->numKeywords("VFPINJ"), 1);

        Opm::VFPProdTable vfpprodTable;

        BOOST_CHECK_THROW(vfpprodTable.init(vfpinjKeyword, units), std::invalid_argument);
    }
}







