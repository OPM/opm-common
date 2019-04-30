/*
  Copyright (c) 2018 Equinor ASA
  Copyright (c) 2018 Statoil ASA

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

#include <opm/output/eclipse/DoubHEAD.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQInput.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>

#include <chrono>
#include <cstddef>
#include <vector>

namespace {

    std::size_t noUDQs(const Opm::Schedule& sched, const std::size_t simStep)
    {
	auto udqCfg = sched.getUDQConfig(simStep);
	// find the number of UDQs for the current time step
	std::size_t no_udqs = udqCfg.noUdqs();
	return no_udqs;
    }  

#if 0
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
    

    void  Opm::RestartIO::Helpers::iUADData::noIUDAs(const Opm::Schedule& sched, const std::size_t simStep, const Opm::UDQActive& udq_active)
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
	
	std::cout << "noIUDAs:  ind, udq_key, ctrl_keywrd, name, ctrl_type wg_kc " << std::endl;
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
	    
	    std::cout << "noIUDAs:" << ind << " " <<  udq_key << " " << ctrl_keywrd << " " << name << " " << ctrl_type << " " << wg_kc << std::endl;
	    
	    //auto hash_key = hash(udq_key, ctrl_keywrd, ctrl_type);
	    const auto key_it = iUADData::UDACtrlType.find(wg_kc);
	    if (key_it == iUADData::UDACtrlType.end()) {
		std::cout << "Invalid argument - end of map loc_iUADData::UDACtrlType: " << wg_kc << std::endl; 
		throw std::invalid_argument("noIUDAs - UDACtrlType - unknown ctrl_key_type " + wg_kc);
	    }
	    else {
		const std::size_t v_typ = static_cast<std::size_t>(key_it->second);
		std::pair<bool,std::size_t> res = findInVector<std::size_t>(wgkey_ctrl_type, v_typ);
		if (res.first) {
		    //key already exist
		    auto key_ind = res.second;
		    no_use_wgkey[key_ind]+=1;
		    std::cout << "key exists - key_ind:" << key_ind << " no_use_wgkey: " << no_use_wgkey[key_ind] << std::endl;
		}
		else {
		    //new key
		    wgkey_ctrl_type[count] = v_typ;
		    const std::size_t var_typ = static_cast<std::size_t>(Opm::UDQ::varType(udq_key));
		    udq_seq_no[count] = udq_cfg.keytype_keyname_seq_no(var_typ, udq_key);
		    no_use_wgkey[count] = 1;
		    first_use_wg[count] =  cnt_inp;
		    
		    std::cout << "new key - key_ind:" << count << " wgkey_ctrl_type: " << wgkey_ctrl_type[count] << " udq_seq_no: " << udq_seq_no[count];
		    std::cout << " no_use_wgkey:" << no_use_wgkey[count] << " first_use_wg: " << first_use_wg[count] <<  std::endl;
		    
		    count+=1;
	      }
	    }
	}
	this->m_wgkey_ctrl_type = wgkey_ctrl_type;
	this->m_udq_seq_no = udq_seq_no;
	this->m_no_use_wgkey = no_use_wgkey;
	this->m_first_use_wg = first_use_wg;
	this->m_count = count;
    }
#endif    

    std::size_t entriesPerIUDQ()
    {
	std::size_t no_entries = 3;
        return no_entries;
    }
    
    std::size_t entriesPerIUAD()
    {
	std::size_t no_entries = 5;
        return no_entries;
    }
} // Anonymous

// #####################################################################
// Public Interface (createUdqDims()) Below Separator
// ---------------------------------------------------------------------

std::vector<int>
Opm::RestartIO::Helpers::
createUdqDims(const Schedule&     	sched,
	      const Opm::UDQActive& 	udq_active,
              const std::size_t        	lookup_step) 
{
    Opm::RestartIO::Helpers::iUADData iuad_data;
    iuad_data.noIUDAs(sched, lookup_step, udq_active);
    const auto& no_iuad = iuad_data.count();
    std::vector<int> udqDims; 
    
    udqDims.emplace_back(noUDQs(sched, lookup_step));
    
    udqDims.emplace_back(entriesPerIUDQ());
    
    udqDims.emplace_back(no_iuad);
    
    udqDims.emplace_back(entriesPerIUAD());

    return udqDims;
}
