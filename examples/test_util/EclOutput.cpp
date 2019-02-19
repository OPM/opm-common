/*
   Copyright 2019 Statoil ASA.

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

#include "EclOutput.hpp"

#include <algorithm>
#include <iterator>
#include <iomanip>
#include <stdexcept>
#include <typeinfo>
#include <sstream> 
#include <stdio.h>

const int EclOutput::flipEndianInt(const int &num) const {

    unsigned int tmp=__builtin_bswap32(num);

    return std::move((int)tmp);
}

const float EclOutput::flipEndianFloat(const float &num) const {

    float value=num;

    char *floatToConvert = reinterpret_cast<char*>(&value);
    std::reverse(floatToConvert, floatToConvert+4);

    return std::move(value);
}

const double EclOutput::flipEndianDouble(const double &num) const {
    double value=num;

    char *doubleToConvert = reinterpret_cast<char*>(&value);
    std::reverse(doubleToConvert, doubleToConvert+8);

    return std::move(value);
}

std::tuple<const int, const int> EclOutput::block_size_data_binary(EIOD::eclArrType arrType){

    switch(arrType) {
	case EIOD::INTE  : return std::make_tuple(EIOD::sizeOfInte,EIOD::MaxBlockSizeInte);   break;
	case EIOD::REAL  : return std::make_tuple(EIOD::sizeOfReal,EIOD::MaxBlockSizeReal);   break;
	case EIOD::DOUB  : return std::make_tuple(EIOD::sizeOfDoub,EIOD::MaxBlockSizeDoub);   break;
	case EIOD::LOGI  : return std::make_tuple(EIOD::sizeOfLogi,EIOD::MaxBlockSizeLogi);   break;
	case EIOD::CHAR  : return std::make_tuple(EIOD::sizeOfChar,EIOD::MaxBlockSizeChar);   break;
	case EIOD::MESS  : throw std::invalid_argument("Type 'MESS' have not assosiated data") ;   break;
    }
}

std::tuple<const int, const int, const int> EclOutput::block_size_data_formatted(EIOD::eclArrType arrType){

    switch(arrType) {
	case EIOD::INTE  : return std::make_tuple(EIOD::MaxNumBlockInte, EIOD::numColumnsInte, EIOD::columnWidthInte);   break;
	case EIOD::REAL  : return std::make_tuple(EIOD::MaxNumBlockReal,EIOD::numColumnsReal, EIOD::columnWidthReal);   break;
	case EIOD::DOUB  : return std::make_tuple(EIOD::MaxNumBlockDoub,EIOD::numColumnsDoub, EIOD::columnWidthDoub);   break;
	case EIOD::LOGI  : return std::make_tuple(EIOD::MaxNumBlockLogi,EIOD::numColumnsLogi, EIOD::columnWidthLogi);   break;
	case EIOD::CHAR  : return std::make_tuple(EIOD::MaxNumBlockChar,EIOD::numColumnsChar, EIOD::columnWidthChar);   break;
	case EIOD::MESS  : throw std::invalid_argument("Type 'MESS' have not assosiated data") ;   break;
    }
}


const std::string trimr(const std::string &str1) {
        
    if (str1=="        "){
        return "";
    } else {
        int p=str1.find_last_not_of(" ");
	return str1.substr(0,p+1);
    }
}


void EclOutput::writeBinaryHeader(const std::string &arrName,const int &size,const EIOD::eclArrType &arrType)  {
    
    std::string name=arrName+std::string(8-arrName.size(),' ');
    
    if (!ofileH->is_open()){
        std::string message="fstream fileH not open for writing";
        throw std::runtime_error(message);
    }

    int flippedSize=flipEndianInt(size);
    int bhead=flipEndianInt(16);
    
    ofileH->write((char *)&bhead, sizeof(bhead));
    
    ofileH->write(name.c_str(), 8);
    ofileH->write((char *)&flippedSize, sizeof(flippedSize));
    
    switch(arrType) {
       case EIOD::INTE  : ofileH->write("INTE", 4);;   break;
       case EIOD::REAL  : ofileH->write("REAL", 4);;   break;
       case EIOD::DOUB  : ofileH->write("DOUB", 4);;   break;
       case EIOD::LOGI  : ofileH->write("LOGI", 4);;   break;
       case EIOD::CHAR  : ofileH->write("CHAR", 4);;   break;
       case EIOD::MESS  : ofileH->write("MESS", 4);;   break;
    }
    
    ofileH->write((char *)&bhead, sizeof(bhead));
}

/*
void EclOutput::writeBinaryHeader(const std::string &arrName,const int &size,const std::string arrType)  {
    
    std::string name=arrName+std::string(8-arrName.size(),' ');
    
    if (!ofileH->is_open()){
        std::string message="fstream fileH not open for writing";
        throw std::runtime_error(message);
    }

    if (arrType.size()!=4){
        std::string message="Length of Array type sting should be 4";
        throw std::runtime_error(message);
    }

    int flippedSize=flipEndianInt(size);
    int bhead=flipEndianInt(16);
    
    ofileH->write((char *)&bhead, sizeof(bhead));
    
    ofileH->write(name.c_str(), 8);
    ofileH->write((char *)&flippedSize, sizeof(flippedSize));

    ofileH->write(arrType.c_str(), 4);
    
    ofileH->write((char *)&bhead, sizeof(bhead));
}
*/

template <typename T>
void EclOutput::writeBinaryArray(const std::vector<T> &data) {

    int rest,num,rval;
    int dhead;
    float value_f;
    double value_d;
    int intVal;

    int n=0;
    int size=data.size();

    EIOD::eclArrType arrType;

    if (typeid(std::vector<T>)==typeid(std::vector<int>)){
        arrType=EIOD::INTE;
    } else if (typeid(std::vector<T>) == typeid(std::vector<float>)){
        arrType=EIOD::REAL;
    } else if (typeid(std::vector<T>)== typeid(std::vector<double>)){
        arrType=EIOD::DOUB; 
    } else if (typeid(std::vector<T>)== typeid(std::vector<bool>)){
        arrType=EIOD::LOGI;
    }
    
    auto sizeData = block_size_data_binary(arrType);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements=maxBlockSize/sizeOfElement;
    
    if (!ofileH->is_open()){
        std::string message="fstream fileH not open for writing";
        throw std::runtime_error(message);
    }
    
    rest=size*sizeOfElement;
    
    while (rest>0) {
        
        if (rest>maxBlockSize){
            rest=rest-maxBlockSize;
            num=maxNumberOfElements;
        } else {
            num=rest/sizeOfElement;
            rest=0;
        }
        
        dhead=flipEndianInt(num*sizeOfElement);
        
        ofileH->write((char *)&dhead, sizeof(dhead));
        
        for (int i=0;i<num;i++){

	        if (arrType==EIOD::INTE){
                rval=flipEndianInt(data[n]);
                ofileH->write((char *)&rval, sizeof(rval));
            } else if (arrType==EIOD::REAL){ 
                value_f=flipEndianFloat(data[n]);
                ofileH->write((char *)&value_f, sizeof(value_f));
            } else if (arrType==EIOD::DOUB){ 
                value_d=flipEndianDouble(data[n]);
                ofileH->write((char *)&value_d, sizeof(value_d));
            } else if (arrType==EIOD::LOGI){ 
                intVal = data[n]==true ? EIOD::true_value : EIOD::false_value;
                ofileH->write((char *)&intVal, sizeOfElement);
            } else {
	            std::cout << "type not supported in write binaryarray"<< std::endl;
		        exit(1);
	        }

	    n++;    
        }

        ofileH->write((char *)&dhead, sizeof(dhead));
    };
    

}

template void EclOutput::writeBinaryArray<int>(const std::vector<int> &data);
template void EclOutput::writeBinaryArray<float>(const std::vector<float> &data);
template void EclOutput::writeBinaryArray<double>(const std::vector<double> &data);
template void EclOutput::writeBinaryArray<bool>(const std::vector<bool> &data);

/*

template <typename T>
void EclOutput::writeBinaryArray2(const std::vector<T> &data,const EIOD::eclArrType &arrType) {
    
    int rest,num,rval;
    int dhead;
    float value_f;
    double value_d;
    int intVal;

    int n=0;
    int size=data.size();

    auto sizeData = block_size_data_binary(arrType);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements=maxBlockSize/sizeOfElement;
    
    if (!ofileH->is_open()){
        std::string message="fstream fileH not open for writing";
        throw std::runtime_error(message);
    }
    
    rest=size*sizeOfElement;
    
    while (rest>0) {
        
        if (rest>maxBlockSize){
            rest=rest-maxBlockSize;
            num=maxNumberOfElements;
        } else {
            num=rest/sizeOfElement;
            rest=0;
        }
        
        dhead=flipEndianInt(num*sizeOfElement);
        
        ofileH->write((char *)&dhead, sizeof(dhead));
        
        for (int i=0;i<num;i++){

	        if (arrType==EIOD::INTE){
	            rval=flipEndianInt(data[n]);
                ofileH->write((char *)&rval, sizeof(rval));
            } else if (arrType==EIOD::REAL){ 
                value_f=flipEndianFloat(data[n]);
                ofileH->write((char *)&value_f, sizeof(value_f));
            } else if (arrType==EIOD::DOUB){ 
                value_d=flipEndianDouble(data[n]);
                ofileH->write((char *)&value_d, sizeof(value_d));
            } else if (arrType==EIOD::LOGI){ 
                intVal = data[n]==true ? EIOD::true_value : EIOD::false_value;
                ofileH->write((char *)&intVal, sizeOfElement);
            } else {
	            std::cout << "type not supported in write binaryarray"<< std::endl;
		        exit(1);
	       }

	       n++;    
        }

        ofileH->write((char *)&dhead, sizeof(dhead));
    };

}

template void EclOutput::writeBinaryArray2<int>(const std::vector<int> &data,const EIOD::eclArrType &arrType);
template void EclOutput::writeBinaryArray2<float>(const std::vector<float> &data,const EIOD::eclArrType &arrType);
template void EclOutput::writeBinaryArray2<double>(const std::vector<double> &data,const EIOD::eclArrType &arrType);
template void EclOutput::writeBinaryArray2<bool>(const std::vector<bool> &data,const EIOD::eclArrType &arrType);
*/

void EclOutput::writeBinaryCharArray(const std::vector<std::string> &data){
    
    int num,dhead;

    int n=0;
    int size=data.size();

    auto sizeData = block_size_data_binary(EIOD::CHAR);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements=maxBlockSize/sizeOfElement;

    int rest=size*sizeOfElement;

    
    if (!ofileH->is_open()){
        std::string message="fstream fileH not open for writing";
        throw std::runtime_error(message);
    }
    
    while (rest>0) {
        if (rest>maxBlockSize){
            rest=rest-maxBlockSize;
            num=maxNumberOfElements;
        } else {
            num=rest/sizeOfElement;
            rest=0;
        }
        
        dhead=flipEndianInt(num*sizeOfElement);

        ofileH->write((char *)&dhead, sizeof(dhead));
        
        for (int i=0;i<num;i++){
            std::string tmpStr=data[n]+std::string(8-data[n].size(),' ');
            ofileH->write(tmpStr.c_str(), sizeOfElement);
            n++;    
        }

        ofileH->write((char *)&dhead, sizeof(dhead));
    };
}

void EclOutput::writeFormattedHeader(const std::string &arrName,const int &size,const EIOD::eclArrType &arrType){
  
    if (!ofileH->is_open()){
        std::string message="ofstream fileH not open for writing";
        throw std::runtime_error(message);
    }

    std::string name=arrName+std::string(8-arrName.size(),' '); 
    
    *ofileH << " '" << name << "' " << std::setw(11) << size;

    switch(arrType) {
        case EIOD::INTE  : *ofileH << " 'INTE'" <<  std::endl;   break;
        case EIOD::REAL  : *ofileH << " 'REAL'" <<  std::endl;   break;
        case EIOD::DOUB  : *ofileH << " 'DOUB'" <<  std::endl;   break;
        case EIOD::LOGI  : *ofileH << " 'LOGI'" <<  std::endl;   break;
        case EIOD::CHAR  : *ofileH << " 'CHAR'" <<  std::endl;   break;
        case EIOD::MESS  : *ofileH << " 'MESS'" <<  std::endl;   break;
    }

}

const std::string EclOutput::make_real_string(const float &value) const {
    
 //  these two are expensive from 0.1 sec, to  1.73 sec  
 //    std::stringstream stream;
 //    stream << std::scientific << std::setprecision(8) << value;

    char buffer [15];
    int n=sprintf (buffer, "%10.7E", value);

    if (value==0.0){
        std::string tmpstr("0.00000000E+00");

        return std::move(tmpstr);
    } else {    
        std::string tmpstr(buffer);
    
        int exp=  value < 0.0 ? std::stoi(tmpstr.substr(11,3)) :  std::stoi(tmpstr.substr(10,3));
    
        if (value <0.0){
            tmpstr="-0."+tmpstr.substr(1,1)+tmpstr.substr(3,7)+"E";
        } else {
            tmpstr="0."+tmpstr.substr(0,1)+tmpstr.substr(2,7)+"E";
        }

        n=sprintf (buffer, "%+03i", exp+1);
        tmpstr=tmpstr+buffer;

        return std::move(tmpstr);
    }
    
}


const std::string EclOutput::make_doub_string(const double &value) const {

    char buffer [20];
    int n=sprintf (buffer, "%19.13E", value);
    
    if (value==0.0){
        std::string tmpstr("0.00000000000000D+00");

        return std::move(tmpstr);
        
    } else {         
    
        std::string tmpstr(buffer);
    
        int exp=  value < 0.0 ? std::stoi(tmpstr.substr(17,4)) :  std::stoi(tmpstr.substr(16,4));
    
        if (value <0.0){
            if (abs(exp)<100){
                tmpstr="-0."+tmpstr.substr(1,1)+tmpstr.substr(3,13)+"D";
            } else {
                tmpstr="-0."+tmpstr.substr(1,1)+tmpstr.substr(3,13);
            }    
        } else {
            if (abs(exp)<100){
                tmpstr="0."+tmpstr.substr(0,1)+tmpstr.substr(2,13)+"D";
            } else {
                tmpstr="0."+tmpstr.substr(0,1)+tmpstr.substr(2,13);
            }
        }

        n=sprintf (buffer, "%+03i", exp+1);
        tmpstr=tmpstr+buffer;

        return std::move(tmpstr);
    }
    
}


template <typename T>
void EclOutput::writeFormattedArray(const std::vector<T> &data){

    if (!ofileH->is_open()){
        std::string message="ofstream fileH not open for writing";
        throw std::runtime_error(message);
    }

    int size=data.size();
    int n=0;

    EIOD::eclArrType arrType;
    
    if (typeid(std::vector<T>)==typeid(std::vector<int>)){
        arrType=EIOD::INTE;
    } else if (typeid(std::vector<T>) == typeid(std::vector<float>)){
        arrType=EIOD::REAL;
    } else if (typeid(std::vector<T>)== typeid(std::vector<double>)){
        arrType=EIOD::DOUB; 
    } else if (typeid(std::vector<T>)== typeid(std::vector<bool>)){
        arrType=EIOD::LOGI;
    }
    
    auto sizeData = block_size_data_formatted(arrType);

    int maxBlockSize=std::get<0>(sizeData);
    int nColumns=std::get<1>(sizeData);
    int columnWidth=std::get<2>(sizeData);
     
     
    for (int i=0;i<size;i++){
        n++;        
        
        switch(arrType) {
           case EIOD::INTE  : *ofileH << std::setw(columnWidth) << data[i];   break;
           case EIOD::REAL  : *ofileH << std::setw(columnWidth) << make_real_string(data[i]);   break;
           case EIOD::DOUB  : *ofileH << std::setw(columnWidth) << make_doub_string(data[i]);   break;
           case EIOD::LOGI  : if (data[i]==true) { *ofileH << "  T"; } else {  *ofileH << "  F";}    break;
        }
        
        if (((n % nColumns)==0) || ((n % maxBlockSize)==0)) {
           *ofileH << std::endl;
        }

        if ((n % maxBlockSize)==0){
           n=0;     
        }
    }

    if (((n % nColumns)!=0) && ((n % maxBlockSize)!=0)) {
        *ofileH << std::endl;
    } 

}

template void EclOutput::writeFormattedArray<int>(const std::vector<int> &data);
template void EclOutput::writeFormattedArray<float>(const std::vector<float> &data);
template void EclOutput::writeFormattedArray<double>(const std::vector<double> &data);
template void EclOutput::writeFormattedArray<bool>(const std::vector<bool> &data);


void EclOutput::writeFormattedCharArray(const std::vector<std::string> &data){

    if (!ofileH->is_open()){
        std::string message="ofstream fileH not open for writing";
        throw std::runtime_error(message);
    }

    auto sizeData = block_size_data_formatted(EIOD::CHAR);

    int nColumns=std::get<1>(sizeData);
    
    int size=data.size();

    for (int i=0;i<size;i++){

        std::string str1(8,' ');
        str1=data[i]+std::string(8-data[i].size(),' ');
	
        *ofileH << " '" << str1 << "'";
        
        if (((i+1) % nColumns)==0){
            *ofileH  << std::endl;
        }
    }

    if ((size % nColumns)!=0){
        *ofileH  << std::endl;
    }
}

EclOutput::EclOutput(std::ofstream &inputFileH){

    if (!inputFileH.is_open()){
        std::string message="ofstream inputFileH not open for writing";
        throw std::runtime_error(message);
    }
       
    ofileH=&inputFileH;
    
}


