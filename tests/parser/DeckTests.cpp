/*
  Copyright 2013 Statoil ASA.

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


#include <stdexcept>
#include <sstream>

#define BOOST_TEST_MODULE DeckTests

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Deck/DeckOutput.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(Initializer_lists) {
    DeckKeyword foo( "foo" );
    DeckKeyword bar( "bar" );

    std::string foostr( "foo" );
    std::string barstr( "bar" );

    BOOST_REQUIRE_NO_THROW( Deck( { foo, bar } ) );
    BOOST_REQUIRE_NO_THROW( Deck( { foostr, barstr } ) );
    BOOST_REQUIRE_NO_THROW( Deck( { "Kappa", "Phi" } ) );
}

BOOST_AUTO_TEST_CASE(hasKeyword_empty_returnFalse) {
    Deck deck;
    BOOST_CHECK_EQUAL(false, deck.hasKeyword("Bjarne"));
    BOOST_CHECK_THROW( deck.getKeyword("Bjarne") , std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(addKeyword_singlekeyword_keywordAdded) {
    Deck deck;
    BOOST_CHECK_NO_THROW(deck.addKeyword( DeckKeyword( "BJARNE" ) ) );
}


BOOST_AUTO_TEST_CASE(getKeywordList_empty_list) {
    Deck deck;
    auto kw_list = deck.getKeywordList("TRULS");
    BOOST_CHECK_EQUAL( kw_list.size() , 0 );
}

BOOST_AUTO_TEST_CASE(getKeyword_singlekeyword_outRange_throws) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "BJARNE" ) );
    BOOST_CHECK_THROW(deck.getKeyword("BJARNE" , 10) , std::out_of_range);
}


BOOST_AUTO_TEST_CASE(getKeywordList_returnOK) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "BJARNE" ) );
    BOOST_CHECK_NO_THROW( deck.getKeywordList("BJARNE") );
}


BOOST_AUTO_TEST_CASE(getKeyword_indexok_returnskeyword) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "BJARNE" ) );
    BOOST_CHECK_NO_THROW(deck.getKeyword(0));
}

BOOST_AUTO_TEST_CASE(numKeyword_singlekeyword_return1) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "BJARNE" ) );
    BOOST_CHECK_EQUAL(1U , deck.count("BJARNE"));
}


BOOST_AUTO_TEST_CASE(numKeyword_twokeyword_return2) {
    Deck deck;
    DeckKeyword keyword("BJARNE");
    deck.addKeyword(keyword);
    deck.addKeyword(keyword);
    BOOST_CHECK_EQUAL(2U , deck.count("BJARNE"));
}


BOOST_AUTO_TEST_CASE(numKeyword_nokeyword_return0) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "BJARNE" ) );
    BOOST_CHECK_EQUAL(0U , deck.count("BJARNEX"));
}


BOOST_AUTO_TEST_CASE(size_twokeyword_return2) {
    Deck deck;
    DeckKeyword keyword ("BJARNE");
    deck.addKeyword(keyword);
    deck.addKeyword(keyword);
    BOOST_CHECK_EQUAL(2U , deck.size());
}

BOOST_AUTO_TEST_CASE(getKeyword_outOfRange_throws) {
    Deck deck;
    deck.addKeyword(DeckKeyword( "TRULS" ) );
    BOOST_CHECK_THROW( deck.getKeyword("TRULS" , 3) , std::out_of_range);
}

BOOST_AUTO_TEST_CASE(getKeywordList_OK) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "TRULS" ) );
    deck.addKeyword( DeckKeyword( "TRULS" ) );
    deck.addKeyword( DeckKeyword( "TRULS" ) );

    const auto& keywordList = deck.getKeywordList("TRULS");
    BOOST_CHECK_EQUAL( 3U , keywordList.size() );
}

BOOST_AUTO_TEST_CASE(keywordList_getnum_OK) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "TRULS" ) );
    deck.addKeyword( DeckKeyword( "TRULS" ) );
    deck.addKeyword( DeckKeyword( "TRULSX" ) );

    BOOST_CHECK_EQUAL( 0U , deck.count( "TRULSY" ));
    BOOST_CHECK_EQUAL( 2U , deck.count( "TRULS" ));
    BOOST_CHECK_EQUAL( 1U , deck.count( "TRULSX" ));
}

BOOST_AUTO_TEST_CASE(keywordList_getbyindexoutofbounds_exceptionthrown) {
    Deck deck;
    BOOST_CHECK_THROW(deck.getKeyword(0), std::out_of_range);
    deck.addKeyword( DeckKeyword( "TRULS" ) );
    deck.addKeyword( DeckKeyword( "TRULS" ) );
    deck.addKeyword( DeckKeyword( "TRULSX" ) );
    BOOST_CHECK_NO_THROW(deck.getKeyword(2));
    BOOST_CHECK_THROW(deck.getKeyword(3), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(keywordList_getbyindex_correctkeywordreturned) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "TRULS" ) );
    deck.addKeyword( DeckKeyword( "TRULS" ) );
    deck.addKeyword( DeckKeyword( "TRULSX" ) );
    BOOST_CHECK_EQUAL("TRULS",  deck.getKeyword(0).name());
    BOOST_CHECK_EQUAL("TRULS",  deck.getKeyword(1).name());
    BOOST_CHECK_EQUAL("TRULSX", deck.getKeyword(2).name());
}

BOOST_AUTO_TEST_CASE(set_and_get_data_file) {
    Deck deck;
    BOOST_CHECK_EQUAL("", deck.getDataFile());
    std::string file("/path/to/file.DATA");
    deck.setDataFile( file );
    BOOST_CHECK_EQUAL(file, deck.getDataFile());
}

BOOST_AUTO_TEST_CASE(DummyDefaultsString) {
    DeckItem deckStringItem("TEST", std::string() );
    BOOST_CHECK_EQUAL(deckStringItem.size(), 0);

    deckStringItem.push_backDummyDefault();
    BOOST_CHECK_EQUAL(deckStringItem.size(), 0);
    BOOST_CHECK_EQUAL(true, deckStringItem.defaultApplied(0));
    BOOST_CHECK_THROW(deckStringItem.get< std::string >(0), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(GetStringAtIndex_NoData_ExceptionThrown) {
    DeckItem deckStringItem( "TEST", std::string() );
    BOOST_CHECK_THROW(deckStringItem.get< std::string >(0), std::out_of_range);
    deckStringItem.push_back("SA");
    BOOST_CHECK_THROW(deckStringItem.get< std::string >(1), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(size_variouspushes_sizecorrect) {
    DeckItem deckStringItem( "TEST", std::string() );

    BOOST_CHECK_EQUAL(0U, deckStringItem.size());
    deckStringItem.push_back("WELL-3");
    BOOST_CHECK_EQUAL(1U, deckStringItem.size());

    deckStringItem.push_back("WELL-4");
    deckStringItem.push_back("WELL-5");
    BOOST_CHECK_EQUAL(3U, deckStringItem.size());
}

BOOST_AUTO_TEST_CASE(DefaultNotAppliedString) {
    DeckItem deckStringItem( "TEST", std::string() );
    BOOST_CHECK( deckStringItem.size() == 0 );

    deckStringItem.push_back( "FOO") ;
    BOOST_CHECK( deckStringItem.size() == 1 );
    BOOST_CHECK( deckStringItem.get< std::string >(0) == "FOO" );
    BOOST_CHECK( !deckStringItem.defaultApplied(0) );
}

BOOST_AUTO_TEST_CASE(DefaultAppliedString) {
    DeckItem deckStringItem( "TEST", std::string() );
    BOOST_CHECK( deckStringItem.size() == 0 );

    deckStringItem.push_backDefault( "FOO" );
    BOOST_CHECK( deckStringItem.size() == 1 );
    BOOST_CHECK( deckStringItem.get< std::string >(0) == "FOO" );
    BOOST_CHECK( deckStringItem.defaultApplied(0) );
}


BOOST_AUTO_TEST_CASE(PushBackMultipleString) {
    DeckItem stringItem( "TEST", std::string() );
    stringItem.push_back("Heisann ", 100U );
    BOOST_CHECK_EQUAL( 100U , stringItem.size() );
    for (size_t i=0; i < 100; i++)
        BOOST_CHECK_EQUAL("Heisann " , stringItem.get< std::string >(i));
}

BOOST_AUTO_TEST_CASE(GetDoubleAtIndex_NoData_ExceptionThrown) {
    DeckItem deckDoubleItem( "TEST", double() );

    BOOST_CHECK_THROW(deckDoubleItem.get< double >(0), std::out_of_range);
    deckDoubleItem.push_back(1.89);
    BOOST_CHECK_THROW(deckDoubleItem.get< double >(1), std::out_of_range);
}


BOOST_AUTO_TEST_CASE(sizeDouble_correct) {
    DeckItem deckDoubleItem( "TEST", double() );

    BOOST_CHECK_EQUAL( 0U , deckDoubleItem.size());
    deckDoubleItem.push_back( 100.0 );
    BOOST_CHECK_EQUAL( 1U , deckDoubleItem.size());

    deckDoubleItem.push_back( 100.0 );
    deckDoubleItem.push_back( 100.0 );
    BOOST_CHECK_EQUAL( 3U , deckDoubleItem.size());
}



BOOST_AUTO_TEST_CASE(SetInDeck) {
    DeckItem deckDoubleItem( "TEST", double() );
    BOOST_CHECK( deckDoubleItem.size() == 0 );

    deckDoubleItem.push_backDefault( 1.0 );
    BOOST_CHECK( deckDoubleItem.size() == 1 );
    BOOST_CHECK_EQUAL( true , deckDoubleItem.defaultApplied(0) );

    deckDoubleItem.push_back( 10.0 );
    BOOST_CHECK( deckDoubleItem.size() == 2 );
    BOOST_CHECK_EQUAL( false , deckDoubleItem.defaultApplied(1) );

    deckDoubleItem.push_backDefault( 1.0 );
    BOOST_CHECK( deckDoubleItem.size() == 3 );
    BOOST_CHECK_EQUAL( true , deckDoubleItem.defaultApplied(2) );
}

BOOST_AUTO_TEST_CASE(DummyDefaultsDouble) {
    DeckItem deckDoubleItem( "TEST", double() );
    BOOST_CHECK_EQUAL(deckDoubleItem.size(), 0);

    deckDoubleItem.push_backDummyDefault();
    BOOST_CHECK_EQUAL(deckDoubleItem.size(), 0);
    BOOST_CHECK_EQUAL(true, deckDoubleItem.defaultApplied(0));
    BOOST_CHECK_THROW(deckDoubleItem.get< double >(0), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(PushBackMultipleDouble) {
    DeckItem item( "HEI", double() );
    item.push_back(10.22 , 100 );
    BOOST_CHECK_EQUAL( 100U , item.size() );
    for (size_t i=0; i < 100; i++)
        BOOST_CHECK_EQUAL(10.22 , item.get< double >(i));
}

BOOST_AUTO_TEST_CASE(PushBackDimension) {
    DeckItem item( "HEI", double() );
    Dimension activeDimension{ "Length" , 100 };
    Dimension defaultDimension{ "Length" , 10 };

    item.push_back(1.234);
    item.push_backDimension( activeDimension , defaultDimension);

    item.push_backDefault(5.678);
    item.push_backDimension( activeDimension , defaultDimension);
}

BOOST_AUTO_TEST_CASE(PushBackDimensionInvalidType) {
    DeckItem item( "HEI", int() );
    Dimension dim{ "Length" , 100 };
    BOOST_CHECK_THROW( item.push_backDimension( dim , dim ) , std::logic_error );
}

BOOST_AUTO_TEST_CASE(GetSIWithoutDimensionThrows) {
    DeckItem item( "HEI", double() );
    item.push_back(10.22 , 100 );

    BOOST_CHECK_THROW( item.getSIDouble(0) , std::invalid_argument );
    BOOST_CHECK_THROW( item.getSIDoubleData() , std::invalid_argument );
}

BOOST_AUTO_TEST_CASE(GetSISingleDimensionCorrect) {
    DeckItem item( "HEI", double() );
    Dimension dim{ "Length" , 100 };

    item.push_back(1.0 , 100 );
    item.push_backDimension( dim , dim );

    BOOST_CHECK_EQUAL( 1.0   , item.get< double >(0) );
    BOOST_CHECK_EQUAL( 100 , item.getSIDouble(0) );
}

BOOST_AUTO_TEST_CASE(GetSISingleDefault) {
    DeckItem item( "HEI", double() );
    Dimension dim{ "Length" , 1 };
    Dimension defaultDim{ "Length" , 100 };

    item.push_backDefault( 1.0 );
    item.push_backDimension( dim , defaultDim );

    BOOST_CHECK_EQUAL( 1   , item.get< double >(0) );
    BOOST_CHECK_EQUAL( 100 , item.getSIDouble(0) );
}

BOOST_AUTO_TEST_CASE(GetSIMultipleDim) {
    DeckItem item( "HEI", double() );
    Dimension dim1{ "Length" , 2 };
    Dimension dim2{ "Length" , 4 };
    Dimension dim3{ "Length" , 8 };
    Dimension dim4{ "Length" ,16 };
    Dimension defaultDim{ "Length" , 100 };

    item.push_back( 1.0, 16 );
    item.push_backDimension( dim1 , defaultDim );
    item.push_backDimension( dim2 , defaultDim );
    item.push_backDimension( dim3 , defaultDim );
    item.push_backDimension( dim4 , defaultDim );

    for (size_t i=0; i < 16; i+= 4) {
        BOOST_CHECK_EQUAL( 2   , item.getSIDouble(i) );
        BOOST_CHECK_EQUAL( 4   , item.getSIDouble(i+ 1) );
        BOOST_CHECK_EQUAL( 8   , item.getSIDouble(i+2) );
        BOOST_CHECK_EQUAL(16   , item.getSIDouble(i+3) );
    }
}

BOOST_AUTO_TEST_CASE(HasValue) {
    DeckItem deckIntItem( "TEST", int() );
    BOOST_CHECK_EQUAL( false , deckIntItem.hasValue(0) );
    deckIntItem.push_back(1);
    BOOST_CHECK_EQUAL( true  , deckIntItem.hasValue(0) );
    BOOST_CHECK_EQUAL( false , deckIntItem.hasValue(1) );
}

BOOST_AUTO_TEST_CASE(DummyDefaultsInt) {
    DeckItem deckIntItem( "TEST", int() );
    BOOST_CHECK_EQUAL(deckIntItem.size(), 0);

    deckIntItem.push_backDummyDefault();
    BOOST_CHECK_EQUAL(deckIntItem.size(), 0);
    BOOST_CHECK_EQUAL(true, deckIntItem.defaultApplied(0));
    BOOST_CHECK_EQUAL( false , deckIntItem.hasValue(0));
    BOOST_CHECK_EQUAL( false , deckIntItem.hasValue(1));
    BOOST_CHECK_THROW(deckIntItem.get< int >(0), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(GetIntAtIndex_NoData_ExceptionThrown) {
    DeckItem deckIntItem( "TEST", int() );
    deckIntItem.push_back(100);
    BOOST_CHECK(deckIntItem.get< int >(0) == 100);
    BOOST_CHECK_THROW(deckIntItem.get< int >(1), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(InitializeDefaultApplied) {
    DeckItem deckIntItem( "TEST", int() );
    BOOST_CHECK( deckIntItem.size() == 0 );
}

BOOST_AUTO_TEST_CASE(size_correct) {
    DeckItem deckIntItem( "TEST", int() );

    BOOST_CHECK_EQUAL( 0U , deckIntItem.size());
    deckIntItem.push_back( 100 );
    BOOST_CHECK_EQUAL( 1U , deckIntItem.size());

    deckIntItem.push_back( 100 );
    deckIntItem.push_back( 100 );
    BOOST_CHECK_EQUAL( 3U , deckIntItem.size());
}

BOOST_AUTO_TEST_CASE(DefaultNotAppliedInt) {
    DeckItem deckIntItem( "TEST", int() );
    BOOST_CHECK( deckIntItem.size() == 0 );

    deckIntItem.push_back( 100 );
    BOOST_CHECK( deckIntItem.size() == 1 );
    BOOST_CHECK( deckIntItem.get< int >(0) == 100 );
    BOOST_CHECK( !deckIntItem.defaultApplied(0) );

    BOOST_CHECK_THROW( deckIntItem.defaultApplied(1), std::out_of_range );
    BOOST_CHECK_THROW( deckIntItem.get< int >(1), std::out_of_range );
}

BOOST_AUTO_TEST_CASE(UseDefault) {
    DeckItem deckIntItem( "TEST", int() );

    deckIntItem.push_backDefault( 100 );

    BOOST_CHECK( deckIntItem.defaultApplied(0) );
    BOOST_CHECK( deckIntItem.get< int >(0) == 100 );

    BOOST_CHECK_THROW( deckIntItem.defaultApplied(1), std::out_of_range );
    BOOST_CHECK_THROW( deckIntItem.get< int >(1), std::out_of_range );
}

BOOST_AUTO_TEST_CASE(DefaultAppliedInt) {
    DeckItem deckIntItem( "TEST", int() );
    BOOST_CHECK( deckIntItem.size() == 0 );

    deckIntItem.push_backDefault( 100 );
    BOOST_CHECK( deckIntItem.size() == 1 );
    BOOST_CHECK( deckIntItem.get< int >(0) == 100 );
    BOOST_CHECK( deckIntItem.defaultApplied(0) );
    deckIntItem.push_back( 10 );
    BOOST_CHECK_EQUAL( false, deckIntItem.defaultApplied(1) );
    deckIntItem.push_backDefault( 1 );
    BOOST_CHECK_EQUAL( true , deckIntItem.defaultApplied(2) );
    BOOST_CHECK_EQUAL( 3 , deckIntItem.size() );
}


BOOST_AUTO_TEST_CASE(PushBackMultipleInt) {
    DeckItem item( "HEI", int() );
    item.push_back(10 , 100U );
    BOOST_CHECK_EQUAL( 100U , item.size() );
    for (size_t i=0; i < 100; i++)
        BOOST_CHECK_EQUAL(10 , item.get< int >(i));
}

BOOST_AUTO_TEST_CASE(size_defaultConstructor_sizezero) {
    DeckRecord deckRecord;
    BOOST_CHECK_EQUAL(0U, deckRecord.size());
}

BOOST_AUTO_TEST_CASE(addItem_singleItem_sizeone) {
    DeckRecord deckRecord;
    deckRecord.addItem( DeckItem { "TEST", int() } );
    BOOST_CHECK_EQUAL(1U, deckRecord.size());
}

BOOST_AUTO_TEST_CASE(addItem_multipleItems_sizecorrect) {

    DeckRecord deckRecord;
    deckRecord.addItem( DeckItem { "TEST", int() } );
    deckRecord.addItem( DeckItem { "TEST2", int() } );
    deckRecord.addItem( DeckItem { "TEST3", int() } );

    BOOST_CHECK_EQUAL(3U, deckRecord.size());
}

BOOST_AUTO_TEST_CASE(addItem_differentItemsSameName_throws) {
    DeckRecord deckRecord;
    deckRecord.addItem( DeckItem { "TEST", int() } );
    BOOST_CHECK_THROW( deckRecord.addItem( DeckItem { "TEST", int() } ), std::invalid_argument );
    std::vector< DeckItem > items = { DeckItem { "TEST", int() }, DeckItem { "TEST" , int() } };
    BOOST_CHECK_THROW( DeckRecord( std::move( items ) ), std::invalid_argument );
}

BOOST_AUTO_TEST_CASE(get_byIndex_returnsItem) {
    DeckRecord deckRecord;
    deckRecord.addItem( DeckItem { "TEST", int() } );
    BOOST_CHECK_NO_THROW(deckRecord.getItem(0U));
}

BOOST_AUTO_TEST_CASE(get_indexoutofbounds_throws) {
    DeckRecord deckRecord;
    deckRecord.addItem( DeckItem { "TEST", int() } );
    BOOST_CHECK_THROW(deckRecord.getItem(1), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(get_byName_returnsItem) {
    DeckRecord deckRecord;
    deckRecord.addItem( DeckItem { "TEST", int() } );
    deckRecord.getItem("TEST");
}

BOOST_AUTO_TEST_CASE(get_byNameNonExisting_throws) {
    DeckRecord deckRecord;
    deckRecord.addItem( DeckItem { "TEST", int() } );
    BOOST_CHECK_THROW(deckRecord.getItem("INVALID"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(StringsWithSpaceOK) {
    ParserItem itemString("STRINGITEM1", "" );
    ParserRecord record1;
    RawRecord rawRecord( " ' VALUE ' " );
    ParseContext parseContext;
    record1.addItem( itemString );


    const auto deckRecord = record1.parse( parseContext , rawRecord );
    BOOST_CHECK_EQUAL(" VALUE " , deckRecord.getItem(0).get< std::string >(0));
}

BOOST_AUTO_TEST_CASE(DataKeyword) {
    DeckKeyword kw("KW");
    BOOST_CHECK_EQUAL( false , kw.isDataKeyword());
    kw.setDataKeyword( );
    BOOST_CHECK_EQUAL( true , kw.isDataKeyword());
    kw.setDataKeyword( false );
    BOOST_CHECK_EQUAL( false , kw.isDataKeyword());
    kw.setDataKeyword( true );
    BOOST_CHECK_EQUAL( true , kw.isDataKeyword());
}



BOOST_AUTO_TEST_CASE(name_nameSetInConstructor_nameReturned) {
    DeckKeyword deckKeyword( "KW" );
    BOOST_CHECK_EQUAL("KW", deckKeyword.name());
}

BOOST_AUTO_TEST_CASE(size_noRecords_returnszero) {
    DeckKeyword deckKeyword( "KW" );
    BOOST_CHECK_EQUAL(0U, deckKeyword.size());
}


BOOST_AUTO_TEST_CASE(addRecord_onerecord_recordadded) {
    DeckKeyword deckKeyword( "KW" );
    deckKeyword.addRecord( DeckRecord() );
    BOOST_CHECK_EQUAL(1U, deckKeyword.size());
    for (auto iter = deckKeyword.begin(); iter != deckKeyword.end(); ++iter) {
        //
    }

}

BOOST_AUTO_TEST_CASE(getRecord_outofrange_exceptionthrown) {
    DeckKeyword deckKeyword( "KW" );
    deckKeyword.addRecord(DeckRecord());
    BOOST_CHECK_THROW(deckKeyword.getRecord(1), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(setUnknown_wasknown_nowunknown) {
    DeckKeyword deckKeyword( "KW", false );
    BOOST_CHECK(!deckKeyword.isKnown());
}



BOOST_AUTO_TEST_CASE(DeckItemWrite) {
    DeckItem item("TEST", int());
    std::stringstream s;
    DeckOutput w(s);

    item.push_back(1);
    item.push_back(2);
    item.push_back(3);

    item.write(w);
    {
        int v1,v2,v3;
        s >> v1;
        s >> v2;
        s >> v3;

        BOOST_CHECK_EQUAL( v1 , 1 );
        BOOST_CHECK_EQUAL( v2 , 2 );
        BOOST_CHECK_EQUAL( v3 , 3 );
    }
}

BOOST_AUTO_TEST_CASE(DeckOutputTest) {
    std::string expected = "KEYWORD\n\
==1-2\n\
==3-1*\n\
==5-1*\n\
==7-8\n\
==1*-10 /\n\
/\n\
ABC";
    std::stringstream s;
    DeckOutput out(s);

    out.record_indent = "==";
    out.item_sep = "-";
    out.columns = 2;
    out.keyword_sep = "ABC";

    out.start_keyword("KEYWORD");
    out.start_record();
    out.write<int>(1);
    out.write<int>(2);
    out.write<int>(3);
    out.stash_default( );
    out.write<int>(5);
    out.stash_default( );
    out.write<int>(7);
    out.write<int>(8);
    out.stash_default( );
    out.write<int>(10);
    out.end_record();
    out.end_keyword(true);
    out.write_string( out.keyword_sep );

    BOOST_CHECK_EQUAL( expected, s.str());
}

BOOST_AUTO_TEST_CASE(DeckItemWriteDefault) {
    DeckItem item("TEST", int());
    item.push_backDefault(1);
    item.push_backDefault(1);
    item.push_backDefault(1);

    {
        std::stringstream s;
        DeckOutput w(s);
        item.write( w );
        BOOST_CHECK_EQUAL( s.str() , "");
    }

    item.push_back(13);
    {
        std::stringstream s;
        DeckOutput w(s);
        item.write( w );
        BOOST_CHECK_EQUAL( s.str() , "3* 13");
    }
}


BOOST_AUTO_TEST_CASE(DeckItemWriteString) {
    DeckItem item("TEST", std::string());
    item.push_back("NO");
    item.push_back("YES");
    std::stringstream s;
    DeckOutput w(s);
    item.write( w );
    BOOST_CHECK_EQUAL( s.str() , "'NO' 'YES'");
}


BOOST_AUTO_TEST_CASE(RecordWrite) {

    DeckRecord deckRecord;
    DeckItem item1("TEST1", int());
    DeckItem item2("TEST2", double());
    DeckItem item3("TEST3", std::string());

    item1.push_back( 123 );
    item2.push_backDefault( 100.0 );
    item3.push_back("VALUE");

    deckRecord.addItem( item1 );
    deckRecord.addItem( item2 );
    deckRecord.addItem( item3 );

    std::stringstream s;
    DeckOutput w(s);
    deckRecord.write_data( w );
    BOOST_CHECK_EQUAL( s.str() , "123 1* 'VALUE'");
}


BOOST_AUTO_TEST_CASE(DeckItemEqual) {
    DeckItem item1("TEST1" , int());
    DeckItem item2("TEST2" , int());
    DeckItem item3("TEST1" , double());
    DeckItem item4("TEST1" , int());
    DeckItem item5("TEST1" , double());

    BOOST_CHECK( item1 != item2 );
    BOOST_CHECK( item1 != item3 );
    BOOST_CHECK( item1 == item1 );
    BOOST_CHECK( item1 == item4 );

    item4.push_back(100);
    BOOST_CHECK( item1 != item4 );
    item1.push_back(100);
    BOOST_CHECK( item1 == item4 );

    item4.push_backDefault( 200 );
    item1.push_back( 200 );
    BOOST_CHECK( item1 == item4 );
    BOOST_CHECK( !item1.equal( item4 , true , true));

    item3.push_back(1.0);
    item5.push_back(1.0);
    BOOST_CHECK( item3.equal( item5 , false, true ));
    BOOST_CHECK( item3.equal( item5 , false, false ));

    item3.push_back(1.0);
    item5.push_back(1.0 - 1e-8);
    BOOST_CHECK( item3.equal( item5 , false, true ));
    BOOST_CHECK( !item3.equal( item5 , false, false ));
}
