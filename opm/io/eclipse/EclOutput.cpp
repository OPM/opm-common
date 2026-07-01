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

#include <opm/io/eclipse/EclOutput.hpp>

#include <opm/common/ErrorMacros.hpp>

#include <opm/io/eclipse/EclUtil.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <ios>
#include <iostream>
#include <iterator>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <fmt/format.h>

namespace {
    // Maximum individual string length among a set of strings.
    auto maxStringLength(std::span<const std::string> strings)
    {
        return strings.empty()
            ? std::string::size_type{}
            : std::ranges::max(strings | std::ranges::views::transform(&std::string::size));
    }

    // Largest string length that fits in exactly three characters.
    constexpr auto c0nnMaxCharPerStr() { return std::string::size_type{999}; }
}

namespace Opm { namespace EclIO {

EclOutput::EclOutput(const std::string&            filename,
                     const bool                    formatted,
                     const std::ios_base::openmode mode)
    : isFormatted{formatted}
{
    const auto binmode = mode | std::ios_base::binary;
    ix_standard = false;

    // Always open in binary mode, including for formatted (text) output: on
    // Windows a text-mode stream translates '\n' to "\r\n", which corrupts the
    // fixed-width records of formatted ECL files (they use '\n' by spec). On
    // Unix binary and text modes are identical, so this is a no-op there.
    this->ofileH.open(filename, binmode);
}


template<>
void EclOutput::write<std::string>(const std::string&              name,
                                   const std::vector<std::string>& data)
{
    const auto max_char   = static_cast<std::string::size_type>(sizeOfChar);
    const auto max_length = std::max(maxStringLength(data), max_char);
    const auto isC0nn     = max_length > max_char;

    if (isC0nn && (max_length > c0nnMaxCharPerStr())) {
        OPM_THROW(std::invalid_argument,
                  fmt::format("Maximum string length {} exceeds C0NN "
                              "format's upper limit of {} characters",
                              max_length, c0nnMaxCharPerStr()));
    }

    const auto charType   = isC0nn ? eclArrType::C0NN             : eclArrType::CHAR;
    const auto charPerStr = isC0nn ? static_cast<int>(max_length) : sizeOfChar;

    this->writeStringVector(name, data, charType, charPerStr);
}

void EclOutput::write(const std::string&              name,
                      const std::vector<std::string>& data,
                      const int                       element_size)
{
    // Note: This overload always writes string data as C0NN and always uses
    // at least "sizeOfChar" characters per string in the output.

    const auto charPerStr = std::max(element_size, sizeOfChar);

    if (static_cast<std::string::size_type>(charPerStr) > c0nnMaxCharPerStr()) {
        OPM_THROW(std::invalid_argument,
                  fmt::format("Maximum requested number of characters "
                              "per string, {}, exceeds C0NN format's "
                              "upper limit of {} characters",
                              charPerStr, c0nnMaxCharPerStr()));
    }

    if (const auto max_length = maxStringLength(data);
        max_length > static_cast<std::string::size_type>(charPerStr))
    {
        OPM_THROW(std::runtime_error,
                  fmt::format("Maximum string length {} "
                              "exceeds permitted size {} "
                              "(requested element_size {}).",
                              max_length, charPerStr, element_size));
    }

    this->writeStringVector(name, data, eclArrType::C0NN, charPerStr);
}

template <>
void EclOutput::write<PaddedOutputString<8>>
    (const std::string&                        name,
     const std::vector<PaddedOutputString<8>>& data)
{
    if (this->isFormatted) {
        writeFormattedHeader(name, data.size(), CHAR, sizeOfChar);
        writeFormattedCharArray(data);
    }
    else {
        writeBinaryHeader(name, data.size(), CHAR, sizeOfChar);
        writeBinaryCharArray(data);
    }
}

void EclOutput::message(const std::string& msg)
{
    // Generate message, i.e., output vector of type eclArrType::MESS,
    // by passing an empty std::vector of element type 'char'.  Entire
    // contents of message contained in the 'msg' string.

    this->write(msg, std::vector<char>{});
}

void EclOutput::flushStream()
{
    this->ofileH.flush();
}

// ===========================================================================
// Private member functions below separator
// ===========================================================================

void EclOutput::writeStringVector(const std::string&              name,
                                  const std::vector<std::string>& strings,
                                  const eclArrType                charType,
                                  const int                       charPerStr)
{
    if (this->isFormatted) {
        this->writeFormattedHeader(name, strings.size(), charType, charPerStr);
        this->writeFormattedCharArray(strings, charPerStr);
    }
    else {
        this->writeBinaryHeader(name, strings.size(), charType, charPerStr);
        this->writeBinaryCharArray(strings, charPerStr);
    }
}

void EclOutput::writeBinaryHeader(const std::string&arrName, std::int64_t size, eclArrType arrType, int element_size)
{
    int bhead = flipEndianInt(16);
    std::string name = arrName + std::string(8 - arrName.size(),' ');

    // write X231 header if size larger that limits for 4 byte integers
    if (size > std::numeric_limits<int>::max()) {
        std::int64_t val231 = std::pow(2,31);
        std::int64_t x231 = size / val231;

        int flippedx231 = flipEndianInt(static_cast<int>( (-1)*x231 ));

        ofileH.write(reinterpret_cast<char*>(&bhead), sizeof(bhead));
        ofileH.write(name.c_str(), 8);
        ofileH.write(reinterpret_cast<char*>(&flippedx231), sizeof(flippedx231));
        ofileH.write("X231", 4);
        ofileH.write(reinterpret_cast<char*>(&bhead), sizeof(bhead));

        size = size - (x231 * val231);
    }

    int flippedSize = flipEndianInt(size);

    ofileH.write(reinterpret_cast<char*>(&bhead), sizeof(bhead));

    ofileH.write(name.c_str(), 8);
    ofileH.write(reinterpret_cast<char*>(&flippedSize), sizeof(flippedSize));

    std::string c0nn_str;

    if (arrType == C0NN){
        std::ostringstream ss;
        ss << "C" << std::setw(3) << std::setfill('0') << element_size;
        c0nn_str = ss.str();
    }

    switch(arrType) {
    case INTE:
        ofileH.write("INTE", 4);
        break;
    case REAL:
        ofileH.write("REAL", 4);
        break;
    case DOUB:
        ofileH.write("DOUB", 4);
        break;
    case LOGI:
        ofileH.write("LOGI", 4);
        break;
    case CHAR:
        ofileH.write("CHAR", 4);
        break;
    case C0NN:
        ofileH.write(c0nn_str.c_str(), 4);
        break;
    case MESS:
        ofileH.write("MESS", 4);
        break;
    }

    ofileH.write(reinterpret_cast<char *>(&bhead), sizeof(bhead));
}

template <typename T>
void EclOutput::writeBinaryArray(const std::vector<T>& data)
{
    int num;
    std::int64_t rest, offset;
    int dhead;
    std::int64_t size = data.size();

    eclArrType arrType = MESS;

    if constexpr (std::is_same_v<T, int>) {
        arrType = INTE;
    }
    else if constexpr (std::is_same_v<T, float>) {
        arrType = REAL;
    }
    else if constexpr (std::is_same_v<T, double>) {
        arrType = DOUB;
    }
    else if constexpr (std::is_same_v<T, bool>) {
        arrType = LOGI;
    }

    auto sizeData = block_size_data_binary(arrType);

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements = maxBlockSize / sizeOfElement;

    if (!ofileH.is_open()) {
        OPM_THROW(std::runtime_error, "fstream fileH not open for writing");
    }

    int logi_true_val = ix_standard ? true_value_ix : true_value_ecl;

    rest = size * static_cast<std::int64_t>(sizeOfElement);

    offset = 0;

    while (rest > 0) {
        if (rest > maxBlockSize) {
            rest -= maxBlockSize;
            num = maxNumberOfElements;
        } else {
            num = static_cast<int>(rest) / sizeOfElement;
            rest = 0;
        }

        dhead = flipEndianInt(num * sizeOfElement);

        ofileH.write(reinterpret_cast<char*>(&dhead), sizeof(dhead));

        if (arrType == INTE) {

            std::vector<int> flipped_data;
            flipped_data.resize(num, 0);

            for (int m = 0; m < num; ++m) {
                flipped_data[m] = flipEndianInt(data[m + offset]);
            }

            ofileH.write(reinterpret_cast<char*>(flipped_data.data()), flipped_data.size() * sizeof(int)) ;

        } else if (arrType == REAL) {

            std::vector<float> flipped_data;
            flipped_data.resize(num, 0);

            for (int m = 0; m < num; ++m) {
                flipped_data[m] = flipEndianFloat(data[m + offset]);
            }

            ofileH.write(reinterpret_cast<char*>(flipped_data.data()), flipped_data.size() * sizeof(float)) ;

        } else if (arrType == DOUB) {

            std::vector<double> flipped_data;
            flipped_data.resize(num, 0);

            for (int m = 0; m < num; ++m) {
                flipped_data[m] = flipEndianDouble(data[m + offset]);
            }

            ofileH.write(reinterpret_cast<char*>(flipped_data.data()), flipped_data.size() * sizeof(double)) ;

        } else if (arrType == LOGI) {

            std::vector<int> logi_data;
            logi_data.resize(num, 0);

            for (int m = 0; m < num; ++m) {
                if (data[m + offset]) {
                    logi_data[m] = logi_true_val;
                }
                else {
                    logi_data[m] = false_value;
                }
            }

            ofileH.write(reinterpret_cast<char*>(logi_data.data()), logi_data.size() * sizeof(int)) ;

        } else {

            std::cerr << "type not supported in write binaryarray\n";
            std::exit(EXIT_FAILURE);
        }

        offset += num;
        ofileH.write(reinterpret_cast<char*>(&dhead), sizeof(dhead));
    }
}


template void EclOutput::writeBinaryArray<int>(const std::vector<int>& data);
template void EclOutput::writeBinaryArray<float>(const std::vector<float>& data);
template void EclOutput::writeBinaryArray<double>(const std::vector<double>& data);
template void EclOutput::writeBinaryArray<bool>(const std::vector<bool>& data);
template void EclOutput::writeBinaryArray<char>(const std::vector<char>& data);


void EclOutput::writeBinaryCharArray(const std::vector<std::string>& data, int element_size)
{
    int num,dhead;

    int n = 0;
    int size = data.size();

    auto sizeData = block_size_data_binary(CHAR);

    if (element_size > sizeOfChar){
        std::get<1>(sizeData)= std::get<1>(sizeData) / std::get<0>(sizeData) * element_size;
        std::get<0>(sizeData) = element_size;
    }

    int sizeOfElement = std::get<0>(sizeData);
    int maxBlockSize = std::get<1>(sizeData);
    int maxNumberOfElements = maxBlockSize / sizeOfElement;

    int rest = size * sizeOfElement;

    if (!ofileH.is_open()) {
        OPM_THROW(std::runtime_error,"fstream fileH not open for writing");
    }

    while (rest > 0) {
        if (rest > maxBlockSize) {
            rest -= maxBlockSize;
            num = maxNumberOfElements;
        } else {
            num = rest / sizeOfElement;
            rest = 0;
        }

        dhead = flipEndianInt(num * sizeOfElement);

        ofileH.write(reinterpret_cast<char*>(&dhead), sizeof(dhead));

        for (int i = 0; i < num; ++i) {
            std::string tmpStr = data[n] + std::string(sizeOfElement - data[n].size(),' ');
            ofileH.write(tmpStr.c_str(), sizeOfElement);
            ++n;
        }

        ofileH.write(reinterpret_cast<char*>(&dhead), sizeof(dhead));
    }
}

void EclOutput::writeBinaryCharArray(const std::vector<PaddedOutputString<8>>& data)
{
    const auto size = data.size();

    const auto sizeData = block_size_data_binary(CHAR);

    const int sizeOfElement       = std::get<0>(sizeData);
    const int maxBlockSize        = std::get<1>(sizeData);
    const int maxNumberOfElements = maxBlockSize / sizeOfElement;

    int rest = size * sizeOfElement;

    if (!ofileH.is_open()) {
        OPM_THROW(std::runtime_error,"fstream fileH not open for writing");
    }

    auto elm = data.begin();
    while (rest > 0) {
        const auto numElm = (rest > maxBlockSize)
            ? maxNumberOfElements
            : rest / sizeOfElement;

        rest = (rest > maxBlockSize) ? rest - maxBlockSize : 0;

        auto dhead = flipEndianInt(numElm * sizeOfElement);

        ofileH.write(reinterpret_cast<char*>(&dhead), sizeof(dhead));

        for (auto i = 0*numElm; i < numElm; ++i, ++elm) {
            ofileH.write(elm->c_str(), sizeOfElement);
        }

        ofileH.write(reinterpret_cast<char*>(&dhead), sizeof(dhead));
    }
}

void EclOutput::writeFormattedHeader(const std::string& arrName, int size, eclArrType arrType, int element_size)
{
    std::string name = arrName + std::string(8 - arrName.size(),' ');

    ofileH << " '" << name << "' " << std::setw(11) << size;

    std::string c0nn_str;

    if (arrType == C0NN){
        std::ostringstream ss;
        ss << "C" << std::setw(3) << std::setfill('0') << element_size;
        c0nn_str = ss.str();
    }


    switch (arrType) {
    case INTE:
        ofileH << " 'INTE'" <<  std::endl;
        break;
    case REAL:
        ofileH << " 'REAL'" <<  std::endl;
        break;
    case DOUB:
        ofileH << " 'DOUB'" <<  std::endl;
        break;
    case LOGI:
        ofileH << " 'LOGI'" <<  std::endl;
        break;
    case CHAR:
        ofileH << " 'CHAR'" <<  std::endl;
        break;
    case C0NN:
        ofileH << " '" << c0nn_str << "'" <<  std::endl;
        break;
    case MESS:
        ofileH << " 'MESS'" <<  std::endl;
        break;
    }
}


std::string EclOutput::make_real_string_ecl(float value) const
{
    constexpr std::size_t buf_size = 15;
    char buffer [buf_size];
    std::snprintf (buffer, sizeof buffer, "%10.7E", value);

    if (value == 0.0) {
        return "0.00000000E+00";
    } else {
        if (std::isnan(value)) {
            return "NAN";
        }

        if (std::isinf(value)) {
            if (value > 0) {
                return "INF";
            }
            else {
                return "-INF";
            }
        }

        std::string tmpstr(buffer);

        int exp =  value < 0.0 ? std::stoi(tmpstr.substr(11, 3)) :  std::stoi(tmpstr.substr(10, 3));

        if (value < 0.0) {
            tmpstr = "-0." + tmpstr.substr(1, 1) + tmpstr.substr(3, 7) + "E";
        } else {
            tmpstr = "0." + tmpstr.substr(0, 1) + tmpstr.substr(2, 7) +"E";
        }

        std::snprintf (buffer, sizeof buffer, "%+03i", exp+1);
        tmpstr = tmpstr+buffer;

        return tmpstr;
    }
}

std::string EclOutput::make_real_string_ix(float value) const
{
    constexpr std::size_t buf_size = 15;
    char buffer [buf_size];
    std::snprintf (buffer, sizeof buffer, "%10.7E", value);

    if (value == 0.0) {
        return " 0.0000000E+00";
    } else {
        if (std::isnan(value)) {
            return "NAN";
        }

        if (std::isinf(value)) {
            if (value > 0) {
                return "INF";
            }
            else {
                return "-INF";
            }
        }

        std::string tmpstr(buffer);

        return tmpstr;
    }
}


std::string EclOutput::make_doub_string_ecl(double value) const
{
    char buffer [21 + 1];
    std::snprintf (buffer, sizeof buffer, "%19.13E", value);

    if (value == 0.0) {
        return "0.00000000000000D+00";
    } else {
        if (std::isnan(value)) {
            return "NAN";
        }

        if (std::isinf(value)) {
            if (value > 0) {
                return "INF";
            }
            else {
                return "-INF";
            }
        }

        std::string tmpstr(buffer);
        int exp = value < 0.0 ? std::stoi(tmpstr.substr(17, 4)) : std::stoi(tmpstr.substr(16, 4));
        const bool use_exp_char = (exp >= -100) && (exp < 99);

        if (value < 0.0) {
            if (use_exp_char) {
                tmpstr = "-0." + tmpstr.substr(1, 1) + tmpstr.substr(3, 13) + "D";
            } else {
                tmpstr = "-0." + tmpstr.substr(1, 1) + tmpstr.substr(3, 13);
            }
        } else {
            if (use_exp_char) {
                tmpstr = "0." + tmpstr.substr(0, 1) + tmpstr.substr(2, 13) + "D";
            } else {
                tmpstr = "0." + tmpstr.substr(0, 1) + tmpstr.substr(2, 13);
            }
        }

        std::snprintf(buffer, sizeof buffer, "%+03i", exp + 1);
        tmpstr = tmpstr + buffer;
        return tmpstr;
    }
}

std::string EclOutput::make_doub_string_ix(double value) const
{
    char buffer [21];
    std::snprintf (buffer, sizeof buffer, "%19.13E", value);

    if (value == 0.0) {
        return " 0.0000000000000E+00";
    } else {
        if (std::isnan(value)) {
            return "NAN";
        }

        if (std::isinf(value)) {
            if (value > 0) {
                return "INF";
            }
            else {
                return "-INF";
            }
        }

        std::string tmpstr(buffer);

        return tmpstr;
    }
}


template <typename T>
void EclOutput::writeFormattedArray(const std::vector<T>& data)
{
    int size = data.size();
    int n = 0;

    eclArrType arrType = MESS;
    if constexpr (std::is_same_v<T, int>) {
        arrType = INTE;
    } else if constexpr (std::is_same_v<T, float>) {
        arrType = REAL;
    } else if constexpr (std::is_same_v<T, double>) {
        arrType = DOUB;
    } else if constexpr (std::is_same_v<T, bool>) {
        arrType = LOGI;
    }

    auto sizeData = block_size_data_formatted(arrType);

    int maxBlockSize = std::get<0>(sizeData);
    int nColumns = std::get<1>(sizeData);
    int columnWidth = std::get<2>(sizeData);

    for (int i = 0; i < size; ++i) {
        ++n;

        switch (arrType) {
        case INTE:
            ofileH << std::setw(columnWidth) << data[i];
            break;
        case REAL:
            if (ix_standard) {
                ofileH << std::setw(columnWidth) << make_real_string_ix(data[i]);
            }
            else {
                ofileH << std::setw(columnWidth) << make_real_string_ecl(data[i]);
            }
            break;
        case DOUB:
            if (ix_standard) {
                ofileH << std::setw(columnWidth) << make_doub_string_ix(data[i]);
            }
            else {
                ofileH << std::setw(columnWidth) << make_doub_string_ecl(data[i]);
            }
            break;
        case LOGI:
            if (data[i]) {
                ofileH << "  T";
            }
            else {
                ofileH << "  F";
            }
            break;
        default:
            break;
        }

        if ((n % nColumns) == 0 || (n % maxBlockSize) == 0) {
            ofileH << std::endl;
        }

        if ((n % maxBlockSize) == 0) {
            n=0;
        }
    }

    if ((n % nColumns) != 0 && (n % maxBlockSize) != 0) {
        ofileH << std::endl;
    }
}


template void EclOutput::writeFormattedArray<int>(const std::vector<int>& data);
template void EclOutput::writeFormattedArray<float>(const std::vector<float>& data);
template void EclOutput::writeFormattedArray<double>(const std::vector<double>& data);
template void EclOutput::writeFormattedArray<bool>(const std::vector<bool>& data);
template void EclOutput::writeFormattedArray<char>(const std::vector<char>& data);


void EclOutput::writeFormattedCharArray(const std::vector<std::string>& data, int element_size)
{
    auto sizeData = block_size_data_formatted(CHAR);
    int maxBlockSize = std::get<0>(sizeData);

    int nColumns;

    if (element_size < 9) {
        element_size = 8;
        nColumns = std::get<1>(sizeData);
    }
    else {
        nColumns = 80 / (element_size + 3);
    }

    int rest = data.size();
    int n = 0;

    while (rest > 0) {
        int size = rest;

        if (size > maxBlockSize) {
            size = maxBlockSize;
        }

        for (int i = 0; i < size; ++i) {
            std::string str1(element_size,' ');
            str1 = data[n] + std::string(element_size - data[n].size(),' ');

            ++n;
            ofileH << " '" << str1 << "'";

            if ((i+1) % nColumns == 0) {
                ofileH  << std::endl;
            }
        }

        if ((size % nColumns) != 0) {
            ofileH  << std::endl;
        }

        rest = (rest > maxBlockSize) ? rest - maxBlockSize : 0;
    }
}


void EclOutput::writeFormattedCharArray(const std::vector<PaddedOutputString<8>>& data)
{
    const auto sizeData = block_size_data_formatted(CHAR);

    const int nColumns = std::get<1>(sizeData);

    const auto size = data.size();

    for (auto i = 0*size; i < size; ++i) {
        ofileH << " '" << data[i].c_str() << '\'';

        if ((i+1) % nColumns == 0) {
            ofileH << '\n';
        }
    }

    if ((size % nColumns) != 0) {
        ofileH << '\n';
    }
}

}} // namespace Opm::EclIO
