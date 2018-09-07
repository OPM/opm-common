/*
   Copyright 2016 Statoil ASA.
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

#include "summaryRegressionTest.hpp"
#include <opm/common/ErrorMacros.hpp>
#include <ert/ecl/ecl_sum.h>
#include <ert/util/stringlist.h>
#include <string>

void SummaryRegressionTest::getRegressionTest(){
    std::vector<double> timeVec1, timeVec2;
    setTimeVecs(timeVec1, timeVec2);  // Sets the time vectors, they are equal for all keywords (WPOR:PROD01 etc)
    setDataSets(timeVec1, timeVec2); //Figures which dataset that contains more/less values pr keyword vector.
    std::cout << "Comparing " << timeVec1.size() << " steps." << std::endl;
    int ivar = 0;
    if(stringlist_get_size(keysShort) != stringlist_get_size(keysLong)){
        int missing_count = 0;
        std::cout << "Keywords missing from one case: " << std::endl;

        for (int i=0; i < stringlist_get_size( keysLong); i++) {
            const char * key = stringlist_iget( keysLong , i );
            if (!stringlist_contains( keysShort , key)) {
                std::cout << key << " ";

                missing_count++;
                if ((missing_count % 8) == 0)
                    std::cout << std::endl;
            }
        }
        std::cout << std::endl;

        HANDLE_ERROR(std::runtime_error, "Different amount of keywords in the two summary files.");
    }
    if(printKeyword){
        printKeywords();
    }


    //Iterates over all keywords from the restricted file, use iterator "ivar". Searches for a  match in the file with more keywords, use the iterator "jvar".
    bool throwAtEnd = false;
    while(ivar < stringlist_get_size(keysShort)){
        const char* keyword = stringlist_iget(keysShort, ivar);
        std::string keywordString(keyword);
        for (int jvar = 0; jvar < stringlist_get_size(keysLong); jvar++){
            if (strcmp(keyword, stringlist_iget(keysLong, jvar)) == 0){ //When the keywords are equal, proceed in comparing summary files.
                if (isRestartFile && keywordString.substr(3,1)=="T"){
                    break;
                }
                throwAtEnd |= !checkForKeyword(timeVec1, timeVec2, keyword);
                break;
            }
            //will only enter here if no keyword match
            if(jvar == stringlist_get_size(keysLong)-1){
                std::cout << "Could not find keyword: " << stringlist_iget(keysShort, ivar) << std::endl;
                OPM_THROW(std::runtime_error, "No match on keyword");
            }
        }
        ivar++;
    }
    if (analysis) {
        std::cout << deviations.size() << " summary keyword"
                  << (deviations.size() > 1 ? "s":"") << " exhibit failures" << std::endl;
        size_t len = ecl_sum_get_data_length(ecl_sum1);
        for (const auto& iter : deviations) {
            std::cout << "\t" << iter.first << std::endl;
            std::cout << "\t\tFails for " << iter.second.size() << " / " << len << " steps." << std::endl;
            std::cout.precision(7);
            double absErr = std::max_element(iter.second.begin(), iter.second.end(),
                                          [](const Deviation& a, const Deviation& b)
                                          {
                                            return a.abs < b.abs;
                                          })->abs;
            double relErr = std::max_element(iter.second.begin(), iter.second.end(),
                                          [](const Deviation& a, const Deviation& b)
                                          {
                                            return a.rel < b.rel;
                                          })->rel;
            std::cout << "\t\tLargest absolute error: "
                      <<  std::scientific << absErr << std::endl;
            std::cout << "\t\tLargest relative error: "
                      <<  std::scientific << relErr << std::endl;
        }
    }
    if (throwAtEnd)
      OPM_THROW(std::runtime_error, "Regression test failed.");
    else if (deviations.empty())
      std::cout << "Regression test succeeded." << std::endl;
}



void SummaryRegressionTest::getRegressionTest(const char* keyword){
    std::vector<double> timeVec1, timeVec2;
    setTimeVecs(timeVec1, timeVec2);  // Sets the time vectors, they are equal for all keywords (WPOR:PROD01 etc)
    setDataSets(timeVec1, timeVec2); //Figures which dataset that contains more/less values pr keyword vector.
    std::string keywordString(keyword);
    if(stringlist_contains(keysShort,keyword) && stringlist_contains(keysLong, keyword)){
        if (isRestartFile && keywordString.substr(3,1)=="T"){
            return;
        }
        if (checkForKeyword(timeVec1, timeVec2, keyword))
          std::cout << "Regression test succeeded." << std::endl;
        else
          OPM_THROW(std::runtime_error, "Regression test failed");

        return;
    }
    std::cout << "The keyword suggested, " << keyword << ", is not supported by one or both of the summary files. Please use a different keyword." << std::endl;
    OPM_THROW(std::runtime_error, "Input keyword from user does not exist in/is not common for the two summary files.");
}



bool SummaryRegressionTest::checkDeviation(Deviation deviation, const char* keyword, int refIndex, int checkIndex){
    double absTol = getAbsTolerance();
    double relTol = getRelTolerance();

    if (deviation.rel > relTol && deviation.abs > absTol){
        if (analysis) {
            deviations[keyword].push_back(deviation);
        } else {
            std::cout << "For keyword " << keyword  << std::endl;
            std::cout << "(days, reference value) and (days, check value) = (" << (*referenceVec)[refIndex] << ", " << (*referenceDataVec)[refIndex]
              << ") and (" << (*checkVec)[checkIndex-1] << ", " << (*checkDataVec)[checkIndex-1] << ")\n";
            // -1 in [checkIndex -1] because checkIndex is updated after leaving getDeviation function
            std::cout << "The absolute deviation is " << deviation.abs << ". The tolerance limit is " << absTol << std::endl;
            std::cout << "The relative deviation is " << deviation.rel << ". The tolerance limit is " << relTol << std::endl;
            HANDLE_ERROR(std::runtime_error, "Deviation exceed the limit.");
        }
        return false;
    }
    return true;
}



bool SummaryRegressionTest::checkForKeyword(std::vector<double>& timeVec1, std::vector<double>& timeVec2, const char* keyword){
    std::vector<double> dataVec1, dataVec2;
    getDataVecs(dataVec1,dataVec2,keyword);
    chooseReference(timeVec1, timeVec2,dataVec1,dataVec2);
    return startTest(keyword);
}



bool SummaryRegressionTest::startTest(const char* keyword){
    size_t jvar = 0;
    Deviation deviation;
    bool result = true;
    for (size_t ivar = 0; ivar < referenceVec->size(); ivar++){
        getDeviation(ivar, jvar, deviation);//Reads from the protected member variables in the super class.
        result = checkDeviation(deviation, keyword,ivar, jvar);
    }

    return result;
}
