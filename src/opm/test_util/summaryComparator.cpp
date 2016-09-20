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

#include <opm/test_util/summaryComparator.hpp>
#include <ert/ecl/ecl_sum.h>
#include <ert/util/stringlist.h>
#include <ert/util/int_vector.h>
#include <ert/util/bool_vector.h>
#include <opm/common/ErrorMacros.hpp>
#include <cmath>
#include <numeric>

SummaryComparator::SummaryComparator(const char* basename1, const char* basename2, double absoluteTol, double relativeTol){
    ecl_sum1 = ecl_sum_fread_alloc_case(basename1, ":");
    ecl_sum2 = ecl_sum_fread_alloc_case(basename2, ":");
    if (ecl_sum1 == nullptr || ecl_sum2 == nullptr) {
        OPM_THROW(std::runtime_error, "Not able to open files");
    }
    absoluteTolerance = absoluteTol;
    relativeTolerance = relativeTol;
    keys1 = stringlist_alloc_new();
    keys2 = stringlist_alloc_new();
    ecl_sum_select_matching_general_var_list( ecl_sum1 , "*" , this->keys1);
    stringlist_sort(this->keys1 , nullptr );
    ecl_sum_select_matching_general_var_list( ecl_sum2 , "*" , this->keys2);
    stringlist_sort(this->keys2 , nullptr );

    if(stringlist_get_size(keys1) <= stringlist_get_size(keys2)){
        this->keysShort = this->keys1;
        this->keysLong = this->keys2;
    }else{
        this->keysShort = this->keys2;
        this->keysLong = this->keys1;
    }
}


SummaryComparator::~SummaryComparator(){
    ecl_sum_free(ecl_sum1);
    ecl_sum_free(ecl_sum2);
    stringlist_free(keys1);
    stringlist_free(keys2);
}


Deviation SummaryComparator::calculateDeviations(double val1, double val2){
    double absDev, relDev;
    Deviation deviation;
    absDev =  std::abs(val1 - val2);
    deviation.abs = absDev;
    if (val1 != 0 || val2 != 0) {
        relDev = absDev/double(std::max(std::abs(val1), std::abs(val2)));
        deviation.rel = relDev;
    }
    return deviation;
}


void SummaryComparator::setTimeVecs(std::vector<double> &timeVec1,
                                    std::vector<double> &timeVec2){
    timeVec1.reserve(ecl_sum_get_data_length(ecl_sum1));
    for (int time_index = 0; time_index < ecl_sum_get_data_length(ecl_sum1); time_index++){
        timeVec1.push_back(ecl_sum_iget_sim_days(ecl_sum1 , time_index ));
    }
    timeVec2.reserve(ecl_sum_get_data_length(ecl_sum2));
    for (int time_index = 0; time_index < ecl_sum_get_data_length(ecl_sum2); time_index++){
        timeVec2.push_back(ecl_sum_iget_sim_days(ecl_sum2 , time_index ));
    }
}


void SummaryComparator::getDataVecs(std::vector<double> &dataVec1,
                                    std::vector<double> &dataVec2,
                                    const char* keyword){
    dataVec1.reserve(ecl_sum_get_data_length(ecl_sum1));
    for (int time_index = 0; time_index < ecl_sum_get_data_length(ecl_sum1); time_index++){
        dataVec1.push_back(ecl_sum_iget(ecl_sum1, time_index, ecl_sum_get_general_var_params_index( ecl_sum1 , keyword )));
    }
    dataVec2.reserve(ecl_sum_get_data_length(ecl_sum2));
    for (int time_index = 0; time_index < ecl_sum_get_data_length(ecl_sum2); time_index++){

        dataVec2.push_back(ecl_sum_iget(ecl_sum2, time_index, ecl_sum_get_general_var_params_index( ecl_sum2 , keyword )));
    }
}


void SummaryComparator::setDataSets(const std::vector<double>& timeVec1,
                                    const std::vector<double>& timeVec2){
    if(timeVec1.size() < timeVec2.size()){
        ecl_sum_fileShort = this->ecl_sum1;
        ecl_sum_fileLong = this->ecl_sum2;
    }
    else{
        ecl_sum_fileShort = this->ecl_sum2;
        ecl_sum_fileLong = this->ecl_sum1;
    }
}


void SummaryComparator::chooseReference(const std::vector<double>& timeVec1,
                                        const std::vector<double>& timeVec2,
                                        const std::vector<double>& dataVec1,
                                        const std::vector<double>& dataVec2){
    if(timeVec1.size() <= timeVec2.size()){
        referenceVec = &timeVec1; // time vector
        referenceDataVec = &dataVec1; //data vector
        checkVec = &timeVec2;
        checkDataVec = &dataVec2;
    }
    else{
        referenceVec = &timeVec2;
        referenceDataVec = &dataVec2;
        checkVec = &timeVec1;
        checkDataVec = &dataVec1;
    }
}


void SummaryComparator::getDeviation(size_t refIndex, size_t &checkIndex, Deviation &dev){
    if((*referenceVec)[refIndex] == (*checkVec)[checkIndex]){
        dev = SummaryComparator::calculateDeviations((*referenceDataVec)[refIndex], (*checkDataVec)[checkIndex]);
        checkIndex++;
        return;
    }
    else if((*referenceVec)[refIndex]<(*checkVec)[checkIndex]){
        double value = SummaryComparator::unitStep((*checkDataVec)[checkIndex]);
        /*Must be a little careful here. Flow writes out old value first,
          than changes value. Say there should be a change in production rate from A to B at timestep 300.
          Then the data of time step 300 is A and the next timestep will have value B. Must use the upper limit. */
        dev = SummaryComparator::calculateDeviations((*referenceDataVec)[refIndex], value);
        checkIndex++;
        return;
    }
    else{
        checkIndex++;
        getDeviation(refIndex, checkIndex , dev);
    }
    if(checkIndex == checkVec->size() -1 ){
        return;
    }
}


void SummaryComparator::printUnits(){
    std::vector<double> timeVec1, timeVec2;
    setTimeVecs(timeVec1, timeVec2);  // Sets the time vectors, they are equal for all keywords (WPOR:PROD01 etc)
    setDataSets(timeVec1, timeVec2);
    for (int jvar = 0; jvar < stringlist_get_size(keysLong); jvar++){
        std::cout << stringlist_iget(keysLong, jvar) << " unit: " << ecl_sum_get_unit(ecl_sum_fileShort, stringlist_iget(keysLong, jvar)) << std::endl;
    }
}


//Called only when the keywords are equal in the getDeviations()-function
const char* SummaryComparator::getUnit(const char* keyword){
    return ecl_sum_get_unit(ecl_sum_fileShort, keyword);
}


void SummaryComparator::printKeywords(){
    int ivar = 0;
    std::vector<std::string> noMatchString;
    std::cout << "Keywords that are common for the files:" << std::endl;
    while(ivar < stringlist_get_size(keysLong)){
        const char* keyword = stringlist_iget(keysLong, ivar);
        if (stringlist_contains(keysLong, keyword) && stringlist_contains(keysShort, keyword)){
            std::cout << keyword << std::endl;
            ivar++;
        }
        else{
            noMatchString.push_back(keyword);
            ivar++;
        }
    }
    if(noMatchString.size() == 0){
        std::cout << "No keywords were different" << std::endl;
        return;
    }
    std::cout << "Keywords that are different: " << std::endl;
    for (const auto& it : noMatchString) std::cout << it << std::endl;

    std::cout << "\nOf the " << stringlist_get_size(keysLong) << " keywords " << stringlist_get_size(keysLong)-noMatchString.size() << " were equal and " << noMatchString.size() << " were different" << std::endl;
}


void SummaryComparator::printDataOfSpecificKeyword(const std::vector<double>& timeVec1,
                                                   const std::vector<double>& timeVec2,
                                                   const char* keyword){
    std::vector<double> dataVec1, dataVec2;

    getDataVecs(dataVec1,dataVec2,keyword);
    chooseReference(timeVec1, timeVec2,dataVec1,dataVec2);
    size_t ivar = 0;
    size_t jvar = 0;
    const char separator = ' ';
    const int numWidth = 14;
    std::cout << std::left << std::setw(numWidth) << std::setfill(separator) << "Time";
    std::cout << std::left << std::setw(numWidth) << std::setfill(separator) << "Ref data";
    std::cout << std::left << std::setw(numWidth) << std::setfill(separator) << "Check data" << std::endl;

    while(ivar < referenceVec->size()){
        if(ivar == referenceVec->size() || jvar == checkVec->size() ){
            break;
        }
        if((*referenceVec)[ivar] == (*checkVec)[jvar]){
            std::cout << std::left << std::setw(numWidth) << std::setfill(separator) << (*referenceVec)[ivar];
            std::cout << std::left << std::setw(numWidth) << std::setfill(separator) << (*referenceDataVec)[ivar];
            std::cout << std::left << std::setw(numWidth) << std::setfill(separator) << (*checkDataVec)[jvar] << std::endl;
            ivar++;
            jvar++;
        }else if((*referenceVec)[ivar] < (*checkVec)[jvar]){
            std::cout << std::left << std::setw(numWidth) << std::setfill(separator) << (*referenceVec)[ivar];
            std::cout << std::left << std::setw(numWidth) << std::setfill(separator) << (*referenceDataVec)[ivar];
            std::cout << std::left << std::setw(numWidth) << std::setfill(separator) << "" << std::endl;
            ivar++;
        }
        else{
            std::cout << std::left << std::setw(numWidth) << std::setfill(separator) << (*checkVec)[jvar];
            std::cout << std::left << std::setw(numWidth) << std::setfill(separator) << "";
            std::cout << std::left << std::setw(numWidth) << std::setfill(separator) << (*checkDataVec)[jvar] << std::endl;
            jvar++;
        }
    }
}
