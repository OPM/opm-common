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
#include <examples/test_util/ESmry.hpp>

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

BOOST_AUTO_TEST_CASE(TestESmry_1) {

    std::string testFile="SPE1CASE1";

    std::vector<std::string> ref_keywords={ "TIME","FGOR","FOPR","WBHP:INJ","WBHP:PROD","WGIR:INJ","WGIR:PROD","WGIT:INJ","WGIT:PROD","WGOR:PROD","WGPR:INJ","WGPR:PROD","WGPT:INJ",
                               "WGPT:PROD","WOIR:INJ","WOIR:PROD","WOIT:INJ","WOIT:PROD","WOPR:INJ","WOPR:PROD","WOPT:INJ","WOPT:PROD","WWIR:INJ","WWIR:PROD","WWIT:INJ",
			       "WWIT:PROD","WWPR:INJ","WWPR:PROD","WWPT:INJ","WWPT:PROD","BPR:1,1,1","BPR:10,10,3"};
			       
    // check that class throws exceptions for non existing file
    BOOST_CHECK_THROW(ESmry smry1("XXXX_XXXX.SMSPEC") , std::runtime_error );
    
    ESmry smry1(testFile);
    
    std::vector<std::string> keywords=smry1.keywordList();
    BOOST_CHECK_EQUAL(keywords.size(), smry1.numberOfVectors());

    BOOST_CHECK_EQUAL(ref_keywords==keywords, true);
    
    BOOST_CHECK_EQUAL(smry1.hasKey("FOPT"),false);
    BOOST_CHECK_EQUAL(smry1.hasKey("FGOR"),true);
    BOOST_CHECK_EQUAL(smry1.hasKey("WBHP:PROD"),true);

    
    // FOPT not present in smspec file, should throw exception when using member function get 
    BOOST_CHECK_THROW(std::vector<float> fopt = smry1.get("FOPT"), std::invalid_argument);
    
    // use class EclFile to get array PARAMS for each time step in UNSMRY file1
    // Make a 2 D vector from these PARAMS arrays (inputData) and check
    // vectors in inputData with vectors stored in class ESmry onject
    
    EclFile file1("SPE1CASE1.UNSMRY");
    file1.loadData();
    
    std::vector<std::tuple<std::string, EIOD::eclArrType, int>> arrayList = file1.getList();
    
    std::vector<std::vector<float>> inputData;
    int n=0;
    
    for (auto array: arrayList) {
        std::string name = std::get<0>(array);

        if (name=="PARAMS"){
            std::vector<float> vect=file1.get<float>(n);
            inputData.push_back(vect);
        }
        n++;
    }
    
    for (unsigned int i=0;i<keywords.size();i++){
        std::vector<float> vect = smry1.get(keywords[i]);
        
        for (unsigned int j=0;j<vect.size();j++){
            BOOST_CHECK_EQUAL(vect[j], inputData[j][i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(TestESmry_2) {

   // same as previous test, but this time reading from formatted inputData
  
    std::string testFile="SPE1CASE1.FSMSPEC";

    ESmry smry1(testFile);
    
    std::vector<std::string> keywords=smry1.keywordList();
    BOOST_CHECK_EQUAL(keywords.size(), smry1.numberOfVectors());

    
    // FOPT not present in smspec file, should throw exception when using member function get 
    BOOST_CHECK_THROW(std::vector<float> fopt = smry1.get("FOPT"), std::invalid_argument);
    
    EclFile file1("SPE1CASE1.FUNSMRY");
    file1.loadData();
    
    std::vector<std::tuple<std::string, EIOD::eclArrType, int>> arrayList = file1.getList();
    
    std::vector<std::vector<float>> inputData;
    int n=0;
    
    for (auto array: arrayList) {
        std::string name = std::get<0>(array);

        if (name=="PARAMS"){
            std::vector<float> vect=file1.get<float>(n);
            inputData.push_back(vect);
        }
        n++;
    }
    
    for (unsigned int i=0;i<keywords.size();i++){
        std::vector<float> vect = smry1.get(keywords[i]);
        
        for (unsigned int j=0;j<vect.size();j++){
            BOOST_CHECK_EQUAL(vect[j], inputData[j][i]);
        }
    }
}

