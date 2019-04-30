/*
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

#ifndef OPM_AGGREGATE_UDQ_DATA_HPP
#define OPM_AGGREGATE_UDQ_DATA_HPP

#include <opm/output/eclipse/WindowedArray.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQInput.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQDefine.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQAssign.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQParams.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQFunctionTable.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <cstddef>
#include <string>
#include <vector>
#include <map>

namespace Opm {
    class Schedule;
    class UDQInput;
    class UDQActive;
} // Opm

namespace Opm { namespace RestartIO { namespace Helpers {

    class iUADData {
    public:
	const std::vector <std::string>& wgkey_udqkey_ctrl_type() const {
	      return m_wgkey_udqkey_ctrl_type;
	}
	const std::vector <int>& wgkey_ctrl_type() const {
	      return m_wgkey_ctrl_type;
	}
	const std::vector<int>&	udq_seq_no() const {
	      return m_udq_seq_no;
	}
	const std::vector<int>&	no_use_wgkey() const {
	      return m_no_use_wgkey;
	}
	const std::vector<int>&	first_use_wg() const {
	      return m_first_use_wg; 
	}
	std::size_t count() {
	  return m_count;
	}
		
    std::unordered_map<std::string,int> UDACtrlType {
      { "NONE", 0 },
      { "GCONPROD_ORAT", 200019 },
      { "GCONPROD_WRAT", 300019 },
      { "GCONPROD_GRAT", 400019 },
      { "GCONPROD_LRAT", 500019 },
      
      { "WCONPROD_ORAT", 300004 },
      { "WCONPROD_WRAT", 400004 },
      { "WCONPROD_GRAT", 500004 },
      { "WCONPROD_LRAT", 600004 },
      
      { "WCONINJE_ORAT", 300003 },
      { "WCONINJE_WRAT", 400003 },
      { "WCONINJE_GRAT", 500003 },
    };

    void noIUDAs(const Opm::Schedule& sched, const std::size_t simStep, const Opm::UDQActive& udq_active);

    private:
	std::vector<std::string>	m_wgkey_udqkey_ctrl_type;
	std::vector<int>		m_wgkey_ctrl_type;
	std::vector<int> 		m_udq_seq_no;
	std::vector<int> 		m_no_use_wgkey;
	std::vector<int> 		m_first_use_wg; 
	std::size_t m_count;
    };
  
    class AggregateUDQData
    {
    public:
	explicit AggregateUDQData(const std::vector<int>& udqDims);

	void captureDeclaredUDQData(const Opm::Schedule&	sched,
				    const Opm::UDQActive& 	udq_active,
				    const std::size_t           simStep);

        const std::vector<int>& getIUDQ() const
        {
            return this->iUDQ_.data();
        }

        const std::vector<int>& getIUAD() const
        {
            return this->iUAD_.data();
        }
#if 0        
        const std::vector<int>& getIUAP() const
        {
            return this->iUAP_.data();
        }
        
        const std::vector<int>& getIGPH() const
        {
            return this->iGPH_.data();
        }

        const std::vector<double>& getDUDW() const
        {
            return this->dUDW_.data();
        }
        
        const std::vector<double>& getDUDF() const
        {
            return this->dUDF_.data();
        }

        const std::vector<CharArrayNullTerm<8>>& getZUDN() const
        {
            return this->zUDN_.data();
        }
        
        const std::vector<CharArrayNullTerm<8>>& getZUDL() const
        {
            return this->zUDL_.data();
        }
#endif


    private:
        /// Aggregate 'IUDQ' array (Integer) for all UDQ data  (3 integers pr UDQ)
        WindowedArray<int> iUDQ_;

	/// Aggregate 'IUAD' array (Integer) for all UDQ data  (5 integers pr UDQ that is used for various well and group controls)
        WindowedArray<int> iUAD_;
#if 0	
	/// Aggregate 'IUAP' array (Integer) for all UDQ data  (1 integer pr UDQ constraint used)
        WindowedArray<int> iUAP_;
	
	/// Aggregate 'IGPH' array (Integer) for all UDQ data  (3 - zeroes - as of current understanding)
        WindowedArray<int> iGPH_;
	
        /// Aggregate 'SWEL' array (Real) for all wells.
        //WindowedArray<float> sGroup_;

        /// Aggregate 'DUDW' array (Double Precision) for all UDQ data. (Dimension = max no wells * noOfUDQ's)
        WindowedArray<double> dUDW_;
	
	/// Aggregate 'DUDF' array (Double Precision) for all UDQ data.  (Dimension = Number of FU - UDQ's, with value equal to the actual constraint)
        WindowedArray<double> dUDF_;

        /// Aggregate 'ZUDN' array (Character) for all UDQ data. (2 * 8 chars pr UDQ -> UNIT keyword)
        WindowedArray<CharArrayNullTerm<8>> zUDN_;

        /// Aggregate 'ZUDL' array (Character) for all UDQ data.  (16 * 8 chars pr UDQ DEFINE "Data for operation - Msth Expression)
	WindowedMatrix<CharArrayNullTerm<8>> zUDL_;
#endif
	

    };

}}} // Opm::RestartIO::Helpers

#endif //OPM_AGGREGATE_WELL_DATA_HPP
