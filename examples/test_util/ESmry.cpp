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
#include <unistd.h>
#include <limits>
#include <set>

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

ESmry::ESmry(const std::string &filename, bool loadBaseRunData)
{
    std::string rootN;
    std::vector<int> actInd;
    bool formatted=false;
    
    char buff[1000];
    getcwd( buff, 1000 );

    std::string currentWorkingDir(buff);
    std::string currentDir=currentWorkingDir;

    std::string smspec_filen;
    std::string unsmry_filen;

    if (filename.substr(filename.length() - 7, 7) == ".SMSPEC") {
        rootN = filename.substr(0,filename.length() -7);
    } else if (filename.substr(filename.length() -8, 8) == ".FSMSPEC") {
        rootN=filename.substr(0,filename.length() -8);
        formatted = true;
    } else {
        rootN = filename;
    }

    path = currentWorkingDir;
    updatePathAndRootName(path, rootN);

    if (formatted) {
        smspec_filen = path + "/" + rootN + ".FSMSPEC";
        unsmry_filen = path + "/" + rootN + ".FUNSMRY";
    } else {
        smspec_filen = path + "/" + rootN + ".SMSPEC";
        unsmry_filen = path + "/" + rootN + ".UNSMRY";
    }

    std::vector<std::pair<std::string,int>> smryArray;

    EclFile smspec1(smspec_filen);

    smspec1.loadData();   // loading all data

    std::set<std::string> keywList;

    std::vector<int> dimens = smspec1.get<int>("DIMENS");

    nI = dimens[1];
    nJ = dimens[2];
    nK = dimens[3];

    std::vector<std::string> restartArray = smspec1.get<std::string>("RESTART");
    std::vector<std::string> keywords = smspec1.get<std::string>("KEYWORDS");
    std::vector<std::string> wgnames = smspec1.get<std::string>("WGNAMES");
    std::vector<int> nums = smspec1.get<int>("NUMS");

    for (unsigned int i=0; i<keywords.size(); i++) {
        std::string str1 = makeKeyString(keywords[i], wgnames[i], nums[i]);
        if (str1.length() > 0) {
            keywList.insert(str1);
        }
    }

    std::string rstRootN = "";
    std::string pathRstFile = path;

    getRstString(restartArray, pathRstFile, rstRootN);

    smryArray.push_back({smspec_filen, dimens[5]});

    // checking if this is a restart run. Supporting nested restarts (restart, from restart, ...)
    // std::set keywList is storing keywords from all runs involved
    
    while ((rstRootN != "") && (loadBaseRunData)) {
 
        std::string rstFile=pathRstFile+"/"+rstRootN+".SMSPEC";

        EclFile smspec_rst(rstFile);
        smspec_rst.loadData();

        std::vector<int> dimens = smspec_rst.get<int>("DIMENS");
        std::vector<std::string> restartArray = smspec_rst.get<std::string>("RESTART");
        std::vector<std::string> keywords = smspec_rst.get<std::string>("KEYWORDS");
        std::vector<std::string> wgnames = smspec_rst.get<std::string>("WGNAMES");
        std::vector<int> nums = smspec_rst.get<int>("NUMS");
        std::vector<std::string> units = smspec_rst.get<std::string>("UNITS");

        for (size_t i = 0; i < keywords.size(); i++) {
            std::string str1 = makeKeyString(keywords[i], wgnames[i], nums[i]);
            if (str1.length() > 0) {
                keywList.insert(str1);
            }
        }

        smryArray.push_back({rstFile,dimens[5]});

        getRstString(restartArray, pathRstFile, rstRootN);
    }

    int nFiles = static_cast<int>(smryArray.size());
    
    // arrayInd should hold indices for each vector and runs
    // n=file number, i = position in param array in file n (one array pr time step), example arrayInd[n][i] = position in keyword list (std::set) 

    std::vector<std::vector<int>> arrayInd;
    
    for (int i = 0; i < nFiles; i++){
        arrayInd.push_back({});
    }
    
    int n = nFiles - 1;
    
    while (n >= 0){

        auto smry = smryArray[n];

        EclFile smspec(std::get<0>(smry));
        smspec.loadData();

        std::vector<int> dimens = smspec.get<int>("DIMENS");

        nI = dimens[1];
        nJ = dimens[2];
        nK = dimens[3];

        std::vector<std::string> keywords = smspec.get<std::string>("KEYWORDS");
        std::vector<std::string> wgnames = smspec.get<std::string>("WGNAMES");
        std::vector<int> nums = smspec.get<int>("NUMS");

	std::vector<int> tmpVect(keywords.size(), -1);
        arrayInd[n]=tmpVect;

        std::set<std::string>::iterator it;

        for (size_t i=0; i < keywords.size(); i++) {
            std::string keyw = makeKeyString(keywords[i], wgnames[i], nums[i]);
            it = std::find(keywList.begin(), keywList.end(), keyw);

            if (it != keywList.end()){
                arrayInd[n][i] = distance(keywList.begin(), it);
            }
        }
        
        n--;
    }

    // param array used to stor data for the object, defined in the private section of the class 
    param.assign(keywList.size(), {});
    
    int fromReportStepNumber = 0;
    int reportStepNumber = 0;
    int toReportStepNumber;

    float time = 0.0;
    int step = 0;

    n = nFiles - 1;

    while (n >= 0){

        reportStepNumber = fromReportStepNumber;

        if (n > 0) {
            auto rstFrom = smryArray[n-1];
            toReportStepNumber = std::get<1>(rstFrom);
        } else {
            toReportStepNumber = std::numeric_limits<int>::max();
        }

        std::string smspecFile = std::get<0>(smryArray[n]);
        std::string unsmryFile = smspecFile.substr(0, smspecFile.size() - 6) + "UNSMRY";

        EclFile unsmry(unsmryFile);
        unsmry.loadData();

        std::vector<EclFile::EclEntry> list1 = unsmry.getList();

        // 2 or 3 arrays pr time step.
        //   If timestep is a report step:  MINISTEP, PARAMS and SEQHDR
        //   else : MINISTEP and PARAMS

        // if summary file starts with a SEQHDR, this will be ignored

        int i = 0;

        if (std::get<0>(list1[0]) == "SEQHDR") {
            i = 1;
        }

        while  (i < static_cast<int>(list1.size())){

            if (std::get<0>(list1[i]) != "MINISTEP"){
                std::string message="Reading summary file, expecting keyword MINISTEP, found '" + std::get<0>(list1[i]) + "'";
                throw std::invalid_argument(message);
            }

            std::vector<int> ministep = unsmry.get<int>(i);

            i++;

            if (std::get<0>(list1[i]) != "PARAMS") {
                std::string message="Reading summary file, expecting keyword PARAMS, found '" + std::get<0>(list1[i]) + "'";
                throw std::invalid_argument(message);
            }

            std::vector<float> tmpData = unsmry.get<float>(i);

            time = tmpData[0];

            if (time == 0.0) {
                seqTime.push_back(time);
                seqIndex.push_back(step);
            }

            i++;

            if (i < static_cast<int>(list1.size())){
                if (std::get<0>(list1[i]) == "SEQHDR") {
                    i++;
                    reportStepNumber++;
                    seqTime.push_back(time);
                    seqIndex.push_back(step);
                }
            } else {
                reportStepNumber++;
                seqTime.push_back(time);
                seqIndex.push_back(step);
            }
            
            // adding defaut values (0.0) in case vector not found in this particular summary file
            
            for (size_t i = 0; i < param.size(); i++){            
                param[i].push_back(0.0);
            }

            for (size_t j = 0; j < tmpData.size(); j++) {
                int ind = arrayInd[n][j];
                
                if (ind > -1) {
                   param[ind][step] = tmpData[j];        
                }
            }
            
            if (reportStepNumber >= toReportStepNumber) {
                i = static_cast<int>(list1.size());
            }

            step++;
        }

        fromReportStepNumber = toReportStepNumber;

        n--;
    }
    
    nVect = keywList.size();
    
    for (auto keyw : keywList){
        keyword.push_back(keyw);
    }
};


void ESmry::getRstString(const std::vector<std::string>& restartArray, std::string& pathRst, std::string& rootN) const {

    rootN = "";

    for (auto str : restartArray) {
        rootN = rootN + str;
    }

    updatePathAndRootName(pathRst, rootN);
}

void ESmry::updatePathAndRootName(std::string& dir, std::string& rootN) const {

    if (rootN.substr(0,2) == "./") {
        rootN = rootN.substr(2, rootN.size() - 2);
    }

    if (rootN.substr(0,1) == "/") {
        int p = rootN.find_last_of("/");
        dir = rootN.substr(0, p);
        rootN = rootN.substr(p + 1, rootN.size() - p - 1);
    } else if (rootN.find_first_of("/") != std::string::npos) {
        int p = rootN.find_last_of("/");
        dir = dir + "/" + rootN.substr(0, p);
        rootN = rootN.substr(p + 1, rootN.size() - p - 1);
    };
}

bool ESmry::hasKey(const std::string &key) const
{
    return std::find(keyword.begin(), keyword.end(), key) != keyword.end();
}


void ESmry::ijk_from_global_index(int glob,int &i,int &j,int &k)
{
    int tmpGlob = glob - 1;

    k = 1 + tmpGlob / (nI * nJ);
    int rest = tmpGlob % (nI * nJ);

    j = 1 + rest / nI;
    i = 1 + rest % nI;
}


std::string ESmry::makeKeyString(const std::string &keyword, const std::string &wgname, int num)
{
    std::string keyStr;
    std::vector<std::string> segmExcep= {"STEPTYPE", "SEPARATE", "SUMTHIN"};

    if (keyword.substr(0, 1) == "A") {
        keyStr = keyword + ":" + std::to_string(num);
    } else if (keyword.substr(0, 1) == "B") {
        int _i,_j,_k;
        ijk_from_global_index(num, _i, _j, _k);

        keyStr = keyword + ":" + std::to_string(_i) + "," + std::to_string(_j) + "," + std::to_string(_k);

    } else if (keyword.substr(0, 1) == "C") {
        int _i,_j,_k;

        if (num > 0) {
            ijk_from_global_index(num, _i, _j, _k);
            keyStr = keyword + ":" + wgname+ ":" + std::to_string(_i) + "," + std::to_string(_j) + "," + std::to_string(_k);
        }
    } else if (keyword.substr(0, 1) == "G") {
        if ( wgname != ":+:+:+:+") {
            keyStr = keyword + ":" + wgname;
        }
    } else if (keyword.substr(0, 1) == "R" && keyword.substr(2, 1) == "F") {
        // NUMS = R1 + 32768*(R2 + 10)
        int r2 = 0;
        int y = 32768 * (r2 + 10) - num;

        while (y <0 ) {
            r2++;
            y = 32768 * (r2 + 10) - num;
        }

        r2--;
        int r1 = num - 32768 * (r2 + 10);

        keyStr = keyword + ":" + std::to_string(r1) + "-" + std::to_string(r2);
    } else if (keyword.substr(0, 1) == "R") {
        keyStr = keyword + ":" + std::to_string(num);
    } else if (keyword.substr(0, 1) == "S") {
        auto it = std::find(segmExcep.begin(), segmExcep.end(), keyword);
        if (it != segmExcep.end()) {
            keyStr = keyword;
        } else {
            keyStr = keyword + ":" + wgname + ":" + std::to_string(num);
        }
    } else if (keyword.substr(0,1) == "W") {
        if (wgname != ":+:+:+:+") {
            keyStr = keyword + ":" + wgname;
        }
    } else {
        keyStr = keyword;
    }

    return keyStr;
}


const std::vector<float>& ESmry::get(const std::string& name) const
{
    auto it = std::find(keyword.begin(), keyword.end(), name);

    if (it == keyword.end()) {
        std::string message="keyword " + name + " not found ";
        OPM_THROW(std::invalid_argument, message);
    }

    int ind = std::distance(keyword.begin(), it);

    return param[ind];
}
