/*
  Copyright 2018 Statoil ASA.

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

#include <algorithm>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQInput.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQParams.hpp>
#include <iostream>

namespace Opm {

    namespace {
        std::string strip_quotes(const std::string& s) {
            if (s[0] == '\'')
                return s.substr(1, s.size() - 2);
            else
                return s;
        }

    }

    UDQInput::UDQInput(const Deck& deck) :
        udq_params(deck),
        udqft(this->udq_params)
    {
    }

    const UDQParams& UDQInput::params() const {
        return this->udq_params;
    }

    void UDQInput::add_record(const DeckRecord& record) {
        auto action = UDQ::actionType(record.getItem("ACTION").get<std::string>(0));
        const auto& quantity = record.getItem("QUANTITY").get<std::string>(0);
        const auto& data = record.getItem("DATA").getData<std::string>();
	typedef std::unordered_map<std::size_t, std::unordered_map<std::string, std::size_t> > map_kksn;

        if (action == UDQAction::UPDATE)
            throw std::invalid_argument("The UDQ action UPDATE is not yet implemented in opm/flow");

        if (action == UDQAction::UNITS)
            this->assign_unit( quantity, data[0] );
        else {
            auto index_iter = this->input_index.find(quantity);
            if (this->input_index.find(quantity) == this->input_index.end())
                this->input_index[quantity] = std::make_pair(this->input_index.size(), action);
            else
                index_iter->second.second = action;


            if (action == UDQAction::ASSIGN) {
                std::vector<std::string> selector(data.begin(), data.end() - 1);
                double value = std::stod(data.back());
		this->m_is_define[quantity] = false;
                auto assignment = this->m_assignments.find(quantity);
                if (assignment == this->m_assignments.end())
                    this->m_assignments.insert( std::make_pair(quantity, UDQAssign(quantity, selector, value )));
                else
                    assignment->second.add_record(selector, value);
            } 
            else if (action == UDQAction::DEFINE) {
                this->m_definitions.insert( std::make_pair(quantity, UDQDefine(this->udq_params, quantity, data)));
		this->m_is_define[quantity] = true;
		std::string all_data = "";
		for (auto it = data.begin(); it != data.end(); it++) {
		    all_data+= *it + " ";
		}
		std::cout << "UDQInput - all_data: " << all_data << std::endl;
		this->m_udqdef_data[quantity] = all_data;
	    }
            else 
                throw std::runtime_error("Internal error - should not be here");
	    
	    //std::cout << "UDQInput_gen - treat new keywords" << std::endl;
	    if (!this->has_udqkey(quantity)) {
	      std::size_t no_udqs = m_udq_keys.size();
	      this->m_udq_keys.emplace_back(quantity);
	      this->m_key_seq_no[quantity] = no_udqs + 1;
	      std::cout << "UDQInput_gen - m_key_seq_no: " << this->m_key_seq_no[quantity] << std::endl;
	      const std::size_t var_typ = static_cast<std::size_t>(UDQ::varType(quantity));

	      map_kksn::const_iterator vt_it = this->m_keytype_keyname_seq_no.find(var_typ);
	      if (vt_it != this->m_keytype_keyname_seq_no.end()) {
		auto vt_sz = (vt_it->second).size();
		this->m_keytype_keyname_seq_no[var_typ][quantity] = vt_sz + 1;
	      }
	      else {
		this->m_keytype_keyname_seq_no[var_typ][quantity] = 1;
	      }
	    }
	    this->keywords.insert(quantity);
	    for (auto item : this->keywords) {
		std::cout << "UDQInput_gen - keywords " << item << std::endl;
	    }
        }
    }


    std::vector<UDQDefine> UDQInput::definitions() const {
        std::vector<UDQDefine> ret;
        for (const auto& index_pair : this->input_index) {
            if (index_pair.second.second == UDQAction::DEFINE) {
                const std::string& key = index_pair.first;
                ret.push_back(this->m_definitions.at(key));
            }
        }
        return ret;
    }


    std::vector<UDQDefine> UDQInput::definitions(UDQVarType var_type) const {
        std::vector<UDQDefine> filtered_defines;
        for (const auto& index_pair : this->input_index) {
            if (index_pair.second.second == UDQAction::DEFINE) {
                const std::string& key = index_pair.first;
                const auto& udq_define = this->m_definitions.at(key);
                if (udq_define.var_type() == var_type)
                    filtered_defines.push_back(udq_define);
            }
        }
        return filtered_defines;
    }


    std::vector<std::pair<size_t, UDQDefine>> UDQInput::input_definitions() const {
        std::vector<std::pair<size_t, UDQDefine>> res;
        for (const auto& index_pair : this->input_index) {
            if (index_pair.second.second == UDQAction::DEFINE) {
                const std::string& key = index_pair.first;
                res.emplace_back(index_pair.second.first, this->m_definitions.at(key));
            }
        }
        return res;
    }


    std::vector<UDQAssign> UDQInput::assignments() const {
        std::vector<UDQAssign> ret;
        for (const auto& index_pair : this->input_index) {
            if (index_pair.second.second == UDQAction::ASSIGN) {
                const std::string& key = index_pair.first;
                ret.push_back(this->m_assignments.at(key));
            }
        }
        return ret;
    }


    std::vector<UDQAssign> UDQInput::assignments(UDQVarType var_type) const {
        std::vector<UDQAssign> filtered_defines;
        for (const auto& index_pair : this->input_index) {
            if (index_pair.second.second == UDQAction::ASSIGN) {
                const std::string& key = index_pair.first;
                const auto& udq_define = this->m_assignments.at(key);
                if (udq_define.var_type() == var_type)
                    filtered_defines.push_back(udq_define);
            }
        }
        return filtered_defines;
    }



    const std::string& UDQInput::unit(const std::string& key) const {
        const auto pair_ptr = this->units.find(key);
        if (pair_ptr == this->units.end())
            throw std::invalid_argument("No such UDQ quantity: " + key);

        return pair_ptr->second;
    }


    void UDQInput::assign_unit(const std::string& keyword, const std::string& quoted_unit) {
        const std::string unit = strip_quotes(quoted_unit);
        const auto pair_ptr = this->units.find(keyword);
        if (pair_ptr != this->units.end()) {
            if (pair_ptr->second != unit)
                throw std::invalid_argument("Illegal to change unit of UDQ keyword runtime");

            return;
        }
        this->units[keyword] = unit;
    }

    bool UDQInput::has_unit(const std::string& keyword) const {
        return (this->units.count(keyword) > 0);
    }


    bool UDQInput::has_keyword(const std::string& keyword) const {
        if (this->m_assignments.count(keyword) > 0)
            return true;

        if (this->m_definitions.count(keyword) > 0)
            return true;

        /*
          That a keyword is mentioned with UNITS is enough to consider it
           as a keyword which is present.
        */
        if (this->units.count(keyword) > 0)
            return true;

        return false;
    }
    
    bool UDQInput::has_udqkey(const std::string& keyword) const {
	auto cnt = std::count (this->m_udq_keys.begin(), this->m_udq_keys.end(), keyword);
        if ( cnt > 0) {
	    std::cout << "keyword: " << keyword << " " << " m_udq_keys - count: "  <<  cnt << std::endl;
	    return true;
	}
        else {
	  std::cout << "keyword: " << keyword << " " << "has_keydef - keyword not found: "  << std::endl;
	  return false;
	}
    }
    
    const std::size_t& UDQInput::key_seq_no(const std::string key) const {
      	const auto pair_ptr = this->m_key_seq_no.find(key);
        if (pair_ptr == this->m_key_seq_no.end()) {
            throw std::invalid_argument("UDQInput - key_seq_no - unknown UDQ quantity" + key);
	}
	else
	  return pair_ptr->second;
    }
    
    const std::string& UDQInput::udqdef_data(const std::string key) const {
      	const auto pair_ptr = this->m_udqdef_data.find(key);
        if (pair_ptr == this->m_udqdef_data.end()) {
            throw std::invalid_argument("UDQInput - udqdef_data - unknown UDQ quantity" + key);
	}
	else
	  return pair_ptr->second;
    }

    
    const std::size_t& UDQInput::keytype_keyname_seq_no(const std::size_t keytype, const std::string keyname) const {
      	const auto outer_ptr = this->m_keytype_keyname_seq_no.find(keytype);
        if (outer_ptr == this->m_keytype_keyname_seq_no.end()) {
            throw std::invalid_argument("UDQInput - m_keytype_keyname_seq_no - unknown key type " + keytype);
	}
	else {
	    const auto map_keyname = outer_ptr->second;
	    const auto inner_ptr = map_keyname.find(keyname);
	    if (inner_ptr == map_keyname.end()) {
		throw std::invalid_argument("UDQInput - m_keytype_keyname_seq_no - unknown key name " + keyname);
	    }
	    else {
		return inner_ptr->second;
	    }
	}
    }

    bool UDQInput::is_define(const std::string& keyword) const {
	bool define = false;
	const auto pair_ptr = this->m_is_define.find(keyword);
        if (pair_ptr == this->m_is_define.end()) {
            throw std::invalid_argument("UDQInput - is_define - unknown UDQ quantity" + keyword);
	}
	else define = pair_ptr->second;
        return define;
    }
    
    const UDQFunctionTable& UDQInput::function_table() const {
        return this->udqft;
    }
    
    const std::string& UDQInput::udqKey(const std::size_t udq_no) const {
        return this->m_udq_keys[udq_no];
    }
    
    const std::size_t UDQInput::noUdqs() {
        return this->m_udq_keys.size();
    }
}
