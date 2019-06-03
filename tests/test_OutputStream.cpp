/*
  Copyright 2019 Equinor

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

#define BOOST_TEST_MODULE OutputStream

#include <boost/test/unit_test.hpp>

#include <opm/io/eclipse/OutputStream.hpp>

#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EclOutput.hpp>
#include <opm/io/eclipse/ERst.hpp>

#include <opm/io/eclipse/EclIOdata.hpp>

#include <algorithm>
#include <iterator>
#include <ostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/filesystem.hpp>

namespace Opm { namespace EclIO {

    // Needed by BOOST_CHECK_EQUAL_COLLECTIONS.
    std::ostream&
    operator<<(std::ostream& os, const EclFile::EclEntry& e)
    {
        os << "{ " << std::get<0>(e)
           << ", " << static_cast<int>(std::get<1>(e))
           << ", " << std::get<2>(e)
           << " }";

        return os;
    }
}} // Namespace Opm::ecl

namespace {
    template <class Coll>
    void check_is_close(const Coll& c1, const Coll& c2)
    {
        using ElmType = typename std::remove_cv<
            typename std::remove_reference<
                typename std::iterator_traits<
                    decltype(std::begin(c1))
                >::value_type
            >::type
        >::type;

        for (auto b1  = c1.begin(), e1 = c1.end(), b2 = c2.begin();
                  b1 != e1; ++b1, ++b2)
        {
            BOOST_CHECK_CLOSE(*b1, *b2, static_cast<ElmType>(1.0e-7));
        }
    }
} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(RestartStream)

BOOST_AUTO_TEST_SUITE(FileName)

BOOST_AUTO_TEST_CASE(ResultSetDescriptor)
{
    const auto odir = std::string{"/x/y/z///"};
    const auto ext  = std::string{"F0123"};

    {
        const auto rset = ::Opm::EclIO::OutputStream::ResultSet {
            odir, "CASE"
        };

        const auto fname = outputFileName(rset, ext);

        BOOST_CHECK_EQUAL(fname, odir + "CASE.F0123");
    }

    {
        const auto rset = ::Opm::EclIO::OutputStream::ResultSet {
            odir, "CASE." // CASE DOT
        };

        const auto fname = outputFileName(rset, ext);

        BOOST_CHECK_EQUAL(fname, odir + "CASE.F0123");
    }

    {
        const auto rset = ::Opm::EclIO::OutputStream::ResultSet {
            odir, "CASE.01"
        };

        const auto fname = outputFileName(rset, ext);

        BOOST_CHECK_EQUAL(fname, odir + "CASE.01.F0123");
    }

    {
        const auto rset = ::Opm::EclIO::OutputStream::ResultSet {
            odir, "CASE.01." // CASE.01 DOT
        };

        const auto fname = outputFileName(rset, ext);

        BOOST_CHECK_EQUAL(fname, odir + "CASE.01.F0123");
    }
}

BOOST_AUTO_TEST_SUITE_END() // FileName

// ==========================================================================

BOOST_AUTO_TEST_SUITE(Class_Restart)

class RSet
{
public:
    explicit RSet(std::string base)
        : odir_(boost::filesystem::temp_directory_path() /
                boost::filesystem::unique_path("rset-%%%%"))
        , base_(std::move(base))
    {
        boost::filesystem::create_directories(this->odir_);
    }

    ~RSet()
    {
        boost::filesystem::remove_all(this->odir_);
    }

    operator ::Opm::EclIO::OutputStream::ResultSet() const
    {
        return { this->odir_.string(), this->base_ };
    }

private:
    boost::filesystem::path odir_;
    std::string             base_;
};

BOOST_AUTO_TEST_CASE(Unformatted_Unified)
{
    const auto rset = RSet("CASE");
    const auto fmt  = ::Opm::EclIO::OutputStream::Formatted{ false };
    const auto unif = ::Opm::EclIO::OutputStream::Unified  { true };

    {
        const auto seqnum = 1;
        auto rst = ::Opm::EclIO::OutputStream::Restart {
            rset, seqnum, fmt, unif
        };

        rst.write("I", std::vector<int>        {1, 7, 2, 9});
        rst.write("L", std::vector<bool>       {true, false, false, true});
        rst.write("S", std::vector<float>      {3.1f, 4.1f, 59.265f});
        rst.write("D", std::vector<double>     {2.71, 8.21});
        rst.write("Z", std::vector<std::string>{"W1", "W2"});
    }

    {
        const auto seqnum = 13;
        auto rst = ::Opm::EclIO::OutputStream::Restart {
            rset, seqnum, fmt, unif
        };

        rst.write("I", std::vector<int>        {35, 51, 13});
        rst.write("L", std::vector<bool>       {true, true, true, false});
        rst.write("S", std::vector<float>      {17.29e-02f, 1.4142f});
        rst.write("D", std::vector<double>     {0.6931, 1.6180, 123.45e6});
        rst.write("Z", std::vector<std::string>{"G1", "FIELD"});
    }

    {
        const auto fname = ::Opm::EclIO::OutputStream::
            outputFileName(rset, "UNRST");

        auto rst = ::Opm::EclIO::ERst{fname};

        BOOST_CHECK(rst.hasReportStepNumber( 1));
        BOOST_CHECK(rst.hasReportStepNumber(13));

        {
            const auto seqnum        = rst.listOfReportStepNumbers();
            const auto expect_seqnum = std::vector<int>{1, 13};

            BOOST_CHECK_EQUAL_COLLECTIONS(seqnum.begin(), seqnum.end(),
                                          expect_seqnum.begin(),
                                          expect_seqnum.end());
        }

        {
            const auto vectors        = rst.listOfRstArrays(13);
            const auto expect_vectors = std::vector<Opm::EclIO::EclFile::EclEntry>{
                Opm::EclIO::EclFile::EclEntry{"SEQNUM", Opm::EclIO::eclArrType::INTE, 1},
                Opm::EclIO::EclFile::EclEntry{"I", Opm::EclIO::eclArrType::INTE, 3},
                Opm::EclIO::EclFile::EclEntry{"L", Opm::EclIO::eclArrType::LOGI, 4},
                Opm::EclIO::EclFile::EclEntry{"S", Opm::EclIO::eclArrType::REAL, 2},
                Opm::EclIO::EclFile::EclEntry{"D", Opm::EclIO::eclArrType::DOUB, 3},
                Opm::EclIO::EclFile::EclEntry{"Z", Opm::EclIO::eclArrType::CHAR, 2},
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(vectors.begin(), vectors.end(),
                                          expect_vectors.begin(),
                                          expect_vectors.end());
        }

        rst.loadReportStepNumber(13);

        {
            const auto& I = rst.getRst<int>("I", 13);
            const auto  expect_I = std::vector<int>{ 35, 51, 13};
            BOOST_CHECK_EQUAL_COLLECTIONS(I.begin(), I.end(),
                                          expect_I.begin(),
                                          expect_I.end());
        }

        {
            const auto& L = rst.getRst<bool>("L", 13);
            const auto  expect_L = std::vector<bool> {
                true, true, true, false,
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(L.begin(), L.end(),
                                          expect_L.begin(),
                                          expect_L.end());
        }

        {
            const auto& S = rst.getRst<float>("S", 13);
            const auto  expect_S = std::vector<float>{
                17.29e-02f, 1.4142f,
            };

            check_is_close(S, expect_S);
        }

        {
            const auto& D = rst.getRst<double>("D", 13);
            const auto  expect_D = std::vector<double>{
                0.6931, 1.6180, 123.45e6,
            };

            check_is_close(D, expect_D);
        }

        {
            const auto& Z = rst.getRst<std::string>("Z", 13);
            const auto  expect_Z = std::vector<std::string>{
                "G1", "FIELD",  // ERst trims trailing blanks
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(Z.begin(), Z.end(),
                                          expect_Z.begin(),
                                          expect_Z.end());
        }
    }

    {
        const auto seqnum = 5;  // Before 13.  Should overwrite 13
        auto rst = ::Opm::EclIO::OutputStream::Restart {
            rset, seqnum, fmt, unif
        };

        rst.write("I", std::vector<int>        {1, 2, 3, 4});
        rst.write("L", std::vector<bool>       {false, false, false, true});
        rst.write("S", std::vector<float>      {1.23e-04f, 1.234e5f, -5.4321e-9f});
        rst.write("D", std::vector<double>     {0.6931, 1.6180});
        rst.write("Z", std::vector<std::string>{"HELLO", ", ", "WORLD"});
    }

    {
        const auto fname = ::Opm::EclIO::OutputStream::
            outputFileName(rset, "UNRST");

        auto rst = ::Opm::EclIO::ERst{fname};

        BOOST_CHECK( rst.hasReportStepNumber( 1));
        BOOST_CHECK( rst.hasReportStepNumber( 5));
        BOOST_CHECK(!rst.hasReportStepNumber(13));

        {
            const auto seqnum        = rst.listOfReportStepNumbers();
            const auto expect_seqnum = std::vector<int>{1, 5};

            BOOST_CHECK_EQUAL_COLLECTIONS(seqnum.begin(), seqnum.end(),
                                          expect_seqnum.begin(),
                                          expect_seqnum.end());
        }

        {
            const auto vectors        = rst.listOfRstArrays(5);
            const auto expect_vectors = std::vector<Opm::EclIO::EclFile::EclEntry>{
                Opm::EclIO::EclFile::EclEntry{"SEQNUM", Opm::EclIO::eclArrType::INTE, 1},
                Opm::EclIO::EclFile::EclEntry{"I", Opm::EclIO::eclArrType::INTE, 4},
                Opm::EclIO::EclFile::EclEntry{"L", Opm::EclIO::eclArrType::LOGI, 4},
                Opm::EclIO::EclFile::EclEntry{"S", Opm::EclIO::eclArrType::REAL, 3},
                Opm::EclIO::EclFile::EclEntry{"D", Opm::EclIO::eclArrType::DOUB, 2},
                Opm::EclIO::EclFile::EclEntry{"Z", Opm::EclIO::eclArrType::CHAR, 3},
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(vectors.begin(), vectors.end(),
                                          expect_vectors.begin(),
                                          expect_vectors.end());
        }

        rst.loadReportStepNumber(5);

        {
            const auto& I = rst.getRst<int>("I", 5);
            const auto  expect_I = std::vector<int>{ 1, 2, 3, 4 };
            BOOST_CHECK_EQUAL_COLLECTIONS(I.begin(), I.end(),
                                          expect_I.begin(),
                                          expect_I.end());
        }

        {
            const auto& L = rst.getRst<bool>("L", 5);
            const auto  expect_L = std::vector<bool> {
                false, false, false, true,
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(L.begin(), L.end(),
                                          expect_L.begin(),
                                          expect_L.end());
        }

        {
            const auto& S = rst.getRst<float>("S", 5);
            const auto  expect_S = std::vector<float>{
                1.23e-04f, 1.234e5f, -5.4321e-9f,
            };

            check_is_close(S, expect_S);
        }

        {
            const auto& D = rst.getRst<double>("D", 5);
            const auto  expect_D = std::vector<double>{
                0.6931, 1.6180,
            };

            check_is_close(D, expect_D);
        }

        {
            const auto& Z = rst.getRst<std::string>("Z", 5);
            const auto  expect_Z = std::vector<std::string>{
                "HELLO", ",", "WORLD",  // ERst trims trailing blanks
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(Z.begin(), Z.end(),
                                          expect_Z.begin(),
                                          expect_Z.end());
        }
    }

    {
        const auto seqnum = 13;
        auto rst = ::Opm::EclIO::OutputStream::Restart {
            rset, seqnum, fmt, unif
        };

        rst.write("I", std::vector<int>        {35, 51, 13});
        rst.write("L", std::vector<bool>       {true, true, true, false});
        rst.write("S", std::vector<float>      {17.29e-02f, 1.4142f});
        rst.write("D", std::vector<double>     {0.6931, 1.6180, 123.45e6});
        rst.write("Z", std::vector<std::string>{"G1", "FIELD"});
    }

    {
        const auto fname = ::Opm::EclIO::OutputStream::
            outputFileName(rset, "UNRST");

        auto rst = ::Opm::EclIO::ERst{fname};

        BOOST_CHECK(rst.hasReportStepNumber( 1));
        BOOST_CHECK(rst.hasReportStepNumber( 5));
        BOOST_CHECK(rst.hasReportStepNumber(13));

        {
            const auto seqnum        = rst.listOfReportStepNumbers();
            const auto expect_seqnum = std::vector<int>{1, 5, 13};

            BOOST_CHECK_EQUAL_COLLECTIONS(seqnum.begin(), seqnum.end(),
                                          expect_seqnum.begin(),
                                          expect_seqnum.end());
        }

        {
            const auto vectors        = rst.listOfRstArrays(13);
            const auto expect_vectors = std::vector<Opm::EclIO::EclFile::EclEntry>{
                Opm::EclIO::EclFile::EclEntry{"SEQNUM", Opm::EclIO::eclArrType::INTE, 1},
                Opm::EclIO::EclFile::EclEntry{"I", Opm::EclIO::eclArrType::INTE, 3},
                Opm::EclIO::EclFile::EclEntry{"L", Opm::EclIO::eclArrType::LOGI, 4},
                Opm::EclIO::EclFile::EclEntry{"S", Opm::EclIO::eclArrType::REAL, 2},
                Opm::EclIO::EclFile::EclEntry{"D", Opm::EclIO::eclArrType::DOUB, 3},
                Opm::EclIO::EclFile::EclEntry{"Z", Opm::EclIO::eclArrType::CHAR, 2},
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(vectors.begin(), vectors.end(),
                                          expect_vectors.begin(),
                                          expect_vectors.end());
        }

        rst.loadReportStepNumber(13);

        {
            const auto& I = rst.getRst<int>("I", 13);
            const auto  expect_I = std::vector<int>{ 35, 51, 13};
            BOOST_CHECK_EQUAL_COLLECTIONS(I.begin(), I.end(),
                                          expect_I.begin(),
                                          expect_I.end());
        }

        {
            const auto& L = rst.getRst<bool>("L", 13);
            const auto  expect_L = std::vector<bool> {
                true, true, true, false,
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(L.begin(), L.end(),
                                          expect_L.begin(),
                                          expect_L.end());
        }

        {
            const auto& S = rst.getRst<float>("S", 13);
            const auto  expect_S = std::vector<float>{
                17.29e-02f, 1.4142f,
            };

            check_is_close(S, expect_S);
        }

        {
            const auto& D = rst.getRst<double>("D", 13);
            const auto  expect_D = std::vector<double>{
                0.6931, 1.6180, 123.45e6,
            };

            check_is_close(D, expect_D);
        }

        {
            const auto& Z = rst.getRst<std::string>("Z", 13);
            const auto  expect_Z = std::vector<std::string>{
                "G1", "FIELD",  // ERst trims trailing blanks
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(Z.begin(), Z.end(),
                                          expect_Z.begin(),
                                          expect_Z.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(Formatted_Separate)
{
    const auto rset = RSet("CASE.T01.");
    const auto fmt  = ::Opm::EclIO::OutputStream::Formatted{ true };
    const auto unif = ::Opm::EclIO::OutputStream::Unified  { false };

    {
        const auto seqnum = 1;
        auto rst = ::Opm::EclIO::OutputStream::Restart {
            rset, seqnum, fmt, unif
        };

        rst.write("I", std::vector<int>        {1, 7, 2, 9});
        rst.write("L", std::vector<bool>       {true, false, false, true});
        rst.write("S", std::vector<float>      {3.1f, 4.1f, 59.265f});
        rst.write("D", std::vector<double>     {2.71, 8.21});
        rst.write("Z", std::vector<std::string>{"W1", "W2"});
    }

    {
        const auto seqnum = 13;
        auto rst = ::Opm::EclIO::OutputStream::Restart {
            rset, seqnum, fmt, unif
        };

        rst.write("I", std::vector<int>        {35, 51, 13});
        rst.write("L", std::vector<bool>       {true, true, true, false});
        rst.write("S", std::vector<float>      {17.29e-02f, 1.4142f});
        rst.write("D", std::vector<double>     {0.6931, 1.6180, 123.45e6});
        rst.write("Z", std::vector<std::string>{"G1", "FIELD"});
    }

    {
        using ::Opm::EclIO::OutputStream::Restart;

        const auto fname = ::Opm::EclIO::OutputStream::
            outputFileName(rset, "F0013");

        auto rst = ::Opm::EclIO::EclFile{fname};

        {
            const auto vectors        = rst.getList();
            const auto expect_vectors = std::vector<Opm::EclIO::EclFile::EclEntry>{
                // No SEQNUM in separate output files
                Opm::EclIO::EclFile::EclEntry{"I", Opm::EclIO::eclArrType::INTE, 3},
                Opm::EclIO::EclFile::EclEntry{"L", Opm::EclIO::eclArrType::LOGI, 4},
                Opm::EclIO::EclFile::EclEntry{"S", Opm::EclIO::eclArrType::REAL, 2},
                Opm::EclIO::EclFile::EclEntry{"D", Opm::EclIO::eclArrType::DOUB, 3},
                Opm::EclIO::EclFile::EclEntry{"Z", Opm::EclIO::eclArrType::CHAR, 2},
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(vectors.begin(), vectors.end(),
                                          expect_vectors.begin(),
                                          expect_vectors.end());
        }

        rst.loadData();

        {
            const auto& I = rst.get<int>("I");
            const auto  expect_I = std::vector<int>{ 35, 51, 13 };
            BOOST_CHECK_EQUAL_COLLECTIONS(I.begin(), I.end(),
                                          expect_I.begin(),
                                          expect_I.end());
        }

        {
            const auto& L = rst.get<bool>("L");
            const auto  expect_L = std::vector<bool> {
                true, true, true, false,
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(L.begin(), L.end(),
                                          expect_L.begin(),
                                          expect_L.end());
        }

        {
            const auto& S = rst.get<float>("S");
            const auto  expect_S = std::vector<float>{
                17.29e-02f, 1.4142f,
            };

            check_is_close(S, expect_S);
        }

        {
            const auto& D = rst.get<double>("D");
            const auto  expect_D = std::vector<double>{
                0.6931, 1.6180, 123.45e6,
            };

            check_is_close(D, expect_D);
        }

        {
            const auto& Z = rst.get<std::string>("Z");
            const auto  expect_Z = std::vector<std::string>{
                "G1", "FIELD",  // ERst trims trailing blanks
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(Z.begin(), Z.end(),
                                          expect_Z.begin(),
                                          expect_Z.end());
        }
    }

    {
        // Separate output.  Step 13 should be unaffected.
        const auto seqnum = 5;
        auto rst = ::Opm::EclIO::OutputStream::Restart {
            rset, seqnum, fmt, unif
        };

        rst.write("I", std::vector<int>        {1, 2, 3, 4});
        rst.write("L", std::vector<bool>       {false, false, false, true});
        rst.write("S", std::vector<float>      {1.23e-04f, 1.234e5f, -5.4321e-9f});
        rst.write("D", std::vector<double>     {0.6931, 1.6180});
        rst.write("Z", std::vector<std::string>{"HELLO", ", ", "WORLD"});
    }

    {
        using ::Opm::EclIO::OutputStream::Restart;

        const auto fname = ::Opm::EclIO::OutputStream::
            outputFileName(rset, "F0005");

        auto rst = ::Opm::EclIO::EclFile{fname};

        {
            const auto vectors        = rst.getList();
            const auto expect_vectors = std::vector<Opm::EclIO::EclFile::EclEntry>{
                // No SEQNUM in separate output files
                Opm::EclIO::EclFile::EclEntry{"I", Opm::EclIO::eclArrType::INTE, 4},
                Opm::EclIO::EclFile::EclEntry{"L", Opm::EclIO::eclArrType::LOGI, 4},
                Opm::EclIO::EclFile::EclEntry{"S", Opm::EclIO::eclArrType::REAL, 3},
                Opm::EclIO::EclFile::EclEntry{"D", Opm::EclIO::eclArrType::DOUB, 2},
                Opm::EclIO::EclFile::EclEntry{"Z", Opm::EclIO::eclArrType::CHAR, 3},
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(vectors.begin(), vectors.end(),
                                          expect_vectors.begin(),
                                          expect_vectors.end());
        }

        rst.loadData();

        {
            const auto& I = rst.get<int>("I");
            const auto  expect_I = std::vector<int>{ 1, 2, 3, 4 };
            BOOST_CHECK_EQUAL_COLLECTIONS(I.begin(), I.end(),
                                          expect_I.begin(),
                                          expect_I.end());
        }

        {
            const auto& L = rst.get<bool>("L");
            const auto  expect_L = std::vector<bool> {
                false, false, false, true,
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(L.begin(), L.end(),
                                          expect_L.begin(),
                                          expect_L.end());
        }

        {
            const auto& S = rst.get<float>("S");
            const auto  expect_S = std::vector<float>{
                1.23e-04f, 1.234e5f, -5.4321e-9f,
            };

            check_is_close(S, expect_S);
        }

        {
            const auto& D = rst.get<double>("D");
            const auto  expect_D = std::vector<double>{
                0.6931, 1.6180,
            };

            check_is_close(D, expect_D);
        }

        {
            const auto& Z = rst.get<std::string>("Z");
            const auto  expect_Z = std::vector<std::string>{
                "HELLO", ",", "WORLD",  // ERst trims trailing blanks
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(Z.begin(), Z.end(),
                                          expect_Z.begin(),
                                          expect_Z.end());
        }
    }

    // -------------------------------------------------------
    // Don't rewrite step 13.  Output file should still exist.
    // -------------------------------------------------------

    {
        using ::Opm::EclIO::OutputStream::Restart;

        const auto fname = ::Opm::EclIO::OutputStream::
            outputFileName(rset, "F0013");

        auto rst = ::Opm::EclIO::EclFile{fname};

        {
            const auto vectors        = rst.getList();
            const auto expect_vectors = std::vector<Opm::EclIO::EclFile::EclEntry>{
                // No SEQNUM in separate output files.
                Opm::EclIO::EclFile::EclEntry{"I", Opm::EclIO::eclArrType::INTE, 3},
                Opm::EclIO::EclFile::EclEntry{"L", Opm::EclIO::eclArrType::LOGI, 4},
                Opm::EclIO::EclFile::EclEntry{"S", Opm::EclIO::eclArrType::REAL, 2},
                Opm::EclIO::EclFile::EclEntry{"D", Opm::EclIO::eclArrType::DOUB, 3},
                Opm::EclIO::EclFile::EclEntry{"Z", Opm::EclIO::eclArrType::CHAR, 2},
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(vectors.begin(), vectors.end(),
                                          expect_vectors.begin(),
                                          expect_vectors.end());
        }

        rst.loadData();

        {
            const auto& I = rst.get<int>("I");
            const auto  expect_I = std::vector<int>{ 35, 51, 13 };
            BOOST_CHECK_EQUAL_COLLECTIONS(I.begin(), I.end(),
                                          expect_I.begin(),
                                          expect_I.end());
        }

        {
            const auto& L = rst.get<bool>("L");
            const auto  expect_L = std::vector<bool> {
                true, true, true, false,
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(L.begin(), L.end(),
                                          expect_L.begin(),
                                          expect_L.end());
        }

        {
            const auto& S = rst.get<float>("S");
            const auto  expect_S = std::vector<float>{
                17.29e-02f, 1.4142f,
            };

            check_is_close(S, expect_S);
        }

        {
            const auto& D = rst.get<double>("D");
            const auto  expect_D = std::vector<double>{
                0.6931, 1.6180, 123.45e6,
            };

            check_is_close(D, expect_D);
        }

        {
            const auto& Z = rst.get<std::string>("Z");
            const auto  expect_Z = std::vector<std::string>{
                "G1", "FIELD",  // ERst trims trailing blanks
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(Z.begin(), Z.end(),
                                          expect_Z.begin(),
                                          expect_Z.end());
        }
    }
}

BOOST_AUTO_TEST_SUITE_END() // Class_Restart

BOOST_AUTO_TEST_SUITE_END() // RestartStream
