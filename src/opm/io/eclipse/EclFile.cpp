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

#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EclUtil.hpp>

#include <array>
#include <exception>
#include <functional>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string.h>
#include <sstream>
#include <iterator>
#include <iomanip>
#include <algorithm>

#include <opm/common/ErrorMacros.hpp>


// anonymous namespace for EclFile

namespace {


bool isFormatted(const std::string& filename)
{
    int p = filename.find_last_of(".");
    int l = filename.length();

    std::string extension = filename.substr(p,l-p);

    return extension.substr(1,1) == "F" || extension.substr(1,1) == "A";
}


bool isEOF(std::fstream* fileH)
{
    int num;
    long int pos = fileH->tellg();
    fileH->read(reinterpret_cast<char*>(&num), sizeof(num));

    if (fileH->eof()) {
        return true;
    } else {
        fileH->seekg (pos);
        return false;
    }
}


void readBinaryHeader(std::fstream& fileH, std::string& arrName,
                      int& size, Opm::ecl::eclArrType &arrType)
{
    int bhead;
    std::string tmpStrName(8,' ');
    std::string tmpStrType(4,' ');

    fileH.read(reinterpret_cast<char*>(&bhead), sizeof(bhead));
    bhead = Opm::ecl::flipEndianInt(bhead);

    if (bhead != 16) {
        std::string message="Error reading binary header. Expected 16 bytes of header data, found " + std::to_string(bhead);
        OPM_THROW(std::runtime_error, message);
    }

    fileH.read(&tmpStrName[0], 8);

    fileH.read(reinterpret_cast<char*>(&size), sizeof(size));
    size = Opm::ecl::flipEndianInt(size);

    fileH.read(&tmpStrType[0], 4);

    fileH.read(reinterpret_cast<char*>(&bhead), sizeof(bhead));
    bhead = Opm::ecl::flipEndianInt(bhead);

    if (bhead != 16) {
        std::string message="Error reading binary header. Expected 16 bytes of header data, found " + std::to_string(bhead);
        OPM_THROW(std::runtime_error, message);
    }

    arrName = tmpStrName;
    if (tmpStrType == "INTE")
        arrType = Opm::ecl::INTE;
    else if (tmpStrType == "REAL")
        arrType = Opm::ecl::REAL;
    else if (tmpStrType == "DOUB")
        arrType = Opm::ecl::DOUB;
    else if (tmpStrType == "CHAR")
        arrType = Opm::ecl::CHAR;
    else if (tmpStrType =="LOGI")
        arrType = Opm::ecl::LOGI;
    else if (tmpStrType == "MESS")
        arrType = Opm::ecl::MESS;
    else
        OPM_THROW(std::runtime_error, "Error, unknown array type '" + tmpStrType +"'");
}


unsigned long int sizeOnDiskBinary(int num, Opm::ecl::eclArrType arrType)
{
    unsigned long int size = 0;

    if (arrType == Opm::ecl::MESS) {
        if (num > 0) {
            std::string message = "In routine calcSizeOfArray, type MESS can not have size > 0";
            OPM_THROW(std::invalid_argument, message);
        }
    } else {
        auto sizeData = Opm::ecl::block_size_data_binary(arrType);

        int sizeOfElement = std::get<0>(sizeData);
        int maxBlockSize = std::get<1>(sizeData);
        int maxNumberOfElements = maxBlockSize / sizeOfElement;

        size = num * sizeOfElement;
        size = size + ((num-1) / maxNumberOfElements) * 2 * Opm::ecl::sizeOfInte; // 8 byte (two integers) every 1000 element

        if (num > 0) {
            size = size + 2 * Opm::ecl::sizeOfInte;
        }
    }

    return size;
}

unsigned long int sizeOnDiskFormatted(const int num, Opm::ecl::eclArrType arrType)
{
    unsigned long int size = 0;

    if (arrType == Opm::ecl::MESS) {
        if (num > 0) {
            OPM_THROW(std::invalid_argument, "In routine calcSizeOfArray, type MESS can not have size > 0");
        }
    } else {
        auto sizeData = block_size_data_formatted(arrType);

        int maxBlockSize = std::get<0>(sizeData);
        int nColumns = std::get<1>(sizeData);
        int columnWidth = std::get<2>(sizeData);

        int nBlocks = num /maxBlockSize;
        int sizeOfLastBlock = num %  maxBlockSize;

        size = 0;

        if (nBlocks > 0) {
            int nLinesBlock = maxBlockSize / nColumns;
            int rest = maxBlockSize % nColumns;

            if (rest > 0) {
                nLinesBlock++;
            }

            long int blockSize = maxBlockSize * columnWidth + nLinesBlock;
            size = nBlocks * blockSize;
        }

        int nLines = sizeOfLastBlock / nColumns;
        int rest = sizeOfLastBlock % nColumns;

        size = size + sizeOfLastBlock * columnWidth + nLines;

        if (rest > 0) {
            size++;
        }
    }

    return size;
}


template<typename T, typename T2>
std::vector<T> readBinaryArray(std::fstream& fileH, const int size, Opm::ecl::eclArrType type,
                               std::function<T(T2)>& flip)
{
    std::vector<T> arr;

    auto sizeData = block_size_data_binary(type);
    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements = maxBlockSize / sizeOfElement;

    arr.reserve(size);

    int rest = size;
    while (rest > 0) {
        int dhead;
        fileH.read(reinterpret_cast<char*>(&dhead), sizeof(dhead));
        dhead = Opm::ecl::flipEndianInt(dhead);

        int num = dhead / sizeOfElement;

        if ((num > maxNumberOfElements) || (num < 0)) {
            OPM_THROW(std::runtime_error, "Error reading binary data, inconsistent header data or incorrect number of elements");
        }

        for (int i = 0; i < num; i++) {
            T2 value;
            fileH.read(reinterpret_cast<char*>(&value), sizeOfElement);
            arr.push_back(flip(value));
        }

        rest -= num;

        if (( num < maxNumberOfElements && rest != 0) ||
            (num == maxNumberOfElements && rest < 0)) {
            std::string message = "Error reading binary data, incorrect number of elements";
            OPM_THROW(std::runtime_error, message);
        }

        int dtail;
        fileH.read(reinterpret_cast<char*>(&dtail), sizeof(dtail));
        dtail = Opm::ecl::flipEndianInt(dtail);

        if (dhead != dtail) {
            OPM_THROW(std::runtime_error, "Error reading binary data, tail not matching header.");
        }
    }

    return arr;
}


std::vector<int> readBinaryInteArray(std::fstream &fileH, const int size)
{
    std::function<int(int)> f = Opm::ecl::flipEndianInt;
    return readBinaryArray<int,int>(fileH, size, Opm::ecl::INTE, f);
}


std::vector<float> readBinaryRealArray(std::fstream& fileH, const int size)
{
    std::function<float(float)> f = Opm::ecl::flipEndianFloat;
    return readBinaryArray<float,float>(fileH, size, Opm::ecl::REAL, f);
}


std::vector<double> readBinaryDoubArray(std::fstream& fileH, const int size)
{
    std::function<double(double)> f = Opm::ecl::flipEndianDouble;
    return readBinaryArray<double,double>(fileH, size, Opm::ecl::DOUB, f);
}

std::vector<bool> readBinaryLogiArray(std::fstream &fileH, const int size)
{
    std::function<bool(unsigned int)> f = [](unsigned int intVal)
                                          {
                                              bool value;
                                              if (intVal == Opm::ecl::true_value) {
                                                  value = true;
                                              } else if (intVal == Opm::ecl::false_value) {
                                                  value = false;
                                              } else {
                                                  OPM_THROW(std::runtime_error, "Error reading logi value");
                                              }

                                              return value;
                                          };
    return readBinaryArray<bool,unsigned int>(fileH, size, Opm::ecl::LOGI, f);
}


std::vector<std::string> readBinaryCharArray(std::fstream& fileH, const int size)
{
    using Char8 = std::array<char, 8>;
    std::function<std::string(Char8)> f = [](const Char8& val)
                                          {
                                              std::string res(val.begin(), val.end());
                                              return Opm::ecl::trimr(res);
                                          };
    return readBinaryArray<std::string,Char8>(fileH, size, Opm::ecl::CHAR, f);
}


std::vector<std::string> split_string(const std::string& inputStr)
{
    std::istringstream iss(inputStr);

    std::vector<std::string> tokens {std::istream_iterator<std::string>{iss},
                                     std::istream_iterator<std::string>{}
                                    };

    return tokens;
}


void readFormattedHeader(std::fstream& fileH, std::string& arrName,
                         int &num, Opm::ecl::eclArrType &arrType)
{
    std::string line;
    std::getline(fileH,line);

    int p1 = line.find_first_of("'");
    int p2 = line.find_first_of("'",p1+1);
    int p3 = line.find_first_of("'",p2+1);
    int p4 = line.find_first_of("'",p3+1);

    if (p1 == -1 || p2 == -1 || p3 == -1 || p4 == -1) {
        OPM_THROW(std::runtime_error, "Header name and type should be enclosed with '");
    }

    arrName = line.substr(p1 + 1, p2 - p1 - 1);
    std::string antStr = line.substr(p2 + 1, p3 - p2 - 1);
    std::string arrTypeStr = line.substr(p3 + 1, p4 - p3 - 1);

    num = std::stoi(antStr);

    if (arrTypeStr == "INTE")
        arrType = Opm::ecl::INTE;
    else if (arrTypeStr == "REAL")
        arrType = Opm::ecl::REAL;
    else if (arrTypeStr == "DOUB")
        arrType = Opm::ecl::DOUB;
    else if (arrTypeStr == "CHAR")
        arrType = Opm::ecl::CHAR;
    else if (arrTypeStr == "LOGI")
        arrType = Opm::ecl::LOGI;
    else if (arrTypeStr == "MESS")
        arrType = Opm::ecl::MESS;
    else
        OPM_THROW(std::runtime_error, "Error, unknown array type '" + arrTypeStr +"'");

    if (arrName.size() != 8) {
        OPM_THROW(std::runtime_error, "Header name should be 8 characters");
    }
}


template<typename T>
std::vector<T> readFormattedArray(std::fstream& fileH, const int size,
                                 std::function<T(const std::string&)>& process)
{
    std::vector<T> arr;
    std::vector<std::string> tokens;
    std::string line;

    int num = 0;
    while (num < size) {
        if (fileH.eof()) {
            OPM_THROW(std::runtime_error, "End of file reached when reading array");
        }

        getline(fileH, line);
        tokens = split_string(line);

        for (const std::string& token : tokens) {
            arr.push_back(process(token));
        }

        num = num + tokens.size();
    }

    return arr;
}


std::vector<int> readFormattedInteArray(std::fstream& fileH, const int size)
{
    std::function<int(const std::string&)> f = [](const std::string& val)
                                               {
                                                   return std::stoi(val);
                                               };
    return readFormattedArray(fileH, size, f);
}


std::vector<std::string> readFormattedCharArray(std::fstream& fileH, const int size)
{
    if (!fileH.is_open()) {
        OPM_THROW(std::runtime_error, "fstream fileH not open for reading");
    }

    std::vector<std::string> arr;
    int num = 0;
    while (num < size) {
        std::string line;
        getline(fileH, line);

        if (line.empty()) {
            std::string message="Reading formatted char array, end of file or blank line, read " + std::to_string(arr.size()) + " of " + std::to_string(size) + " elements";
            OPM_THROW(std::runtime_error, message);
        }

        int p1 = line.find_first_of("'");

        if (p1 == -1) {
            std::string message="Reading formatted char array, all strings must be enclosed by apostrophe (')";
            OPM_THROW(std::runtime_error, message);
        }

        while (p1 > -1) {
            int p2 = line.find_first_of("'", p1 + 1);

            if (p2 == -1) {
                std::string message="Reading formatted char array, all strings must be enclosed by apostrophe (')";
                OPM_THROW(std::runtime_error, message);
            }

            std::string value = line.substr(p1 + 1, p2 - p1 - 1);

            if (value.size() != 8) {
                std::string message="Reading formatted char array, all strings should have 8 characters";
                OPM_THROW(std::runtime_error, message);
            }

            if (value == "        ") {
                arr.push_back("");
            } else {
                arr.push_back(Opm::ecl::trimr(value));
            }

            num++;

            p1 = line.find_first_of("'", p2 + 1);
        }
    }

    return arr;
}


std::vector<float> readFormattedRealArray(std::fstream &fileH, const int size)
{
    std::function<float(const std::string&)> f = [](const std::string& val)
                                                 {
                                                     // tskille: temporary fix, need to be discussed. OPM flow writes numbers
                                                     // that are outside valid range for float, and function stof will fail
                                                     double dtmpv = std::stod(val);
                                                     return dtmpv;
                                                 };
    return readFormattedArray<float>(fileH, size, f);
}


std::vector<bool> readFormattedLogiArray(std::fstream& fileH, const int size)
{
    std::function<bool(const std::string&)> f = [](const std::string& val)
                                                {
                                                    if (val == "T") {
                                                        return true;
                                                    } else if (val == "F") {
                                                        return false;
                                                    } else {
                                                        std::string message="Could not convert '" + val + "' to a bool value ";
                                                        OPM_THROW(std::invalid_argument, message);
                                                    }
                                                };
    return readFormattedArray<bool>(fileH, size, f);
}

std::vector<double> readFormattedDoubArray(std::fstream& fileH, const int size)
{
    std::function<double(const std::string&)> f = [](const std::string& value)
                                                  {
                                                      std::string val(value);
                                                      int p1 = val.find_first_of("D");

                                                      if (p1 == -1) {
                                                          p1 = val.find_first_of("-", 1);
                                                          if (p1 > -1) {
                                                              val = val.insert(p1,"E");
                                                          } else {
                                                              p1 = val.find_first_of("+", 1);

                                                              if (p1 == -1) {
                                                                  std::string message="In Routine Read readFormattedDoubArray, could not convert '" + val + "' to double.";
                                                                  OPM_THROW(std::invalid_argument,message);
                                                              }

                                                              val = val.insert(p1,"E");
                                                          }
                                                      } else {
                                                          val.replace(p1,1,"E");
                                                      }

                                                      return std::stod(val);
                                                  };
    return readFormattedArray<double>(fileH, size, f);
}

} // anonymous namespace

// ==========================================================================

namespace Opm { namespace ecl {

EclFile::EclFile(const std::string& filename) : inputFilename(filename)
{
    std::fstream fileH;

    formatted = isFormatted(filename);

    if (formatted) {
        fileH.open(filename, std::ios::in);
    } else {
        fileH.open(filename, std::ios::in |  std::ios::binary);
    }

    if (!fileH) {
        std::string message="Could not open file: " + filename;
        OPM_THROW(std::runtime_error, message);
    }

    int n = 0;
    while (!isEOF(&fileH)) {
        std::string arrName(8,' ');
        eclArrType arrType;
        int num;

        if (formatted) {
            readFormattedHeader(fileH,arrName,num,arrType);
        } else {
            readBinaryHeader(fileH,arrName,num,arrType);
        }

        array_size.push_back(num);
        array_type.push_back(arrType);

        array_name.push_back(trimr(arrName));
        array_index[array_name[n]] = n;

        unsigned long int pos = fileH.tellg();
        ifStreamPos.push_back(pos);

        arrayLoaded.push_back(false);

        if (formatted) {
            unsigned long int sizeOfNextArray = sizeOnDiskFormatted(num, arrType);
            fileH.ignore(sizeOfNextArray);
        } else {
            unsigned long int sizeOfNextArray = sizeOnDiskBinary(num, arrType);
            fileH.ignore(sizeOfNextArray);
        }

        n++;
    };

    fileH.close();
}


void EclFile::loadArray(std::fstream& fileH, int arrIndex)
{
    fileH.seekg (ifStreamPos[arrIndex], fileH.beg);

    if (formatted) {
        switch (array_type[arrIndex]) {
        case INTE:
            inte_array[arrIndex] = readFormattedInteArray(fileH, array_size[arrIndex]);
            break;
        case REAL:
            real_array[arrIndex] = readFormattedRealArray(fileH, array_size[arrIndex]);
            break;
        case DOUB:
            doub_array[arrIndex] = readFormattedDoubArray(fileH, array_size[arrIndex]);
            break;
        case LOGI:
            logi_array[arrIndex] = readFormattedLogiArray(fileH, array_size[arrIndex]);
            break;
        case CHAR:
            char_array[arrIndex] = readFormattedCharArray(fileH, array_size[arrIndex]);
            break;
        case MESS:
            break;
        default:
            OPM_THROW(std::runtime_error, "Asked to read unexpected array type");
            break;
        }

    } else {
        switch (array_type[arrIndex]) {
        case INTE:
            inte_array[arrIndex] = readBinaryInteArray(fileH, array_size[arrIndex]);
            break;
        case REAL:
            real_array[arrIndex] = readBinaryRealArray(fileH, array_size[arrIndex]);
            break;
        case DOUB:
            doub_array[arrIndex] = readBinaryDoubArray(fileH, array_size[arrIndex]);
            break;
        case LOGI:
            logi_array[arrIndex] = readBinaryLogiArray(fileH, array_size[arrIndex]);
            break;
        case CHAR:
            char_array[arrIndex] = readBinaryCharArray(fileH, array_size[arrIndex]);
            break;
        case MESS:
            break;
        default:
            OPM_THROW(std::runtime_error, "Asked to read unexpected array type");
            break;
        }
    }

    arrayLoaded[arrIndex] = true;
}


void EclFile::loadData()
{
    std::fstream fileH;

    if (formatted) {
        fileH.open(inputFilename, std::ios::in);
    } else {
        fileH.open(inputFilename, std::ios::in |  std::ios::binary);
    }

    if (!fileH) {
        std::string message="Could not open file: '" + inputFilename +"'";
        OPM_THROW(std::runtime_error, message);
    }

    for (size_t i = 0; i < array_name.size(); i++) {
        loadArray(fileH, i);
    }

    fileH.close();
}


void EclFile::loadData(const std::string& name)
{
    std::fstream fileH;

    if (formatted) {
        fileH.open(inputFilename, std::ios::in);
    } else {
        fileH.open(inputFilename, std::ios::in |  std::ios::binary);
    }

    if (!fileH) {
        std::string message="Could not open file: '" + inputFilename +"'";
        OPM_THROW(std::runtime_error, message);
    }

    for (size_t i = 0; i < array_name.size(); i++) {
        if (array_name[i] == name) {
            loadArray(fileH, i);
        }
    }

    fileH.close();
}


void EclFile::loadData(const std::vector<int>& arrIndex)
{
    std::fstream fileH;

    if (formatted) {
        fileH.open(inputFilename, std::ios::in);
    } else {
        fileH.open(inputFilename, std::ios::in |  std::ios::binary);
    }

    if (!fileH) {
        std::string message="Could not open file: '" + inputFilename +"'";
        OPM_THROW(std::runtime_error, message);
    }

    for (int ind : arrIndex) {
        loadArray(fileH, ind);
    }

    fileH.close();
}


void EclFile::loadData(int arrIndex)
{
    std::fstream fileH;

    if (formatted) {
        fileH.open(inputFilename, std::ios::in);
    } else {
        fileH.open(inputFilename, std::ios::in |  std::ios::binary);
    }

    if (!fileH) {
        std::string message="Could not open file: '" + inputFilename +"'";
        OPM_THROW(std::runtime_error, message);
    }

    loadArray(fileH, arrIndex);

    fileH.close();
}


std::vector<EclFile::EclEntry> EclFile::getList() const
{
    std::vector<EclEntry> list;
    list.reserve(this->array_name.size());

    for (size_t i = 0; i < array_name.size(); i++) {
        list.emplace_back(array_name[i], array_type[i], array_size[i]);
    }

    return list;
}


template<>
const std::vector<int>& EclFile::get<int>(int arrIndex)
{
    return getImpl(arrIndex, INTE, inte_array, "integer");
}

template<>
const std::vector<float>&
EclFile::get<float>(int arrIndex)
{
    return getImpl(arrIndex, REAL, real_array, "float");
}


template<>
const std::vector<double> &EclFile::get<double>(int arrIndex)
{
    return getImpl(arrIndex, DOUB, doub_array, "double");
}


template<>
const std::vector<bool>& EclFile::get<bool>(int arrIndex)
{
    return getImpl(arrIndex, LOGI, logi_array, "bool");
}


template<>
const std::vector<std::string>& EclFile::get<std::string>(int arrIndex)
{
    return getImpl(arrIndex, CHAR, char_array, "string");
}


bool EclFile::hasKey(const std::string &name) const
{
    auto search = array_index.find(name);
    return search != array_index.end();
}


template<>
const std::vector<int>& EclFile::get<int>(const std::string& name)
{
    auto search = array_index.find(name);

    if (search == array_index.end()) {
        std::string message="key '"+name + "' not found";
        OPM_THROW(std::invalid_argument, message);
    }

    return getImpl(search->second, INTE, inte_array, "integer");
}

template<>
const std::vector<float>& EclFile::get<float>(const std::string& name)
{
    auto search = array_index.find(name);

    if (search == array_index.end()) {
        std::string message="key '"+name + "' not found";
        OPM_THROW(std::invalid_argument, message);
    }

    return getImpl(search->second, REAL, real_array, "float");
}


template<>
const std::vector<double>& EclFile::get<double>(const std::string &name)
{
    auto search = array_index.find(name);

    if (search == array_index.end()) {
        std::string message="key '"+name + "' not found";
        OPM_THROW(std::invalid_argument, message);
    }

    return getImpl(search->second, DOUB, doub_array, "double");
}


template<>
const std::vector<bool>& EclFile::get<bool>(const std::string &name)
{
    auto search = array_index.find(name);

    if (search == array_index.end()) {
        std::string message="key '"+name + "' not found";
        OPM_THROW(std::invalid_argument, message);
    }

    return getImpl(search->second, LOGI, logi_array, "bool");
}


template<>
const std::vector<std::string>& EclFile::get<std::string>(const std::string &name)
{
    auto search = array_index.find(name);

    if (search == array_index.end()) {
        std::string message="key '"+name + "' not found";
        OPM_THROW(std::invalid_argument, message);
    }

    return getImpl(search->second, CHAR, char_array, "string");
}

}} // namespace Opm::ecl
