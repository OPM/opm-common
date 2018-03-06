#define BOOST_TEST_MODULE StringTests

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Utility/String.hpp>
#include <opm/parser/eclipse/Utility/Stringview.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE( uppercase_copy ) {
    const std::string src = "string";
    const std::string dst = uppercase( src );

    BOOST_CHECK_EQUAL( src, "string" );
    BOOST_CHECK_EQUAL( dst, "STRING" );
}

BOOST_AUTO_TEST_CASE( uppercase_inplace ) {
    std::string src = "string";
    auto& ref = uppercase( src, src );

    BOOST_CHECK_EQUAL( src, "STRING" );
    BOOST_CHECK_EQUAL( src, ref );
    BOOST_CHECK_EQUAL( std::addressof( src ), std::addressof( ref ) );
}

BOOST_AUTO_TEST_CASE( nonconst_ref ) {
    std::string src = "string";
    auto dst = uppercase( src );

    BOOST_CHECK_EQUAL( src, "string" );
    BOOST_CHECK_EQUAL( dst, "STRING" );
}

BOOST_AUTO_TEST_CASE( uppercase_move ) {
    std::string src = "string";
    auto dst = uppercase( std::move( src ) );

    BOOST_CHECK_EQUAL( dst, "STRING" );
}

BOOST_AUTO_TEST_CASE( uppercase_mixed_type ) {
    std::string src = "string";
    string_view view( src );


    std::string dst = "string";
    uppercase( view, dst );
    BOOST_CHECK_EQUAL( dst, "STRING" );
    BOOST_CHECK_EQUAL( view, "string" );
}

BOOST_AUTO_TEST_CASE( write_parts_of_dst ) {
    std::string src = "string";
    string_view view( src );


    std::string dst = "stringmixed";
    uppercase( view, dst );
    BOOST_CHECK_EQUAL( dst, "STRINGmixed" );
    BOOST_CHECK_EQUAL( view, "string" );
}

BOOST_AUTO_TEST_CASE(fullStringView) {
    std::string srcstr = "lorem ipsum";
    string_view view( srcstr );

    BOOST_CHECK_EQUAL_COLLECTIONS(
            srcstr.begin(), srcstr.end(),
            view.begin(), view.end() );
}

BOOST_AUTO_TEST_CASE(viewCorrectSize) {
    std::string srcstr = "lorem ipsum";

    string_view full( srcstr );
    BOOST_CHECK_EQUAL( srcstr.size(), full.size() );

    string_view view( srcstr, 5 );
    BOOST_CHECK_EQUAL( 5, view.size() );
    BOOST_CHECK_EQUAL( 5, view.length() );
}

BOOST_AUTO_TEST_CASE(viewOperatorAt) {
    std::string srcstr = "lorem ipsum";
    string_view view( srcstr );

    for( size_t i = 0; i < view.size(); ++i )
        BOOST_CHECK_EQUAL( view[ i ], srcstr[ i ] );
}

BOOST_AUTO_TEST_CASE(viewFrontBack) {
    std::string srcstr = "lorem ipsum";
    string_view view( srcstr );

    BOOST_CHECK_EQUAL( view.front(), 'l' );
    BOOST_CHECK_EQUAL( view.back(), 'm' );
}


BOOST_AUTO_TEST_CASE(viewSubstr) {
    std::string srcstr = "lorem ipsum";
    string_view view( srcstr );

    BOOST_CHECK_NO_THROW( view.string() );
    BOOST_CHECK_EQUAL( srcstr, view.string() );
    BOOST_CHECK_EQUAL( srcstr, view.substr() );
    BOOST_CHECK_EQUAL( "", view.substr( 0, 0 ) );

    BOOST_CHECK_EQUAL( srcstr.substr( 1 ), view.substr( 1 ) );

    BOOST_CHECK_THROW( view.substr( srcstr.size() + 1 ), std::out_of_range );
    BOOST_CHECK_THROW( view.substr( 0, srcstr.size() + 1 ), std::out_of_range );
    BOOST_CHECK_THROW( view.substr( 1, 0 ), std::invalid_argument );
    BOOST_CHECK_NO_THROW( view.substr( 0, 0 ) );
}

BOOST_AUTO_TEST_CASE(viewStream) {
    std::string srcstr = "lorem ipsum";
    string_view view( srcstr );

    std::stringstream str;
    str << view;

    BOOST_CHECK_EQUAL( srcstr, str.str() );
}

BOOST_AUTO_TEST_CASE(equalityOperators) {
    std::string srcstr = "lorem ipsum";
    std::string diffstr = "lorem";
    string_view view( srcstr );

    BOOST_CHECK_EQUAL( srcstr, view );
    BOOST_CHECK_NE( diffstr, view );

    BOOST_CHECK_EQUAL( view, srcstr );
    BOOST_CHECK_NE( view, diffstr );

    BOOST_CHECK_EQUAL( "lorem ipsum", view );
    BOOST_CHECK_NE( "lorem", view );

    BOOST_CHECK_EQUAL( view, "lorem ipsum" );
    BOOST_CHECK_NE( view, "lorem" );
}

BOOST_AUTO_TEST_CASE(plusOperator) {
    std::string total = "lorem ipsum";
    std::string lhs = "lorem";
    std::string ws = " ";
    std::string rhs = "ipsum";

    string_view lhs_view( lhs );
    string_view rhs_view( rhs );

    BOOST_CHECK_EQUAL( total, lhs_view + ws + rhs_view );
    BOOST_CHECK_EQUAL( lhs + ws, lhs_view + ws );
    BOOST_CHECK_EQUAL( ws + rhs, ws + rhs_view );
}
