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

#include <string>
#include <string.h>
#include <sstream>
#include <iterator>
#include <iomanip>
#include <algorithm>

#include "EclFile.hpp"


// anonymous namespace for EclFile

namespace {
  
bool isEOF(std::fstream *fileH){
    
    /* try to read 4 byte (integer), if passed eof of, return true.
       if not, seek back to current file possition  */
    
    int num;
    long int pos=fileH->tellg();
    fileH->read((char *)&num, sizeof(num));
    
    if (fileH->eof()){
        return true;
    } else {
        fileH->seekg (pos);
        return false;
    }
}

int flipEndianInt(const int &num) {

   unsigned int tmp=__builtin_bswap32(num);
   
   return (int)tmp;
}

float flipEndianFloat(const float &num) {

    float value=num;

    char *floatToConvert = reinterpret_cast<char*>(&value);
    std::reverse(floatToConvert, floatToConvert+4);
    
    return value;
}

double flipEndianDouble(const double &num) {
    double value=num;
    
    char *doubleToConvert = reinterpret_cast<char*>(&value);
    std::reverse(doubleToConvert, doubleToConvert+8);

    return value;
}

std::tuple<const int, const int> block_size_data(EIOD::eclArrType arrType){

    switch(arrType) {
        case EIOD::INTE  : return std::make_tuple(EIOD::sizeOfInte,EIOD::MaxBlockSizeInte);   break;
        case EIOD::REAL  : return std::make_tuple(EIOD::sizeOfReal,EIOD::MaxBlockSizeReal);   break;
        case EIOD::DOUB  : return std::make_tuple(EIOD::sizeOfDoub,EIOD::MaxBlockSizeDoub);   break;
        case EIOD::LOGI  : return std::make_tuple(EIOD::sizeOfLogi,EIOD::MaxBlockSizeLogi);   break;
        case EIOD::CHAR  : return std::make_tuple(EIOD::sizeOfChar,EIOD::MaxBlockSizeChar);   break;
        case EIOD::MESS  : throw std::invalid_argument("Type 'MESS' have not assosiated data") ;   break;
        default          : throw std::invalid_argument("Unknown type"); break;
    }
}

std::string trimr(const std::string &str1) {

    std::string tmpStr=str1;
    
    while (tmpStr.back()==' '){
        tmpStr.pop_back();
    }

    return std::move(tmpStr);
}


void readBinaryHeader(std::fstream &fileH, std::string &arrName, int &size, EIOD::eclArrType &arrType){

    int bhead;
    std::string tmpStrName(8,' ');
    std::string tmpStrType(4,' ');

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    fileH.read((char *)&bhead, sizeof(bhead));
    bhead=flipEndianInt(bhead);
    
    if (bhead!=16){
        std::string message="Error reading binary header. Expected 16 bytes of header data, found " + std::to_string(bhead);
        throw std::runtime_error(message);
    }
    
    fileH.read(&tmpStrName[0], 8);
    
    fileH.read((char *)&size, sizeof(size));
    size=flipEndianInt(size);

    fileH.read(&tmpStrType[0], 4);

    fileH.read((char *)&bhead, sizeof(bhead));
    bhead=flipEndianInt(bhead);
    
    if (bhead!=16){
        std::string message="Error reading binary header. Expected 16 bytes of header data, found " + std::to_string(bhead);
        throw std::runtime_error(message);
    }
    
    arrName=tmpStrName;
    
    if (tmpStrType=="INTE")
        arrType=EIOD::INTE;
    else if (tmpStrType=="REAL")    
        arrType=EIOD::REAL;
    else if (tmpStrType=="DOUB")    
        arrType=EIOD::DOUB;
    else if (tmpStrType=="CHAR")    
        arrType=EIOD::CHAR;
    else if (tmpStrType=="LOGI")    
        arrType=EIOD::LOGI;
    else if (tmpStrType=="MESS")    
        arrType=EIOD::MESS;
    else
        throw std::runtime_error("Error, unknown array type '" + tmpStrType +"'");
    
}


unsigned long int sizeOnDisk(const int num, const EIOD::eclArrType &arrType){
    
    unsigned long int size=0;

    if (arrType==EIOD::MESS){
        if (num>0) {
            std::string message="In routine calcSizeOfArray, type MESS can not have size > 0";
            throw std::invalid_argument(message);
        }
    } else {

        auto sizeData = block_size_data(arrType);

	    int sizeOfElement = std::get<0>(sizeData);
        int maxBlockSize = std::get<1>(sizeData);
        int maxNumberOfElements=maxBlockSize/sizeOfElement;
       
        size=num*sizeOfElement;
        size=size + ((num-1) / maxNumberOfElements)*2*EIOD::sizeOfInte;    // 8 byte (two integers) every 1000 element        
        
        if (num>0){
            size=size+2*EIOD::sizeOfInte;
        }
    }
         
    return size;
}


std::vector<int> readBinaryInteArray(std::fstream &fileH, const int size){

    std::vector<int> arr;
    int rest=size;
    
    auto sizeData = block_size_data(EIOD::INTE);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements=maxBlockSize/sizeOfElement;

    arr.reserve(size);
    
    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }
   
    while (rest > 0) {   
 
        int num,dhead,dtail;

        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=flipEndianInt(dhead);
        
        num=dhead/sizeOfElement;

        if ((num > maxNumberOfElements) || (num < 0)){
            std::string message="Error reading binary inte data, inconsistent header data or incorrect number of elements";
            throw std::runtime_error(message);
        }
       
        for (int i=0;i<num;i++){
            int value;
            fileH.read((char *)&value, sizeOfElement);
            value=flipEndianInt(value);
            arr.push_back(value);
        }
       
        rest=rest-num;
       
        if (((num < maxNumberOfElements) && (rest!=0)) || ((num==maxNumberOfElements) && (rest < 0))) {
            std::string message="Error reading binary integer data, incorrect number of elements";
            throw std::runtime_error(message);
        }
       
        fileH.read((char *)&dtail, sizeof(dtail));
        dtail=flipEndianInt(dtail);
       
        if (dhead!=dtail){ 
            std::string message="Error reading binary real data, tail not matching header.";
            throw std::runtime_error(message);
        }
    } 

    return arr;
}

std::vector<float> readBinaryRealArray(std::fstream &fileH, const int size){

    std::vector<float> arr;
    arr.reserve(size);

    auto sizeData = block_size_data(EIOD::REAL);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements=maxBlockSize/sizeOfElement;

    
    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    int rest=size;

    while (rest > 0) {   
 
        int num,dhead,dtail;

        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=flipEndianInt(dhead);
   
        num=dhead/sizeOfElement;
       
        if ((num > maxNumberOfElements) || (num < 0)){
            std::string message="Error reading binary real data, inconsistent header data or incorrect number of elements";
            throw std::runtime_error(message);
        }
       
        for (int i=0;i<num;i++){
            float value;
            fileH.read((char *)&value, sizeOfElement);
            value=flipEndianFloat(value);
            arr.push_back(value);
        }
   
        rest=rest-num;
       
        if (((num < maxNumberOfElements) && (rest!=0)) || ((num==maxNumberOfElements) && (rest < 0))) {
            std::string message="Error reading binary real data, incorrect number of elements";
            throw std::runtime_error(message);
        }
       
        fileH.read((char *)&dtail, sizeof(dtail));
        dtail=flipEndianInt(dtail);
 
        if (dhead!=dtail){ 
            std::string message="Error reading binary real data, tail not matching header.";
            throw std::runtime_error(message);
        }
    } 
   
    return arr;
}

std::vector<double> readBinaryDoubArray(std::fstream &fileH, const int size){

    std::vector<double> arr;
    arr.reserve(size);

    auto sizeData = block_size_data(EIOD::DOUB);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements=maxBlockSize/sizeOfElement;

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }
   
    int rest=size;

    while (rest > 0) {   
 
        int num,dhead,dtail;

        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=flipEndianInt(dhead);
   
        num=dhead/sizeOfElement;
       
        if ((num > maxNumberOfElements) || (num < 0)){
            std::string message="Error reading binary doub data, inconsistent header data or incorrect number of elements";
            throw std::runtime_error(message);
        }
       
        for (int i=0;i<num;i++){
            double value;
            fileH.read((char *)&value, sizeOfElement);
            value=flipEndianDouble(value);
            arr.push_back(value);
        }
   
        rest=rest-num;
       
        if (((num < maxNumberOfElements) && (rest!=0)) || ((num==maxNumberOfElements) && (rest < 0))) {
            std::string message="Error reading binary doub data, incorrect number of elements";
            throw std::runtime_error(message);
        }

        fileH.read((char *)&dtail, sizeof(dtail));
        dtail=flipEndianInt(dtail);
 
        if (dhead!=dtail){ 
            std::string message="Error reading binary doub data, tail not matching header.";
            throw std::runtime_error(message);
        }
    } 
   
    return arr;
}

std::vector<bool> readBinaryLogiArray(std::fstream &fileH, const int size){

    std::vector<bool> arr;
    arr.reserve(size);

    int rest=size;

    auto sizeData = block_size_data(EIOD::LOGI);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements=maxBlockSize/sizeOfElement;
    
    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }
   
    while (rest > 0) {   
 
        int num,dhead,dtail;

        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=flipEndianInt(dhead);
   
        num=dhead/sizeOfElement;

        if ((num > maxNumberOfElements) || (num < 0)){
            std::string message="This ?? Error reading binary logi data, inconsistent header data or incorrect number of elements";
            throw std::runtime_error(message);
        }

        for (int i=0;i<num;i++){

            bool value;
            unsigned int intVal;

            fileH.read((char *)&intVal, sizeOfElement);
            
            if (intVal==EIOD::true_value){
                value=true;
            } else if (intVal==EIOD::false_value){
                value=false;
            } else {
                std::string message="Error reading log value from element " + std::to_string(i);
                throw std::runtime_error(message);
            }
           
            arr.push_back(value);
        }
   
        rest=rest-num;

        if (((num < maxNumberOfElements) && (rest!=0)) || ((num==maxNumberOfElements) && (rest < 0))) {
            std::string message="Error reading binary logi data, incorrect number of elements";
            throw std::runtime_error(message);
        }
       
        fileH.read((char *)&dtail, sizeof(dtail));
        dtail=flipEndianInt(dtail);
 
        if (dhead!=dtail){ 
            std::string message="Error reading binary logi data, tail not matching header.";
            throw std::runtime_error(message);
        }
    } 
   
    return arr;
}


std::vector<std::string> readBinaryCharArray(std::fstream &fileH, const int size){

    std::vector<std::string> arr;
    arr.reserve(size);
   
    int rest=size;

    auto sizeData = block_size_data(EIOD::CHAR);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements=maxBlockSize/sizeOfElement;

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }
   
    while (rest > 0) {   
 
        int num,dhead,dtail;

        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=flipEndianInt(dhead);
        
        num=dhead/sizeOfElement;

        if ((num > maxNumberOfElements) || (num < 0)){
            std::string message="Error reading binary char data, inconsistent header data or incorrect number of elements";
            throw std::runtime_error(message);
        }

        for (int i=0;i<num;i++){
            std::string str8(8,' ');
            fileH.read(&str8[0], sizeOfElement);
            arr.push_back(trimr(str8));
        }
   
        rest=rest-num;

        if ((num < maxNumberOfElements) && (rest!=0)){
            std::string message="Error reading binary char data, incorrect number of elements";
            throw std::runtime_error(message);
        }
       
        fileH.read((char *)&dtail, sizeof(dtail));
        dtail=flipEndianInt(dtail);
 
        if (dhead!=dtail){ 
            std::string message="Error reading binary char data, tail not matching header.";
            throw std::runtime_error(message);
        }
    } 
   
    return arr;
}

} // anonymous namespace

EclFile::EclFile(const std::string& filename){

    std::fstream fileH;
    
    fileH.open(filename, std::ios::in |  std::ios::binary);

    if (!fileH.good()) {
        std::string message="Could not open file";
        throw std::runtime_error(message);
    }

    fileH.close();

     
}


