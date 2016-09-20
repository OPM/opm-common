/*
   Copyright 2016 Statoil ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms of
   the GNU General Public License as published   by the Free Software Foundation, either
   version 3 of the License, or (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */


#include <opm/test_util/summaryIntegrationTest.hpp>
#include <opm/common/ErrorMacros.hpp>
#include <ert/ecl/ecl_sum.h>
#include <ert/util/stringlist.h>


void IntegrationTest::getIntegrationTest(){
    std::vector<double> timeVec1, timeVec2;
    setTimeVecs(timeVec1, timeVec2);  // Sets the time vectors, they are equal for all keywords (WPOR:PROD01 etc)
    setDataSets(timeVec1, timeVec2);

    int ivar = 0;
    if(!allowDifferentAmountOfKeywords){
        if(stringlist_get_size(keysShort) != stringlist_get_size(keysLong)){
            OPM_THROW(std::invalid_argument, "Different ammont of keywords in the two summary files.");
        }
    }
    if(printKeyword){
        printKeywords();
        return;
    }

    std::string keywordWithGreatestErrorRatio;
    double greatestRatio = 0;

    //Iterates over all keywords from the restricted file, use iterator "ivar". Searches for a  match in the file with more keywords, use the itarator "jvar".
    while(ivar < stringlist_get_size(keysShort)){
        const char* keyword = stringlist_iget(keysShort, ivar);

        if(oneOfTheMainVariables){
            std::string keywordString(keyword);
            std::string substr = keywordString.substr(0,4);
            if(substr!= mainVariable){
                ivar++;
                continue;
            }
        }
        for (int jvar = 0; jvar < stringlist_get_size(keysLong); jvar++){

            if (strcmp(keyword, stringlist_iget(keysLong, jvar)) == 0){ //When the keywords are equal, proceed in comparing summary files.
                /*	if(!checkUnits(keyword)){
                    OPM_THROW(std::runtime_error, "For keyword " << keyword << " the unit of the two files is not equal. Not possible to compare.");
                    } //Comparing the unit of the two vectors.*/
                checkForKeyword(timeVec1, timeVec2, keyword);
                if(findVectorWithGreatestErrorRatio){
                    WellProductionVolume volume = getSpecificWellVolume(timeVec1,timeVec2, keyword);
                    findGreatestErrorRatio(volume,greatestRatio, keyword, keywordWithGreatestErrorRatio);
                }
                break;
            }
            //will only enter here if no keyword match
            if(jvar == stringlist_get_size(keysLong)-1){
                if(!allowDifferentAmountOfKeywords){
                    OPM_THROW(std::invalid_argument, "No match on keyword");
                }
            }
        }
        ivar++;
    }
    if(findVectorWithGreatestErrorRatio){
        std::cout << "The keyword " << keywordWithGreatestErrorRatio << " had the greatest error ratio, which was " << greatestRatio << std::endl;
    }
    if((findVolumeError || oneOfTheMainVariables) && !findVectorWithGreatestErrorRatio){
        evaluateWellProductionVolume();
    }
    if(allowSpikes){
        std::cout << "checkWithSpikes succeeded." << std::endl;
    }
}


void IntegrationTest::getIntegrationTest(const char* keyword){
    if(stringlist_contains(keysShort,keyword) && stringlist_contains(keysLong, keyword)){
        std::vector<double> timeVec1, timeVec2;
        setTimeVecs(timeVec1, timeVec2);  // Sets the time vectors, they are equal for all keywords (WPOR:PROD01 etc)
        setDataSets(timeVec1, timeVec2);

        if(printSpecificKeyword){
            printDataOfSpecificKeyword(timeVec1, timeVec2, keyword);
        }
        if(findVolumeError){
            WellProductionVolume volume = getSpecificWellVolume(timeVec1, timeVec2, keyword);
            if(volume.error == 0){
                std::cout << "For keyword " << keyword << " the total production volume is 0" << std::endl;
            }
            else{
                std::cout << "For keyword " << keyword << " the total production volume is "<< volume.total;
                std::cout << ", the error volume is " << volume.error << " the error ratio is " << volume.error/volume.total << std::endl;
            }
        }
        checkForKeyword(timeVec1, timeVec2, keyword);
        return;
    }
    OPM_THROW(std::invalid_argument, "The keyword used is not common for the two files.");
}


void IntegrationTest::checkForKeyword(const std::vector<double>& timeVec1,
                                      const std::vector<double>& timeVec2,
                                      const char* keyword){
    std::vector<double> dataVec1, dataVec2;
    getDataVecs(dataVec1,dataVec2,keyword);
    chooseReference(timeVec1, timeVec2,dataVec1,dataVec2);
    if(allowSpikes){
        checkWithSpikes(keyword);
    }
    if(findVolumeError ||oneOfTheMainVariables ){
        volumeErrorCheck(keyword);
    }
}


int IntegrationTest::checkDeviation(const Deviation& deviation){
    double absTol = getAbsTolerance();
    double relTol = getRelTolerance();
    if (deviation.rel> relTol && deviation.abs > absTol){
        return 1;
    }
    return 0;
}


void IntegrationTest::findGreatestErrorRatio(const WellProductionVolume& volume,
                                             double &greatestRatio,
                                             const char* currentKeyword,
                                             std::string &greatestErrorRatio){
    if (volume.total != 0 && (volume.total - volume.error > getAbsTolerance()) ){
        if(volume.error/volume.total > greatestRatio){
            greatestRatio = volume.error/volume.total;
            std::string currentKeywordStr(currentKeyword);
            greatestErrorRatio = currentKeywordStr;
        }
    }
}


void IntegrationTest::volumeErrorCheck(const char* keyword){
    const smspec_node_type * node = ecl_sum_get_general_var_node (ecl_sum_fileShort ,keyword);//doesn't matter which ecl_sum_file one uses, the kewyord SHOULD be equal in terms of smspec data.
    bool hist = smspec_node_is_historical(node);
    /* returns true if the keyword corresponds to a summary vector "history".
       E.g. WOPRH, where the last character, 'H', indicates that it is a HISTORY vector.*/
    if(hist){
        return;//To make sure we do not include history vectors.
    }
    if (!mainVariable.empty()){
        std::string keywordString(keyword);
        std::string firstFour = keywordString.substr(0,4);

        if(mainVariable == firstFour && firstFour == "WOPR"){
            if(firstFour == "WOPR"){
                WellProductionVolume result =  getWellProductionVolume(keyword);
                WOP += result;
                return;
            }
        }
        if(mainVariable == firstFour && firstFour == "WWPR"){
            if(firstFour == "WWPR"){
                WellProductionVolume result = getWellProductionVolume(keyword);
                WWP += result;
                return;
            }
        }
        if(mainVariable == firstFour && firstFour == "WGPR"){
            if(firstFour == "WGPR"){
                WellProductionVolume result = getWellProductionVolume(keyword);
                WGP += result;
                return;
            }
        }
        if(mainVariable == firstFour && firstFour == "WBHP"){
            if(firstFour == "WBHP"){
                WellProductionVolume result = getWellProductionVolume(keyword);
                WBHP += result;
                return;
            }
        }
    }
    updateVolumeError(keyword);
}


void IntegrationTest::updateVolumeError(const char* keyword){
    std::string keywordString(keyword);
    std::string firstFour = keywordString.substr(0,4);

    if(firstFour == "WOPR"){
        WellProductionVolume result =  getWellProductionVolume(keyword);
        WOP += result;
    }
    if(firstFour == "WWPR"){
        WellProductionVolume result = getWellProductionVolume(keyword);
        WWP += result;
    }
    if(firstFour == "WGPR"){
        WellProductionVolume result = getWellProductionVolume(keyword);
        WGP += result;

    }
    if(firstFour == "WBHP"){
        WellProductionVolume result = getWellProductionVolume(keyword);
        WBHP += result;
    }
}


WellProductionVolume IntegrationTest::getWellProductionVolume(const char * keyword){
    double total = integrate(*referenceVec, *referenceDataVec);
    double error = integrateError(*referenceVec, *referenceDataVec,
                                  *checkVec, *checkDataVec);
    WellProductionVolume wPV;
    wPV.total = total;
    wPV.error = error;
    if(wPV.total != 0 && wPV.total-wPV.error > getAbsTolerance()){
        if( (wPV.error/wPV.total > getRelTolerance()) && throwExceptionForTooGreatErrorRatio){
            OPM_THROW(std::runtime_error, "For the keyword "<< keyword << " the error ratio was " << wPV.error/wPV.total << " which is greater than the tolerance " << getRelTolerance());
        }
    }
    return wPV;
}


void IntegrationTest::evaluateWellProductionVolume(){
    if(mainVariable.empty()){
        double ratioWOP, ratioWWP, ratioWGP, ratioWBHP;
        ratioWOP = WOP.error/WOP.total;
        ratioWWP = WWP.error/WWP.total;
        ratioWGP = WGP.error/WGP.total;
        ratioWBHP = WBHP.error/WBHP.total;
        std::cout << "\n The total oil volume is " << WOP.total << ". The error volume is "<< WOP.error <<  ". The error ratio is " << ratioWOP << std::endl;
        std::cout << "\n The total water volume is " << WWP.total << ". The error volume is "<< WWP.error <<  ". The error ratio is " << ratioWWP << std::endl;
        std::cout << "\n The total gas volume is " << WGP.total <<". The error volume is "<< WGP.error <<  ". The error ratio is " << ratioWGP << std::endl;
        std::cout << "\n The total area under the WBHP curve is " << WBHP.total << ". The area under the error curve is "<< WBHP.error <<  ". The error ratio is " << ratioWBHP << std::endl << std::endl;
    }
    if(mainVariable == "WOPR"){
        std::cout << "\nThe total oil volume is " << WOP.total << ". The error volume is "<< WOP.error <<  ". The error ratio is " << WOP.error/WOP.total << std::endl<< std::endl;
    }
    if(mainVariable == "WWPR"){
        std::cout << "\nThe total water volume is " << WWP.total << ". The error volume is "<< WWP.error <<  ". The error ratio is " << WWP.error/WWP.total << std::endl<< std::endl;
    }
    if(mainVariable == "WGPR"){
        std::cout << "\nThe total gas volume is " << WGP.total <<". The error volume is "<< WGP.error <<  ". The error ratio is " << WGP.error/WGP.total << std::endl<< std::endl;
    }
    if(mainVariable == "WBHP"){
        std::cout << "\nThe total area under the WBHP curve " << WBHP.total << ". The area under the error curve is "<< WBHP.error <<  ". The error ratio is " << WBHP.error/WBHP.total << std::endl << std::endl;
    }
}


void IntegrationTest::checkWithSpikes(const char* keyword){
    int errorOccurrences = 0;
    size_t jvar = 0 ;
    bool spikePrev = false;
    bool spikeCurrent = false;
    Deviation deviation;

    for (size_t ivar = 0; ivar < referenceVec->size(); ivar++){
        int errorOccurrencesPrev = errorOccurrences;
        spikePrev = spikeCurrent;
        getDeviation(ivar,jvar, deviation);
        errorOccurrences += checkDeviation(deviation);
        if (errorOccurrences != errorOccurrencesPrev){
            spikeCurrent = true;
        } else{
            spikeCurrent = false;
        }
        if(spikePrev&&spikeCurrent){
            std::cout << "For keyword " << keyword << " at time step " << (*referenceVec)[ivar] <<std::endl;
            OPM_THROW(std::invalid_argument, "For keyword " << keyword << " at time step " << (*referenceVec)[ivar] << ", wwo deviations in a row exceed the limit. Not a spike value. Integration test fails." );
        }
        if(errorOccurrences > this->spikeLimit){
            std::cout << "For keyword " << keyword << std::endl;
            OPM_THROW(std::invalid_argument, "For keyword " << keyword << " too many spikes in the vector. Integration test fails.");
        }
    }
}


WellProductionVolume
IntegrationTest::getSpecificWellVolume(const std::vector<double>& timeVec1,
                                       const std::vector<double>& timeVec2,
                                       const char* keyword){
    std::vector<double> dataVec1, dataVec2;
    getDataVecs(dataVec1,dataVec2,keyword);
    chooseReference(timeVec1, timeVec2,dataVec1,dataVec2);
    return getWellProductionVolume(keyword);
}


bool IntegrationTest::checkUnits(const char * keyword){
    const smspec_node_type * node1 = ecl_sum_get_general_var_node (ecl_sum_fileShort ,keyword);
    const smspec_node_type * node2 = ecl_sum_get_general_var_node (ecl_sum_fileLong ,keyword);
    if(strcmp(smspec_node_get_unit(node1),smspec_node_get_unit(node2)) == 0){
        return true;
    }
    return false;
}


double IntegrationTest::integrate(const std::vector<double>& timeVec,
                                  const std::vector<double>& dataVec){
    double totalSum = 0;
    double width, height;
    if(timeVec.size() != dataVec.size()){
        OPM_THROW(std::runtime_error, "The size of the time vector does not match the size of the data vector.");
    }
    for(size_t i = 0; i < timeVec.size()-1; i++){
        width = timeVec[i+1] - timeVec[i];
        height = dataVec[i+1];
        totalSum += getRectangleArea(height, width);
    }
    return totalSum;
}


double IntegrationTest::integrateError(const std::vector<double>& timeVec1,
                                       const std::vector<double>& dataVec1,
                                       const std::vector<double>& timeVec2,
                                       const std::vector<double>& dataVec2){
    // When the data corresponds to a rate the integration will become a Riemann
    // sum.  This function calculates the Riemann sum of the error.  The reason why
    // a Riemann sum is used is because of the way the data is written to file.
    // When a change occur (e.g. change of a rate), the data (value and time) is
    // written to file, THEN the change happens in the simulator, i.e., we will
    // notice the change at the next step.
    //
    // Keep in mind that the summary vector is NOT a continuous curve, only points
    // of data (time, value).  We have to guess what happens between the data
    // points, we do this by saying: "There are no change, the only change happens
    // at the data points."  As stated above, the value of this constant "height" of
    // the rectangle corresponds to the value of the last time step.  Thus we have
    // to use the "right hand side value of the rectangle as height
    //
    // someDataVector[ivar] instead of someDataVector[ivar-1]
    //
    // (which intuition is saying is the correct value to use).

    if(timeVec1.size() != dataVec1.size() || timeVec2.size() != dataVec2.size() ){
        OPM_THROW(std::runtime_error, "The size of the time vector does not match the size of the data vector.");
    }
    double errorSum = 0;
    double rightEdge, leftEdge, width;
    size_t i = 1;
    size_t j = 1;
    leftEdge = timeVec1[0];
    while(i < timeVec1.size()){
        if(j == timeVec2.size() ){
            break;
        }
        if(timeVec1[i] == timeVec2[j]){
            rightEdge = timeVec1[i];
            width = rightEdge - leftEdge;
            double dev = std::abs(dataVec1[i] - dataVec2[j]);
            errorSum += getRectangleArea(dev, width);
            leftEdge = rightEdge;
            i++;
            j++;
            continue;
        }
        if(timeVec1[i] < timeVec2[j]){
            rightEdge = timeVec1[i];
            width = rightEdge - leftEdge;
            double value = unitStep(dataVec2[j]);
            double dev = std::abs(dataVec1[i]-value);
            errorSum += getRectangleArea(dev, width);
            leftEdge = rightEdge;
            i++;
            continue;
        }
        if(timeVec2[j] < timeVec1[i]){
            rightEdge = timeVec2[j];
            width = rightEdge - leftEdge;
            double value = unitStep(dataVec1[i]);
            double dev = std::abs(dataVec2[j]-value);
            errorSum += getRectangleArea(dev, width);
            leftEdge = rightEdge;
            j++;
            continue;
        }
    }
    return errorSum;
}
