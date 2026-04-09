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
#ifndef OPM_IO_ECLOUTPUT_HPP
#define OPM_IO_ECLOUTPUT_HPP

#include <opm/io/eclipse/EclIOdata.hpp>
#include <opm/io/eclipse/PaddedOutputString.hpp>

#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace Opm::EclIO::OutputStream {
    class Restart;
    class SummarySpecification;
} // namespace Opm::EclIO::OutputStream

namespace Opm::EclIO {

class EclOutput
{
public:
    EclOutput(const std::string&            filename,
              const bool                    formatted,
              const std::ios_base::openmode mode = std::ios::out);

    template<typename T>
    void write(const std::string& name,
               const std::vector<T>& data)
    {
        static_assert(std::is_same_v<T, int>    ||
                      std::is_same_v<T, float>  ||
                      std::is_same_v<T, double> ||
                      std::is_same_v<T, bool>   ||
                      std::is_same_v<T, char>,
                      "EclOutput::write<T>: T must be int, float, double, bool, or char");

        eclArrType arrType = MESS;
        int element_size = 4;

        if constexpr (std::is_same_v<T, int>) {
            arrType = INTE;
        }
        else if constexpr (std::is_same_v<T, float>) {
            arrType = REAL;
        }
        else if constexpr (std::is_same_v<T, double>) {
            arrType = DOUB;
            element_size = 8;
        }
        else if constexpr (std::is_same_v<T, bool>) {
            arrType = LOGI;
        }
        else if constexpr (std::is_same_v<T, char>) {
            if (!data.empty()) {
                throw std::invalid_argument {
                    "EclOutput::write<char>: non-empty data is not supported; "
                    "use message() for MESS-type records"
                };
            }
        }

        if (isFormatted) {
            writeFormattedHeader(name, data.size(), arrType, element_size);
            if (arrType != MESS) {
                writeFormattedArray(data);
            }
        }
        else {
            writeBinaryHeader(name, data.size(), arrType, element_size);
            if (arrType != MESS) {
                writeBinaryArray(data);
            }
        }
    }

    // when this function is used array type will be assumed C0NN (not CHAR).
    // Also in cases where element size is 8 or less, element size will be 8.

    void write(const std::string& name, const std::vector<std::string>& data, int element_size);

    void message(const std::string& msg);
    void flushStream();

    void set_ix() { ix_standard = true; }

    friend class OutputStream::Restart;
    friend class OutputStream::SummarySpecification;

private:
    void writeBinaryHeader(const std::string& arrName, int64_t size, eclArrType arrType, int element_size);

    template <typename T>
    void writeBinaryArray(const std::vector<T>& data);

    void writeBinaryCharArray(const std::vector<std::string>& data, int element_size);
    void writeBinaryCharArray(const std::vector<PaddedOutputString<8>>& data);

    void writeFormattedHeader(const std::string& arrName, int size, eclArrType arrType, int element_size);

    template <typename T>
    void writeFormattedArray(const std::vector<T>& data);

    void writeFormattedCharArray(const std::vector<std::string>& data, int element_size);
    void writeFormattedCharArray(const std::vector<PaddedOutputString<8>>& data);

    void writeArrayType(const eclArrType arrType);
    std::string make_real_string_ecl(float value) const;
    std::string make_real_string_ix(float value) const;
    std::string make_doub_string_ecl(double value) const;
    std::string make_doub_string_ix(double value) const;

    bool isFormatted, ix_standard;
    std::ofstream ofileH;
};


template<>
void EclOutput::write<std::string>(const std::string& name,
                                   const std::vector<std::string>& data);

template <>
void EclOutput::write<PaddedOutputString<8>>
    (const std::string&                        name,
     const std::vector<PaddedOutputString<8>>& data);

} // namespace Opm::EclIO

#endif // OPM_IO_ECLOUTPUT_HPP
