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

#include <algorithm>
#include <chrono>
#include <exception>
#include <iterator>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <fnmatch.h>

#include <opm/common/utility/FileSystem.hpp>
#include <opm/common/utility/TimeService.hpp>
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


namespace Opm { namespace EclIO {

ESmry::ESmry(const std::string &filename, bool loadBaseRunData) :
    inputFileName { filename },
    summaryNodes { }
{

    Opm::filesystem::path rootName = inputFileName.parent_path() / inputFileName.stem();

    // if root name (without any extension) given as first argument in constructor, binary will then be assumed
    if (inputFileName.extension()==""){
        inputFileName+=".SMSPEC";
    }

    std::vector<bool> formattedVect;

    if ((inputFileName.extension()!=".SMSPEC") && (inputFileName.extension()!=".FSMSPEC")){
        throw std::invalid_argument("Inptut file should have extension .SMSPEC or .FSMSPEC");
    }

    const bool formatted = inputFileName.extension()==".SMSPEC" ? false : true;
    formattedVect.push_back(formatted);

    Opm::filesystem::path path = Opm::filesystem::current_path();

    updatePathAndRootName(path, rootName);

    Opm::filesystem::path smspec_file = path / rootName;
    smspec_file += inputFileName.extension();

    Opm::filesystem::path rstRootN;
    Opm::filesystem::path pathRstFile = path;

    std::set<std::string> keywList;
    std::vector<std::pair<std::string,int>> smryArray;

    const std::unordered_set<std::string> segmentExceptions {
        "SEPARATE",
        "STEPTYPE",
        "SUMTHIN",
    } ;

    // Read data from the summary into local data members.
    {
        EclFile smspec(smspec_file.string());

        smspec.loadData();   // loading all data

        const std::vector<int> dimens = smspec.get<int>("DIMENS");

        nI = dimens[1]; // This is correct -- dimens[0] is something else!
        nJ = dimens[2];
        nK = dimens[3];

        const std::vector<std::string> restartArray = smspec.get<std::string>("RESTART");
        const std::vector<std::string> keywords = smspec.get<std::string>("KEYWORDS");
        const std::vector<std::string> wgnames = smspec.get<std::string>("WGNAMES");
        const std::vector<int> nums = smspec.get<int>("NUMS");
        const std::vector<std::string> units = smspec.get<std::string>("UNITS");

        this->startdat = make_date(smspec.get<int>("STARTDAT"));

        for (unsigned int i=0; i<keywords.size(); i++) {
            const std::string keyString = makeKeyString(keywords[i], wgnames[i], nums[i]);
            if (keyString.length() > 0) {
                summaryNodes.push_back({
                    keywords[i],
                    SummaryNode::category_from_keyword(keywords[i], segmentExceptions),
                    SummaryNode::Type::Undefined,
                    wgnames[i],
                    nums[i]
                });

                keywList.insert(keyString);
                kwunits[keyString] = units[i];
            }
        }

        getRstString(restartArray, pathRstFile, rstRootN);

        smryArray.push_back({smspec_file.string(), dimens[5]});
    }

    // checking if this is a restart run. Supporting nested restarts (restart, from restart, ...)
    // std::set keywList is storing keywords from all runs involved

    while ((rstRootN.string() != "") && (loadBaseRunData)) {

        Opm::filesystem::path rstFile = pathRstFile / rstRootN;
        rstFile += ".SMSPEC";

        bool baseRunFmt = false;

        // if unformatted file not exists, check for formatted file
        if (!Opm::filesystem::exists(rstFile)){
            rstFile = pathRstFile / rstRootN;
            rstFile += ".FSMSPEC";

            baseRunFmt = true;
        }

        EclFile smspec_rst(rstFile.string());
        smspec_rst.loadData();

        const std::vector<int> dimens = smspec_rst.get<int>("DIMENS");
        const std::vector<std::string> restartArray = smspec_rst.get<std::string>("RESTART");
        const std::vector<std::string> keywords = smspec_rst.get<std::string>("KEYWORDS");
        const std::vector<std::string> wgnames = smspec_rst.get<std::string>("WGNAMES");
        const std::vector<int> nums = smspec_rst.get<int>("NUMS");
        const std::vector<std::string> units = smspec_rst.get<std::string>("UNITS");

        this->startdat = make_date(smspec_rst.get<int>("STARTDAT"));

        for (size_t i = 0; i < keywords.size(); i++) {
            const std::string keyString = makeKeyString(keywords[i], wgnames[i], nums[i]);
            if (keyString.length() > 0) {
                summaryNodes.push_back({
                    keywords[i],
                    SummaryNode::category_from_keyword(keywords[i], segmentExceptions),
                    SummaryNode::Type::Undefined,
                    wgnames[i],
                    nums[i]
                });

                keywList.insert(keyString);
                kwunits[keyString] = units[i];
            }
        }

        smryArray.push_back({rstFile.string(),dimens[5]});

        formattedVect.push_back(baseRunFmt);

        getRstString(restartArray, pathRstFile, rstRootN);
    }

    const int nFiles = static_cast<int>(smryArray.size());

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

        const std::vector<int> dimens = smspec.get<int>("DIMENS");

        nI = dimens[1];
        nJ = dimens[2];
        nK = dimens[3];

        const std::vector<std::string> keywords = smspec.get<std::string>("KEYWORDS");
        const std::vector<std::string> wgnames = smspec.get<std::string>("WGNAMES");
        const std::vector<int> nums = smspec.get<int>("NUMS");

        const std::vector<int> tmpVect(keywords.size(), -1);
        arrayInd[n]=tmpVect;

        std::set<std::string>::iterator it;

        for (size_t i=0; i < keywords.size(); i++) {
            const std::string keyw = makeKeyString(keywords[i], wgnames[i], nums[i]);
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

        Opm::filesystem::path smspecFile(std::get<0>(smryArray[n]));
        rootName = smspecFile.parent_path() / smspecFile.stem();


        // check if multiple or unified result files should be used
        // to import data, no information in smspec file regarding this
        // if both unified and non-unified files exists, will use most recent based on
        // time stamp

        Opm::filesystem::path unsmryFile = rootName;

        formattedVect[n] ? unsmryFile += ".FUNSMRY" : unsmryFile += ".UNSMRY";

        const bool use_unified = Opm::filesystem::exists(unsmryFile.string());

        const std::vector<std::string> multFileList = checkForMultipleResultFiles(rootName, formattedVect[n]);

        std::vector<std::string> resultsFileList;

        if ((!use_unified) && (multFileList.size()==0)){
            throw std::runtime_error("neigther unified or non-unified result files found");
        } else if ((use_unified) && (multFileList.size()>0)){
            auto time_multiple = Opm::filesystem::last_write_time(multFileList.back());
            auto time_unified = Opm::filesystem::last_write_time(unsmryFile);

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

            const std::vector<EclFile::EclEntry> arrayList = unsmry.getList();

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

        std::string prevFile;
        std::unique_ptr<EclFile> pEclFile;

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

            if (std::get<1>(arraySourceList[i]) != prevFile){
                pEclFile = std::make_unique<EclFile>(std::get<1>(arraySourceList[i]));
                pEclFile->loadData();

                prevFile = std::get<1>(arraySourceList[i]);
            }

            const int m = std::get<2>(arraySourceList[i]);

            const std::vector<float> tmpData = pEclFile->get<float>(m);

            time = tmpData[0];

            if (time == 0.0) {
                seqIndex.push_back(step);
            }

            i++;

            if (i < arraySourceList.size()){
                if (std::get<0>(arraySourceList[i]) == "SEQHDR") {
                    i++;
                    reportStepNumber++;
                    seqIndex.push_back(step);
                }
            } else {
                reportStepNumber++;
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

    for (const auto& keyw : keywList){
        keyword.push_back(keyw);
    }
}


std::vector<std::string> ESmry::checkForMultipleResultFiles(const Opm::filesystem::path& rootN, bool formatted) const {

    std::vector<std::string> fileList;
    const std::string pathRootN = rootN.parent_path().string();

    const std::string fileFilter = formatted ? rootN.stem().string()+".A" : rootN.stem().string()+".S";

    for (Opm::filesystem::directory_iterator itr(pathRootN); itr != Opm::filesystem::directory_iterator(); ++itr)
    {
        const std::string file = itr->path().filename().string();

        if ((file.find(fileFilter) != std::string::npos) && (file.find("SMSPEC") == std::string::npos)) {
            fileList.push_back(pathRootN + "/" + file);
        }
    }

    std::sort(fileList.begin(), fileList.end());

    return fileList;
}

void ESmry::getRstString(const std::vector<std::string>& restartArray, Opm::filesystem::path& pathRst, Opm::filesystem::path& rootN) const {

    std::string rootNameStr="";

    for (const auto& str : restartArray) {
        rootNameStr = rootNameStr + str;
    }

    rootN = Opm::filesystem::path(rootNameStr);

    updatePathAndRootName(pathRst, rootN);
}

void ESmry::updatePathAndRootName(Opm::filesystem::path& dir, Opm::filesystem::path& rootN) const {

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
    const int tmpGlob = glob - 1;

    k = 1 + tmpGlob / (nI * nJ);
    const int rest = tmpGlob % (nI * nJ);

    j = 1 + rest / nI;
    i = 1 + rest % nI;
}


std::string ESmry::makeKeyString(const std::string& keywordArg, const std::string& wgname, int num) const
{
    std::string keyStr;
    const std::vector<std::string> segmExcep= {"STEPTYPE", "SEPARATE", "SUMTHIN"};

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
        const int r1 = num - 32768 * (r2 + 10);

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

std::string ESmry::unpackNumber(const SummaryNode& node) const {
    if (node.category == SummaryNode::Category::Block ||
        node.category == SummaryNode::Category::Connection) {
        int _i,_j,_k;
        ijk_from_global_index(node.number, _i, _j, _k);

        return std::to_string(_i) + "," + std::to_string(_j) + "," + std::to_string(_k);
    } else if (node.category == SummaryNode::Category::Region && node.keyword[2] == 'F') {
        const auto r1 =  node.number % (1 << 15);
        const auto r2 = (node.number / (1 << 15)) - 10;

        return std::to_string(r1) + "-" + std::to_string(r2);
    } else {
        return std::to_string(node.number);
    }
}

std::string ESmry::lookupKey(const SummaryNode& node) const {
    return node.unique_key(std::bind( &ESmry::unpackNumber, this, std::placeholders::_1 ));
}

const std::vector<float>& ESmry::get(const SummaryNode& node) const {
    return get(lookupKey(node));
}

std::vector<float> ESmry::get_at_rstep(const SummaryNode& node) const {
    return get_at_rstep(lookupKey(node));
}

const std::string& ESmry::get_unit(const SummaryNode& node) const {
    return get_unit(lookupKey(node));
}

const std::vector<float>& ESmry::get(const std::string& name) const
{
    auto it = std::find(keyword.begin(), keyword.end(), name);

    if (it == keyword.end()) {
        const std::string message="keyword " + name + " not found ";
        OPM_THROW(std::invalid_argument, message);
    }

    int ind = std::distance(keyword.begin(), it);

    return param[ind];
}

std::vector<float> ESmry::get_at_rstep(const std::string& name) const
{
    return this->rstep_vector( this->get(name) );
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

const std::string& ESmry::get_unit(const std::string& name) const {
    return kwunits.at(name);
}

const std::vector<std::string>& ESmry::keywordList() const
{
    return keyword;
}

std::vector<std::string> ESmry::keywordList(const std::string& pattern) const
{
    std::vector<std::string> list;

    for (auto key : keyword)
        if (fnmatch( pattern.c_str(), key.c_str(), 0 ) == 0 )
            list.push_back(key);

    return list;
}



const std::vector<SummaryNode>& ESmry::summaryNodeList() const {
    return summaryNodes;
}

std::vector<std::chrono::system_clock::time_point> ESmry::dates() const {
    double time_unit = 24 * 3600;
    std::vector<std::chrono::system_clock::time_point> d;

    using namespace std::chrono;
    using TP      = time_point<system_clock>;
    using DoubSec = duration<double, seconds::period>;

    for (const auto& t : this->get("TIME"))
        d.push_back( this->startdat + duration_cast<TP::duration>(DoubSec(t * time_unit)));

    return d;
}

std::vector<std::chrono::system_clock::time_point> ESmry::dates_at_rstep() const {
    const auto& full_vector = this->dates();
    return this->rstep_vector(full_vector);
}


}} // namespace Opm::ecl
