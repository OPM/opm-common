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

#include <opm/io/eclipse/ESmry.hpp>

#include <exception>
#include <string>
#include <string.h>
#include <sstream>
#include <iterator>
#include <iomanip>
#include <algorithm>
#include <unistd.h>
#include <limits>
#include <limits.h>
#include <set>
#include <stdexcept>

#include <boost/filesystem.hpp> 

#include <opm/io/eclipse/EclFile.hpp>
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

namespace Opm { namespace EclIO {

ESmry::ESmry(const std::string &filename, bool loadBaseRunData)
{

    boost::filesystem::path inputFileName(filename);
    boost::filesystem::path rootName = inputFileName.parent_path() / inputFileName.stem();
    
    // if root name (without any extension) given as first argument in constructor, binary will then be assumed
    if (inputFileName.extension()==""){
        inputFileName+=".SMSPEC";
    }
    
    std::vector<bool> formattedVect;
    
    if ((inputFileName.extension()!=".SMSPEC") && (inputFileName.extension()!=".FSMSPEC")){
        throw std::invalid_argument("Inptut file should have extension .SMSPEC or .FSMSPEC");
    }

    bool formatted = inputFileName.extension()==".SMSPEC" ? false : true;
    formattedVect.push_back(formatted);
    
    boost::filesystem::path path = boost::filesystem::current_path();;

    updatePathAndRootName(path, rootName);
    
    boost::filesystem::path smspec_file = path / rootName;
    smspec_file += inputFileName.extension();

    boost::filesystem::path rstRootN;
    boost::filesystem::path pathRstFile = path; 
    
    std::set<std::string> keywList;
    std::vector<std::pair<std::string,int>> smryArray;

    // Read data from the summary into local data members.
    {
        EclFile smspec1(smspec_file.string());

        smspec1.loadData();   // loading all data

        std::vector<int> dimens = smspec1.get<int>("DIMENS");

        nI = dimens[1]; // This is correct -- dimens[0] is something else!
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
        
        getRstString(restartArray, pathRstFile, rstRootN);
        
        smryArray.push_back({smspec_file.string(), dimens[5]});
    }

    // checking if this is a restart run. Supporting nested restarts (restart, from restart, ...)
    // std::set keywList is storing keywords from all runs involved
    
    while ((rstRootN.string() != "") && (loadBaseRunData)) {
        
        boost::filesystem::path rstFile = pathRstFile / rstRootN;
        rstFile += ".SMSPEC";
        
        bool baseRunFmt = false;
        
        // if unformatted file not exists, check for formatted file
        if (!boost::filesystem::exists(rstFile)){
            rstFile = pathRstFile / rstRootN;
            rstFile += ".FSMSPEC";
            
            baseRunFmt = true;
        }
        
        EclFile smspec_rst(rstFile.string());
        smspec_rst.loadData();

        std::vector<int> dimens = smspec_rst.get<int>("DIMENS");
        std::vector<std::string> restartArray = smspec_rst.get<std::string>("RESTART");
        std::vector<std::string> keywords = smspec_rst.get<std::string>("KEYWORDS");
        std::vector<std::string> wgnames = smspec_rst.get<std::string>("WGNAMES");
        std::vector<int> nums = smspec_rst.get<int>("NUMS");

        for (size_t i = 0; i < keywords.size(); i++) {
            std::string str1 = makeKeyString(keywords[i], wgnames[i], nums[i]);
            if (str1.length() > 0) {
                keywList.insert(str1);
            }
        }

        smryArray.push_back({rstFile.string(),dimens[5]});
        
        formattedVect.push_back(baseRunFmt);

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
    int toReportStepNumber;

    float time = 0.0;
    int step = 0;

    n = nFiles - 1;

    while (n >= 0){

        int reportStepNumber = fromReportStepNumber;

        if (n > 0) {
            auto rstFrom = smryArray[n-1];
            toReportStepNumber = std::get<1>(rstFrom);
        } else {
            toReportStepNumber = std::numeric_limits<int>::max();
        }

        boost::filesystem::path smspecFile(std::get<0>(smryArray[n]));
        rootName = smspecFile.parent_path() / smspecFile.stem();

        
        // check if multiple or unified result files should be used 
        // to import data, no information in smspec file regarding this
        // if both unified and non-unified files exists, will use most recent based on 
        // time stamp

        boost::filesystem::path unsmryFile = rootName; 
        
        formattedVect[n] ? unsmryFile += ".FUNSMRY" : unsmryFile += ".UNSMRY";

        bool use_unified = boost::filesystem::exists(unsmryFile.string());

        std::vector<std::string> multFileList = checkForMultipleResultFiles(rootName, formattedVect[n]);

        std::vector<std::string> resultsFileList;
        
        if ((!use_unified) && (multFileList.size()==0)){
            throw std::runtime_error("neigther unified or non-unified result files found");
        } else if ((use_unified) && (multFileList.size()>0)){
            auto time_multiple = boost::filesystem::last_write_time(multFileList.back());
            auto time_unified = boost::filesystem::last_write_time(unsmryFile);
            
            if (time_multiple > time_unified){
                resultsFileList=multFileList;
            } else {
                resultsFileList.push_back(unsmryFile.string());
            }
            
        } else if (use_unified){
            resultsFileList.push_back(unsmryFile.string());
        } else {
            resultsFileList=multFileList;
        }
        
        // make array list with reference to source files (unifed or non unified)
        
        std::vector<std::tuple<std::string, std::string, int>> arraySourceList;

        for (std::string fileName : resultsFileList){
            EclFile unsmry(fileName);
                
            std::vector<EclFile::EclEntry> arrayList = unsmry.getList();

            for (size_t nn = 0; nn < arrayList.size(); nn++){
                std::tuple<std::string, std::string, int> t1 = std::make_tuple(std::get<0>(arrayList[nn]), fileName, static_cast<int>(nn));
                arraySourceList.push_back(t1);
            }
        }

        // loop through arrays and extract symmary data from result files, arrays PARAMS
        //
        //    2 or 3 arrays pr time step.
        //       If timestep is a report step:  MINISTEP, PARAMS and SEQHDR
        //       else : MINISTEP and PARAMS
        
        
        size_t i = std::get<0>(arraySourceList[0]) == "SEQHDR" ? 1 : 0 ;

        while  (i < arraySourceList.size()){
            
            if (std::get<0>(arraySourceList[i]) != "MINISTEP"){
                std::string message="Reading summary file, expecting keyword MINISTEP, found '" + std::get<0>(arraySourceList[i]) + "'";
                throw std::invalid_argument(message);
            }

            if (std::get<0>(arraySourceList[i+1]) != "PARAMS") {
                std::string message="Reading summary file, expecting keyword PARAMS, found '" + std::get<0>(arraySourceList[i]) + "'";
                throw std::invalid_argument(message);
            }
            
            i++;

            EclFile resfile(std::get<1>(arraySourceList[i]));
            int m = std::get<2>(arraySourceList[i]);
            
            std::vector<float> tmpData = resfile.get<float>(m);

            time = tmpData[0];

            if (time == 0.0) {
                seqTime.push_back(time);
                seqIndex.push_back(step);
            }

            i++;

            if (i < arraySourceList.size()){
                if (std::get<0>(arraySourceList[i]) == "SEQHDR") {
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
            
            for (size_t ii = 0; ii < param.size(); ii++){            
                param[ii].push_back(0.0);
            }

            for (size_t j = 0; j < tmpData.size(); j++) {
                int ind = arrayInd[n][j];
                
                if (ind > -1) {
                   param[ind][step] = tmpData[j];        
                }
            }
            
            if (reportStepNumber >= toReportStepNumber) {
                i = arraySourceList.size();
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
}


std::vector<std::string> ESmry::checkForMultipleResultFiles(const boost::filesystem::path& rootN, bool formatted) const {
    
    std::vector<std::string> fileList;
    std::string pathRootN = rootN.parent_path().string();

    std::string fileFilter = formatted ? rootN.stem().string()+".A" : rootN.stem().string()+".S"; 

    for (boost::filesystem::directory_iterator itr(pathRootN); itr!=boost::filesystem::directory_iterator(); ++itr)
    {
        std::string file = itr->path().filename().string();
        
        if ((file.find(fileFilter) != std::string::npos) && (file.find("SMSPEC") == std::string::npos)) {
            fileList.push_back(pathRootN + "/" + file);
        }
    }

    std::sort(fileList.begin(), fileList.end());
    
    return fileList;
}

void ESmry::getRstString(const std::vector<std::string>& restartArray, boost::filesystem::path& pathRst, boost::filesystem::path& rootN) const {

    std::string rootNameStr="";
    
    for (auto str : restartArray) {
        rootNameStr = rootNameStr + str;
    }

    rootN = boost::filesystem::path(rootNameStr);
    
    updatePathAndRootName(pathRst, rootN);
}

void ESmry::updatePathAndRootName(boost::filesystem::path& dir, boost::filesystem::path& rootN) const {

    if (rootN.parent_path().is_absolute()){
        dir = rootN.parent_path();
    } else {
        dir = dir / rootN.parent_path();
    }        

    rootN = rootN.stem();
}

bool ESmry::hasKey(const std::string &key) const
{
    return std::find(keyword.begin(), keyword.end(), key) != keyword.end();
}


void ESmry::ijk_from_global_index(int glob,int &i,int &j,int &k) const
{
    int tmpGlob = glob - 1;

    k = 1 + tmpGlob / (nI * nJ);
    int rest = tmpGlob % (nI * nJ);

    j = 1 + rest / nI;
    i = 1 + rest % nI;
}


std::string ESmry::makeKeyString(const std::string& keywordArg, const std::string& wgname, int num) const
{
    std::string keyStr;
    std::vector<std::string> segmExcep= {"STEPTYPE", "SEPARATE", "SUMTHIN"};

    if (keywordArg.substr(0, 1) == "A") {
        keyStr = keywordArg + ":" + std::to_string(num);
    } else if (keywordArg.substr(0, 1) == "B") {
        int _i,_j,_k;
        ijk_from_global_index(num, _i, _j, _k);

        keyStr = keywordArg + ":" + std::to_string(_i) + "," + std::to_string(_j) + "," + std::to_string(_k);

    } else if (keywordArg.substr(0, 1) == "C") {
        if (num > 0) {
            int _i,_j,_k;
            ijk_from_global_index(num, _i, _j, _k);
            keyStr = keywordArg + ":" + wgname+ ":" + std::to_string(_i) + "," + std::to_string(_j) + "," + std::to_string(_k);
        }
    } else if (keywordArg.substr(0, 1) == "G") {
        if ( wgname != ":+:+:+:+") {
            keyStr = keywordArg + ":" + wgname;
        }
    } else if (keywordArg.substr(0, 1) == "R" && keywordArg.substr(2, 1) == "F") {
        // NUMS = R1 + 32768*(R2 + 10)
        int r2 = 0;
        int y = 32768 * (r2 + 10) - num;

        while (y <0 ) {
            r2++;
            y = 32768 * (r2 + 10) - num;
        }

        r2--;
        int r1 = num - 32768 * (r2 + 10);

        keyStr = keywordArg + ":" + std::to_string(r1) + "-" + std::to_string(r2);
    } else if (keywordArg.substr(0, 1) == "R") {
        keyStr = keywordArg + ":" + std::to_string(num);
    } else if (keywordArg.substr(0, 1) == "S") {
        auto it = std::find(segmExcep.begin(), segmExcep.end(), keywordArg);
        if (it != segmExcep.end()) {
            keyStr = keywordArg;
        } else {
            keyStr = keywordArg + ":" + wgname + ":" + std::to_string(num);
        }
    } else if (keywordArg.substr(0,1) == "W") {
        if (wgname != ":+:+:+:+") {
            keyStr = keywordArg + ":" + wgname;
        }
    } else {
        keyStr = keywordArg;
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

std::vector<float> ESmry::get_at_rstep(const std::string& name) const
{
    std::vector<float> full_vector= this->get(name);

    std::vector<float> rstep_vector;
    rstep_vector.reserve(seqIndex.size());
    
    for (auto ind : seqIndex){
        rstep_vector.push_back(full_vector[ind]);
    }
    
    return rstep_vector;
}

int ESmry::timestepIdxAtReportstepStart(const int reportStep) const
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

}} // namespace Opm::ecl
