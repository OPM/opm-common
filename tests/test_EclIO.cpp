/*
   +   Copyright 2019 Equinor ASA.
   +
   +   This file is part of the Open Porous Media project (OPM).
   +
   +   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   +   the Free Software Foundation, either version 3 of the License, or
   +   (at your option) any later version.
   +
   +   OPM is distributed in the hope that it will be useful,
   +   but WITHOUT ANY WARRANTY; without even the implied warranty of
   +   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   +   GNU General Public License for more details.
   +
   +   You should have received a copy of the GNU General Public License
   +   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   +   */


#include "config.h"
#include <iostream>
#include <stdio.h>
#include <iomanip>
#include <math.h>
#include <algorithm>
#include <tuple>

#include <examples/test_util/EclFile.hpp>
#include <examples/test_util/ERst.hpp>
#include <examples/test_util/ESmry.hpp>
#include <examples/test_util/EclOutput.hpp>

#define BOOST_TEST_MODULE Test EclIO
#include <boost/test/unit_test.hpp>


template<typename InputIterator1, typename InputIterator2>
bool
range_equal(InputIterator1 first1, InputIterator1 last1,
            InputIterator2 first2, InputIterator2 last2)
{
    while(first1 != last1 && first2 != last2)
    {
        if(*first1 != *first2) return false;
        ++first1;
        ++first2;
    }
    return (first1 == last1) && (first2 == last2);
}

bool compare_files(const std::string& filename1, const std::string& filename2)
{
    std::ifstream file1(filename1);
    std::ifstream file2(filename2);

    std::istreambuf_iterator<char> begin1(file1);
    std::istreambuf_iterator<char> begin2(file2);

    std::istreambuf_iterator<char> end;

    return range_equal(begin1, end, begin2, end);
}

template <typename T>
bool operator==(const std::vector<T> & t1, const std::vector<T> & t2)
{
    return std::equal(t1.begin(), t1.end(), t2.begin(), t2.end());
}


BOOST_AUTO_TEST_CASE(TestEclFile_BINARY) {

    std::string testFile="ECLFILE.INIT";

    // check that exception is thrown when file not exists

    BOOST_CHECK_THROW(EclFile file1("DUMMY.DAT") , std::runtime_error );

    EclFile file1(testFile);

    // check that exception is thrown when vector exists, but data not loaded

    BOOST_CHECK_THROW(std::vector<int> vect1=file1.get<int>(0) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<int> vect1=file1.get<int>("ICON") , std::runtime_error );

    BOOST_CHECK_THROW(std::vector<bool> vect2=file1.get<bool>(1) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<bool> vect2=file1.get<bool>("LOGIHEAD") , std::runtime_error );

    BOOST_CHECK_THROW(std::vector<float> vect3=file1.get<float>(2) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<float> vect3=file1.get<float>("PORV") , std::runtime_error );

    BOOST_CHECK_THROW(std::vector<double> vect4=file1.get<double>(3) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<double> vect4=file1.get<double>("XCON") , std::runtime_error );

    BOOST_CHECK_THROW(std::vector<std::string> vect5=file1.get<std::string>(4) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<std::string> vect5=file1.get<std::string>("KEYWORDS") , std::runtime_error );

    file1.loadData();

    // check that exeption is thrown when member function get is used with wrong type

    BOOST_CHECK_THROW(std::vector<int> vect1=file1.get<int>(2) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<int> vect1=file1.get<int>("PORV") , std::invalid_argument );

    BOOST_CHECK_THROW(std::vector<float> vect1=file1.get<float>(0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<float> vect1=file1.get<float>("ICON") , std::invalid_argument );

    BOOST_CHECK_THROW(std::vector<double> vect1=file1.get<double>(0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<double> vect1=file1.get<double>("KEYWORDS") , std::invalid_argument );

    BOOST_CHECK_THROW(std::vector<bool> vect1=file1.get<bool>(0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<bool> vect1=file1.get<bool>("XCON") , std::invalid_argument );

    BOOST_CHECK_THROW(std::vector<std::string> vect1=file1.get<std::string>(0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<std::string> vect1=file1.get<std::string>("XCON") , std::invalid_argument );

    // check member function hasKey

    BOOST_CHECK_EQUAL(file1.hasKey("PORV"), true);
    BOOST_CHECK_EQUAL(file1.hasKey("XPORV"), false);

    // test member functon get, check size of loaded vectors

    std::vector<int> vect1a=file1.get<int>(0);
    std::vector<int> vect1b=file1.get<int>("ICON");

    BOOST_CHECK_EQUAL(vect1a.size(), 1875);
    BOOST_CHECK_EQUAL(vect1b.size(), 1875);

    std::vector<bool> vect2a=file1.get<bool>(1);
    std::vector<bool> vect2b=file1.get<bool>("LOGIHEAD");

    BOOST_CHECK_EQUAL(vect2a.size(), 121);
    BOOST_CHECK_EQUAL(vect2b.size(), 121);

    std::vector<float> vect3a=file1.get<float>(2);
    std::vector<float> vect3b=file1.get<float>("PORV");

    BOOST_CHECK_EQUAL(vect3a.size(), 3146);
    BOOST_CHECK_EQUAL(vect3b.size(), 3146);

    std::vector<double> vect4a=file1.get<double>(3);
    std::vector<double> vect4b=file1.get<double>("XCON");

    BOOST_CHECK_EQUAL(vect4a.size(), 1740);
    BOOST_CHECK_EQUAL(vect4b.size(), 1740);

    std::vector<std::string> vect5a=file1.get<std::string>(4);
    std::vector<std::string> vect5b=file1.get<std::string>("KEYWORDS");

    BOOST_CHECK_EQUAL(vect5a.size(), 312);
    BOOST_CHECK_EQUAL(vect5b.size(), 312);

}

BOOST_AUTO_TEST_CASE(TestEclFile_FORMATTED) {

    std::string testFile1="ECLFILE.INIT";
    std::string testFile2="ECLFILE.FINIT";

    EclFile file1(testFile1);
    file1.loadData();

    EclFile file2(testFile2);
    file2.loadData();

    std::vector<int> vect1a=file1.get<int>("ICON");
    std::vector<int> vect1b=file2.get<int>("ICON");

    BOOST_CHECK_EQUAL(vect1a.size(), vect1b.size());
    BOOST_CHECK_EQUAL(vect1a==vect1b,true);

    std::vector<float> vect2a=file1.get<float>("PORV");
    std::vector<float> vect2b=file2.get<float>("PORV");

    BOOST_CHECK_EQUAL(vect2a.size(), vect2b.size());
    BOOST_CHECK_EQUAL(vect2a==vect2b,true);

    std::vector<double> vect3a=file1.get<double>("XCON");
    std::vector<double> vect3b=file2.get<double>("XCON");

    BOOST_CHECK_EQUAL(vect3a.size(), vect3b.size());
    BOOST_CHECK_EQUAL(vect3a==vect3b,true);

    std::vector<bool> vect4a=file1.get<bool>("LOGIHEAD");
    std::vector<bool> vect4b=file2.get<bool>("LOGIHEAD");

    BOOST_CHECK_EQUAL(vect4a.size(), vect4b.size());
    BOOST_CHECK_EQUAL(vect4a==vect4b,true);

    std::vector<std::string> vect5a=file1.get<std::string>("KEYWORDS");
    std::vector<std::string> vect5b=file2.get<std::string>("KEYWORDS");

    BOOST_CHECK_EQUAL(vect5a.size(), vect5b.size());
    BOOST_CHECK_EQUAL(vect5a==vect5b,true);

}

BOOST_AUTO_TEST_CASE(TestEcl_Write_binary) {

    std::string inputFile="ECLFILE.INIT";
    std::string testFile="TEST.DAT";

    // loading vectors from binary file

    EclFile file1(inputFile);
    file1.loadData();

    std::vector<int> icon=file1.get<int>("ICON");
    std::vector<float> porv=file1.get<float>("PORV");
    std::vector<double> xcon=file1.get<double>("XCON");
    std::vector<bool> logihead=file1.get<bool>("LOGIHEAD");
    std::vector<std::string> keywords=file1.get<std::string>("KEYWORDS");

    // writing vectors to test file (TEST.DAT) using class EclOutput

    std::ofstream outFileH;
    outFileH.open(testFile, std::ios::out |  std::ios::binary  );

    EclOutput eclTest(outFileH);

    eclTest.writeBinaryHeader("ICON",icon.size(),EIOD::INTE);
    eclTest.writeBinaryArray(icon);

    eclTest.writeBinaryHeader("LOGIHEAD",logihead.size(),EIOD::LOGI);
    eclTest.writeBinaryArray(logihead);

    eclTest.writeBinaryHeader("PORV",porv.size(),EIOD::REAL);
    eclTest.writeBinaryArray(porv);

    eclTest.writeBinaryHeader("XCON",xcon.size(),EIOD::DOUB);
    eclTest.writeBinaryArray(xcon);

    eclTest.writeBinaryHeader("KEYWORDS",keywords.size(),EIOD::CHAR);
    eclTest.writeBinaryCharArray(keywords);

    eclTest.writeBinaryHeader("ENDSOL",0,EIOD::MESS);

    outFileH.close();

    BOOST_CHECK_EQUAL(compare_files(inputFile, testFile), true);

    if (remove(testFile.c_str())==-1) {
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };

}

BOOST_AUTO_TEST_CASE(TestEcl_Write_formatted) {

    std::string inputFile="ECLFILE.FINIT";
    std::string testFile="TEST.FDAT";

    // loading vectors from binary file

    EclFile file1(inputFile);
    file1.loadData();

    std::vector<int> icon=file1.get<int>("ICON");
    std::vector<float> porv=file1.get<float>("PORV");
    std::vector<double> xcon=file1.get<double>("XCON");
    std::vector<bool> logihead=file1.get<bool>("LOGIHEAD");
    std::vector<std::string> keywords=file1.get<std::string>("KEYWORDS");

    // writing vectors to test file (TEST.FDAT) using class EclOutput

    std::ofstream outFileH;
    outFileH.open(testFile, std::ios::out  );

    EclOutput eclTest(outFileH);

    eclTest.writeFormattedHeader("ICON",icon.size(),EIOD::INTE);
    eclTest.writeFormattedArray(icon);

    eclTest.writeFormattedHeader("LOGIHEAD",logihead.size(),EIOD::LOGI);
    eclTest.writeFormattedArray(logihead);

    eclTest.writeFormattedHeader("PORV",porv.size(),EIOD::REAL);
    eclTest.writeFormattedArray(porv);

    eclTest.writeFormattedHeader("XCON",xcon.size(),EIOD::DOUB);
    eclTest.writeFormattedArray(xcon);

    eclTest.writeFormattedHeader("KEYWORDS",keywords.size(),EIOD::CHAR);
    eclTest.writeFormattedCharArray(keywords);

    eclTest.writeFormattedHeader("ENDSOL",0,EIOD::MESS);

    outFileH.close();

    BOOST_CHECK_EQUAL(compare_files(inputFile, testFile), true);

    if (remove(testFile.c_str())==-1) {
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };

}


BOOST_AUTO_TEST_CASE(TestEcl_getList) {

    std::string inputFile="ECLFILE.INIT";
    std::string testFile="TEST.DAT";

    EclFile file1(inputFile);
    file1.loadData();

    std::ofstream outFileH;
    outFileH.open(testFile, std::ios::out |  std::ios::binary  );

    EclOutput eclTest(outFileH);

    std::vector<std::tuple<std::string, EIOD::eclArrType, int>> arrayList = file1.getList();

    int n=0;

    for (auto array: arrayList) {

        std::string name = std::get<0>(array);
        EIOD::eclArrType arrType = std::get<1>(array);
        int size = std::get<2>(array);

        eclTest.writeBinaryHeader(name,size,arrType);

        if (arrType==EIOD::INTE) {
            std::vector<int> vect=file1.get<int>(n);
            eclTest.writeBinaryArray(vect);
        } else if (arrType==EIOD::REAL) {
            std::vector<float> vect=file1.get<float>(n);
            eclTest.writeBinaryArray(vect);
        } else if (arrType==EIOD::DOUB) {
            std::vector<double> vect=file1.get<double>(n);
            eclTest.writeBinaryArray(vect);
        } else if (arrType==EIOD::LOGI) {
            std::vector<bool> vect=file1.get<bool>(n);
            eclTest.writeBinaryArray(vect);
        } else if (arrType==EIOD::CHAR) {
            std::vector<std::string> vect=file1.get<std::string>(n);
            eclTest.writeBinaryCharArray(vect);
        } else if (arrType==EIOD::MESS) {
            // pass, no associated data
        } else {
            std::cout << "unknown type " << std::endl;
            exit(1);
        }

        n++;
    }

    outFileH.close();

    BOOST_CHECK_EQUAL(compare_files(inputFile, testFile), true);

    if (remove(testFile.c_str())==-1) {
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };


}




