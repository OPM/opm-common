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
#ifndef ECL_OUTPUT_HPP
#define ECL_OUTPUT_HPP

#include <fstream>
#include <ios>
#include <string>
#include <typeinfo>
#include <vector>

#include <opm/output/eclipse/FileService/EclIOdata.hpp>

#include <opm/output/eclipse/CharArrayNullTerm.hpp>

namespace EIOD = Opm::ecl;


class EclOutput
{
public:
    EclOutput(const std::string& inputFile, bool formatted);

    template<typename T>
    void write(const std::string& name,
               const std::vector<T>& data)
    {
        EIOD::eclArrType arrType = EIOD::MESS;
        if (typeid(T) == typeid(int))
            arrType = EIOD::INTE;
        else if (typeid(T) == typeid(float))
            arrType = EIOD::REAL;
        else if (typeid(T) == typeid(double))
            arrType = EIOD::DOUB;
        else if (typeid(T) == typeid(bool))
            arrType = EIOD::LOGI;
        else if (typeid(T) == typeid(char))
            arrType = EIOD::MESS;

        if (isFormatted)
        {
            writeFormattedHeader(name, data.size(), arrType);
            if (arrType != EIOD::MESS)
                writeFormattedArray(data);
        }
        else
        {
            writeBinaryHeader(name, data.size(), arrType);
            if (arrType != EIOD::MESS)
                writeBinaryArray(data);
        }
    }

private:
    void writeBinaryHeader(const std::string& arrName, int size, EIOD::eclArrType arrType);

    template <typename T>
    void writeBinaryArray(const std::vector<T>& data);

    void writeBinaryCharArray(const std::vector<std::string>& data);
    void writeBinaryCharArray(const std::vector<Opm::RestartIO::Helpers::CharArrayNullTerm<8>>& data);

    void writeFormattedHeader(const std::string& arrName, int size, EIOD::eclArrType arrType);

    template <typename T>
    void writeFormattedArray(const std::vector<T>& data);

    void writeFormattedCharArray(const std::vector<std::string>& data);
    void writeFormattedCharArray(const std::vector<Opm::RestartIO::Helpers::CharArrayNullTerm<8>>& data);

    std::string make_real_string(float value) const;
    std::string make_doub_string(double value) const;

    std::ofstream ofileH;
    bool isFormatted;
};


template<>
void EclOutput::write<std::string>(const std::string& name,
                                   const std::vector<std::string>& data);

template <>
void EclOutput::write<Opm::RestartIO::Helpers::CharArrayNullTerm<8>>
    (const std::string&                                                name,
     const std::vector<Opm::RestartIO::Helpers::CharArrayNullTerm<8>>& data);

#endif
