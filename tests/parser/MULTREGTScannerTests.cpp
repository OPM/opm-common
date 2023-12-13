/*
  Copyright 2014--2023 Equinor ASA.

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

#define BOOST_TEST_MODULE MULTREGTScannerTests

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/Grid/MULTREGTScanner.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FaceDir.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/M.hpp>

#include <array>
#include <initializer_list>
#include <stdexcept>
#include <vector>

BOOST_AUTO_TEST_SUITE(Basic)

BOOST_AUTO_TEST_CASE(TestRegionName) {
    BOOST_CHECK_EQUAL( "FLUXNUM" , Opm::MULTREGT::RegionNameFromDeckValue( "F"));
    BOOST_CHECK_EQUAL( "MULTNUM" , Opm::MULTREGT::RegionNameFromDeckValue( "M"));
    BOOST_CHECK_EQUAL( "OPERNUM" , Opm::MULTREGT::RegionNameFromDeckValue( "O"));

    BOOST_CHECK_THROW( Opm::MULTREGT::RegionNameFromDeckValue("o") , std::invalid_argument);
    BOOST_CHECK_THROW( Opm::MULTREGT::RegionNameFromDeckValue("X") , std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(TestNNCBehaviourEnum) {
    BOOST_CHECK_MESSAGE(Opm::MULTREGT::NNCBehaviourEnum::ALL == Opm::MULTREGT::NNCBehaviourFromString("ALL"),
                        R"(Behaviour("ALL") must be ALL)");

    BOOST_CHECK_MESSAGE(Opm::MULTREGT::NNCBehaviourEnum::NNC == Opm::MULTREGT::NNCBehaviourFromString("NNC"),
                        R"(Behaviour("NNC") must be NNC)");

    BOOST_CHECK_MESSAGE(Opm::MULTREGT::NNCBehaviourEnum::NONNC == Opm::MULTREGT::NNCBehaviourFromString("NONNC"),
                        R"(Behaviour("NONNC") must be NONNC)");

    BOOST_CHECK_MESSAGE(Opm::MULTREGT::NNCBehaviourEnum::NOAQUNNC == Opm::MULTREGT::NNCBehaviourFromString("NOAQUNNC"),
                        R"(Behaviour("NOAQUNNC") must be NOAQUNNC)");

    BOOST_CHECK_THROW(Opm::MULTREGT::NNCBehaviourFromString("Invalid"), std::invalid_argument);
}

namespace {
    Opm::Deck createInvalidMULTREGTDeck()
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 3 3 2 /
GRID
DX
18*0.25 /
DY
18*0.25 /
DZ
18*0.25 /
TOPS
9*0.25 /
FLUXNUM
1 1 2
1 1 2
1 1 2
3 4 5
3 4 5
3 4 5
/
MULTREGT
1  2   0.50   G   ALL    M / -- Invalid direction
/
MULTREGT
1  2   0.50   X   ALL    G / -- Invalid region
/
MULTREGT
1  2   0.50   X   ALL    M / -- Region not in deck
/
EDIT
)");
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(InvalidInput) {
    Opm::Deck deck = createInvalidMULTREGTDeck();
    Opm::EclipseGrid grid( deck );
    Opm::TableManager tm(deck);
    Opm::EclipseGrid eg( deck );
    Opm::FieldPropsManager fp(deck, Opm::Phases{true, true, true}, eg, tm);

    // Invalid direction
    std::vector<const Opm::DeckKeyword*> keywords0;
    const auto& multregtKeyword0 = deck["MULTREGT"][0];
    keywords0.push_back( &multregtKeyword0 );
    BOOST_CHECK_THROW( Opm::MULTREGTScanner scanner( grid, &fp, keywords0 ); , std::invalid_argument );

    // Not supported region
    std::vector<const Opm::DeckKeyword*> keywords1;
    const auto& multregtKeyword1 = deck["MULTREGT"][1];
    keywords1.push_back( &multregtKeyword1 );
    BOOST_CHECK_THROW( Opm::MULTREGTScanner scanner( grid, &fp, keywords1 ); , std::invalid_argument );

    // The keyword is ok; but it refers to a region which is not in the deck.
    std::vector<const Opm::DeckKeyword*> keywords2;
    const auto& multregtKeyword2 = deck["MULTREGT"][2];
    keywords2.push_back( &multregtKeyword2 );
    BOOST_CHECK_THROW( Opm::MULTREGTScanner scanner( grid, &fp, keywords2 ); , std::logic_error );
}

namespace {
    Opm::Deck createIncludeSelfMULTREGTDeck()
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 3 3 2 /
GRID
DX
18*0.25 /
DY
18*0.25 /
DZ
18*0.25 /
TOPS
9*0.25 /
FLUXNUM
1 1 2
1 1 2
1 1 2
3 4 5
3 4 5
3 4 5
/
MULTREGT
1  2   0.50   X   NOAQUNNC  F / -- Not support NOAQUNNC behaviour
/
MULTREGT
2  2   0.50   YZ   ALL    F / -- Region values equal
/
EDIT
)");
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(IncludeSelf) {
    Opm::Deck deck = createIncludeSelfMULTREGTDeck();
    Opm::EclipseGrid grid( deck );
    Opm::TableManager tm(deck);
    Opm::EclipseGrid eg( deck );
    Opm::FieldPropsManager fp(deck, Opm::Phases{true, true, true}, eg, tm);

    // srcValue == targetValue - not supported
    std::vector<const Opm::DeckKeyword*> keywords1;
    const Opm::DeckKeyword& multregtKeyword1 = deck["MULTREGT"][1];
    keywords1.push_back( &multregtKeyword1 );
    Opm::MULTREGTScanner scanner1 = { grid, &fp, keywords1 };

    // Region 2 to 2
    BOOST_CHECK_EQUAL( scanner1.getRegionMultiplier(grid.getGlobalIndex(2,0,0), grid.getGlobalIndex(2,1,0),
                                                    Opm::FaceDir::YPlus ), 0.50);
    // Region 2 to 5
    BOOST_CHECK_EQUAL( scanner1.getRegionMultiplier(grid.getGlobalIndex(2,0,0), grid.getGlobalIndex(2,0,1),
                                                    Opm::FaceDir::ZPlus ), 0.5);
    BOOST_CHECK_EQUAL( scanner1.getRegionMultiplier(grid.getGlobalIndex(0,0,0), grid.getGlobalIndex(0,0,1),
                                                    Opm::FaceDir::ZMinus ), 1.0);

}

namespace {
    Opm::Deck createDefaultedRegions()
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 3 3 2 /
GRID
DX
18*0.25 /
DY
18*0.25 /
DZ
18*0.25 /
TOPS
9*0.25 /
FLUXNUM
1 1 2
1 1 2
1 1 2
3 4 5
3 4 5
3 4 5
/
MULTREGT
3  4   1.25   XYZ   ALL    F /
2  -1   0   XYZ   ALL    F / -- Defaulted from region value
1  -1   0   XYZ   ALL    F / -- Defaulted from region value
2  1   1      XYZ   ALL    F / Override default
/
MULTREGT
2  *   0.75   XYZ   ALL    F / -- Defaulted to region value
/
EDIT
)");
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(DefaultedRegions) {
  Opm::Deck deck = createDefaultedRegions();
  Opm::EclipseGrid grid( deck );
  Opm::TableManager tm(deck);
  Opm::EclipseGrid eg( deck );
  Opm::FieldPropsManager fp(deck, Opm::Phases{true, true, true}, eg, tm);


  std::vector<const Opm::DeckKeyword*> keywords0;
  const auto& multregtKeyword0 = deck["MULTREGT"][0];
  keywords0.push_back( &multregtKeyword0 );
  Opm::MULTREGTScanner scanner0(grid, &fp, keywords0);
  BOOST_CHECK_EQUAL( scanner0.getRegionMultiplier(grid.getGlobalIndex(0,0,1), grid.getGlobalIndex(1,0,1), Opm::FaceDir::XPlus ), 1.25);
  BOOST_CHECK_EQUAL( scanner0.getRegionMultiplier(grid.getGlobalIndex(1,0,0), grid.getGlobalIndex(2,0,0), Opm::FaceDir::XPlus ), 1.0);
  BOOST_CHECK_EQUAL( scanner0.getRegionMultiplier(grid.getGlobalIndex(2,0,1), grid.getGlobalIndex(2,0,0), Opm::FaceDir::ZMinus ), 0.0);

  std::vector<const Opm::DeckKeyword*> keywords1;
  const Opm::DeckKeyword& multregtKeyword1 = deck["MULTREGT"][1];
  keywords1.push_back( &multregtKeyword1 );
  Opm::MULTREGTScanner scanner1(grid, &fp, keywords1 );
  BOOST_CHECK_EQUAL( scanner1.getRegionMultiplier(grid.getGlobalIndex(2,0,0), grid.getGlobalIndex(1,0,0), Opm::FaceDir::XMinus ), 0.75);
  BOOST_CHECK_EQUAL( scanner1.getRegionMultiplier(grid.getGlobalIndex(2,0,0), grid.getGlobalIndex(2,0,1), Opm::FaceDir::ZPlus), 0.75);
}

namespace {
    Opm::Deck createCopyMULTNUMDeck()
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
2 2 2 /
GRID
DX
8*0.25 /
DY
8*0.25 /
DZ
8*0.25 /
TOPS
4*0.25 /
FLUXNUM
1 2
1 2
3 4
3 4
/
COPY
 FLUXNUM  MULTNUM /
/
MULTREGT
1  2   0.50/
/
EDIT
)");
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(MULTREGT_COPY_MULTNUM) {
    Opm::Deck deck = createCopyMULTNUMDeck();
    Opm::TableManager tm(deck);
    Opm::EclipseGrid eg(deck);
    Opm::FieldPropsManager fp(deck, Opm::Phases{true, true, true}, eg, tm);

    BOOST_CHECK_NO_THROW(fp.has_int("FLUXNUM"));
    BOOST_CHECK_NO_THROW(fp.has_int("MULTNUM"));
    const auto& fdata = fp.get_global_int("FLUXNUM");
    const auto& mdata = fp.get_global_int("MULTNUM");
    std::vector<int> data = { 1, 2, 1, 2, 3, 4, 3, 4 };

    for (auto i = 0; i < 2 * 2 * 2; i++) {
        BOOST_CHECK_EQUAL(fdata[i], mdata[i]);
        BOOST_CHECK_EQUAL(fdata[i], data[i]);
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Basic

// ===========================================================================

BOOST_AUTO_TEST_SUITE(AquNNC)

namespace {
    Opm::Deck aquNNCDeck_OneAquCell()
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 1 6 2 /

AQUDIMS
-- mxnaqn  mxnaqc  niftbl  nriftb  nanaqu  ncamax  mxnali  mxaaql
   1       1       1*      1*      1*      1       1*      1*  /

GRID

DXV
  100.0 /

DYV
  6*100.0 /

DZV
  2*10.0 /

DEPTHZ
  14*2000.0 /

PORO
 12*0.25 /

PERMX
 12*100.0 /

PERMZ
 12*10.0 /

COPY
  PERMX PERMY /
/

MULTNUM
-- J= 1 2 3 4 5 6
      1 2 2 2 1 1   -- K=1
      1 2 2 2 1 1 / -- K=2

ACTNUM
-- J= 1 2 3 4 5 6
      1 1 1 1 0 0   -- K=1
      1 1 1 1 0 0 / -- K=2

MULTREGT  -- 0
  1 2 0.1  1*  'NONNC' /
/

MULTREGT  -- 1
  1 2 0.2  1*  'ALL' /
/

MULTREGT  -- 2
  1 2 0.3  1*  'NOAQUNNC' /
/

MULTREGT  -- 3
  1 2 0.4  1*  'NNC' /
/

AQUNUM
--AQnr.  I  J  K   Area       Length Poro    Perm   Depth    Initial.Pr   PVTNUM   SATNUM
   1     1  5  2   100.0E+3   1000   0.25    400    2005.00  300.0        1        1  / MULTNUM=1
/

AQUCON
--  Connect numerical aquifer to the reservoir
--  Id.nr  I1 I2     J1  J2    K1  K2	 Face	 Trans.mult.  Trans.opt.
     1     1  1      4   4     2   2     'J+'    1.00         1*  /
/
)");
    }

    Opm::Deck aquNNCDeck_ThreeAquCells()
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 1 10 2 /

AQUDIMS
-- mxnaqn  mxnaqc  niftbl  nriftb  nanaqu  ncamax  mxnali  mxaaql
   1       1       1*      1*      1*      1       1*      1*  /

GRID

DXV
  100.0 /

DYV
  10*100.0 /

DZV
  2*10.0 /

DEPTHZ
  22*2000.0 /

PORO
 20*0.25 /

PERMX
 20*100.0 /

PERMZ
 20*10.0 /

COPY
  PERMX PERMY /
/

MULTNUM
-- J= 1 2 3 4 5 6 7 8 9 10
      1 2 2 2 2 2 3 4 4  4   -- K=1
      1 2 2 2 2 2 3 4 4  4 / -- K=2

ACTNUM
-- J= 1 2 3 4 5 6 7 8 9 10
      1 1 1 1 0 0 0 0 0  0   -- K=1
      1 1 1 1 0 0 0 0 0  0 / -- K=2

MULTREGT  -- 0
  1 2 0.5   1*  'ALL' /
  2 3 0.1   1*  'ALL' /
  3 4 0.01  1*  'ALL' /
/

MULTREGT  -- 1
  1 2 0.5   1*  'NONNC' /
  2 3 0.1   1*  'NONNC' /
  3 4 0.01  1*  'NONNC' /
/

MULTREGT  -- 2
  1 2 0.5   1*  'NOAQUNNC' /
  2 3 0.1   1*  'NOAQUNNC' /
  3 4 0.01  1*  'NOAQUNNC' /
/

MULTREGT  -- 3
  1 2 0.5   1*  'NNC' /
  2 3 0.1   1*  'NNC' /
  3 4 0.01  1*  'NNC' /
/

AQUNUM
--AQnr.  I  J  K   Area       Length Poro    Perm   Depth    Initial.Pr   PVTNUM   SATNUM
   1     1  6  2   100.0E+3   1000   0.25    400    2005.00  300.0        1        1  / MULTNUM=2
   1     1  7  2   100.0E+3   1000   0.25    400    2005.00  300.0        1        1  / MULTNUM=3
   1     1  8  2   100.0E+3   1000   0.25    400    2005.00  300.0        1        1  / MULTNUM=4
/

AQUCON
--  Connect numerical aquifer to the reservoir
--  Id.nr  I1 I2     J1  J2    K1  K2	 Face	 Trans.mult.  Trans.opt.
     1     1  1      4   4     2   2     'J+'    1.00         1*  /
/
)");
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(AQUNNC_Handling_OneAquCell)
{
    const auto deck = aquNNCDeck_OneAquCell();
    const auto grid = Opm::EclipseGrid { deck };
    const auto fp   = Opm::FieldPropsManager {
        deck, Opm::Phases { true, true, true },
        grid, Opm::TableManager { deck }
    };

    auto makeScanner = [&deck, &grid, &fp, aquNum = Opm::NumericalAquifers { deck, grid, fp }]
        (const std::size_t mrtID)
    {
        auto scanner = Opm::MULTREGTScanner {
            grid, &fp, { &deck.get<Opm::ParserKeywords::MULTREGT>()[mrtID] }
        };

        scanner.applyNumericalAquifer(aquNum.allAquiferCellIds());

        return scanner;
    };

    auto getMultRegular = [&grid]
        (const Opm::MULTREGTScanner& scanner,
         const std::array<int,3>&    c1,
         const std::array<int,3>&    c2,
         const Opm::FaceDir::DirEnum direction)
    {
        return scanner.getRegionMultiplier(grid.getGlobalIndex(c1[0], c1[1], c1[2]),
                                           grid.getGlobalIndex(c2[0], c2[1], c2[2]),
                                           direction);
    };

    auto getMultNNC = [&grid]
        (const Opm::MULTREGTScanner& scanner,
         const std::array<int,3>&    c1,
         const std::array<int,3>&    c2)
    {
        return scanner.getRegionMultiplierNNC(grid.getGlobalIndex(c1[0], c1[1], c1[2]),
                                              grid.getGlobalIndex(c2[0], c2[1], c2[2]));
    };

    // 1->2: 0.1, NONNC
    {
        const auto scanner = makeScanner(0);

        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 0.1, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 0.1, 1.0e-8);

        // Both cells in MULTNUM == 2
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);

        // Numerical aquifer
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 3, 1 }, { 0, 4, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);

        // NNCs
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 0, 0 }, { 0, 1, 1 }), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 0, 1 }, { 0, 1, 0 }), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 3, 1 }, { 0, 4, 1 }), 1.0, 1.0e-8); // Numerical aquifer
    }

    // 1->2: 0.2, ALL
    {
        const auto scanner = makeScanner(1);

        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 0.2, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 0.2, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 0.2, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 0.2, 1.0e-8);

        // Both cells in MULTNUM == 2
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);

        // Numerical aquifer
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 3, 1 }, { 0, 4, 1 }, Opm::FaceDir::YPlus), 0.2, 1.0e-8);

        // NNCs
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 0, 0 }, { 0, 1, 1 }), 0.2, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 0, 1 }, { 0, 1, 0 }), 0.2, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 3, 1 }, { 0, 4, 1 }), 0.2, 1.0e-8); // Numerical aquifer
    }

    // 1->2: 0.3, NOAQUNNC
    {
        const auto scanner = makeScanner(2);

        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 0.3, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 0.3, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 0.3, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 0.3, 1.0e-8);

        // Both cells in MULTNUM == 2
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);

        // Numerical aquifer
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 3, 1 }, { 0, 4, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);

        // NNCs
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 0, 0 }, { 0, 1, 1 }), 0.3, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 0, 1 }, { 0, 1, 0 }), 0.3, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 3, 1 }, { 0, 4, 1 }), 1.0, 1.0e-8); // Numerical aquifer
    }

    // 1->2: 0.4, NNC
    {
        const auto scanner = makeScanner(3);

        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 0.4, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 0.4, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);

        // Both cells in MULTNUM == 2
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);

        // Numerical aquifer
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 3, 1 }, { 0, 4, 1 }, Opm::FaceDir::YPlus), 0.4, 1.0e-8);

        // NNCs
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 0, 0 }, { 0, 1, 1 }), 0.4, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 0, 1 }, { 0, 1, 0 }), 0.4, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 3, 1 }, { 0, 4, 1 }), 0.4, 1.0e-8); // Numerical aquifer
    }
}

BOOST_AUTO_TEST_CASE(AQUNNC_Handling_ThreeAquCells)
{
    const auto deck = aquNNCDeck_ThreeAquCells();
    const auto grid = Opm::EclipseGrid { deck };
    const auto fp   = Opm::FieldPropsManager {
        deck, Opm::Phases { true, true, true },
        grid, Opm::TableManager { deck }
    };

    auto makeScanner = [&deck, &grid, &fp, aquNum = Opm::NumericalAquifers { deck, grid, fp }]
        (const std::size_t mrtID)
    {
        auto scanner = Opm::MULTREGTScanner {
            grid, &fp, { &deck.get<Opm::ParserKeywords::MULTREGT>()[mrtID] }
        };

        scanner.applyNumericalAquifer(aquNum.allAquiferCellIds());

        return scanner;
    };

    auto getMultRegular = [&grid]
        (const Opm::MULTREGTScanner& scanner,
         const std::array<int,3>&    c1,
         const std::array<int,3>&    c2,
         const Opm::FaceDir::DirEnum direction)
    {
        return scanner.getRegionMultiplier(grid.getGlobalIndex(c1[0], c1[1], c1[2]),
                                           grid.getGlobalIndex(c2[0], c2[1], c2[2]),
                                           direction);
    };

    auto getMultNNC = [&grid]
        (const Opm::MULTREGTScanner& scanner,
         const std::array<int,3>&    c1,
         const std::array<int,3>&    c2)
    {
        return scanner.getRegionMultiplierNNC(grid.getGlobalIndex(c1[0], c1[1], c1[2]),
                                              grid.getGlobalIndex(c2[0], c2[1], c2[2]));
    };

    // 1->2: 0.5 , ALL
    // 2->3: 0.1 , ALL
    // 3->4: 0.01, ALL
    {
        const auto scanner = makeScanner(0);

        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 0.5, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 0.5, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 0.5, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 0.5, 1.0e-8);

        // Both cells in MULTNUM == 2
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);

        // Numerical aquifer
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 3, 1 }, { 0, 5, 1 }, Opm::FaceDir::YPlus), 1.0 , 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 5, 1 }, { 0, 6, 1 }, Opm::FaceDir::YPlus), 0.1 , 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 6, 1 }, { 0, 7, 1 }, Opm::FaceDir::YPlus), 0.01, 1.0e-8);

        // NNCs
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 0, 0 }, { 0, 1, 1 }), 0.5 , 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 3, 1 }, { 0, 5, 1 }), 1.0 , 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 5, 1 }, { 0, 6, 1 }), 0.1 , 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 6, 1 }, { 0, 7, 1 }), 0.01, 1.0e-8);
    }

    // 1->2: 0.5 , NONNC
    // 2->3: 0.1 , NONNC
    // 3->4: 0.01, NONNC
    {
        const auto scanner = makeScanner(1);

        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 0.5, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 0.5, 1.0e-8);

        // Both cells in MULTNUM == 2
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);

        // Numerical aquifer
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 3, 1 }, { 0, 5, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 5, 1 }, { 0, 6, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 6, 1 }, { 0, 7, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);

        // NNCs
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 0, 0 }, { 0, 1, 1 }), 1.0 , 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 3, 1 }, { 0, 5, 1 }), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 5, 1 }, { 0, 6, 1 }), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 6, 1 }, { 0, 7, 1 }), 1.0, 1.0e-8);
    }

    // 1->2: 0.5 , NOAQUNNC
    // 2->3: 0.1 , NOAQUNNC
    // 3->4: 0.01, NOAQUNNC
    {
        const auto scanner = makeScanner(2);

        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 0.5, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 0.5, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 0.5, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 0.5, 1.0e-8);

        // Both cells in MULTNUM == 2
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);

        // Numerical aquifer
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 3, 1 }, { 0, 5, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 5, 1 }, { 0, 6, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 6, 1 }, { 0, 7, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);

        // NNCs
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 0, 0 }, { 0, 1, 1 }), 0.5 , 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 3, 1 }, { 0, 5, 1 }), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 5, 1 }, { 0, 6, 1 }), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 6, 1 }, { 0, 7, 1 }), 1.0, 1.0e-8);
    }

    // 1->2: 0.5 , NNC
    // 2->3: 0.1 , NNC
    // 3->4: 0.01, NNC
    {
        const auto scanner = makeScanner(3);

        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 0 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 0.5, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 0 }, Opm::FaceDir::YPlus), 0.5, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 0, 1 }, { 0, 1, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);

        // Both cells in MULTNUM == 2
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 0 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 0 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8); // NNC
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 1, 1 }, { 0, 2, 1 }, Opm::FaceDir::YPlus), 1.0, 1.0e-8);

        // Numerical aquifer
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 3, 1 }, { 0, 5, 1 }, Opm::FaceDir::YPlus), 1.0 , 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 5, 1 }, { 0, 6, 1 }, Opm::FaceDir::YPlus), 0.1 , 1.0e-8);
        BOOST_CHECK_CLOSE(getMultRegular(scanner, { 0, 6, 1 }, { 0, 7, 1 }, Opm::FaceDir::YPlus), 0.01, 1.0e-8);

        // NNCs
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 0, 0 }, { 0, 1, 1 }), 0.5 , 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 3, 1 }, { 0, 5, 1 }), 1.0 , 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 5, 1 }, { 0, 6, 1 }), 0.1 , 1.0e-8);
        BOOST_CHECK_CLOSE(getMultNNC(scanner, { 0, 6, 1 }, { 0, 7, 1 }), 0.01, 1.0e-8);
    }
}

BOOST_AUTO_TEST_SUITE_END()     // AquNNC

// ===========================================================================

BOOST_AUTO_TEST_SUITE(MultiRegSet)

namespace {
    class TMultRegion
    {
    public:
        explicit TMultRegion(const Opm::Deck& deck);

        double regular(const std::array<int,3>&    c1,
                       const std::array<int,3>&    c2,
                       const Opm::FaceDir::DirEnum direction) const;

        double nnc(const std::array<int,3>& c1,
                   const std::array<int,3>& c2) const;

    private:
        Opm::EclipseGrid grid_;
        Opm::FieldPropsManager fp_;
        Opm::MULTREGTScanner scanner_;
    };

    TMultRegion::TMultRegion(const Opm::Deck& deck)
        : grid_    { deck }
        , fp_      { deck, Opm::Phases { true, true, true }, grid_, Opm::TableManager { deck } }
        , scanner_ { grid_, &fp_, deck.getKeywordList<Opm::ParserKeywords::MULTREGT>() }
    {}

    double TMultRegion::regular(const std::array<int,3>&    c1,
                                const std::array<int,3>&    c2,
                                const Opm::FaceDir::DirEnum direction) const
    {
        return this->scanner_
            .getRegionMultiplier(this->grid_.getGlobalIndex(c1[0], c1[1], c1[2]),
                                 this->grid_.getGlobalIndex(c2[0], c2[1], c2[2]),
                                 direction);
    }

    double TMultRegion::nnc(const std::array<int,3>& c1,
                            const std::array<int,3>& c2) const
    {
        return this->scanner_
            .getRegionMultiplierNNC(this->grid_.getGlobalIndex(c1[0], c1[1], c1[2]),
                                    this->grid_.getGlobalIndex(c2[0], c2[1], c2[2]));
    }

    Opm::Deck setup(const std::string& regsets,
                    const std::string& multregt)
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 1 6 2 /

GRID

DXV
  100.0 /

DYV
  6*100.0 /

DZV
  2*10.0 /

DEPTHZ
  14*2000.0 /

PORO
 12*0.25 /

PERMX
 12*100.0 /

PERMZ
 12*10.0 /

COPY
  PERMX PERMY /
/
)" + regsets + multregt);
    }

    namespace Regions {
        std::string same()
        {
            return { R"(MULTNUM
1 5*2   -- K=1
1 5*2 / -- K=2

FLUXNUM
1 5*2   -- K=1
1 5*2 / -- K=2
)" };
        }

        std::string f_plus_one()
        {
            return { R"(MULTNUM
2 5*3   -- K=1
2 5*3 / -- K=2

FLUXNUM
1 5*2   -- K=1
1 5*2 / -- K=2
)" };
        }
    } // namespace Regions

    namespace Multregt {
        std::string none() { return {}; }

        std::string repeated()
        {
            return { R"(
MULTREGT
  1 2  0.5  1*  'NNC'   'M' /
  1 2  0.1  1*  'NNC'   'M' /
/
)" };
        }

        std::string repeated_different_regsets()
        {
            return { R"(
MULTREGT
  1 2  0.5  1*  'NNC'   'F' /
  1 2  0.2  1*  'NNC'   'M' /
/
)" };
        }

        std::string same_but_different()
        {
            return { R"(
MULTREGT
  1 2  0.5  1*  'NNC'   'F' /
  2 3  0.1  1*  'NNC'   'M' /
/
)" };
        }
    } // namespace Multregt
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(No_Multiplier)
{
    const auto rmult = TMultRegion { setup(Regions::same(), Multregt::none()) };

    BOOST_CHECK_CLOSE(rmult.regular({ 0, 1, 0 }, { 0, 0, 1 }, Opm::FaceDir::DirEnum::YPlus), 1.0, 1.0e-8);
    BOOST_CHECK_CLOSE(rmult.nnc({ 0, 1, 0 }, { 0, 0, 1 }), 1.0, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Repeated_Take_Last)
{
    const auto rmult = TMultRegion { setup(Regions::same(), Multregt::repeated()) };

    BOOST_CHECK_CLOSE(rmult.regular({ 0, 1, 0 }, { 0, 0, 1 }, Opm::FaceDir::DirEnum::YPlus), 0.1, 1.0e-8);
    BOOST_CHECK_CLOSE(rmult.nnc({ 0, 1, 0 }, { 0, 0, 1 }), 0.1, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Repeated_Take_Last_Different_Regsets)
{
    const auto rmult = TMultRegion { setup(Regions::same(), Multregt::repeated_different_regsets()) };

    BOOST_CHECK_CLOSE(rmult.regular({ 0, 1, 0 }, { 0, 0, 1 }, Opm::FaceDir::DirEnum::YPlus), 0.2, 1.0e-8);
    BOOST_CHECK_CLOSE(rmult.nnc({ 0, 1, 0 }, { 0, 0, 1 }), 0.2, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Same_Conn_From_Multiple_Regsets)
{
    const auto rmult = TMultRegion { setup(Regions::f_plus_one(), Multregt::same_but_different()) };

    BOOST_CHECK_CLOSE(rmult.regular({ 0, 1, 0 }, { 0, 0, 1 }, Opm::FaceDir::DirEnum::YPlus), 0.05, 1.0e-8);
    BOOST_CHECK_CLOSE(rmult.nnc({ 0, 1, 0 }, { 0, 0, 1 }), 0.05, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END()     // MultiRegSet
