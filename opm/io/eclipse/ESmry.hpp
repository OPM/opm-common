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
#include <vector>

namespace Opm { namespace EclIO {

class ESmry
{
public:
    explicit ESmry(const std::string& filename, bool loadBaseRunData=false);   // filename (smspec file) or file root name

    int numberOfVectors() const { return nVect; }

    bool hasKey(const std::string& key) const;

    const std::vector<float>& get(const std::string& name) const;

    std::vector<float> get_at_rstep(const std::string& name) const;

    const std::vector<std::string>& keywordList() const { return keyword; }

    int timestepIdxAtReportstepStart(const int reportStep) const;

private:
    int nVect, nI, nJ, nK;
    std::string path="";

    void ijk_from_global_index(int glob, int &i, int &j, int &k) const;
    std::vector<std::vector<float>> param;
    std::vector<std::string> keyword;

    std::vector<int> seqIndex;
    std::vector<float> seqTime;

    void getRstString(const std::vector<std::string> &restartArray, std::string &path, std::string &rootN) const;
    void updatePathAndRootName(std::string &path, std::string &rootN) const;

    std::string makeKeyString(const std::string& keyword, const std::string& wgname, int num) const;
};

}} // namespace Opm::EclIO

#endif // OPM_IO_ESMRY_HPP
