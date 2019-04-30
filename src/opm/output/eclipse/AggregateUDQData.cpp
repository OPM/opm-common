/*
  Copyright 2018 Statoil ASA

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <opm/output/eclipse/AggregateUDQData.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
//#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
//#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
//#include <opm/parser/eclipse/EclipseState/Schedule/GroupTree.hpp>
//#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQInput.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQDefine.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQAssign.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQParams.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQFunctionTable.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>
#include <iostream>

// #####################################################################
// Class Opm::RestartIO::Helpers::AggregateGroupData
// ---------------------------------------------------------------------


namespace {


    template <typename UDQOp>
    void UDQLoop(const std::size_t no_udqs,
                  UDQOp&&          udqOp)
    {
        auto udqID = std::size_t{0};
        for (std::size_t iudq = 0; iudq < no_udqs; iudq++) {
            udqID += 1;

            udqOp(iudq, udqID - 1);
        }
    }
    
    /*std::string hash(const std::string& udq, const std::string& keyword, const std::string& control) {
	return udq + keyword + control;
    }*/
    

    
    namespace iUdq {

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(udqDims[0]) },
                WV::WindowSize{ static_cast<std::size_t>(udqDims[1]) }
            };
        }

        template <class IUDQArray>
        void staticContrib(const Opm::Schedule& sched, 
			   const std::size_t simStep,
			   const int indUdq,
			   IUDQArray& iUdq)
        {
	    // loop over the current UDQs and write the required properties to the restart file
	    
	    //Get UDQ config data for the actual report step
	    auto udqCfg = sched.getUDQConfig(simStep);
	    const std::string& key = udqCfg.udqKey(indUdq);
	    if (udqCfg.has_keyword(key)) {
		// entry 1 is type of UDQ (0: ASSIGN and UPDATE-OFF, 1: ASSIGN and UPDATE-NEXT, 2: DEFINE and UPDATE-ON)
		iUdq[0] = (udqCfg.is_define(key)) ? 2 : 0;
		// entry 2: 0 for ASSIGN - numerical value, -4: DEFINE , 
		iUdq[1] =  (udqCfg.is_define(key)) ? -4 : 0;
		// entry 3 sequence number of UDQ pr type (CU, FU, GU, RU, , SU, WU, AU or BU etc.) (1-based)
		const std::size_t var_typ = static_cast<std::size_t>(Opm::UDQ::varType(key));
		iUdq[2] = udqCfg.keytype_keyname_seq_no(var_typ, key);
	    }
	}
    } // iUdq
    
        namespace iUad {

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(udqDims[2]) },
                WV::WindowSize{ static_cast<std::size_t>(udqDims[3]) }
            };
        }

        template <class IUADArray>
        void staticContrib(const Opm::RestartIO::Helpers::iUADData& iuad_data, 
			   const int indUad,
			   IUADArray& iUad)
        {
	    // numerical key for well/group control keyword plus control type, see iUADData::UDACtrlType
	    iUad[0] = iuad_data.wgkey_ctrl_type()[indUad];
	    // sequence number of UDQ used (from input sequence) for the actual constraint/target
	    iUad[1] =  iuad_data.udq_seq_no()[indUad];
	    // entry 3  - unknown meaning - value = 1
	    iUad[2] = 1;
	    // entry 4  - 
	    iUad[3] = iuad_data.no_use_wgkey()[indUad];
	    // entry 3  - 
	    iUad[4] = iuad_data.first_use_wg()[indUad]; 
	}
    } // iUad
    
} // Anonymous

// =====================================================================


    template < typename T>
    std::pair<bool, int > findInVector(const std::vector<T>  & vecOfElements, const T  & element)
    {
	std::pair<bool, int > result;
    
	// Find given element in vector
	auto it = std::find(vecOfElements.begin(), vecOfElements.end(), element);
    
	if (it != vecOfElements.end())
	{
	    result.second = std::distance(vecOfElements.begin(), it);
	    result.first = true;
	}
	else
	{
	    result.first = false;
	    result.second = -1;
	}
        return result;
    }
    /*void
    Opm::RestartIO::Helpers::iUADData::
    noIUDAs(const Opm::Schedule& sched, const std::size_t simStep, const Opm::UDQActive& udq_active)
    {
	auto udq_cfg = sched.getUDQConfig(simStep);
	//auto udq_active = sched.udqActive(simStep);
	
	// Loop over the number of Active UDQs and set all UDQActive restart data items
	
	std::vector<std::size_t>	wgkey_ctrl_type;
	std::vector<std::size_t> 	udq_seq_no;
	std::vector<std::size_t> 	no_use_wgkey;
	std::vector<std::size_t> 	first_use_wg; 
	std::size_t count = 0;
	
	auto mx_iuads = udq_active.size();
	wgkey_ctrl_type.resize(mx_iuads, 0);
	udq_seq_no.resize(mx_iuads, 0);
	no_use_wgkey.resize(mx_iuads, 0);
	first_use_wg.resize(mx_iuads, 0);
	
	std::cout << "noIUDAs:  ind, udq_key, ctrl_keywrd, name, ctrl_type" << std::endl;
	std::size_t cnt_inp = 0;
	for (auto it = udq_active.begin(); it != udq_active.end(); it++) 
	{
	    cnt_inp+=1;
	    auto ind = it->index;
	    auto udq_key = it->udq;
	    auto ctrl_keywrd = it->keyword;
	    auto name = it->wgname;
	    auto ctrl_type = it->control;
	    std::string wg_kc = ctrl_keywrd + "_" + ctrl_type;
	    
	    //auto hash_key = hash(udq_key, ctrl_keywrd, ctrl_type);
	    const auto key_it = iUADData::UDACtrlType.find(wg_kc);
	    const std::size_t v_typ = static_cast<std::size_t>(key_it->second);
	    std::pair<bool,std::size_t> res = findInVector<std::size_t>(wgkey_ctrl_type, v_typ);
	    if (res.first) {
		//key already exist
		auto key_ind = res.second;
		no_use_wgkey[key_ind]+=1;
	    }
	    else {
		//new key
		wgkey_ctrl_type[count] = v_typ;
		const std::size_t var_typ = static_cast<std::size_t>(Opm::UDQ::varType(udq_key));
		udq_seq_no[count] = udq_cfg.keytype_keyname_seq_no(var_typ, udq_key);
		no_use_wgkey[count] = 1;
		first_use_wg[count] =  cnt_inp;
		count+=1;
	    }
	    
	}
	this->m_wgkey_ctrl_type = wgkey_ctrl_type;
	this->m_udq_seq_no = udq_seq_no;
	this->m_no_use_wgkey = no_use_wgkey;
	this->m_first_use_wg = first_use_wg;
	this->m_count = count;
     } */

         void  Opm::RestartIO::Helpers::iUADData::noIUDAs(const Opm::Schedule& sched, const std::size_t simStep, const Opm::UDQActive& udq_active)
    {
	auto udq_cfg = sched.getUDQConfig(simStep);
	//auto udq_active = sched.udqActive(simStep);
	
	// Loop over the number of Active UDQs and set all UDQActive restart data items
	
	std::vector<std::string>	wgkey_udqkey_ctrl_type;
	std::vector<int>		wgkey_ctrl_type;
	std::vector<int> 		udq_seq_no;
	std::vector<int> 		no_use_wgkey;
	std::vector<int> 		first_use_wg; 
	std::size_t count = 0;
	
	auto mx_iuads = udq_active.size();
	wgkey_udqkey_ctrl_type.resize(mx_iuads, "");
	wgkey_ctrl_type.resize(mx_iuads, 0);
	udq_seq_no.resize(mx_iuads, 0);
	no_use_wgkey.resize(mx_iuads, 0);
	first_use_wg.resize(mx_iuads, 0);
	
	std::cout << "noIUDAs:  ind, udq_key, ctrl_keywrd, name, ctrl_type wg_kc wg_udqk_kc" << std::endl;
	//std::size_t cnt_inp = 0;
	for (auto it = udq_active.begin(); it != udq_active.end(); it++) 
	{
	    //cnt_inp+=1;
	    auto ind = it->index;
	    auto udq_key = it->udq;
	    auto ctrl_keywrd = it->keyword;
	    auto name = it->wgname;
	    auto ctrl_type = it->control;
	    std::string wg_kc = ctrl_keywrd + "_" + ctrl_type;
	    std::string wg_udqk_kc = ctrl_keywrd + "_" + udq_key + "_" + ctrl_type;
	    
	    std::cout << "noIUDAs:" << ind << " " <<  udq_key << " " << ctrl_keywrd << " " << name << " " << ctrl_type << " " << wg_kc << " " << wg_udqk_kc << std::endl;
	    
	    const auto key_it = iUADData::UDACtrlType.find(wg_kc);
	    if (key_it == iUADData::UDACtrlType.end()) {
		std::cout << "Invalid argument - end of map loc_iUADData::UDACtrlType: " << wg_kc << std::endl; 
		throw std::invalid_argument("noIUDAs - UDACtrlType - unknown ctrl_key_type " + wg_kc);
	    }
	    else {
		const int v_typ = key_it->second;
		std::pair<bool,int> res = findInVector<std::string>(wgkey_udqkey_ctrl_type, wg_udqk_kc);
		if (res.first) {
		    //key already exist
		    auto key_ind = res.second;
		    no_use_wgkey[key_ind] += 1;
		    std::cout << "key exists - key_ind:" << key_ind << " no_use_wgkey: " << no_use_wgkey[key_ind] << std::endl;
		}
		else {
		    //new key
		    wgkey_ctrl_type[count] = v_typ;
		    wgkey_udqkey_ctrl_type[count] = wg_udqk_kc;
		    //const std::size_t var_typ = static_cast<std::size_t>(Opm::UDQ::varType(udq_key));
		    //udq_seq_no[count] = udq_cfg.keytype_keyname_seq_no(var_typ, udq_key);
		    udq_seq_no[count] = udq_cfg.key_seq_no(udq_key);
		    no_use_wgkey[count] = 1;
		    //first_use_wg[count] =  cnt_use;
		    
		    std::cout << "new key - key_ind:" << count << " wgkey_ctrl_type: " << wgkey_ctrl_type[count] << " udq_seq_no: " << udq_seq_no[count];
		    std::cout << " no_use_wgkey:" << no_use_wgkey[count] << " first_use_wg: " << first_use_wg[count] <<  std::endl;
		    
		    count+=1;
	      }
	    }
	}
	
	// Loop over all iUADs to set the first use paramter
	int cnt_use = 0;
	for (auto it = 0; it < count; it++) {
	    first_use_wg[it] =  cnt_use + 1;
	    cnt_use += no_use_wgkey[it];
	}
	
	this->m_wgkey_ctrl_type = wgkey_ctrl_type;
	this->m_udq_seq_no = udq_seq_no;
	this->m_no_use_wgkey = no_use_wgkey;
	this->m_first_use_wg = first_use_wg;
	this->m_count = count;
     }
     
Opm::RestartIO::Helpers::AggregateUDQData::
AggregateUDQData(const std::vector<int>& udqDims)
    : iUDQ_ (iUdq::allocate(udqDims)),
      iUAD_ (iUad::allocate(udqDims))
{}

// ---------------------------------------------------------------------

void
Opm::RestartIO::Helpers::AggregateUDQData::
captureDeclaredUDQData(const Opm::Schedule&                 sched,
		       const Opm::UDQActive& 		udq_active,
		       const std::size_t                    simStep)
{
    //get list of current UDQs
    auto udqCfg = sched.getUDQConfig(simStep);
    //get the number of UDQs present
    const auto& no_udq = udqCfg.noUdqs();
    
    Opm::RestartIO::Helpers::iUADData iuad_data;
    iuad_data.noIUDAs(sched, simStep, udq_active);
    const auto& no_iuad = iuad_data.count(); 
    
    UDQLoop(no_udq, [&sched, simStep, this]
        (const std::size_t& ind_iudq, const std::size_t udqID) -> void
    {
        auto i_udq = this->iUDQ_[udqID];

        iUdq::staticContrib(sched, simStep, ind_iudq, i_udq);
    });
    
        UDQLoop(no_iuad, [&iuad_data, this]
        (const std::size_t& ind_iuad, const std::size_t uadID) -> void
    {
        auto i_uad = this->iUAD_[uadID];

        iUad::staticContrib(iuad_data, ind_iuad, i_uad);
    });
}