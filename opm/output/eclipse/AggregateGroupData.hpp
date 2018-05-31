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

#ifndef OPM_AGGREGATE_GROUP_DATA_HPP
#define OPM_AGGREGATE_GROUP_DATA_HPP

#include <opm/output/eclipse/CharArrayNullTerm.hpp>
#include <opm/output/eclipse/WindowedArray.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace Opm {
    class Schedule;
    class SummaryState;
} // Opm

namespace Opm { namespace RestartIO { namespace Helpers {

    class AggregateGroupData
    {
    public:
	explicit AggregateGroupData(const std::vector<int>& inteHead);

	void captureDeclaredGroupData(const Opm::Schedule&                 sched,
				      const std::vector<std::string>&      restart_group_keys,
				      const std::vector<std::string>&      restart_field_keys,
				      const std::map<std::string, size_t>& groupKeyToIndex,
				      const std::map<std::string, size_t>& fieldKeyToIndex,
				      const std::size_t                    simStep,
				      const Opm::SummaryState&             sumState,
				      const std::vector<int>&              inteHead);

#if 0
        void captureDynamicGroupData(const Opm::Phases&          phases,
                                     const Opm::Schedule&        sched,
                                     const std::size_t           rptStep,
                                     const Opm::data::WellRates& wRates);
#endif

        /// Retrieve Integer Group Data Array.
        const std::vector<int>& getIGroup() const
        {
            return this->iGroup_.data();
        }

        /// Retrieve Floating-Point (Real) Group Data Array.
        const std::vector<float>& getSGroup() const
        {
            return this->sGroup_.data();
        }

        /// Retrieve Floating-Point (Double Precision) Group Data Array.
        const std::vector<double>& getXGroup() const
        {
            return this->xGroup_.data();
        }

        /// Retrieve Character Group Data Array.
        const std::vector<CharArrayNullTerm<8>>& getZGroup() const
        {
            return this->zGroup_.data();
        }

        const std::vector<std::string> restart_group_keys = {"GOPP", "GWPP", "GOPR", "GWPR", "GGPR",
							     "GVPR", "GWIR", "GGIR", "GWCT", "GGOR", 
							     "GOPT", "GWPT", "GGPT", "GVPT", "GWIT",
							     "GGIT"};

	const std::vector<std::string> restart_field_keys = {"FOPP", "FWPP", "FOPR", "FWPR", "FGPR", 
							     "FVPR", "FWIR", "FGIR", "FWCT", "FGOR", 
							     "FOPT", "FWPT", "FGPT", "FVPT", "FWIT",
							     "FGIT"};

	const std::map<std::string, size_t> groupKeyToIndex = {
	  {"GOPR",  0},
	  {"GWPR",  1},
	  {"GGPR",  2},
	  {"GVPR",  3},
	  {"GWIR",  5},
	  {"GGIR",  6},
	  {"GWCT",  8},
	  {"GGOR",  8},
	  {"GOPT", 10},
	  {"GWPT", 11},
	  {"GGPT", 12},
	  {"GVPT", 13},
	  {"GWIT", 15},
	  {"GGIT", 16},
	  {"GOPP", 22},
	  {"GWPP", 23},
	};

	const std::map<std::string, size_t> fieldKeyToIndex = {
	  {"FOPR",  0},
	  {"FWPR",  1},
	  {"FGPR",  2},
	  {"FVPR",  3},
	  {"FWIR",  5},
	  {"FGIR",  6},
	  {"FWCT",  8},
	  {"FGOR",  8},
	  {"FOPT", 10},
	  {"FWPT", 11},
	  {"FGPT", 12},
	  {"FVPT", 13},
	  {"FWIT", 15},
	  {"FGIT", 16},
	  {"FOPP", 22},
	  {"FWPP", 23},
	};

    private:
        /// Aggregate 'IWEL' array (Integer) for all wells.
        WindowedArray<int> iGroup_;

        /// Aggregate 'SWEL' array (Real) for all wells.
        WindowedArray<float> sGroup_;

        /// Aggregate 'XWEL' array (Double Precision) for all wells.
        WindowedArray<double> xGroup_;

        /// Aggregate 'ZWEL' array (Character) for all wells.
        WindowedArray<CharArrayNullTerm<8>> zGroup_;

        /// Maximum number of wells in a group.
        int nWGMax_;

        /// Maximum number of groups
        int nGMaxz_;
    };

}}} // Opm::RestartIO::Helpers

#endif // OPM_AGGREGATE_WELL_DATA_HPP
