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


BOOST_AUTO_TEST_CASE(TestERst_1) {

    std::string testFile="SPE1_TESTCASE.UNRST";
    std::vector<int> refSeqnumList= {1,2,5,10,15,25,50,100,120};

    ERst rst1(testFile);
    rst1.loadReportStepNumber(5);

    std::vector<int> seqnums = rst1.listOfReportStepNumbers();

    BOOST_CHECK_EQUAL(seqnums==refSeqnumList, true);

    BOOST_CHECK_EQUAL(rst1.hasReportStepNumber(4), false);
    BOOST_CHECK_EQUAL(rst1.hasReportStepNumber(5), true);

    BOOST_CHECK_THROW(rst1.loadReportStepNumber(4) , std::invalid_argument );

    // tskille: should code throw an exception for non existing restart step or return an empty vector ?

    std::vector<std::tuple<std::string, EIOD::eclArrType, int>> rstArrays = rst1.listOfRstArrays(4);
    BOOST_CHECK_EQUAL(rstArrays.size(), 0);

    // non exising sequence
    BOOST_CHECK_THROW(std::vector<int> vect1=rst1.getRst<int>("ICON",0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<float> vect2=rst1.getRst<float>("PRESSURE",0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<double> vect3=rst1.getRst<double>("XGRP",0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<bool> vect4=rst1.getRst<bool>("LOGIHEAD",0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<std::string> vect4=rst1.getRst<std::string>("ZWEL",0) , std::invalid_argument );

    // sequence exists but data is not loaded
    BOOST_CHECK_THROW(std::vector<int> vect1=rst1.getRst<int>("ICON",10) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<float> vect2=rst1.getRst<float>("PRESSURE",10) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<double> vect3=rst1.getRst<double>("XGRP",10) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<bool> vect4=rst1.getRst<bool>("LOGIHEAD",10) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<std::string> vect4=rst1.getRst<std::string>("ZWEL",10) , std::runtime_error );

    // check call with wrong type as well

    BOOST_CHECK_THROW(std::vector<float> vect1=rst1.getRst<float>("ICON",5) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<int> vect2=rst1.getRst<int>("PRESSURE",5), std::runtime_error );
    BOOST_CHECK_THROW(std::vector<float> vect3=rst1.getRst<float>("XGRP",5), std::runtime_error );
    BOOST_CHECK_THROW(std::vector<double> vect4=rst1.getRst<double>("LOGIHEAD",5), std::runtime_error );
    BOOST_CHECK_THROW(std::vector<bool> vect5=rst1.getRst<bool>("ZWEL",5), std::runtime_error );

    rst1.loadReportStepNumber(25);

    std::vector<int> vect1=rst1.getRst<int>("ICON",25);
    std::vector<float> vect2=rst1.getRst<float>("PRESSURE",25);
    std::vector<double> vect3=rst1.getRst<double>("XGRP",25);
    std::vector<bool> vect4=rst1.getRst<bool>("LOGIHEAD",25);
    std::vector<std::string> vect5=rst1.getRst<std::string>("ZWEL",25);
}


BOOST_AUTO_TEST_CASE(TestERst_2) {

    std::string testFile="SPE1_TESTCASE.UNRST";
    std::string outFile="TEST.UNRST";

    ERst rst1(testFile);

    std::ofstream outFileH;
    outFileH.open(outFile, std::ios::out |  std::ios::binary  );

    EclOutput eclTest(outFileH);

    std::vector<int> seqnums = rst1.listOfReportStepNumbers();

    for (unsigned int i=0; i<seqnums.size(); i++) {
        rst1.loadReportStepNumber(seqnums[i]);

        std::vector<std::tuple<std::string, EIOD::eclArrType, int>> rstArrays = rst1.listOfRstArrays(seqnums[i]);

        for (unsigned int j=0; j<rstArrays.size(); j++) {

            std::string name=std::get<0>(rstArrays[j]);
            EIOD::eclArrType arrType=std::get<1>(rstArrays[j]);
            int size=std::get<2>(rstArrays[j]);

            eclTest.writeBinaryHeader(name,size,arrType);

            if (arrType==EIOD::INTE) {
                std::vector<int> vect=rst1.getRst<int>(name,seqnums[i]);
                eclTest.writeBinaryArray(vect);
            } else if (arrType==EIOD::REAL) {
                std::vector<float> vect=rst1.getRst<float>(name,seqnums[i]);
                eclTest.writeBinaryArray(vect);
            } else if (arrType==EIOD::DOUB) {
                std::vector<double> vect=rst1.getRst<double>(name,seqnums[i]);
                eclTest.writeBinaryArray(vect);
            } else if (arrType==EIOD::LOGI) {
                std::vector<bool> vect=rst1.getRst<bool>(name,seqnums[i]);
                eclTest.writeBinaryArray(vect);
            } else if (arrType==EIOD::CHAR) {
                std::vector<std::string> vect=rst1.getRst<std::string>(name,seqnums[i]);
                eclTest.writeBinaryCharArray(vect);
            } else if (arrType==EIOD::MESS) {
                // pass, no associated data
            } else {
                std::cout << "unknown type " << std::endl;
                exit(1);
            }
        }
    }

    outFileH.close();

    BOOST_CHECK_EQUAL(compare_files(testFile, outFile), true);

    if (remove(outFile.c_str())==-1) {
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}

BOOST_AUTO_TEST_CASE(TestERst_3) {

    std::string testFile="SPE1_TESTCASE.FUNRST";
    std::string outFile="TEST.FUNRST";

    ERst rst1(testFile);

    std::ofstream outFileH;
    outFileH.open(outFile, std::ios::out  );

    EclOutput eclTest(outFileH);

    std::vector<int> seqnums = rst1.listOfReportStepNumbers();

    for (unsigned int i=0; i<seqnums.size(); i++) {
        rst1.loadReportStepNumber(seqnums[i]);

        std::vector<std::tuple<std::string, EIOD::eclArrType, int>> rstArrays = rst1.listOfRstArrays(seqnums[i]);

        for (unsigned int j=0; j<rstArrays.size(); j++) {

            std::string name=std::get<0>(rstArrays[j]);
            EIOD::eclArrType arrType=std::get<1>(rstArrays[j]);
            int size=std::get<2>(rstArrays[j]);

            eclTest.writeFormattedHeader(name,size,arrType);

            if (arrType==EIOD::INTE) {
                std::vector<int> vect=rst1.getRst<int>(name,seqnums[i]);
                eclTest.writeFormattedArray(vect);
            } else if (arrType==EIOD::REAL) {
                std::vector<float> vect=rst1.getRst<float>(name,seqnums[i]);
                eclTest.writeFormattedArray(vect);
            } else if (arrType==EIOD::DOUB) {
                std::vector<double> vect=rst1.getRst<double>(name,seqnums[i]);
                eclTest.writeFormattedArray(vect);
            } else if (arrType==EIOD::LOGI) {
                std::vector<bool> vect=rst1.getRst<bool>(name,seqnums[i]);
                eclTest.writeFormattedArray(vect);
            } else if (arrType==EIOD::CHAR) {
                std::vector<std::string> vect=rst1.getRst<std::string>(name,seqnums[i]);
                eclTest.writeFormattedCharArray(vect);
            } else if (arrType==EIOD::MESS) {
                // pass, no associated data
            } else {
                std::cout << "unknown type " << std::endl;
                exit(1);
            }
        }
    }

    outFileH.close();

    BOOST_CHECK_EQUAL(compare_files(testFile, outFile), true);

    if (remove(outFile.c_str())==-1) {
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
  
}

