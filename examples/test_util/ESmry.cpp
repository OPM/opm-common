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


#include <string>
#include <string.h>
#include <sstream>
#include <iterator>
#include <iomanip>
#include <algorithm>

#include "EclFile.hpp"
#include "ESmry.hpp"

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


const bool ESmry::hasKey(const std::string &key) const {

    auto it=std::find(keyword.begin(),keyword.end(),key);

    if (it!=keyword.end()) {
        return true;
    } else {
        return false;
    }
}

void ESmry::ijk_from_global_index(int glob,int &i,int &j,int &k) {

    int tmpGlob=glob-1;

    k=tmpGlob/(nI*nJ)+1;
    int rest=tmpGlob % (nI*nJ);

    j=rest/(nI)+1;
    i=rest % (nI)+1;
}

const std::string ESmry::makeKeyString(const std::string &keyword, const std::string &wgname, const int &num) {

    std::string keyStr;
    std::vector<std::string> segmExcep= {"STEPTYPE", "SEPARATE", "SUMTHIN"};

    if (keyword.substr(0,1)=="A") {
        keyStr=keyword+":"+std::to_string(num);
    } else if (keyword.substr(0,1)=="B") {
        int _i,_j,_k;

        ijk_from_global_index(num,_i,_j,_k);

        keyStr=keyword+":"+std::to_string(_i)+","+std::to_string(_j)+","+std::to_string(_k);

    } else if (keyword.substr(0,1)=="C") {

        int _i,_j,_k;

        if (num>0) {
            ijk_from_global_index(num,_i,_j,_k);
            keyStr=keyword+":"+wgname+":"+std::to_string(_i)+","+std::to_string(_j)+","+std::to_string(_k);
        } else {
            keyStr="";
        }

    } else if (keyword.substr(0,1)=="G") {

        if (wgname!=":+:+:+:+") {
            keyStr=keyword+":"+wgname;
        } else {
            keyStr="";
        }

    } else if ((keyword.substr(0,1)=="R") && (keyword.substr(2,1)=="F")) {

        // NUMS = R1 + 32768*(R2 + 10)

        int r2=0;
        int y=32768*(r2+10)-num;

        while (y<0) {
            r2++;
            y=32768*(r2+10)-num;
        }

        r2--;
        int r1=num-32768*(r2+10);

        keyStr=keyword+":"+std::to_string(r1)+"-"+std::to_string(r2);

    } else if (keyword.substr(0,1)=="R") {

        keyStr=keyword+":"+std::to_string(num);

    } else if (keyword.substr(0,1)=="S") {

        std::vector<std::string>::iterator it = std::find(segmExcep.begin(),segmExcep.end(),keyword);

        if (it!=segmExcep.end()) {
            keyStr=keyword;
        } else {
            keyStr=keyword+":"+wgname+":"+std::to_string(num);
        }

    } else if (keyword.substr(0,1)=="W") {

        if (wgname!=":+:+:+:+") {
            keyStr=keyword+":"+wgname;
        } else {
            keyStr="";
        }

    } else {
        keyStr=keyword;
    }

    return std::move(keyStr);
}

const std::vector<float> &ESmry::get(const std::string &name) const {

    auto it=find(keyword.begin(),keyword.end(),name);

    if (it==keyword.end()) {
        std::string message="keyword " + name + " not found ";
        throw std::invalid_argument(message);
    }

    int ind=distance(keyword.begin(),it);

    return param[ind];
}


ESmry::ESmry(const std::string &filename)
{
    std::string rootN;
    std::vector<int> actInd;
    bool formatted=false;

    std::string smspec_filen;
    std::string unsmry_filen;

    if (filename.substr(filename.length()-7,7)==".SMSPEC") {
        rootN=filename.substr(0,filename.length()-7);
    } else if (filename.substr(filename.length()-8,8)==".FSMSPEC") {
        rootN=filename.substr(0,filename.length()-8);
        formatted=true;
    } else {
        rootN=filename;
    }

    if (formatted){
        smspec_filen=rootN+".FSMSPEC";
        unsmry_filen=rootN+".FUNSMRY";
    } else {
        smspec_filen=rootN+".SMSPEC";
        unsmry_filen=rootN+".UNSMRY";
    }

    EclFile smspec(smspec_filen);

    smspec.loadData();   // loading all data

    std::vector<int> dimens=smspec.get<int>("DIMENS");

    nVect=dimens[0];

    actInd.reserve(nVect);
    keyword.reserve(nVect);

    nI=dimens[1];
    nJ=dimens[2];
    nK=dimens[3];

    const std::vector<std::string> keywords=smspec.get<std::string>("KEYWORDS");
    const std::vector<std::string> wgnames=smspec.get<std::string>("WGNAMES");
    const std::vector<int> nums=smspec.get<int>("NUMS");
    const std::vector<std::string> units=smspec.get<std::string>("UNITS");

    for (int i=0; i<nVect; i++) {
        std::string keywStr=makeKeyString(keywords[i], wgnames[i] , nums[i]);

        if ((keywStr!="") && (hasKey(keywStr)!=true)) {
            keyword.push_back(keywStr);
            actInd.push_back(i);
        }
    }

    int nAct=actInd.size();

    param.reserve(nAct);

    for (int i=0; i<nAct; i++) {
        param.push_back({});
    }

    EclFile unsmry(unsmry_filen);
    unsmry.loadData();

    std::vector<std::tuple<std::string, EIOD::eclArrType, int >> list1 = unsmry.getList();
    float time=0.0;
    int step=0;

    for (unsigned int i=0; i<list1.size(); i++) {
        std::string name=std::get<0>(list1[i]);

        if (name=="SEQHDR") {
            seqTime.push_back(time);
            seqIndex.push_back(step);
        } else if (name=="PARAMS") {
            std::vector<float> data=unsmry.get<float>(i);

            for (int j=0; j<nAct; j++) {
                param[j].push_back(data[actInd[j]]);
            }
            step++;
        }
    }
};




