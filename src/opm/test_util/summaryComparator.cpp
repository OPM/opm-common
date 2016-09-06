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



void SummaryComparator::setTimeVecs(std::vector<double> &timeVec1,std::vector<double> &timeVec2){
    timeVec1.reserve(ecl_sum_get_data_length(ecl_sum1));
    for (int time_index = 0; time_index < ecl_sum_get_data_length(ecl_sum1); time_index++){
        timeVec1.push_back(ecl_sum_iget_sim_days(ecl_sum1 , time_index ));
    }
    timeVec2.reserve(ecl_sum_get_data_length(ecl_sum2));
    for (int time_index = 0; time_index < ecl_sum_get_data_length(ecl_sum2); time_index++){
        timeVec2.push_back(ecl_sum_iget_sim_days(ecl_sum2 , time_index ));
    }
}



void SummaryComparator::getDataVecs(std::vector<double> &dataVec1,std::vector<double> &dataVec2,const char* keyword){
    dataVec1.reserve(ecl_sum_get_data_length(ecl_sum1));
    for (int time_index = 0; time_index < ecl_sum_get_data_length(ecl_sum1); time_index++){
        dataVec1.push_back(ecl_sum_iget(ecl_sum1, time_index, ecl_sum_get_general_var_params_index( ecl_sum1 , keyword )));
    }
    dataVec2.reserve(ecl_sum_get_data_length(ecl_sum2));
    for (int time_index = 0; time_index < ecl_sum_get_data_length(ecl_sum2); time_index++){

        dataVec2.push_back(ecl_sum_iget(ecl_sum2, time_index, ecl_sum_get_general_var_params_index( ecl_sum2 , keyword )));
    }
}



void SummaryComparator::setDataSets(std::vector<double> &timeVec1,std::vector<double> &timeVec2){
    if(timeVec1.size()< timeVec2.size()){
        ecl_sum_fileShort = this->ecl_sum1;
        ecl_sum_fileLong = this->ecl_sum2;
    }
    else{
        ecl_sum_fileShort = this->ecl_sum2;
        ecl_sum_fileLong = this->ecl_sum1;
    }
}



void SummaryComparator::chooseReference(std::vector<double> &timeVec1,std::vector<double> &timeVec2,std::vector<double> &dataVec1,std::vector<double> &dataVec2){
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
        double value = SummaryComparator::unitStep((*checkDataVec)[checkIndex-1]);
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



double SummaryComparator::average(std::vector<double> &vec){
    return std::accumulate(vec.begin(), vec.end(), 0.0) / vec.size();
}



double SummaryComparator::interpolation(double check_value, double check_value_prev , double time_array[3]){
    //does a Linear Polation
    double time_check = time_array[0]; //From the check time vector
    double time_check_prev = time_array[1];//From the check time vector
    double time_reference = time_array[2];// The time of interest, from the reference vector
    double sloap, factor, lp_value;
    sloap = (check_value - check_value_prev)/double(time_check - time_check_prev);
    factor = (time_reference - time_check_prev)/double(time_check - time_check_prev);
    lp_value = check_value_prev + factor*sloap*(time_check - time_check_prev);
    return lp_value;
}



double SummaryComparator::median(std::vector<double> vec) {
    // Sorts and returns the median in a std::vector<double>
    if(vec.empty()){
        return 0;
    }
    else {
        std::sort(vec.begin(), vec.end());
        if(vec.size() % 2 == 0)
            return (vec[vec.size()/2 - 1] + vec[vec.size()/2]) / 2;
        else
            return vec[vec.size()/2];
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



const char* SummaryComparator::getUnit(const char* keyword){
    return ecl_sum_get_unit(ecl_sum_fileShort, keyword); //Called only when the keywords are equal in the getDeviations()-function
}



double SummaryComparator::unitStep(double value){
    return value;
}
