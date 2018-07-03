#define BOOST_TEST_MODULE Aggregate_Well_Data

#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/CharArrayNullTerm.hpp>

// =====================================================================

BOOST_AUTO_TEST_SUITE(AChar8)

BOOST_AUTO_TEST_CASE (Basic_Operations)
{
    // Default Constructor
    {
        const auto s = Opm::RestartIO::Helpers::CharArrayNullTerm<8>{};

        BOOST_CHECK_EQUAL(s.c_str(), std::string(8, ' '));
    }

    // Construct from Constant String
    {
        const auto s = Opm::RestartIO::Helpers::CharArrayNullTerm<8>{"Inj-1"};

        BOOST_CHECK_EQUAL(s.c_str(), std::string{"Inj-1   "});
    }

    // Copy Construction
    {
        const auto s1 = Opm::RestartIO::Helpers::CharArrayNullTerm<8>{"Inj-1"};
        const auto s2 = s1;

        BOOST_CHECK_EQUAL(s2.c_str(), std::string{"Inj-1   "});
    }

    // Move Construction
    {
        const auto s1 = Opm::RestartIO::Helpers::CharArrayNullTerm<8>{"Inj-1"};
        const auto s2 = s1;

        BOOST_CHECK_EQUAL(s2.c_str(), std::string{"Inj-1   "});
    }

    // Move Construction
    {
        auto s1 = Opm::RestartIO::Helpers::CharArrayNullTerm<8>{"Inj-1"};
        const auto s2 = std::move(s1);

        BOOST_CHECK_EQUAL(s2.c_str(), std::string{"Inj-1   "});
    }

    // Assignment Operator
    {
        const auto s1 = Opm::RestartIO::Helpers::CharArrayNullTerm<8>{"Inj-1"};
        auto s2 = Opm::RestartIO::Helpers::CharArrayNullTerm<8>{"Prod-2"};

        s2 = s1;
        BOOST_CHECK_EQUAL(s2.c_str(), std::string{"Inj-1   "});
    }

    // Move Assignment Operator
    {
        auto s1 = Opm::RestartIO::Helpers::CharArrayNullTerm<8>{"Inj-1"};
        auto s2 = Opm::RestartIO::Helpers::CharArrayNullTerm<8>{"Prod-2"};

        s2 = std::move(s1);
        BOOST_CHECK_EQUAL(s2.c_str(), std::string{"Inj-1   "});
    }

    // Assign std::string
    {
        auto s = Opm::RestartIO::Helpers::CharArrayNullTerm<8>{"@Hi Hoo@"};

        s = "Prod-2";
        BOOST_CHECK_EQUAL(s.c_str(), std::string{"Prod-2  "});
    }
}

BOOST_AUTO_TEST_CASE (String_Shortening)
{
    // Construct from string of more than N characters
    {
        const auto s = Opm::RestartIO::Helpers::CharArrayNullTerm<10>{
            "String too long"
        };

        BOOST_CHECK_EQUAL(s.c_str(), std::string{"String too"});
    }

    // Assign string of more than N characters
    {
        auto s = Opm::RestartIO::Helpers::CharArrayNullTerm<11>{};

        s = "This string has too many characters";

        BOOST_CHECK_EQUAL(s.c_str(), std::string{"This string"});
    }
}

BOOST_AUTO_TEST_SUITE_END()
