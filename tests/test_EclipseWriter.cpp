/*
  Copyright 2014 Andreas Lauser

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
#include "config.h"

#if HAVE_DYNAMIC_BOOST_TEST
#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE EclipseWriter
#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/EclipseWriter.hpp>
#include <opm/output/Cells.hpp>

#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/Units/ConversionFactors.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

// ERT stuff
#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/ecl_endian_flip.h>
#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_util.h>
#include <ert/util/ert_unique_ptr.hpp>
#include <ert/util/TestArea.hpp>

#include <memory>

using namespace Opm;

data::Solution createBlackoilState( int timeStepIdx, int numCells ) {

    std::vector< double > pressure( numCells );
    std::vector< double > swat( numCells );
    std::vector< double > sgas( numCells );
    std::vector< double > rs( numCells );
    std::vector< double > rv( numCells );

    for( int cellIdx = 0; cellIdx < numCells; ++cellIdx) {

        pressure[cellIdx] = timeStepIdx*1e5 + 1e4 + cellIdx;
        sgas[cellIdx] = timeStepIdx*1e5 +2.2e4 + cellIdx;
        swat[cellIdx] = timeStepIdx*1e5 +2.3e4 + cellIdx;

        // oil vaporization factor
        rv[cellIdx] = timeStepIdx*1e5 +3e4 + cellIdx;
        // gas dissolution factor
        rs[cellIdx] = timeStepIdx*1e5 + 4e4 + cellIdx;
    }

    data::Solution solution;
    using ds = data::Solution::key;

    solution.insert( ds::PRESSURE, pressure );
    solution.insert( ds::SWAT, swat );
    solution.insert( ds::SGAS, sgas );
    solution.insert( ds::RS, rs );
    solution.insert( ds::RV, rv );
    return solution;
}

template< typename T >
std::vector< T > getErtData( ecl_kw_type *eclKeyword ) {
    size_t kwSize = ecl_kw_get_size(eclKeyword);
    T* ertData = static_cast< T* >(ecl_kw_iget_ptr(eclKeyword, 0));

    return { ertData, ertData + kwSize };
}

template< typename T, typename U >
void compareErtData(const std::vector< T > &src,
                    const std::vector< U > &dst,
                    double tolerance ) {
    BOOST_CHECK_EQUAL(src.size(), dst.size());
    if (src.size() != dst.size())
        return;

    for (size_t i = 0; i < src.size(); ++i)
        BOOST_CHECK_CLOSE(src[i], dst[i], tolerance);
}

void compareErtData(const std::vector<int> &src, const std::vector<int> &dst)
{
    BOOST_CHECK_EQUAL_COLLECTIONS( src.begin(), src.end(),
                                   dst.begin(), dst.end() );
}

void checkEgridFile( const EclipseGrid& eclGrid ) {
    // use ERT directly to inspect the EGRID file produced by EclipseWriter
    auto egridFile = fortio_open_reader("FOO.EGRID", /*isFormated=*/0, ECL_ENDIAN_FLIP);

    const auto numCells = eclGrid.getNX() * eclGrid.getNY() * eclGrid.getNZ();

    while( auto* eclKeyword = ecl_kw_fread_alloc( egridFile ) ) {
        std::string keywordName(ecl_kw_get_header(eclKeyword));
        if (keywordName == "COORD") {
            std::vector< double > sourceData;
            eclGrid.exportCOORD( sourceData );
            auto resultData = getErtData< float >( eclKeyword );
            compareErtData(sourceData, resultData, 1e-6);
        }
        else if (keywordName == "ZCORN") {
            std::vector< double > sourceData;
            eclGrid.exportZCORN(sourceData);
            auto resultData = getErtData< float >( eclKeyword );
            compareErtData(sourceData, resultData, /*percentTolerance=*/1e-6);
        }
        else if (keywordName == "ACTNUM") {
            std::vector< int > sourceData( numCells );
            eclGrid.exportACTNUM(sourceData);
            auto resultData = getErtData< int >( eclKeyword );

            if( sourceData.empty() )
                sourceData.assign( numCells, 1 );

            compareErtData( sourceData, resultData );
        }

        ecl_kw_free(eclKeyword);
    }

    fortio_fclose(egridFile);
}

void checkInitFile( const EclipseGrid& grid , const Deck& deck, const std::vector<data::CellData>& simProps) {
    // use ERT directly to inspect the INIT file produced by EclipseWriter
    ERT::ert_unique_ptr<ecl_file_type , ecl_file_close> initFile(ecl_file_open( "FOO.INIT" , 0 ));

    for (int k=0; k < ecl_file_get_size(  initFile.get() ); k++) {
        ecl_kw_type * eclKeyword = ecl_file_iget_kw( initFile.get( ) , k );
        std::string keywordName(ecl_kw_get_header(eclKeyword));

        if (keywordName == "PORO") {
            const auto &sourceData = deck.getKeyword("PORO").getSIDoubleData();
            auto resultData = getErtData< float >( eclKeyword );
            compareErtData(sourceData, resultData, 1e-4);
        }

        if (keywordName == "PERMX") {
            const auto& sourceData = deck.getKeyword("PERMX").getSIDoubleData();
            auto resultData = getErtData< float >( eclKeyword );

            // convert the data from ERT from Field to SI units (mD to m^2)
            for (size_t i = 0; i < resultData.size(); ++i) {
                resultData[i] *= 9.869233e-16;
            }

            compareErtData(sourceData, resultData, 1e-4);
        }
    }

    BOOST_CHECK( ecl_file_has_kw( initFile.get() , "FIPNUM" ));
    BOOST_CHECK( ecl_file_has_kw( initFile.get() , "SATNUM" ));

    for (const auto& prop : simProps) {
        BOOST_CHECK( ecl_file_has_kw( initFile.get() , prop.name.c_str()) );
    }
}

void checkRestartFile( int timeStepIdx ) {
    using ds = data::Solution::key;

    for (int i = 0; i <= timeStepIdx; ++i) {
        auto sol = createBlackoilState( i, 3 * 3 * 3 );

        // use ERT directly to inspect the restart file produced by EclipseWriter
        auto rstFile = fortio_open_reader("FOO.UNRST", /*isFormated=*/0, ECL_ENDIAN_FLIP);

        int curSeqnum = -1;
        while( auto* eclKeyword = ecl_kw_fread_alloc(rstFile) ) {
            std::string keywordName(ecl_kw_get_header(eclKeyword));

            if (keywordName == "SEQNUM") {
                curSeqnum = *static_cast<int*>(ecl_kw_iget_ptr(eclKeyword, 0));
            }
            if (curSeqnum != i)
                continue;

            if (keywordName == "PRESSURE") {
                const auto resultData = getErtData< float >( eclKeyword );
                for( auto& x : sol[ ds::PRESSURE ] )
                    x /= Metric::Pressure;

                compareErtData( sol[ ds::PRESSURE ], resultData, 1e-4 );
            }

            if (keywordName == "SWAT") {
                const auto resultData = getErtData< float >( eclKeyword );
                compareErtData(sol[ ds::SWAT ], resultData, 1e-4);
            }

            if (keywordName == "SGAS") {
                const auto resultData = getErtData< float >( eclKeyword );
                compareErtData( sol[ ds::SGAS ], resultData, 1e-4 );
            }

            if (keywordName == "KRO")
                BOOST_CHECK_EQUAL( 1.0 * i * ecl_kw_get_size( eclKeyword ) , ecl_kw_element_sum_float( eclKeyword ));

            if (keywordName == "KRG")
                BOOST_CHECK_EQUAL( 10.0 * i * ecl_kw_get_size( eclKeyword ) , ecl_kw_element_sum_float( eclKeyword ));
        }

        fortio_fclose(rstFile);
    }
}

BOOST_AUTO_TEST_CASE(EclipseWriterIntegration)
{
    const char *deckString =
        "RUNSPEC\n"
        "UNIFOUT\n"
        "OIL\n"
        "GAS\n"
        "WATER\n"
        "METRIC\n"
        "DIMENS\n"
        "3 3 3/\n"
        "GRID\n"
        "INIT\n"
        "DXV\n"
        "1.0 2.0 3.0 /\n"
        "DYV\n"
        "4.0 5.0 6.0 /\n"
        "DZV\n"
        "7.0 8.0 9.0 /\n"
        "TOPS\n"
        "9*100 /\n"
        "PROPS\n"
        "PORO\n"
        "27*0.3 /\n"
        "PERMX\n"
        "27*1 /\n"
        "REGIONS\n"
        "SATNUM\n"
        "27*2 /\n"
        "FIPNUM\n"
        "27*3 /\n"
        "SOLUTION\n"
        "RPTRST\n"
        "BASIC=2\n"
        "/\n"
        "SCHEDULE\n"
        "TSTEP\n"
        "1.0 2.0 3.0 4.0 /\n"
        "WELSPECS\n"
        "'INJ' 'G' 1 1 2000 'GAS' /\n"
        "'PROD' 'G' 3 3 1000 'OIL' /\n"
        "/\n";

    auto deck = Parser().parseString( deckString, ParseContext() );
    auto es = std::make_shared< EclipseState >( Parser::parse( *deck ) );
    es->getIOConfig()->setBaseName( "FOO" );

    auto& eclGrid = *es->getInputGrid();

    {
        ERT::TestArea ta("test_ecl_writer");
        EclipseWriter eclWriter( es, eclGrid);

        auto start_time = ecl_util_make_date( 10, 10, 2008 );
        auto first_step = ecl_util_make_date( 10, 11, 2008 );
        std::vector<double> tranx(3*3*3);
        std::vector<double> trany(3*3*3);
        std::vector<double> tranz(3*3*3);
        std::vector<data::CellData> eGridProps{{"TRANX" , UnitSystem::measure::transmissibility, tranx},
                                              {"TRANY" , UnitSystem::measure::transmissibility, trany},
                                              {"TRANZ" , UnitSystem::measure::transmissibility, tranz}};

        eclWriter.writeInitAndEgrid( );
        eclWriter.writeInitAndEgrid( eGridProps );

        data::Wells wells;

        for( int i = 0; i < 5; ++i ) {
            std::vector<data::CellData> timesStepProps{{"KRO" , UnitSystem::measure::identity , std::vector<double>(3*3*3 , i)},
                                                 {"KRG" , UnitSystem::measure::identity , std::vector<double>(3*3*3 , i*10)}};

            eclWriter.writeTimeStep( i,
                                     false,
                                     first_step - start_time,
                                     createBlackoilState( i, 3 * 3 * 3 ),
                                     wells,
                                     timesStepProps);

            checkRestartFile( i );

        }
        checkInitFile( eclGrid , *deck , eGridProps);
        checkEgridFile( eclGrid );
    }
}
