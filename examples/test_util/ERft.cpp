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

#include "ERft.hpp"

#include <string>
#include <string.h>
#include <sstream>
#include <iterator>
#include <iomanip>
#include <algorithm>


ERft::ERft(const std::string &filename) : EclFile(filename)
{

    loadData();
    std::vector<int> first;
    std::vector<int> second;

    std::vector<std::string> wellName;
    std::vector<std::tuple<int, int, int>> dates;


    const std::vector<std::tuple<std::string, EIOD::eclArrType, int>> listOfArrays=getList();

    for (unsigned int i=0;i<listOfArrays.size();i++){
        std::string name=std::get<0>(listOfArrays[i]);

	if (name=="TIME"){
	   first.push_back(i);
	   std::vector<float> vect1= get<float>(i);
	   timeList.push_back(vect1[0]);
        }

        if (name=="DATE"){
	    std::vector<int> vect1= get<int>(i);
            std::tuple<int, int, int> date(vect1[2],vect1[1],vect1[0]);
            dateList.insert(date);
	    dates.push_back(date);
        }

        if (name=="WELLETC"){
	    std::vector<std::string> vect1= get<std::string>(i);
            wellList.insert(vect1[1]);
	    wellName.push_back(vect1[1]);
        }
    }

    for (unsigned int i=0;i<first.size();i++){

        std::pair<int,int> range;
        range.first=first[i];
	
	if (i==first.size()-1){
	   range.second=listOfArrays.size();
	} else {
	   range.second=first[i+1];
	}

        arrIndexRange[i]=range;
    }

    numReports=first.size();

    for (unsigned int i=0;i<wellName.size();i++){
        std::pair<std::string,std::tuple<int,int,int>> wellDatePair(wellName[i],dates[i]);
        reportIndex[wellDatePair]=i;

        rftReportList.push_back(wellDatePair);
    }
};


bool ERft::hasRft(const std::string &wellName, const std::tuple<int,int,int> &date) const {

    std::pair<std::string,std::tuple<int,int,int>> wellDatePair(wellName,date);

    auto rIndIt = reportIndex.find(wellDatePair);

    if (rIndIt==reportIndex.end()){
        return false;
    } else {
        return true;
    }
}

bool ERft::hasRft(const std::string &wellName, const int &year, const int &month, const int &day) const {

    std::tuple<int,int,int> date(year, month, day);
    std::pair<std::string,std::tuple<int,int,int>> wellDatePair(wellName,date);

    auto rIndIt = reportIndex.find(wellDatePair);

    if (rIndIt==reportIndex.end()){
        return false;
    } else {
        return true;
    }
}

int ERft::getReportIndex(const std::string &wellName, const std::tuple<int,int,int> &date) const {

    std::pair<std::string,std::tuple<int,int,int>> wellDatePair(wellName,date);

    auto rIndIt = reportIndex.find(wellDatePair);

    if (rIndIt==reportIndex.end()){

        int y=std::get<0>(date);
        int m=std::get<1>(date);
        int d=std::get<2>(date);

	std::string dateStr=std::to_string(y) + "/" + std::to_string(m) + "/" + std::to_string(d);
        std::string message="RFT data not found for well  " + wellName + " at date: " + dateStr;
        throw std::invalid_argument(message);
    };

    return rIndIt->second;
}

bool ERft::hasArray(const std::string arrayName, const std::string &wellName, const std::tuple<int,int,int> &date) const {

    int reportInd=getReportIndex(wellName, date);

    auto searchInd = arrIndexRange.find(reportInd);

    int fromInd =searchInd->second.first;
    int toInd = searchInd->second.second;

    auto it=std::find(array_name.begin()+fromInd,array_name.begin()+toInd,arrayName);

    if (it== array_name.begin()+toInd){
        return false;
    } else {
        return true;
    }
}


int ERft::getArrayIndex(const std::string &name, const std::string &wellName, const std::tuple<int,int,int> &date) const {

    int rInd= getReportIndex(wellName, date);

    auto searchInd = arrIndexRange.find(rInd);

    int fromInd =searchInd->second.first;
    int toInd = searchInd->second.second;

    auto it=std::find(array_name.begin()+fromInd,array_name.begin()+toInd,name);

    if (distance(array_name.begin(),it)==toInd) {

        int y=std::get<0>(date);
        int m=std::get<1>(date);
        int d=std::get<2>(date);

	std::string dateStr=std::to_string(y) + "/" + std::to_string(m) + "/" + std::to_string(d);
        std::string message="Array " + name + " not found for RFT, well: " + wellName + " date: " + dateStr;
        throw std::invalid_argument(message);
    }

    return std::distance(array_name.begin(),it);
}


template<>
const std::vector<float> &ERft::getRft<float>(const std::string &name, const std::string &wellName, const std::tuple<int,int,int> &date) const {

    int arrInd= getArrayIndex(name, wellName, date);

    if (array_type[arrInd]!=EIOD::REAL){
        std::string message="Array " + name + " found in RFT file for selected date and well, but called with wrong type";
        throw std::runtime_error(message);
    }

    auto search_array = real_array.find(arrInd);

    return search_array->second;
}

template<>
const std::vector<double> &ERft::getRft<double>(const std::string &name, const std::string &wellName, const std::tuple<int,int,int> &date) const {

    int arrInd= getArrayIndex(name, wellName, date);

    if (array_type[arrInd]!=EIOD::DOUB){
        std::string message="Array " + name + " found in RFT file for selected date and well, but called with wrong type";
        throw std::runtime_error(message);
    }

    auto search_array = doub_array.find(arrInd);

    return search_array->second;
}

template<>
const std::vector<int> &ERft::getRft<int>(const std::string &name, const std::string &wellName, const std::tuple<int,int,int> &date) const {

    int arrInd= getArrayIndex(name, wellName, date);

    if (array_type[arrInd]!=EIOD::INTE){
        std::string message="Array " + name + " found in RFT file for selected date and well, but called with wrong type";
        throw std::runtime_error(message);
    }

    auto search_array = inte_array.find(arrInd);

    return search_array->second;
}


template<>
const std::vector<bool> &ERft::getRft<bool>(const std::string &name, const std::string &wellName, const std::tuple<int,int,int> &date) const {

    int arrInd= getArrayIndex(name, wellName, date);

    if (array_type[arrInd]!=EIOD::LOGI){
        std::string message="Array " + name + " found in RFT file for selected date and well, but called with wrong type";
        throw std::runtime_error(message);
    }

    auto search_array = logi_array.find(arrInd);

    return search_array->second;
}

template<>
const std::vector<std::string> &ERft::getRft<std::string>(const std::string &name, const std::string &wellName, const std::tuple<int,int,int> &date) const {

    int arrInd= getArrayIndex(name, wellName, date);

    if (array_type[arrInd]!=EIOD::CHAR){
        std::string message="Array " + name + " found in RFT file for selected date and well, but called with wrong type";
        throw std::runtime_error(message);
    }

    auto search_array = char_array.find(arrInd);

    return search_array->second;
}


template<>
const std::vector<int> &ERft::getRft<int>(const std::string &name, const std::string &wellName,  const int &year, const int &month, const int &day) const {

    std::tuple<int,int,int> date(year, month, day);
    return getRft<int>(name, wellName, date);
}

template<>
const std::vector<float> &ERft::getRft<float>(const std::string &name, const std::string &wellName,  const int &year, const int &month, const int &day) const {

    std::tuple<int,int,int> date(year, month, day);
    return getRft<float>(name, wellName, date);
}

template<>
const std::vector<double> &ERft::getRft<double>(const std::string &name, const std::string &wellName,  const int &year, const int &month, const int &day) const {

    std::tuple<int,int,int> date(year, month, day);
    return getRft<double>(name, wellName, date);
}

template<>
const std::vector<std::string> &ERft::getRft<std::string>(const std::string &name, const std::string &wellName,  const int &year, const int &month, const int &day) const {

    std::tuple<int,int,int> date(year, month, day);
    return getRft<std::string>(name, wellName, date);
}

template<>
const std::vector<bool> &ERft::getRft<bool>(const std::string &name, const std::string &wellName,  const int &year, const int &month, const int &day) const {

    std::tuple<int,int,int> date(year, month, day);
    return getRft<bool>(name, wellName, date);
}


const std::vector<std::tuple<std::string, EIOD::eclArrType, int>> ERft::listOfRftArrays(const std::string &wellName, const std::tuple<int,int,int> &date) const {

    std::vector<std::tuple<std::string, EIOD::eclArrType, int>> list;

    int rInd= getReportIndex(wellName, date);

    auto searchInd = arrIndexRange.find(rInd);

    for (int i=searchInd->second.first; i<searchInd->second.second; i++) {
        std::tuple<std::string, EIOD::eclArrType, int> t(array_name[i],array_type[i], array_size[i]);
        list.push_back(t);
    }

    return  std::move(list);
}

const std::vector<std::tuple<std::string, EIOD::eclArrType, int>> ERft::listOfRftArrays(const std::string &wellName, const int &year, const int &month, const int &day) const {

    std::tuple<int,int,int> date(year, month, day);
    std::vector<std::tuple<std::string, EIOD::eclArrType, int>> list;

    int rInd= getReportIndex(wellName, date);

    auto searchInd = arrIndexRange.find(rInd);

    for (int i=searchInd->second.first; i<searchInd->second.second; i++) {
        std::tuple<std::string, EIOD::eclArrType, int> t(array_name[i],array_type[i], array_size[i]);
        list.push_back(t);
    }

    return  std::move(list);
}


const std::vector<std::string> ERft::listOfWells() const {

  std::vector<std::string> resVect;

  for (auto it = wellList.begin(); it != wellList.end(); it++) {
      resVect.push_back(*it);
  }

  return std::move(resVect);
}

const std::vector<std::tuple<int, int, int>> ERft::listOfdates() const {

  std::vector<std::tuple<int, int, int>> resVect;

  for (auto it = dateList.begin(); it != dateList.end(); it++) {
      resVect.push_back(*it);
  }

  return std::move(resVect);
}

