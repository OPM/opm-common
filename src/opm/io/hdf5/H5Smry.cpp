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

#include <opm/io/hdf5/H5Smry.hpp>

#include <opm/common/utility/FileSystem.hpp>
#include <opm/common/utility/TimeService.hpp>
#include <opm/common/ErrorMacros.hpp>

#include <opm/io/hdf5/Hdf5Util.hpp>

#include <algorithm>
#include <chrono>
#include <exception>
#include <iterator>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <fnmatch.h>
#include <fstream>
#include <cmath>
#include <cstring>

#include <iostream>

/*

     KEYWORDS       WGNAMES        NUMS              |   PARAM index   Corresponding ERT key
     ------------------------------------------------+--------------------------------------------------
     WGOR           OP_1           0                 |        0        WGOR:OP_1
     FOPT           +-+-+-+-       0                 |        1        FOPT
     WWCT           OP_1           0                 |        2        WWCT:OP_1
     WIR            OP_1           0                 |        3        WIR:OP_1
     WGOR           WI_1           0                 |        4        WWCT:OP_1
     WWCT           W1_1           0                 |        5        WWCT:WI_1
     BPR            +-+-+-         12675             |        6        BPR:12675, BPR:i,j,k
     RPR            +-+-+-         1                 |        7        RPR:1
     FOPT           +-+-+-         0                 |        8        FOPT
     GGPR           NORTH          0                 |        9        GGPR:NORTH
     COPR           OP_1           5628              |       10        COPR:OP_1:56286, COPR:OP_1:i,j,k
     RXF            +-+-+-         32768*R1(R2 + 10) |       11        RXF:2-3
     SOFX           OP_1           12675             |       12        SOFX:OP_1:12675, SOFX:OP_1:i,j,jk

*/

namespace {

std::chrono::system_clock::time_point make_date(const std::vector<int>& datetime) {
    auto day = datetime[0];
    auto month = datetime[1];
    auto year = datetime[2];
    auto hour = 0;
    auto minute = 0;
    auto second = 0;

    if (datetime.size() == 6) {
        hour = datetime[3];
        minute = datetime[4];
        auto total_usec = datetime[5];
        second = total_usec / 1000000;
    }

    const auto ts = Opm::TimeStampUTC{ Opm::TimeStampUTC::YMD{ year, month, day}}.hour(hour).minutes(minute).seconds(second);

    return std::chrono::system_clock::from_time_t( Opm::asTimeT(ts) );
}


}


namespace Opm { namespace Hdf5IO {

H5Smry::H5Smry(const std::string &filename)
{
    inputFileName = filename;

    hid_t file_id = H5Fopen( inputFileName.c_str(), H5F_ACC_RDONLY | H5F_ACC_SWMR_READ, H5P_DEFAULT);

    auto startd = Opm::Hdf5IO::get_1d_hdf5<int>(file_id, "START_DATE");

    this->startdat = make_date(startd);

    this->start_ts = Opm::TimeStampUTC{ Opm::TimeStampUTC::YMD{ startd[2], startd[1], startd[0]}}.hour(startd[3]).minutes(startd[4]).seconds(startd[5]).microseconds(startd[6]);

    auto rstep = Opm::Hdf5IO::get_1d_hdf5<int>(file_id, "RSTEP");

    auto it = std::find(rstep.begin(), rstep.end(), -1);

    if (it != rstep.end()){
        size_t length = std::distance(rstep.begin(), it);
        rstep.resize(length);
    }

    nTstep = rstep.size();

    seqIndex.reserve(rstep.size());

    for (size_t ind=0; ind < rstep.size(); ind++)
        if (rstep[ind] == 1)
            seqIndex.push_back(ind);

    keyword = Opm::Hdf5IO::get_1d_hdf5<std::string>(file_id, "KEYS");

    nVect = keyword.size();

    vectorData.reserve(nVect);

    auto units = Opm::Hdf5IO::get_1d_hdf5<std::string>(file_id, "UNITS");

    if (units.size() != nVect)
        throw std::invalid_argument("size of units and keyword in h5smry file not equal");

    H5Fclose(file_id);

    for (size_t n=0; n < keyword.size(); n++){
        keyIndex[keyword[n]] = n;
        keyUnits[keyword[n]] = units[n];
    }

    vectorLoaded.resize(nVect, false);
}


bool H5Smry::hasKey(const std::string &key) const
{
    return std::find(keyword.begin(), keyword.end(), key) != keyword.end();
}


void H5Smry::LoadData() const
{
    hid_t file_id = H5Fopen( inputFileName.c_str(), H5F_ACC_RDONLY|H5F_ACC_SWMR_READ, H5P_DEFAULT);

    vectorData = Opm::Hdf5IO::get_2d_hdf5<float>(file_id, "SMRYDATA", nTstep);

    H5Fclose(file_id);

    for (size_t n=0; n < nVect; n++)
        vectorLoaded[n] = true;
}


void H5Smry::LoadData(const std::vector<std::string>& vectList) const
{
    hid_t fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    hid_t file_id = H5Fopen( inputFileName.c_str(), H5F_ACC_RDONLY|H5F_ACC_SWMR_READ, fapl_id);

    for (auto key : vectList){
        if (!hasKey(key))
            OPM_THROW(std::invalid_argument, "error loading key " + key );

        auto it = keyIndex.find(key);

        vectorData[it->second] = Opm::Hdf5IO::get_1d_from_2d_hdf5<float>(file_id, "SMRYDATA", it->second, nTstep);
        vectorLoaded[it->second] = true;
    }

    H5Fclose(file_id);
}


const std::vector<float>& H5Smry::get(const std::string& name) const
{
    if (!hasKey(name))
        OPM_THROW(std::invalid_argument, "keyword " + name + " not found");

    auto it = keyIndex.find(name);

    if (!vectorLoaded[it->second]){
        LoadData({name});
        vectorLoaded[it->second]=true;
    }

    return vectorData[it->second];
}


std::vector<float> H5Smry::get_at_rstep(const std::string& name) const
{
    return this->rstep_vector( this->get(name) );
}

const std::string& H5Smry::get_unit(const std::string& name) const
{
    if (!hasKey(name))
        OPM_THROW(std::invalid_argument, "keyword " + name + " not found");

    auto it = keyUnits.find(name);

    return it->second;
}


int H5Smry::timestepIdxAtReportstepStart(const int reportStep) const
{
    const auto nReport = static_cast<int>(seqIndex.size());

    if ((reportStep < 1) || (reportStep > nReport)) {
        throw std::invalid_argument {
            "Report step " + std::to_string(reportStep)
            + " outside valid range 1 .. " + std::to_string(nReport)
        };
    }

    return seqIndex[reportStep - 1];
}


std::vector<std::string> H5Smry::keywordList(const std::string& pattern) const
{
    std::vector<std::string> list;

    for (auto key : keyword)
        if (fnmatch( pattern.c_str(), key.c_str(), 0 ) == 0 )
            list.push_back(key);

    return list;
}



}} // namespace Opm::Hdf5IO
