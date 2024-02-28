// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2024 Equinor

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

#define BOOST_TEST_MODULE Datum_Depth_Manager

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/SimulationConfig/DatumDepth.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <stdexcept>
#include <string>

namespace {
    Opm::DatumDepth makeDatumDepth(const std::string& inputString)
    {
        return Opm::DatumDepth {
            Opm::SOLUTIONSection { Opm::Parser{}.parseString(inputString) }
        };
    }
}

BOOST_AUTO_TEST_SUITE(Basic_Operations)

BOOST_AUTO_TEST_CASE(Default)
{
    const auto dd = Opm::DatumDepth{};

    BOOST_CHECK_CLOSE(dd(1), 0.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 11), 0.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("NOSUCHREG", 22), 0.0, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END()     // Basic_Operations

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Unset)

BOOST_AUTO_TEST_CASE(No_Solution_Data)
{
    const auto dd = makeDatumDepth(R"(RUNSPEC
DIMENS
1 5 2 /
GRID
DXV
100.0 /
DYV
5*100.0 /
DZV
2*10.0 /
DEPTHZ
12*2000.0 /
END
)");

    BOOST_CHECK_CLOSE(dd(1), 0.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 11), 0.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("NOSUCHREG", 22), 0.0, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(No_Equilibration_Data)
{
    const auto dd = makeDatumDepth(R"(RUNSPEC
DIMENS
1 5 2 /
GRID
DXV
100.0 /
DYV
5*100.0 /
DZV
2*10.0 /
DEPTHZ
12*2000.0 /
SOLUTION
PRESSURE
10*123.4 /
SWAT
10*0.123 /
SGAS
10*0.4 /
END
)");

    BOOST_CHECK_CLOSE(dd(1), 0.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 11), 0.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("NOSUCHREG", 22), 0.0, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END()     // Unset

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Global)

BOOST_AUTO_TEST_CASE(Equilibration_Single_Region)
{
    const auto dd = makeDatumDepth(R"(RUNSPEC
DIMENS
1 5 2 /
EQLDIMS
/
GRID
DXV
100.0 /
DYV
5*100.0 /
DZV
2*10.0 /
DEPTHZ
12*2000.0 /
SOLUTION
EQUIL
  2005.0 123.4 2015.0 2.34 1995.0 0.0 /
END
)");

    // Datum depth in first equilibration region
    const auto expect = 2005.0;

    BOOST_CHECK_CLOSE(dd(1), expect, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 11), expect, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("NOSUCHREG", 22), expect, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Equilibration_Multiple_Regions)
{
    const auto dd = makeDatumDepth(R"(RUNSPEC
DIMENS
1 5 2 /
EQLDIMS
3 /
GRID
DXV
100.0 /
DYV
5*100.0 /
DZV
2*10.0 /
DEPTHZ
12*2000.0 /
SOLUTION
EQUIL
  2005.0 123.4 2015.0 2.34 1995.0 0.0 /
  2100.0 157.9 2015.0 2.34 1995.0 0.0 /
  / -- Copy of region 2
END
)");

    // Datum depth in first equilibration region
    const auto expect = 2005.0;

    BOOST_CHECK_CLOSE(dd(1), expect, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 11), expect, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("NOSUCHREG", 22), expect, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Explicit_Datum_Keyword)
{
    const auto dd = makeDatumDepth(R"(RUNSPEC
DIMENS
1 5 2 /
EQLDIMS
3 /
GRID
DXV
100.0 /
DYV
5*100.0 /
DZV
2*10.0 /
DEPTHZ
12*2000.0 /
SOLUTION
DATUM
  2007.5 /
EQUIL
  2005.0 123.4 2015.0 2.34 1995.0 0.0 /
  2100.0 157.9 2015.0 2.34 1995.0 0.0 /
  / -- Copy of region 2
END
)");

    // Datum depth from 'DATUM' keyword
    const auto expect = 2007.5;

    BOOST_CHECK_CLOSE(dd(1), expect, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 11), expect, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("NOSUCHREG", 22), expect, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Explicit_Datum_Keyword_Order_Reversed)
{
    const auto dd = makeDatumDepth(R"(RUNSPEC
DIMENS
1 5 2 /
EQLDIMS
/
GRID
DXV
100.0 /
DYV
5*100.0 /
DZV
2*10.0 /
DEPTHZ
12*2000.0 /
SOLUTION
EQUIL
  2005.0 123.4 2015.0 2.34 1995.0 0.0 /
DATUM
  2007.5 /
END
)");

    // Datum depth from 'DATUM' keyword
    const auto expect = 2007.5;

    BOOST_CHECK_CLOSE(dd(1), expect, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 11), expect, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("NOSUCHREG", 22), expect, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END()     // Global

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Default_RegSet)

BOOST_AUTO_TEST_CASE(Fully_Specified)
{
    const auto dd = makeDatumDepth(R"(RUNSPEC
DIMENS
1 5 2 /
TABDIMS
 4* 5 / // NTFIP (=TABDIMS(5)) = 5
EQLDIMS
/
GRID
DXV
100.0 /
DYV
5*100.0 /
DZV
2*10.0 /
DEPTHZ
12*2000.0 /
SOLUTION
EQUIL
  2015.0 123.4 2015.0 2.34 1995.0 0.0 /
DATUMR
  2001.0 2002.0 2003.0 2004.0 2005.0 /
END
)");

    BOOST_CHECK_CLOSE(dd(1 - 1), 2001.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(2 - 1), 2002.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(3 - 1), 2003.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(4 - 1), 2004.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(5 - 1), 2005.0, 1.0e-8);

    BOOST_CHECK_CLOSE(dd("FIPRE2", 1 - 1), 2001.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 2 - 1), 2002.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 3 - 1), 2003.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 4 - 1), 2004.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 5 - 1), 2005.0, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Defaulted_High_Regions)
{
    const auto dd = makeDatumDepth(R"(RUNSPEC
DIMENS
1 5 2 /
TABDIMS
 4* 5 / // NTFIP (=TABDIMS(5)) = 5
EQLDIMS
/
GRID
DXV
100.0 /
DYV
5*100.0 /
DZV
2*10.0 /
DEPTHZ
12*2000.0 /
SOLUTION
EQUIL
  2015.0 123.4 2015.0 2.34 1995.0 0.0 /
DATUMR
  2001.0 2002.0 2003.0 /
END
)");

    BOOST_CHECK_CLOSE(dd(1 - 1), 2001.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(2 - 1), 2002.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(3 - 1), 2003.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(4 - 1), 2003.0, 1.0e-8); // Defaults to region 3
    BOOST_CHECK_CLOSE(dd(5 - 1), 2003.0, 1.0e-8); // Defaults to region 3

    BOOST_CHECK_CLOSE(dd("FIPRE2", 1 - 1), 2001.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 2 - 1), 2002.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 3 - 1), 2003.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 4 - 1), 2003.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 5 - 1), 2003.0, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END()     // Default_RegSet

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Extended_Region_Sets)

BOOST_AUTO_TEST_CASE(Fully_Specified)
{
    const auto dd = makeDatumDepth(R"(RUNSPEC
DIMENS
1 5 2 /
TABDIMS
 4* 5 / // NTFIP (=TABDIMS(5)) = 5
EQLDIMS
/
GRID
DXV
100.0 /
DYV
5*100.0 /
DZV
2*10.0 /
DEPTHZ
12*2000.0 /
SOLUTION
EQUIL
  2015.0 123.4 2015.0 2.34 1995.0 0.0 /
DATUMRX
  '' 2001.0 2002.0 2003.0 2004.0 2005.0 / -- FIPNUM
  'FIPRE2' 2001.5 2002.5 2003.5 2004.5 2005.5 /
/
END
)");

    BOOST_CHECK_CLOSE(dd(1 - 1), 2001.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(2 - 1), 2002.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(3 - 1), 2003.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(4 - 1), 2004.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(5 - 1), 2005.0, 1.0e-8);

    BOOST_CHECK_CLOSE(dd("FIPRE2", 1 - 1), 2001.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 2 - 1), 2002.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 3 - 1), 2003.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 4 - 1), 2004.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 5 - 1), 2005.5, 1.0e-8);

    // No default datum depth (DATUMR missing) => exception for unknown sets.
    BOOST_CHECK_THROW(dd("FIPUNKNW", 5 - 1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Defaulted_High_Regions)
{
    const auto dd = makeDatumDepth(R"(RUNSPEC
DIMENS
1 5 2 /
TABDIMS
 4* 5 / // NTFIP (=TABDIMS(5)) = 5
EQLDIMS
/
GRID
DXV
100.0 /
DYV
5*100.0 /
DZV
2*10.0 /
DEPTHZ
12*2000.0 /
SOLUTION
EQUIL
  2015.0 123.4 2015.0 2.34 1995.0 0.0 /
DATUMRX
  '' 2001.0 2002.0 2003.0 / -- FIPNUM
  'FIPRE2' 2001.5 2002.5 /
/
END
)");

    BOOST_CHECK_CLOSE(dd(1 - 1), 2001.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(2 - 1), 2002.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(3 - 1), 2003.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(4 - 1), 2003.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(5 - 1), 2003.0, 1.0e-8);

    BOOST_CHECK_CLOSE(dd("FIPRE2", 1 - 1), 2001.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 2 - 1), 2002.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 3 - 1), 2002.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 4 - 1), 2002.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 5 - 1), 2002.5, 1.0e-8);

    // No default datum depth (DATUMR missing) => exception for unknown sets.
    BOOST_CHECK_THROW(dd("FIPUNKNW", 5 - 1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(DatumR_Fallback)
{
    const auto dd = makeDatumDepth(R"(RUNSPEC
DIMENS
1 5 2 /
TABDIMS
 4* 5 / // NTFIP (=TABDIMS(5)) = 5
EQLDIMS
/
GRID
DXV
100.0 /
DYV
5*100.0 /
DZV
2*10.0 /
DEPTHZ
12*2000.0 /
SOLUTION
EQUIL
  2015.0 123.4 2015.0 2.34 1995.0 0.0 /
DATUMR
  1995.1 1996.1 1997.1 1998.1 1999.1 /
DATUMRX
  '' 2001.0 2002.0 2003.0 2004.0 2005.0 / -- FIPNUM
  'FIPRE2' 2001.5 2002.5 2003.5 2004.5 2005.5 /
/
END
)");

    BOOST_CHECK_CLOSE(dd(1 - 1), 2001.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(2 - 1), 2002.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(3 - 1), 2003.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(4 - 1), 2004.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(5 - 1), 2005.0, 1.0e-8);

    BOOST_CHECK_CLOSE(dd("FIPRE2", 1 - 1), 2001.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 2 - 1), 2002.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 3 - 1), 2003.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 4 - 1), 2004.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 5 - 1), 2005.5, 1.0e-8);

    BOOST_CHECK_CLOSE(dd("FIPABC", 1 - 1), 1995.1, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPABC", 2 - 1), 1996.1, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPABC", 3 - 1), 1997.1, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPABC", 4 - 1), 1998.1, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPABC", 5 - 1), 1999.1, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(DatumR_No_Fallback)
{
    const auto dd = makeDatumDepth(R"(RUNSPEC
DIMENS
1 5 2 /
TABDIMS
 4* 5 / // NTFIP (=TABDIMS(5)) = 5
EQLDIMS
/
GRID
DXV
100.0 /
DYV
5*100.0 /
DZV
2*10.0 /
DEPTHZ
12*2000.0 /
SOLUTION
EQUIL
  2015.0 123.4 2015.0 2.34 1995.0 0.0 /
DATUMRX
  '' 2001.0 2002.0 2003.0 2004.0 2005.0 / -- FIPNUM
  'FIPRE2' 2001.5 2002.5 2003.5 2004.5 2005.5 /
--
-- DATUMR *after* DATUMRX => no fallback
--
DATUMR
  1995.1 1996.1 1997.1 1998.1 1999.1 /
/
END
)");

    BOOST_CHECK_CLOSE(dd(1 - 1), 2001.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(2 - 1), 2002.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(3 - 1), 2003.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(4 - 1), 2004.0, 1.0e-8);
    BOOST_CHECK_CLOSE(dd(5 - 1), 2005.0, 1.0e-8);

    BOOST_CHECK_CLOSE(dd("FIPRE2", 1 - 1), 2001.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 2 - 1), 2002.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 3 - 1), 2003.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 4 - 1), 2004.5, 1.0e-8);
    BOOST_CHECK_CLOSE(dd("FIPRE2", 5 - 1), 2005.5, 1.0e-8);

    BOOST_CHECK_THROW(dd("FIPABC", 1 - 1), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()      // Extended_Region_Sets
