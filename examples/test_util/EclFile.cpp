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


bool isFormatted(std::string filename) {

    int p=filename.find_last_of(".");
    int l=filename.length();

    std::string rootN=filename.substr(0,p);
    std::string extension=filename.substr(p,l-p);

    if (extension.substr(1,1)=="F") {
        return true;
    } else {
        return false;
    }
}

bool isEOF(std::fstream *fileH) {

    int num;
    long int pos=fileH->tellg();
    fileH->read((char *)&num, sizeof(num));

    if (fileH->eof()) {
        return true;
    } else {
        fileH->seekg (pos);
        return false;
    }
}

const int flipEndianInt(const int &num) {

    unsigned int tmp=__builtin_bswap32(num);

    return std::move((int)tmp);
}

const float flipEndianFloat(const float &num) {

    float value=num;

    char *floatToConvert = reinterpret_cast<char*>(&value);
    std::reverse(floatToConvert, floatToConvert+4);

    return std::move(value);
}

const double flipEndianDouble(const double &num) {

    double value=num;

    char *doubleToConvert = reinterpret_cast<char*>(&value);
    std::reverse(doubleToConvert, doubleToConvert+8);

    return std::move(value);
}

std::tuple<const int, const int> block_size_data_binary(EIOD::eclArrType arrType) {

    switch(arrType) {
    case EIOD::INTE  :
        return std::make_tuple(EIOD::sizeOfInte,EIOD::MaxBlockSizeInte);
        break;
    case EIOD::REAL  :
        return std::make_tuple(EIOD::sizeOfReal,EIOD::MaxBlockSizeReal);
        break;
    case EIOD::DOUB  :
        return std::make_tuple(EIOD::sizeOfDoub,EIOD::MaxBlockSizeDoub);
        break;
    case EIOD::LOGI  :
        return std::make_tuple(EIOD::sizeOfLogi,EIOD::MaxBlockSizeLogi);
        break;
    case EIOD::CHAR  :
        return std::make_tuple(EIOD::sizeOfChar,EIOD::MaxBlockSizeChar);
        break;
    case EIOD::MESS  :
        throw std::invalid_argument("Type 'MESS' have not assosiated data") ;
        break;
    }
}

std::tuple<const int, const int, const int> block_size_data_formatted(EIOD::eclArrType arrType) {

    switch(arrType) {
    case EIOD::INTE  :
        return std::make_tuple(EIOD::MaxNumBlockInte, EIOD::numColumnsInte, EIOD::columnWidthInte);
        break;
    case EIOD::REAL  :
        return std::make_tuple(EIOD::MaxNumBlockReal,EIOD::numColumnsReal, EIOD::columnWidthReal);
        break;
    case EIOD::DOUB  :
        return std::make_tuple(EIOD::MaxNumBlockDoub,EIOD::numColumnsDoub, EIOD::columnWidthDoub);
        break;
    case EIOD::LOGI  :
        return std::make_tuple(EIOD::MaxNumBlockLogi,EIOD::numColumnsLogi, EIOD::columnWidthLogi);
        break;
    case EIOD::CHAR  :
        return std::make_tuple(EIOD::MaxNumBlockChar,EIOD::numColumnsChar, EIOD::columnWidthChar);
        break;
    case EIOD::MESS  :
        throw std::invalid_argument("Type 'MESS' have not assosiated data") ;
        break;
    }
}


std::string trimr(const std::string &str1) {

    if (str1=="        ") {
        return "";
    } else {
        int p=str1.find_last_not_of(" ");

        return str1.substr(0,p+1);
    }
}


void readBinaryHeader(std::fstream &fileH, std::string &arrName, int &size, EIOD::eclArrType &arrType) {

    int bhead;
    std::string tmpStrName(8,' ');
    std::string tmpStrType(4,' ');

    if (!fileH.is_open()) {
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    fileH.read((char *)&bhead, sizeof(bhead));
    bhead=flipEndianInt(bhead);

    if (bhead!=16) {
        std::string message="Error reading binary header. Expected 16 bytes of header data, found " + std::to_string(bhead);
        throw std::runtime_error(message);
    }

    fileH.read(&tmpStrName[0], 8);

    fileH.read((char *)&size, sizeof(size));
    size=flipEndianInt(size);

    fileH.read(&tmpStrType[0], 4);

    fileH.read((char *)&bhead, sizeof(bhead));
    bhead=flipEndianInt(bhead);

    if (bhead!=16) {
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


unsigned long int sizeOnDiskBinary(const int num, const EIOD::eclArrType &arrType) {

    unsigned long int size=0;

    if (arrType==EIOD::MESS) {
        if (num>0) {
            std::string message="In routine calcSizeOfArray, type MESS can not have size > 0";
            throw std::invalid_argument(message);
        }
    } else {

        auto sizeData = block_size_data_binary(arrType);

        int sizeOfElement = std::get<0>(sizeData);
        int maxBlockSize = std::get<1>(sizeData);
        int maxNumberOfElements=maxBlockSize/sizeOfElement;

        size=num*sizeOfElement;
        size=size + ((num-1) / maxNumberOfElements)*2*EIOD::sizeOfInte;    // 8 byte (two integers) every 1000 element

        if (num>0) {
            size=size+2*EIOD::sizeOfInte;
        }
    }

    return size;
}

unsigned long int sizeOnDiskFormatted(const int num, const EIOD::eclArrType &arrType) {

    unsigned long int size=0;

    if (arrType==EIOD::MESS) {
        if (num>0) {
            std::string message="In routine calcSizeOfArray, type MESS can not have size > 0";
            throw std::invalid_argument(message);
        }
    } else {

        auto sizeData = block_size_data_formatted(arrType);

        int maxBlockSize=std::get<0>(sizeData);
        int nColumns=std::get<1>(sizeData);
        int columnWidth=std::get<2>(sizeData);

        int nBlocks=num /maxBlockSize;
        int sizeOfLastBlock = num %  maxBlockSize;

        size=0;

        if (nBlocks>0) {
            int nLinesBlock=maxBlockSize/nColumns;
            int rest=maxBlockSize % nColumns;

            if (rest > 0) {
                nLinesBlock++;
            }

            long int blockSize=maxBlockSize*columnWidth+nLinesBlock;
            size=nBlocks*blockSize;
        }

        int nLines=sizeOfLastBlock/nColumns;
        int rest=sizeOfLastBlock % nColumns;

        size= size+sizeOfLastBlock*columnWidth+nLines;

        if (rest > 0) {
            size++;
        }
    }

    return size;
}


std::vector<int> readBinaryInteArray(std::fstream &fileH, const int size) {

    std::vector<int> arr;
    int rest=size;

    auto sizeData = block_size_data_binary(EIOD::INTE);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements=maxBlockSize/sizeOfElement;

    arr.reserve(size);

    if (!fileH.is_open()) {
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    while (rest > 0) {

        int num,dhead,dtail;

        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=flipEndianInt(dhead);

        num=dhead/sizeOfElement;

        if ((num > maxNumberOfElements) || (num < 0)) {
            std::string message="Error reading binary inte data, inconsistent header data or incorrect number of elements";
            throw std::runtime_error(message);
        }

        for (int i=0; i<num; i++) {
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

        if (dhead!=dtail) {
            std::string message="Error reading binary real data, tail not matching header.";
            throw std::runtime_error(message);
        }
    }

    return arr;
}

std::vector<float> readBinaryRealArray(std::fstream &fileH, const int size) {

    std::vector<float> arr;
    arr.reserve(size);

    auto sizeData = block_size_data_binary(EIOD::REAL);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements=maxBlockSize/sizeOfElement;


    if (!fileH.is_open()) {
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    int rest=size;

    while (rest > 0) {

        int num,dhead,dtail;

        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=flipEndianInt(dhead);

        num=dhead/sizeOfElement;

        if ((num > maxNumberOfElements) || (num < 0)) {
            std::string message="Error reading binary real data, inconsistent header data or incorrect number of elements";
            throw std::runtime_error(message);
        }

        for (int i=0; i<num; i++) {
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

        if (dhead!=dtail) {
            std::string message="Error reading binary real data, tail not matching header.";
            throw std::runtime_error(message);
        }
    }

    return arr;
}

std::vector<double> readBinaryDoubArray(std::fstream &fileH, const int size) {

    std::vector<double> arr;
    arr.reserve(size);

    auto sizeData = block_size_data_binary(EIOD::DOUB);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements=maxBlockSize/sizeOfElement;

    if (!fileH.is_open()) {
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    int rest=size;

    while (rest > 0) {

        int num,dhead,dtail;

        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=flipEndianInt(dhead);

        num=dhead/sizeOfElement;

        if ((num > maxNumberOfElements) || (num < 0)) {
            std::string message="Error reading binary doub data, inconsistent header data or incorrect number of elements";
            throw std::runtime_error(message);
        }

        for (int i=0; i<num; i++) {
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

        if (dhead!=dtail) {
            std::string message="Error reading binary doub data, tail not matching header.";
            throw std::runtime_error(message);
        }
    }

    return arr;
}

std::vector<bool> readBinaryLogiArray(std::fstream &fileH, const int size) {

    std::vector<bool> arr;
    arr.reserve(size);

    int rest=size;

    auto sizeData = block_size_data_binary(EIOD::LOGI);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements=maxBlockSize/sizeOfElement;

    if (!fileH.is_open()) {
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    while (rest > 0) {

        int num,dhead,dtail;

        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=flipEndianInt(dhead);

        num=dhead/sizeOfElement;

        if ((num > maxNumberOfElements) || (num < 0)) {
            std::string message="This ?? Error reading binary logi data, inconsistent header data or incorrect number of elements";
            throw std::runtime_error(message);
        }

        for (int i=0; i<num; i++) {

            bool value;
            unsigned int intVal;

            fileH.read((char *)&intVal, sizeOfElement);

            if (intVal==EIOD::true_value) {
                value=true;
            } else if (intVal==EIOD::false_value) {
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

        if (dhead!=dtail) {
            std::string message="Error reading binary logi data, tail not matching header.";
            throw std::runtime_error(message);
        }
    }

    return arr;
}


std::vector<std::string> readBinaryCharArray(std::fstream &fileH, const int size) {

    std::vector<std::string> arr;
    arr.reserve(size);

    int rest=size;

    auto sizeData = block_size_data_binary(EIOD::CHAR);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements=maxBlockSize/sizeOfElement;

    if (!fileH.is_open()) {
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    while (rest > 0) {

        int num,dhead,dtail;

        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=flipEndianInt(dhead);

        num=dhead/sizeOfElement;

        if ((num > maxNumberOfElements) || (num < 0)) {
            std::string message="Error reading binary char data, inconsistent header data or incorrect number of elements";
            throw std::runtime_error(message);
        }

        for (int i=0; i<num; i++) {
            std::string str8(8,' ');
            fileH.read(&str8[0], sizeOfElement);
            arr.push_back(trimr(str8));
        }
	
        rest=rest-num;

        if ((num < maxNumberOfElements) && (rest!=0)) {
            std::string message="Error reading binary char data, incorrect number of elements";
            throw std::runtime_error(message);
        }

        fileH.read((char *)&dtail, sizeof(dtail));
        dtail=flipEndianInt(dtail);

        if (dhead!=dtail) {
            std::string message="Error reading binary char data, tail not matching header.";
            throw std::runtime_error(message);
        }
    }

    return arr;
}

// functions for reading formatted files .FEGRID,  .FINIT,  ...
// not part of PR 633, hence not reviewed. tskille 2019/02/19

std::vector<std::string> split_string(const std::string &inputStr) {

    std::istringstream iss(inputStr);

    std::vector<std::string> tokens {std::istream_iterator<std::string>{iss},
                                     std::istream_iterator<std::string>{}
                                    };

    return tokens;
}



void readFormattedHeader(std::fstream &fileH,std::string &arrName,int &num,EIOD::eclArrType &arrType) {

    int p1,p2,p3,p4;
    std::string antStr;
    std::string line;
    std::string arrTypeStr;

    if (!fileH.is_open()) {
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    getline (fileH,line);

    p1=line.find_first_of("'");
    p2=line.find_first_of("'",p1+1);
    p3=line.find_first_of("'",p2+1);
    p4=line.find_first_of("'",p3+1);

    if ((p1==-1) || (p2==-1)|| (p3==-1) || (p4==-1)) {
        std::string message="Header name and type should be enclosed with '";
        throw std::runtime_error(message);
    }

    arrName=line.substr(p1+1,p2-p1-1);
    antStr=line.substr(p2+1,p3-p2-1);
    arrTypeStr=line.substr(p3+1,p4-p3-1);

    num=std::stoi(antStr);

    if (arrTypeStr=="INTE")
        arrType=EIOD::INTE;
    else if (arrTypeStr=="REAL")
        arrType=EIOD::REAL;
    else if (arrTypeStr=="DOUB")
        arrType=EIOD::DOUB;
    else if (arrTypeStr=="CHAR")
        arrType=EIOD::CHAR;
    else if (arrTypeStr=="LOGI")
        arrType=EIOD::LOGI;
    else if (arrTypeStr=="MESS")
        arrType=EIOD::MESS;
    else
        throw std::runtime_error("Error, unknown array type '" + arrTypeStr +"'");

    if (arrName.size()!=8)  {
        std::string message="Header name should be 8 characters";
        throw std::runtime_error(message);
    }
}

std::vector<int> readFormattedInteArray(std::fstream &fileH,const int size) {

    std::vector<int> arr;
    std::vector<std::string> tokens;
    std::string line;
    int value;
    int num=0;

    if (!fileH.is_open()) {
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    while (num<size) {

        if (fileH.eof()) {
            std::string message="End of file reached when reading integer array";
            throw std::runtime_error(message);
        }

        getline (fileH,line);
        tokens=split_string(line);

        for (unsigned int i=0; i<tokens.size(); i++) {
            value=stoi(tokens[i]);
            arr.push_back(value);
        }

        num=num+tokens.size();
    }

    return arr;
}


std::vector<std::string> readFormattedCharArray(std::fstream &fileH,const int size) {

    std::vector<std::string> arr;
    std::string line,value;
    int num=0;
    int p1,p2;

    if (!fileH.is_open()) {
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }


    while (num<size) {

        getline (fileH,line);

        if (line.length()==0) {
            std::string message="Reading formatted char array, end of file or blank line, read " + std::to_string(arr.size()) + " of " + std::to_string(size) + " elements";
            throw std::runtime_error(message);
        }

        p1=line.find_first_of("'");

        if (p1==-1) {
            std::string message="Reading formatted char array, all strings must be enclosed by apostrophe (')";
            throw std::runtime_error(message);
        }

        while (p1>-1) {
            p2=line.find_first_of("'",p1+1);

            if (p2==-1) {
                std::string message="Reading formatted char array, all strings must be enclosed by apostrophe (')";
                throw std::runtime_error(message);
            }

            value=line.substr(p1+1,p2-p1-1);

            if (value.size()!=8) {
                std::string message="Reading formatted char array, all strings should have 8 characters";
                throw std::runtime_error(message);
            }

            if (value=="        ") {
                arr.push_back("");
            } else {
                arr.push_back(trimr(value));
            }

            num++;

            p1=line.find_first_of("'",p2+1);
        }
    }

    return arr;
}


std::vector<float> readFormattedRealArray(std::fstream &fileH,const int size) {

    std::vector<float> arr;
    std::vector<std::string> tokens;
    std::string line;
    int num=0;
    float value;

    if (!fileH.is_open()) {
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }
    
    
    while (num<size) {

        if (fileH.eof()) {
            std::string message="End of file reached when reading real array";
            throw std::runtime_error(message);
        }

        getline (fileH,line);
        tokens=split_string(line);

	   // tskille: temporary fix, need to be discussed. OPM flow writes numbers
	   // that are outside valid range for float, and function stof will fail 
	   // with runtime error.

        for (unsigned int i=0; i<tokens.size(); i++) {
            // value=stof(tokens[i]);
            
	    double dtmpv=stod(tokens[i]);	   
	    value=static_cast<float>(dtmpv);
            arr.push_back(value);
        }

        num=num+tokens.size();
    }

    return arr;
}


std::vector<bool> readFormattedLogiArray(std::fstream &fileH,const int size) {

    std::vector<bool> arr;
    std::vector<std::string> tokens;
    std::string line;
    int num=0;

    if (!fileH.is_open()) {
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    while (num<size) {

        if (fileH.eof()) {
            std::string message="End of file reached when reading logi array";
            throw std::runtime_error(message);
        }

        getline (fileH,line);

        tokens=split_string(line);

        for (unsigned int i=0; i<tokens.size(); i++) {

            if (tokens[i]=="T") {
                arr.push_back(true);
            } else if (tokens[i]=="F")   {
                arr.push_back(false);
            } else {
                std::string message="Could not convert '" + tokens[i] + "' to a bool value ";
                throw std::invalid_argument(message);
            }
        }

        num=num+tokens.size();
    }

    return arr;
}

std::vector<double> readFormattedDoubArray(std::fstream &fileH,const int size) {

    std::vector<double> arr;
    std::vector<std::string> tokens;
    std::string line;
    int num,p1;
    double value;
    num=0;

    if (!fileH.is_open()) {
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    while (num<size) {

        if (fileH.eof()) {
            std::string message="End of file reached when reading double array";
            throw std::runtime_error(message);
        }

        getline (fileH,line);
        tokens=split_string(line);


        for (unsigned int i=0; i<tokens.size(); i++) {

            p1=tokens[i].find_first_of("D");

            if (p1==-1) {
                p1=tokens[i].find_first_of("-",1);

                if (p1>-1) {
                    tokens[i]=tokens[i].insert(p1,"E");
                } else {
                    p1=tokens[i].find_first_of("+",1);

                    if (p1==-1) {
                        std::string message="In Routine Read readFormattedDoubArray, could not convert '" + tokens[i] + "' to double.";
                        throw std::invalid_argument(message);
                    }

                    tokens[i]=tokens[i].insert(p1,"E");
                }
            } else {
                tokens[i].replace(p1,1,"E");
            }

            value=stod(tokens[i]);
            arr.push_back(value);
        }
        num=num+tokens.size();
    }

    return arr;
}

} // anonymous namespace


EclFile::EclFile(std::string filename) {

    std::fstream fileH;

    formatted = isFormatted(filename);
    inputFilename=filename;

    if (formatted) {
        fileH.open(filename, std::ios::in);
    } else {
        fileH.open(filename, std::ios::in |  std::ios::binary);
    }

    if (!fileH) {
        std::string message="Could not open file: " + filename;
        throw std::runtime_error(message);
    }

    int n=0;
    
    while (!isEOF(&fileH)) {

        std::string arrName(8,' ');
        EIOD::eclArrType arrType;
        int num;

        if (formatted) {
            readFormattedHeader(fileH,arrName,num,arrType);
        } else {
            readBinaryHeader(fileH,arrName,num,arrType);
        }

        array_size.push_back(num);
        array_type.push_back(arrType);

        array_name.push_back(trimr(arrName));
        array_index[array_name[n]]=n;

        unsigned long int pos = fileH.tellg();
        ifStreamPos.push_back(pos);

        arrayLoaded.push_back(false);

        if (formatted) {
            unsigned long int sizeOfNextArray=sizeOnDiskFormatted(num, arrType);
            fileH.ignore(sizeOfNextArray);
        } else {
            unsigned long int sizeOfNextArray=sizeOnDiskBinary(num, arrType);
            fileH.ignore(sizeOfNextArray);
        }

        n++;
    };

    fileH.close();
}

void EclFile::loadArray(std::fstream &fileH,int arrIndex) {

    fileH.seekg (ifStreamPos[arrIndex], fileH.beg);

    if (formatted) {
        switch(array_type[arrIndex]) {
        case EIOD::INTE  :
            inte_array[arrIndex]=readFormattedInteArray(fileH,array_size[arrIndex]);
            break;
        case EIOD::REAL  :
            real_array[arrIndex]=readFormattedRealArray(fileH,array_size[arrIndex]);
            break;
        case EIOD::DOUB  :
            doub_array[arrIndex]=readFormattedDoubArray(fileH,array_size[arrIndex]);
            break;
        case EIOD::LOGI  :
            logi_array[arrIndex]=readFormattedLogiArray(fileH,array_size[arrIndex]);
            break;
        case EIOD::CHAR  :
            char_array[arrIndex]=readFormattedCharArray(fileH,array_size[arrIndex]);
            break;
        }

    } else {
        switch(array_type[arrIndex]) {
        case EIOD::INTE  :
            inte_array[arrIndex]=readBinaryInteArray(fileH,array_size[arrIndex]);
            break;
        case EIOD::REAL  :
            real_array[arrIndex]=readBinaryRealArray(fileH,array_size[arrIndex]);
            break;
        case EIOD::DOUB  :
            doub_array[arrIndex]=readBinaryDoubArray(fileH,array_size[arrIndex]);
            break;
        case EIOD::LOGI  :
            logi_array[arrIndex]=readBinaryLogiArray(fileH,array_size[arrIndex]);
            break;
        case EIOD::CHAR  :
            char_array[arrIndex]=readBinaryCharArray(fileH,array_size[arrIndex]);
            break;
        }
    }

    arrayLoaded[arrIndex]=true;

}


void EclFile::checkIfLoaded(const int &arrIndex) const {

    if (!arrayLoaded[arrIndex]) {
        std::string message="Array with index "+std::to_string(arrIndex) + " is not loaded";
        throw std::runtime_error(message);
    }
}


void EclFile::loadData() {

    std::fstream fileH;
    
    if (formatted) {
        fileH.open(inputFilename, std::ios::in);
    } else {
        fileH.open(inputFilename, std::ios::in |  std::ios::binary);
    }

    if (!fileH) {
        std::string message="Could not open file: '" + inputFilename +"'";
        throw std::runtime_error(message);
    }

    for (unsigned int i=0; i<array_name.size(); i++) {
        loadArray(fileH,i);
    }

    fileH.close();
}

void EclFile::loadData(std::string name) {

    std::fstream fileH;

    if (formatted) {
        fileH.open(inputFilename, std::ios::in);
    } else {
        fileH.open(inputFilename, std::ios::in |  std::ios::binary);
    }

    if (!fileH) {
        std::string message="Could not open file: '" + inputFilename +"'";
        throw std::runtime_error(message);
    }

    for (unsigned int i=0; i<array_name.size(); i++) {

        if (array_name[i]==name) {
            loadArray(fileH,i);
        }

    }

    fileH.close();
}

void EclFile::loadData(std::vector<int> arrIndex) {

    std::fstream fileH;

    if (formatted) {
        fileH.open(inputFilename, std::ios::in);
    } else {
        fileH.open(inputFilename, std::ios::in |  std::ios::binary);
    }

    if (!fileH) {
        std::string message="Could not open file: '" + inputFilename +"'";
        throw std::runtime_error(message);
    }

    for (auto ind : arrIndex) {
        loadArray(fileH,ind);
    }

    fileH.close();

}

void EclFile::loadData(int arrIndex) {

    std::fstream fileH;

    if (formatted) {
        fileH.open(inputFilename, std::ios::in);
    } else {
        fileH.open(inputFilename, std::ios::in |  std::ios::binary);
    }

    if (!fileH) {
        std::string message="Could not open file: '" + inputFilename +"'";
        throw std::runtime_error(message);
    }

    loadArray(fileH,arrIndex);

    fileH.close();

}


const std::vector<std::tuple<std::string, EIOD::eclArrType, int>> EclFile::getList() const {

    std::vector<std::tuple<std::string, EIOD::eclArrType, int>> list;

    for (unsigned int i=0; i<array_name.size(); i++) {
        std::tuple<std::string, EIOD::eclArrType, int> t(array_name[i],array_type[i],array_size[i]);
        list.push_back(t);
    }

    return std::move(list);
}

template<>
const std::vector<int> &EclFile::get<int>(const int &arrIndex) const {

    if (array_type[arrIndex]!=EIOD::INTE) {
        std::string message="Array with index "+std::to_string(arrIndex) + " is not of type integer";
        throw std::invalid_argument(message);
    }

    checkIfLoaded(arrIndex);

    auto it = inte_array.find(arrIndex);

    return it->second;

}

template<>
const std::vector<float> &EclFile::get<float>(const int &arrIndex) const {

    if (array_type[arrIndex]!=EIOD::REAL) {
        std::string message="Array with index "+std::to_string(arrIndex) + " is not of type float";
        throw std::invalid_argument(message);
    }

    checkIfLoaded(arrIndex);

    auto it = real_array.find(arrIndex);

    return it->second;

}


template<>
const std::vector<double> &EclFile::get<double>(const int &arrIndex) const {

    if (array_type[arrIndex]!=EIOD::DOUB) {
        std::string message="Array with index "+std::to_string(arrIndex) + " is not of type double";
        throw std::invalid_argument(message);
    }

    checkIfLoaded(arrIndex);

    auto it = doub_array.find(arrIndex);

    return it->second;
}


template<>
const std::vector<bool> &EclFile::get<bool>(const int &arrIndex) const {

    if (array_type[arrIndex]!=EIOD::LOGI) {
        std::string message="Array with index "+std::to_string(arrIndex) + " is not of type bool";
        throw std::invalid_argument(message);
    }

    checkIfLoaded(arrIndex);

    auto it = logi_array.find(arrIndex);

    return it->second;
}


template<>
const std::vector<std::string> &EclFile::get<std::string>(const int &arrIndex) const {

    if (array_type[arrIndex]!=EIOD::CHAR) {
        std::string message="Array with index "+std::to_string(arrIndex) + " is not of type std::string";
        throw std::invalid_argument(message);
    }

    checkIfLoaded(arrIndex);

    auto it = char_array.find(arrIndex);

    return it->second;
}

const bool EclFile::hasKey(const std::string &name) const {

    auto search = array_index.find(name);
    bool result = search == array_index.end() ? false : true ;

    return result;
}


template<>
const std::vector<int> &EclFile::get<int>(const std::string &name) const {

    auto search = array_index.find(name);

    if (search == array_index.end()) {
        std::string message="key '"+name + "' not found";
        throw std::invalid_argument(message);
    }

    if (array_type[search->second]!=EIOD::INTE) {
        std::string message="Array '" + name + "' is not of type integer";
        throw std::invalid_argument(message);
    }

    checkIfLoaded(search->second);

    auto it = inte_array.find(search->second);

    return it->second;
}

template<>
const std::vector<float> &EclFile::get<float>(const std::string &name) const {

    auto search = array_index.find(name);

    if (search == array_index.end()) {
        std::string message="key '" + name + "' not found";
        throw std::invalid_argument(message);
    }

    if (array_type[search->second]!=EIOD::REAL) {
        std::string message="Array '" + name + "' is not of type float";
        throw std::invalid_argument(message);
    }

    checkIfLoaded(search->second);

    auto it = real_array.find(search->second);

    return it->second;
}


template<>
const std::vector<double> &EclFile::get<double>(const std::string &name) const {

    auto search = array_index.find(name);

    if (search == array_index.end()) {
        std::string message="key '"+name + "' not found";
        throw std::invalid_argument(message);
    }

    if (array_type[search->second]!=EIOD::DOUB) {
        std::string message="Array '" + name + "' is not of type double";
        throw std::invalid_argument(message);
    }

    checkIfLoaded(search->second);

    auto it = doub_array.find(search->second);

    return it->second;
}


template<>
const std::vector<bool> &EclFile::get<bool>(const std::string &name) const {

    auto search = array_index.find(name);

    if (search == array_index.end()) {
        std::string message="key '"+name + "' not found";
        throw std::invalid_argument(message);
    }

    if (array_type[search->second]!=EIOD::LOGI) {
        std::string message="Array '" + name + "' is not of type bool";
        throw std::invalid_argument(message);
    }

    checkIfLoaded(search->second);

    auto it = logi_array.find(search->second);

    return it->second;
}


template<>
const std::vector<std::string> &EclFile::get<std::string>(const std::string &name) const {

    auto search = array_index.find(name);

    if (search == array_index.end()) {
        std::string message="key '"+name + "' not found";
        throw std::invalid_argument(message);
    }

    if (array_type[search->second]!=EIOD::CHAR) {
        std::string message="Array '" + name + "' is not of type std::string";
        throw std::invalid_argument(message);
    }

    checkIfLoaded(search->second);

    auto it = char_array.find(search->second);

    return it->second;
}
