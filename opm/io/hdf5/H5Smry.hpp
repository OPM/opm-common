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

#ifndef OPM_IO_H5SMRY_HPP
#define OPM_IO_H5SMRY_HPP

#include <chrono>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <map>
#include <stdint.h>

#include <opm/common/utility/FileSystem.hpp>
#include <opm/common/utility/TimeService.hpp>

namespace Opm { namespace Hdf5IO {


class H5Smry
{
public:

    explicit H5Smry(const std::string& filename);

    int numberOfVectors() const { return nVect; }

    bool hasKey(const std::string& key) const;

    const std::vector<float>& get(const std::string& name) const;

    std::vector<float> get_at_rstep(const std::string& name) const;

    void LoadData() const;
    void LoadData(const std::vector<std::string>& vectList) const;


    std::chrono::system_clock::time_point startdate() const { return startdat; }

    const std::vector<std::string>& keywordList() const {return keyword; };

    std::vector<std::string> keywordList(const std::string& pattern) const;

    int timestepIdxAtReportstepStart(const int reportStep) const;

    size_t numberOfTimeSteps() const { return nTstep; }

    const std::string& get_unit(const std::string& name) const;


    template <typename T>
    std::vector<T> rstep_vector(const std::vector<T>& full_vector) const {
        std::vector<T> result;
        result.reserve(seqIndex.size());

        for (const auto& ind : seqIndex){
            result.push_back(full_vector[ind]);
        }

        return result;
    }

private:
    Opm::filesystem::path inputFileName;
    size_t nVect, nTstep;
    std::chrono::system_clock::time_point startdat;

    Opm::TimeStampUTC start_ts;

    mutable std::vector<std::vector<float>> vectorData;
    mutable std::vector<bool> vectorLoaded;

    std::vector<std::string> keyword;

    std::vector<int> seqIndex;

    std::unordered_map<std::string, std::string> keyUnits;
    std::unordered_map<std::string, size_t> keyIndex;

};

}} // namespace Opm::Hdf5IO


#endif // OPM_IO_H5SMRY_HPP
