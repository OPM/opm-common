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

#ifndef ECLFILE_HPP
#define ECLFILE_HPP

#include <opm/common/ErrorMacros.hpp>
#include <examples/test_util/data/EclIOdata.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <ctime>
#include <map>
#include <unordered_map>
#include <stdio.h>

namespace EIOD = Opm::ecl;

class EclFile
{
public:
    explicit EclFile(const std::string& filename);
    bool formattedInput() { return formatted; }

    void loadData();                            // load all data
    void loadData(int arrIndex);                // load data based on array indices in vector arrIndex
    void loadData(const std::vector<int>& arrIndex);   // load data based on array indices in vector arrIndex
    void loadData(const std::string& arrName);         // load all arrays with array name equal to arrName

    using EclEntry = std::tuple<std::string, EIOD::eclArrType, int>;
    std::vector<EclEntry> getList() const;

    template <typename T>
    const std::vector<T>& get(int arrIndex) const;

    template <typename T>
    const std::vector<T>& get(const std::string& name) const;

    bool hasKey(const std::string &name) const;

protected:
    bool formatted;
    std::string inputFilename;

    std::unordered_map<int, std::vector<int>> inte_array;
    std::unordered_map<int, std::vector<bool>> logi_array;
    std::unordered_map<int, std::vector<double>> doub_array;
    std::unordered_map<int, std::vector<float>> real_array;
    std::unordered_map<int, std::vector<std::string>> char_array;

    std::vector<std::string> array_name;
    std::vector<EIOD::eclArrType> array_type;
    std::vector<int> array_size;

    std::vector<unsigned long int> ifStreamPos;

    std::map<std::string, int> array_index;

    template<class T>
    const std::vector<T>& getImpl(int arrIndex, EIOD::eclArrType type,
                                  const std::unordered_map<int, std::vector<T>>& array,
                                  const std::string& typeStr) const
    {
        if (array_type[arrIndex] != type) {
            std::string message = "Array with index " + std::to_string(arrIndex) + " is not of type " + typeStr;
            OPM_THROW(std::runtime_error, message);
        }

        checkIfLoaded(arrIndex);

        return array.find(arrIndex)->second;
    }

private:
    std::vector<bool> arrayLoaded;

    void checkIfLoaded(int arrIndex) const;
    void loadArray(std::fstream& fileH, int arrIndex);
};

#endif
