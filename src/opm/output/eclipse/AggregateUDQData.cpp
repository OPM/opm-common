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
#include <opm/output/eclipse/AggregateGroupData.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
//#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>


#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQConfig.hpp>
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
  
    // maximum number of groups
    std::size_t ngmaxz(const std::vector<int>& inteHead)
    {
        return inteHead[20];
    }


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
        void staticContrib(const Opm::UDQInput& udq_input, IUDQArray& iUdq)
        {
            if (udq_input.is<Opm::UDQDefine>()) {
                iUdq[0] = 2;
                iUdq[1] = -4;
            } else {
                iUdq[0] = 0;
                iUdq[1] = 0;
            }
            iUdq[2] = udq_input.index.typed_insert_index;
        }

    } // iUdq

    namespace iUad {

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;
            printf("iUAD num:%d  ws:%d \n ", udqDims[2], udqDims[3]);
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


    namespace zUdn {

        Opm::RestartIO::Helpers::WindowedArray<
            Opm::EclIO::PaddedOutputString<8>
        >
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<
                Opm::EclIO::PaddedOutputString<8>
            >;

            return WV {
                WV::NumWindows{ static_cast<std::size_t>(udqDims[0]) },
                WV::WindowSize{ static_cast<std::size_t>(udqDims[4]) }
            };
        }

    template <class zUdnArray>
    void staticContrib(const Opm::UDQInput& udq_input, zUdnArray& zUdn)
    {
        // entry 1 is udq keyword
        zUdn[0] = udq_input.keyword();
        zUdn[1] = udq_input.unit();
    }
    } // zUdn

    namespace zUdl {

        Opm::RestartIO::Helpers::WindowedArray<
            Opm::EclIO::PaddedOutputString<8>
        >
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<
                Opm::EclIO::PaddedOutputString<8>
            >;

            return WV {
                WV::NumWindows{ static_cast<std::size_t>(udqDims[0]) },
                WV::WindowSize{ static_cast<std::size_t>(udqDims[5]) }
            };
        }

    template <class zUdlArray>
    void staticContrib(const Opm::UDQInput& input, zUdlArray& zUdl)
        {
            int l_sstr = 8;
            int max_l_str = 128;
            // write out the input formula if key is a DEFINE udq
            if (input.is<Opm::UDQDefine>()) {
                const auto& udq_define = input.get<Opm::UDQDefine>();
                const std::string& z_data = udq_define.input_string();
                int n_sstr =  z_data.size()/l_sstr;
                if (static_cast<int>(z_data.size()) > max_l_str) {
                    std::cout << "Too long input data string (max 128 characters): " << z_data << std::endl;
                    throw std::invalid_argument("UDQ - variable: " + udq_define.keyword());
                }
                else {
                    for (int i = 0; i < n_sstr; i++) {
                        zUdl[i] = z_data.substr(i*l_sstr, l_sstr);
                    }
                    //add remainder of last non-zero string
                    if ((z_data.size() % l_sstr) > 0)
                        zUdl[n_sstr] = z_data.substr(n_sstr*l_sstr);
                }
            }
        }
    } // zUdl

    namespace iGph {

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(udqDims[6]) },
                WV::WindowSize{ static_cast<std::size_t>(1) }
            };
        }

        template <class IGPHArray>
        void staticContrib(const Opm::Schedule&    sched,
			   const Opm::Group&       group,
			   const std::size_t       simStep,
			   const int 		   indGph,
			   IGPHArray& 		   iGph)
        {
	    
	}
    } // iGph

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

    void  Opm::RestartIO::Helpers::iUADData::iuad(const Opm::Schedule& sched, const std::size_t simStep)
    {
	auto udq_cfg = sched.getUDQConfig(simStep);
	auto udq_active = sched.udqActive(simStep);
	
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
	
	std::size_t cnt_inp = 0;
	for (auto it = udq_active.begin(); it != udq_active.end(); it++) 
	{
	    cnt_inp+=1;
	    auto udq_key = it->udq;
	    //auto ctrl_keywrd = it->keyword;
	    auto name = it->wgname;
	    auto ctrl_type = it->control;
	    //std::cout << "ctrl_type: " << static_cast<int>(ctrl_type) << std::endl;
	    std::string wg_udqk_kc = udq_key + "_" + std::to_string(static_cast<int>(ctrl_type));
	    
	    //std::cout << "iuad:" << ind << " " <<  udq_key << " " << name << " " << static_cast<int>(ctrl_type) << " " << wg_udqk_kc << std::endl;

	    const auto v_typ = UDQ::uadCode(ctrl_type);
	    std::pair<bool,int> res = findInVector<std::string>(wgkey_udqkey_ctrl_type, wg_udqk_kc);
	    if (res.first) {
		auto key_ind = res.second;
		no_use_wgkey[key_ind] += 1;
		
		//std::cout << "key exists - key_ind:" << key_ind << " no_use_wgkey: " << no_use_wgkey[key_ind] << std::endl;
	    }
	    else {
		wgkey_ctrl_type[count] = v_typ;
		wgkey_udqkey_ctrl_type[count] = wg_udqk_kc;
		udq_seq_no[count] = udq_cfg[udq_key].index.insert_index;
		no_use_wgkey[count] = 1;
		
		//std::cout << "new key - key_ind:" << count << " wgkey_ctrl_type: " << wgkey_ctrl_type[count] << " udq_seq_no: " << udq_seq_no[count];
		//std::cout << " no_use_wgkey:" << no_use_wgkey[count] << " first_use_wg: " << first_use_wg[count] <<  std::endl;
		
		count+=1;
	      }
	}
	
	// Loop over all iUADs to set the first use paramter
	int cnt_use = 0;
	for (std::size_t it = 0; it < count; it++) {
	    first_use_wg[it] =  cnt_use + 1;
	    cnt_use += no_use_wgkey[it];
	}

	this->m_wgkey_ctrl_type = wgkey_ctrl_type;
	this->m_udq_seq_no = udq_seq_no;
	this->m_no_use_wgkey = no_use_wgkey;
	this->m_first_use_wg = first_use_wg;
	this->m_count = count;
     }
     
    const std::map <size_t, const Opm::Group*>  Opm::RestartIO::Helpers::igphData::currentGroupMapIndexGroup(const Opm::Schedule& sched, 
									  const size_t simStep, 
									  const std::vector<int>& inteHead)
    {
        // make group index for current report step
        std::map <size_t, const Opm::Group*> indexGroupMap;
        for (const auto& group_name : sched.groupNames(simStep)) {
            const auto& group = sched.getGroup(group_name);
            int ind = (group.name() == "FIELD")
                ? ngmaxz(inteHead)-1 : group.seqIndex()-1;

            const std::pair<size_t, const Opm::Group*> groupPair = std::make_pair(static_cast<size_t>(ind), std::addressof(group));
            indexGroupMap.insert(groupPair);
        }
        return indexGroupMap;
    }
     
     const std::vector<int> Opm::RestartIO::Helpers::igphData::ig_phase(const Opm::Schedule& sched, 
								   const std::size_t simStep, 
								   const std::vector<int>& inteHead) 
     {
	//construct the current list of groups to output the IGPH array
	const auto indexGroupMap = Opm::RestartIO::Helpers::igphData::currentGroupMapIndexGroup(sched, simStep, inteHead);
	//std::vector<const Opm::Group*> curGroups(ngmaxz(inteHead), nullptr);
	std::vector<int> inj_phase(ngmaxz(inteHead), 0);
	
	auto it = indexGroupMap.begin();
	while (it != indexGroupMap.end())
	{
	    auto ind = static_cast<int>(it->first);
	    auto group_ptr = it->second;
	    if (group_ptr->isInjectionGroup(simStep)) {
		auto phase = group_ptr->getInjectionPhase(simStep);
	      if ( phase == Opm::Phase::WATER ) inj_phase[ind] = 2;
	    }
	    it++;
	}
     }
     
Opm::RestartIO::Helpers::AggregateUDQData::
AggregateUDQData(const std::vector<int>& udqDims)
    : iUDQ_ (iUdq::allocate(udqDims)),
      iUAD_ (iUad::allocate(udqDims)),
      zUDN_ (zUdn::allocate(udqDims)),
      zUDL_ (zUdl::allocate(udqDims)),
      iGPH_ (iGph::allocate(udqDims))
{}

// ---------------------------------------------------------------------

void
Opm::RestartIO::Helpers::AggregateUDQData::
captureDeclaredUDQData(const Opm::Schedule&                 sched,
                       const std::size_t                    simStep,
                       const std::vector<int>&              inteHead)
{
    auto udqCfg = sched.getUDQConfig(simStep);
    auto udq_active = sched.udqActive(simStep);
    for (const auto& udq_input : udqCfg.input()) {
        auto udq_index = udq_input.index.insert_index;
        {
            auto i_udq = this->iUDQ_[udq_index];
            iUdq::staticContrib(udq_input, i_udq);
        }
        {
            auto z_udn = this->zUDN_[udq_index];
            zUdn::staticContrib(udq_input, z_udn);
        }
        {
            auto z_udl = this->zUDL_[udq_index];
            zUdl::staticContrib(udq_input, z_udl);
        }
    }

    if (udq_active) {
        Opm::RestartIO::Helpers::iUADData iuad_data;
        iuad_data.noIUADs(sched, simStep);

        // Loop not correct ...
        for (const auto& udq_input : udqCfg.input()) {
            auto i_uad = this->iUAD_[udq_index];
            iUad::staticContrib(iuad_data, udq_input.index.insert_index, i_uad);
        }
    }

}
