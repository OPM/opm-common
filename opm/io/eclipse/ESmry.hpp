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

#ifndef OPM_IO_ESMRY_HPP
#define OPM_IO_ESMRY_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <opm/common/utility/FileSystem.hpp>

namespace Opm { namespace EclIO {

class ESmry
{
public:

    // input is smspec (or fsmspec file)
    explicit ESmry(const std::string& filename, bool loadBaseRunData=false);

    int numberOfVectors() const { return nVect; }

    bool hasKey(const std::string& key) const;

    const std::vector<float>& get(const std::string& name) const;

    std::vector<float> get_at_rstep(const std::string& name) const;

    const std::vector<std::string>& keywordList() const { return keyword; }

    int timestepIdxAtReportstepStart(const int reportStep) const;

    const std::string& get_unit(const std::string& name) const;

private:
    int nVect, nI, nJ, nK;

    void ijk_from_global_index(int glob, int &i, int &j, int &k) const;
    std::vector<std::vector<float>> param;
    std::vector<std::string> keyword;
    std::unordered_map<std::string, std::string> kwunits;

    std::vector<int> seqIndex;
    std::vector<float> seqTime;

    std::vector<std::string> checkForMultipleResultFiles(const Opm::filesystem::path& rootN, bool formatted) const;

    void getRstString(const std::vector<std::string>& restartArray,
                      Opm::filesystem::path& pathRst,
                      Opm::filesystem::path& rootN) const;

    void updatePathAndRootName(Opm::filesystem::path& dir, Opm::filesystem::path& rootN) const;

    std::string makeKeyString(const std::string& keyword, const std::string& wgname, int num) const;
};

}} // namespace Opm::EclIO

#endif // OPM_IO_ESMRY_HPP
