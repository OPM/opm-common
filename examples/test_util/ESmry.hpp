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

#ifndef ESMRY_HPP
#define ESMRY_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <ctime>
#include <map>

class ESmry
{

private:
    int nVect,nI,nJ,nK;

    void ijk_from_global_index(int glob,int &i,int &j,int &k);
    std::vector<std::vector<float> > param;
    std::vector<std::string> keyword;

    std::vector<int> seqIndex;
    std::vector<float> seqTime;

    const std::string makeKeyString(const std::string &keyword, const std::string &wgname, const int &num);


public:

    const int numberOfVectors() const { return nVect;};

    const bool hasKey(const std::string &key) const;

    const std::vector<float> &get(const std::string &name) const;

    const std::vector<std::string> keywordList() const {return keyword;};

    ESmry(const std::string &filename);   // filename (smspec file) or file root name

};

#endif


