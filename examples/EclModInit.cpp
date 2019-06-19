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

#include <examples/EclModInit.hpp> 

#include <opm/common/utility/numeric/calculateCellVol.hpp> 

#include <string>
#include <string.h>
#include <sstream>
#include <iterator>
#include <iomanip>
#include <algorithm>

using EclEntry = std::tuple<std::string, Opm::EclIO::eclArrType, int>;


EclModInit::EclModInit(const std::string& filename)
{
    hasEgrid=false;

    std::string rootN;

    if (filename.substr(filename.size()-5,5)==".INIT") {
        rootN=filename.substr(0,filename.size()-5);

    } else {
        rootN=filename;
    }

    std::fstream fileH;
    fileH.open(rootN+".EGRID", std::ios::in |  std::ios::binary);

    if (fileH) {
        hasEgrid=true;
	gridfile = new Opm::EclIO::EGrid(rootN+".EGRID");
    }

    initfile = new Opm::EclIO::EclFile(rootN+".INIT");


    if (!initfile->hasKey("INTEHEAD")) {
        std::string message="parameter INTEHEAD not found in selected init file " + rootN+".INIT";
        throw std::invalid_argument(message);
    }

    std::vector<int> inteh=initfile->get<int>("INTEHEAD");

    nI=inteh[8];
    nJ=inteh[9];
    nK=inteh[10];
    nActive=inteh[11];

    if (!initfile->hasKey("PORV")) {
        std::string message="parameter PORV not found in selected init file " + rootN+".INIT";
        throw std::invalid_argument(message);
    }

    PORV.reserve(nActive);
    I.reserve(nActive);
    J.reserve(nActive);
    K.reserve(nActive);

    ActFilter.reserve(nActive);

    for (int n=0; n<nActive; n++) {
        ActFilter.push_back(true);
    }


    std::vector<float> porv=initfile->get<float>("PORV");

    int n=0;

    for (int k=0; k<nK; k++) {
        for (int j=0; j<nJ; j++) {
            for (int i=0; i<nI; i++) {
                if (porv[n]>0.0) {
                    PORV.push_back(porv[n]);
                    I.push_back(i+1);
                    J.push_back(j+1);
                    K.push_back(k+1);
                }
                n++;
            }
        }
    }

    // locating init parameters
    //  => pick up vectors with length equal to active cells

    std::vector<EclEntry> initArrList=initfile->getList();

    int index=0;

    for (int n=0; n<initArrList.size(); n++) {
        std::string name=std::get<0>(initArrList[n]);
        Opm::EclIO::eclArrType arrType=std::get<1>(initArrList[n]);
        int sizeArray=std::get<2>(initArrList[n]);

        if (sizeArray==nActive) {
            initParam[name]=index;
            initParamName.push_back(name);
            initParamType.push_back(arrType);
            indInInitEclfile.push_back(n);

            index++;
        }
    }

    rstfile0 = new Opm::EclIO::EclFile(rootN+".UNRST");

    // locating solution parameters for step 0
    //  => pick up vectors located between STARSOL and ENDSOL keyword

    std::vector<EclEntry> rstArrList=rstfile0->getList();

    int step;
    bool solparam=false;

    index=-1;

    for (int n=0; n<rstArrList.size(); n++) {
        std::string name=std::get<0>(rstArrList[n]);
        Opm::EclIO::eclArrType arrType=std::get<1>(rstArrList[n]);
        int sizeArray=std::get<2>(rstArrList[n]);

        if (name == "SEQNUM") {
            std::vector<int> seqn = rstfile0->get<int>(n);
            step=seqn[0];
        }

        if (name == "ENDSOL") {
            solparam=false;
        }

        if ((step==0) && (solparam==true) && (sizeArray==nActive)) {
            index++;
            solutionParam[name]=index;
            solutionParamName.push_back(name);
            solutionParamType.push_back(arrType);
            indInRstEclfile.push_back(n);
        }

        if (name == "STARTSOL") {
            solparam=true;
        }
    }

    celVolCalculated=false;
    activeFilter=false;
}


EclModInit::~EclModInit() {

    delete initfile;
    delete rstfile0;
}

void EclModInit::calcCellVol(){

    if (!hasEgrid){
        std::string message = "\nnot possible to calculate cell columes without an Egrid file. ";
        message = message + "The grid file must have same root name as the init file selected for this object";
        throw std::runtime_error(message);
    }
    
    CELLVOL.clear();
    
    int i,j,k;
    
    std::vector<double> X1(8,0.0), Y1(8,0.0), Z1(8,0.0);

    for (int n=0;n<nActive;n++){
        
        i=I[n]-1;  j=J[n]-1;  k=K[n]-1;  
        
        gridfile->getCellCorners({i,j,k}, X1, Y1, Z1);
        
        CELLVOL.push_back(static_cast<float>(calculateCellVol(X1, Y1, Z1)));
    }
    
    celVolCalculated=true;
}



std::vector<std::tuple<std::string, Opm::EclIO::eclArrType>> EclModInit::getListOfParameters() const {

    std::vector<std::tuple<std::string, Opm::EclIO::eclArrType>> res;

    for (int i=0; i<initParamName.size(); i++) {
	std::tuple<std::string, Opm::EclIO::eclArrType> entry = std::make_tuple(initParamName[i],initParamType[i]);
        res.push_back(entry);
    }

    for (int i=0; i<solutionParamName.size(); i++) {
        std::tuple<std::string, Opm::EclIO::eclArrType> entry = std::make_tuple(solutionParamName[i],solutionParamType[i]);
        res.push_back(entry);
    }

    return res;
}

int EclModInit::getNumberOfActiveCells() {

    int nCells=0;

    for (int i=0; i<ActFilter.size(); i++) {
        if (ActFilter[i]) {
            nCells++;
        }
    }

    return nCells;
}

bool EclModInit::hasInitParameter(const std::string &name) const
{
    auto search = initParam.find(name);
    return search != initParam.end();
}

bool EclModInit::hasSolutionParameter(const std::string &name) const
{
    auto search = solutionParam.find(name);
    return search != solutionParam.end();
}

bool EclModInit::hasParameter(const std::string &name) const
{
    return hasInitParameter(name) || hasSolutionParameter(name);
}

bool EclModInit::hasInitReportStep() const
{
    return solutionParam.size()>0;
}

void EclModInit::resetFilter() {

    activeFilter=false;

    for (int n=0; n<nActive; n++) {
        ActFilter[n]=true;
    }
}

template <typename T2>
void EclModInit::updateActiveFilter(const std::vector<T2>& paramVect, const std::string opperator, T2 value)  {

    if (opperator=="eq") {

        for (int i=0; i<paramVect.size(); i++) {
            if ((ActFilter[i]) && (paramVect[i]!=value)) {
                ActFilter[i]=false;
            }
        }

    } else if (opperator=="lt") {

        for (int i=0; i<paramVect.size(); i++) {
            if ((ActFilter[i]) && (paramVect[i]>=value)) {
                ActFilter[i]=false;
            }
        }

    } else if (opperator=="gt") {

        for (int i=0; i<paramVect.size(); i++) {
            if ((ActFilter[i]) && (paramVect[i]<=value)) {
                ActFilter[i]=false;
            }
        }

    } else {
        std::string message = "Unknown opprator " + opperator + ", used to set filter";
        throw std::invalid_argument(message);
    }

    activeFilter=true;

}

template <typename T2>
void EclModInit::updateActiveFilter(const std::vector<T2>& paramVect, const std::string opperator, T2 value1, T2 value2)  {

    if (opperator=="between") {
        
        for (int i=0; i<paramVect.size(); i++) {
            if ((ActFilter[i]) && ((paramVect[i]<=value1) || (paramVect[i]>=value2))) {
                ActFilter[i]=false;
            }
        }

    } else {
        std::string message = "Unknown opprator " + opperator + ", used to set filter";
        throw std::invalid_argument(message);
    }

    activeFilter=true;

}


template <>
void EclModInit::addFilter<int>(std::string param1, std::string opperator, int num) {

    std::vector<int> paramVect;

    if ((param1=="I") || (param1=="ROW")) {
        paramVect=I;
    } else if ((param1=="J") || (param1=="COLUMN")) {
        paramVect=J;
    } else if ((param1=="K") || (param1=="LAYER")) {
        paramVect=K;
    } else if (hasInitParameter(param1)) {
        paramVect = getInitInt(param1);
    } else {
        std::string message = "parameter " + param1 + ", used to set filter, could not be found";
        throw std::invalid_argument(message);
    }

    updateActiveFilter(paramVect, opperator, num);
}

template <>
void EclModInit::addFilter<int>(std::string param1, std::string opperator, int num1, int num2) {

    std::vector<int> paramVect;

    if ((param1=="I") || (param1=="ROW")) {
        paramVect=I;
    } else if ((param1=="J") || (param1=="COLUMN")) {
        paramVect=J;
    } else if ((param1=="K") || (param1=="LAYER")) {
        paramVect=K;
    } else if (hasInitParameter(param1)) {
        paramVect = getInitInt(param1);
    } else {
        std::string message = "parameter " + param1 + ", used to set filter, could not be found";
        throw std::invalid_argument(message);
    }

    updateActiveFilter(paramVect, opperator, num1, num2);
}


template <>
void EclModInit::addFilter<float>(std::string param1, std::string opperator, float num) {

    std::vector<float> paramVect;

    if (param1=="PORV") {
        std::cout << "add filter float, parameter PORV not tested !!" << std::endl;
        exit(1);
        paramVect=PORV;
      
    } else if (param1=="CELLVOL") {

        if (!celVolCalculated){
            calcCellVol();
        }
        paramVect=CELLVOL;
    
    } else if (hasInitParameter(param1)) {
        paramVect = getInitFloat(param1);
    } else if (hasSolutionParameter(param1)) {
        paramVect = getSolutionFloat(param1);
    } else {
        std::string message = "parameter " + param1 + ", used to set filter, could not be found";
        throw std::invalid_argument(message);
    }

    updateActiveFilter(paramVect, opperator, num);
}


template <>
void EclModInit::addFilter<float>(std::string param1, std::string opperator, float num1, float num2) {

    std::vector<float> paramVect;

    if (param1=="PORV") {
        std::cout << "add filter float, parameter PORV not tested !!" << std::endl;
        exit(1);
        paramVect=PORV;
    } else if (param1=="CELLVOL") {

        if (!celVolCalculated){
            calcCellVol();  
        }
        paramVect=CELLVOL;
    
    } else if (hasInitParameter(param1)) {
        paramVect = getInitFloat(param1);
    } else if (hasSolutionParameter(param1)) {
        paramVect = getSolutionFloat(param1);
    } else {
        std::string message = "parameter " + param1 + ", used to set filter, could not be found";
        throw std::invalid_argument(message);
    }
    
    updateActiveFilter(paramVect, opperator, num1, num2);
}


void EclModInit::addHCvolFilter(){
    
    if (FreeWaterlevel.size()==0){
        throw std::runtime_error("free water level needs to be inputted via function setDepthfwl before using filter HC filter");
    }

    std::vector<int> eqlnum = getInitInt("EQLNUM");
    std::vector<float> depth = getInitFloat("DEPTH");
    
    for (int n=0; n< eqlnum.size();n++){
        
        int eql=eqlnum[n];
        float fwl=FreeWaterlevel[eql-1];
        
        if ((ActFilter[n]) && (depth[n]>fwl)) {
            ActFilter[n]=false;
        }
    }
}


template <>
const std::vector<float>& EclModInit::getParam<float>(const std::string& name) {
    
    if (activeFilter) {
        std::vector<float> tmp;

        if (name=="PORV") {
            tmp=PORV;
        } else if (name=="CELLVOL") {
            if (!celVolCalculated){
                calcCellVol();  
            }
            
            tmp=CELLVOL;
	  
        } else if (hasInitParameter(name)) {
            tmp=getInitFloat(name);
        } else if (hasSolutionParameter(name)) {
            tmp=getSolutionFloat(name);
        } else {
            std::string message = "parameter " + name + ", could not be found";
            throw std::invalid_argument(message);
        }

        filteredFloatVect.clear();

        for (int i=0; i<tmp.size(); i++) {
            if (ActFilter[i]) {
                filteredFloatVect.push_back(tmp[i]);
            }
        }

        return filteredFloatVect;

    } else {

        if (name=="PORV") {
            return PORV;
      
        } else if (name=="CELLVOL") {

	    if (!celVolCalculated){
                calcCellVol(); 
            }
            return CELLVOL;
       } else if (hasInitParameter(name)) {
            return getInitFloat(name);
        } else if (hasSolutionParameter(name)) {
            return getSolutionFloat(name);
        } else {
            std::string message = "parameter " + name + ", could not be found";
            throw std::invalid_argument(message);
        }
    }
}


template <>
const std::vector<int>& EclModInit::getParam<int>(const std::string& name) {

    if (activeFilter) {

        std::vector<int> tmp;

        if ((name=="K") || (name=="LAYER")) {
            tmp=K;
        } else if (hasInitParameter(name)) {
            tmp=getInitInt(name);
        } else {
            std::string message = "parameter " + name + ", could not be found";
            throw std::invalid_argument(message);
        }

        filteredIntVect.clear();

        for (int i=0; i<tmp.size(); i++) {
            if (ActFilter[i]) {
                filteredIntVect.push_back(tmp[i]);
            }
        }

        return filteredIntVect;

    } else {
        if ((name=="K") || (name=="LAYER")) {
            return K;
        } else if (hasInitParameter(name)) {
            return getInitInt(name);
        } else {
            std::string message = "parameter " + name + ", could not be found";
            throw std::invalid_argument(message);
        }
    }
}


const std::vector<int>& EclModInit::getInitInt(const std::string& name)
{

    return initfile->get<int>(name);

}

const std::vector<float>& EclModInit::getInitFloat(const std::string& name)
{

    if (name=="PORV") {
        return PORV;
    } else {
        return initfile->get<float>(name);
    }
}


const std::vector<float>& EclModInit::getSolutionFloat(const std::string& name)
{

    if (!hasSolutionParameter(name)) {
        std::string message = "parameter " + name + " not found for step 0 in restart file ";
        throw std::invalid_argument(message);
    }

    auto search = solutionParam.find(name);

    int eclFileIndex=indInRstEclfile[search->second];

    return rstfile0->get<float>(eclFileIndex);
}


void EclModInit::setDepthfwl(const std::vector<float>& fwl)
{
    nEqlnum=fwl.size();
    FreeWaterlevel=fwl;

    std::vector<int> eqlnum = getInitInt("EQLNUM");
    
    int maxEqlnum=-1;
    
    for (int i : eqlnum){
        if (i > maxEqlnum){
            maxEqlnum=i;        
        }
    }
    
    if (maxEqlnum>nEqlnum){
        std::string message= "FWL not defined for all eql regions. # Contacts input: " + std::to_string(nEqlnum) + " needed (max value in EQLNUM): " + std::to_string(maxEqlnum);
        throw std::invalid_argument(message);        
    }
}

