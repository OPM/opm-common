/*
   Copyright 2019 Equinor ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#include "ERst.hpp"

#include <string>
#include <string.h>
#include <sstream>
#include <iterator>
#include <iomanip>
#include <algorithm>


ERst::ERst(std::string filename) : EclFile(filename)
{

    loadData("SEQNUM");

    std::vector<int> firstIndex;

    for (unsigned int i=0; i<array_name.size(); i++) {
        if (array_name[i]=="SEQNUM") {
            std::vector<int> seqn=get<int>(i);
            seqnum.push_back(seqn[0]);
            firstIndex.push_back(i);
        }
    }


    for (unsigned int i=0; i<seqnum.size(); i++) {

        std::pair<int,int> range;
        range.first=firstIndex[i];

        if (i!=(seqnum.size()-1)) {
            range.second=firstIndex[i+1];
        } else {
            range.second=array_name.size()-0;
        }

        arrIndexRange[seqnum[i]]=range;
    }

    nReports=seqnum.size();

    for (int i=0; i<nReports; i++) {
        reportLoaded[seqnum[i]]=false;
    }

};


const bool ERst::hasReportStepNumber(const int &number) const {

    auto search = arrIndexRange.find(number);

    if (search != arrIndexRange.end()) {
        return true;

    } else {
        return false;
    }
}

void ERst::loadReportStepNumber(const int &number)
{

    if (!hasReportStepNumber(number)) {
        std::string message="Trying to load non existing sequence " + std::to_string(number);
        throw std::invalid_argument(message);
    }

    std::vector<int> arrayIndexList;
    arrayIndexList.reserve(arrIndexRange[number].second-arrIndexRange[number].first+1);

    for (int i=arrIndexRange[number].first; i<arrIndexRange[number].second; i++) {
        arrayIndexList.push_back(i);
    }

    loadData(arrayIndexList);

    reportLoaded[number]=true;
}

const std::vector<std::tuple<std::string, EIOD::eclArrType,int>> ERst::listOfRstArrays(const int &reportStepNumber) {

    std::vector<std::tuple<std::string, EIOD::eclArrType, int>> list;

    for (int i=arrIndexRange[reportStepNumber].first; i<arrIndexRange[reportStepNumber].second; i++) {
        std::tuple<std::string, EIOD::eclArrType, int> t(array_name[i],array_type[i], array_size[i]);
        list.push_back(t);
    }

    return std::move(list);
}

const int ERst::getArrayIndex(const std::string &name,const int &number) const {

    if (!hasReportStepNumber(number)) {
        std::string message="Trying to get vector " + name + " from non existing sequence " + std::to_string(number);
        throw std::invalid_argument(message);
    }

    auto search = reportLoaded.find(number);

    if (!search->second) {
        std::string message="Data not loaded for sequence " + std::to_string(number);
        throw std::runtime_error(message);
    }

    auto range_it = arrIndexRange.find(number);

    std::pair<int,int> indexRange = range_it->second;

    auto it=find(array_name.begin()+indexRange.first,array_name.begin()+indexRange.second,name);

    if (distance(array_name.begin(),it)==indexRange.second) {
        std::string message="Array " + name + " not found in sequence " + std::to_string(number);
        throw std::runtime_error(message);
    }

    return distance(array_name.begin(),it);
}


template<>
const std::vector<int> &ERst::getRst<int>(const std::string &name,const int &reportStepNumber) const {

    int ind = getArrayIndex(name, reportStepNumber);

    if (array_type[ind]!=EIOD::INTE) {
        std::string message="Array " + name + " found in sequence, but called with wrong type";
        throw std::runtime_error(message);
    }

    auto search_array = inte_array.find(ind);

    return search_array->second;

}

template<>
const std::vector<float> &ERst::getRst<float>(const std::string &name,const int &reportStepNumber) const {

    int ind = getArrayIndex(name, reportStepNumber);

    if (array_type[ind]!=EIOD::REAL) {
        std::string message="Array " + name + " found in sequence, but called with wrong type";
        throw std::runtime_error(message);
    }

    auto search_array = real_array.find(ind);

    return search_array->second;
}

template<>
const std::vector<double> &ERst::getRst<double>(const std::string &name,const int &reportStepNumber) const {

    int ind = getArrayIndex(name, reportStepNumber);

    if (array_type[ind]!=EIOD::DOUB) {
        std::string message="Array " + name + " found in sequence, but called with wrong type";
        throw std::runtime_error(message);
    }

    auto search_array = doub_array.find(ind);

    return search_array->second;
}

template<>
const std::vector<bool> &ERst::getRst<bool>(const std::string &name,const int &reportStepNumber) const {

    int ind = getArrayIndex(name, reportStepNumber);

    if (array_type[ind]!=EIOD::LOGI) {
        std::string message="Array " + name + " found in sequence, but called with wrong type";
        throw std::runtime_error(message);
    }

    auto search_array = logi_array.find(ind);

    return search_array->second;
}

template<>
const std::vector<std::string> &ERst::getRst<std::string>(const std::string &name,const int &reportStepNumber) const {

    int ind = getArrayIndex(name, reportStepNumber);

    if (array_type[ind]!=EIOD::CHAR) {
        std::string message="Array " + name + " found in sequence, but called with wrong type";
        throw std::runtime_error(message);
    }

    auto search_array = char_array.find(ind);

    return search_array->second;
}


