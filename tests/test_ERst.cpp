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

#include <opm/output/eclipse/FileService/ERst.hpp>
#include <opm/output/eclipse/FileService/EclOutput.hpp>

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
    std::vector<int> refReportStepNumbers= {1,2,5,10,15,25,50,100,120};

    ERst rst1(testFile);
    rst1.loadReportStepNumber(5);
    
    std::vector<int> reportStepNumbers = rst1.listOfReportStepNumbers();

    BOOST_CHECK_EQUAL(reportStepNumbers==refReportStepNumbers, true);

    BOOST_CHECK_EQUAL(rst1.hasReportStepNumber(4), false);
    BOOST_CHECK_EQUAL(rst1.hasReportStepNumber(5), true);

    // try loading non-existing report step, should throw exception
    BOOST_CHECK_THROW(rst1.loadReportStepNumber(4) , std::invalid_argument );


    // try to get a list of vectors from non-existing report step, should throw exception
    std::vector<std::tuple<std::string, EIOD::eclArrType, int>> rstArrays; // = rst1.listOfRstArrays(4);
    BOOST_CHECK_THROW(rstArrays = rst1.listOfRstArrays(4), std::invalid_argument);

    // non exising report step number, should throw exception 
    
    BOOST_CHECK_THROW(std::vector<int> vect1=rst1.getRst<int>("ICON",0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<float> vect2=rst1.getRst<float>("PRESSURE",0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<double> vect3=rst1.getRst<double>("XGRP",0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<bool> vect4=rst1.getRst<bool>("LOGIHEAD",0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<std::string> vect4=rst1.getRst<std::string>("ZWEL",0) , std::invalid_argument );

    // report step number exists, but data is not loaded. Should throw exception
    BOOST_CHECK_THROW(std::vector<int> vect1=rst1.getRst<int>("ICON",10) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<float> vect2=rst1.getRst<float>("PRESSURE",10) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<double> vect3=rst1.getRst<double>("XGRP",10) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<bool> vect4=rst1.getRst<bool>("LOGIHEAD",10) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<std::string> vect4=rst1.getRst<std::string>("ZWEL",10) , std::runtime_error );

    // calling getRst<T> member function with wrong type, should throw exception

    BOOST_CHECK_THROW(std::vector<float> vect1=rst1.getRst<float>("ICON",5) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<int> vect2=rst1.getRst<int>("PRESSURE",5), std::runtime_error );
    BOOST_CHECK_THROW(std::vector<float> vect3=rst1.getRst<float>("XGRP",5), std::runtime_error );
    BOOST_CHECK_THROW(std::vector<double> vect4=rst1.getRst<double>("LOGIHEAD",5), std::runtime_error );
    BOOST_CHECK_THROW(std::vector<bool> vect5=rst1.getRst<bool>("ZWEL",5), std::runtime_error );

    rst1.loadReportStepNumber(25);

    std::vector<int> vect1 = rst1.getRst<int>("ICON",25);
    std::vector<float> vect2 = rst1.getRst<float>("PRESSURE",25);
    std::vector<double> vect3 = rst1.getRst<double>("XGRP",25);
    std::vector<bool> vect4 = rst1.getRst<bool>("LOGIHEAD",25);
    std::vector<std::string> vect5 = rst1.getRst<std::string>("ZWEL",25);
}


static void readAndWrite(EclOutput& eclTest, ERst& rst1,
                         const std::string& name, int seqnum,
                         EIOD::eclArrType arrType)
{
    if (arrType == EIOD::INTE) {
        std::vector<int> vect = rst1.getRst<int>(name, seqnum);
        eclTest.write(name, vect);
    } else if (arrType == EIOD::REAL) {
        std::vector<float> vect = rst1.getRst<float>(name, seqnum);
        eclTest.write(name, vect);
    } else if (arrType == EIOD::DOUB) {
        std::vector<double> vect = rst1.getRst<double>(name, seqnum);
        eclTest.write(name, vect);
    } else if (arrType == EIOD::LOGI) {
        std::vector<bool> vect = rst1.getRst<bool>(name, seqnum);
        eclTest.write(name, vect);
    } else if (arrType == EIOD::CHAR) {
        std::vector<std::string> vect = rst1.getRst<std::string>(name, seqnum);
        eclTest.write(name, vect);
    } else if (arrType == EIOD::MESS) {
        eclTest.write(name, std::vector<char>());
    } else {
        std::cout << "unknown type " << std::endl;
        exit(1);
    }
}


BOOST_AUTO_TEST_CASE(TestERst_2) {
    
    std::string testFile="SPE1_TESTCASE.UNRST";
    std::string outFile="TEST.UNRST";

    // using API for ERst to read all array from a binary unified restart file1
    // Then write the data back to a new file and check that new file is identical with input file 
    
    ERst rst1(testFile);

    {
        EclOutput eclTest(outFile, false);

        std::vector<int> seqnums = rst1.listOfReportStepNumbers();

        for (size_t i = 0; i < seqnums.size(); i++) {
            rst1.loadReportStepNumber(seqnums[i]);

            auto rstArrays = rst1.listOfRstArrays(seqnums[i]);

            for (auto& array : rstArrays) {
                std::string name = std::get<0>(array);
                EIOD::eclArrType arrType = std::get<1>(array);
                readAndWrite(eclTest, rst1, name, seqnums[i], arrType);
            }
        }
    }

    BOOST_CHECK_EQUAL(compare_files(testFile, outFile), true);

    if (remove(outFile.c_str()) == -1) {
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}

BOOST_AUTO_TEST_CASE(TestERst_3) {

    std::string testFile="SPE1_TESTCASE.FUNRST";
    std::string outFile="TEST.FUNRST";

    // using API for ERst to read all array from a formatted unified restart file1
    // Then write the data back to a new file and check that new file is identical with input file 

    ERst rst1(testFile);

    EclOutput eclTest(outFile, true);

    std::vector<int> seqnums = rst1.listOfReportStepNumbers();
    for (unsigned int i=0; i<seqnums.size(); i++) {
        rst1.loadReportStepNumber(seqnums[i]);

        auto rstArrays = rst1.listOfRstArrays(seqnums[i]);

        for (auto& array : rstArrays) {
            std::string name = std::get<0>(array);
            EIOD::eclArrType arrType = std::get<1>(array);
            readAndWrite(eclTest, rst1, name, seqnums[i], arrType);
        }
    }

    BOOST_CHECK_EQUAL(compare_files(testFile, outFile), true);

    if (remove(outFile.c_str() )== -1) {
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}

