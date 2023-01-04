//===========================================================================
//
// File: ParameterRequirement.hpp
//
// Created: Tue Jun  2 19:05:02 2009
//
// Author(s): Bård Skaflestad     <bard.skaflestad@sintef.no>
//            Atgeirr F Rasmussen <atgeirr@sintef.no>
//
// $Date$
//
// $Revision$
//
//===========================================================================

/*
  Copyright 2009, 2010 SINTEF ICT, Applied Mathematics.
  Copyright 2009, 2010 Statoil ASA.

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

#ifndef OPM_PARAMETERREQUIREMENT_HEADER
#define OPM_PARAMETERREQUIREMENT_HEADER

#include <string>
#include <vector>

namespace Opm {
	/// @brief
	/// @todo Doc me!
	/// @tparam
	/// @param
	/// @return
	struct ParameterRequirementNone {
	    template<typename T>
	    std::string operator()(const T&) const {
		return "";
	    }
	};

	/// @brief
	/// @todo Doc me!
	/// @param
	/// @return
        struct ParameterRequirementProbability {
            std::string operator()(double x) const;
	};

	/// @brief
	/// @todo Doc me!
	/// @tparam
	/// @param
	/// @return
	struct ParameterRequirementPositive {
	    template<typename T>
	    std::string operator()(const T& x) const {
		if (x > 0) {
		    return "";
		} else {
                    return "The value '" + std::to_string(x) +
                           "' is not positive.";
		}
	    }
	};

	/// @brief
	/// @todo Doc me!
	/// @tparam
	/// @param
	/// @return
	struct ParameterRequirementNegative {
	    template<typename T>
	    std::string operator()(const T& x) const {
		if (x < 0) {
		    return "";
		} else {
                    return "The value '" + std::to_string(x) +
                           "' is not negative.";
		}
	    }
	};

	/// @brief
	/// @todo Doc me!
	/// @tparam
	/// @param
	/// @return
	struct ParameterRequirementNonPositive {
	    template<typename T>
	    std::string operator()(const T& x) const {
		if (x > 0) {
                    return "The value '" + std::to_string(x) +
                           "' is positive.";
		} else {
		    return "";
		}
	    }
	};

	/// @brief
	/// @todo Doc me!
	/// @tparam
	/// @param
	/// @return
	struct ParameterRequirementNonNegative {
	    template<typename T>
	    std::string operator()(const T& x) const {
		if (x < 0) {
                    return "The value '" + std::to_string(x) +
                           "' is negative.";
		} else {
		    return "";
		}
	    }
	};

	/// @brief
	/// @todo Doc me!
	/// @tparam
	/// @param
	/// @return
	struct ParameterRequirementNonZero {
	    template<typename T>
	    std::string operator()(const T& x) const {
		if (x != 0) {
		    return "";
		} else {
		    return "The value was zero.";
		}
	    }
	};

	/// @brief
	/// @todo Doc me!
	/// @param
	/// @return
	struct ParameterRequirementNonEmpty {
	    std::string operator()(const std::string& x) const {
		if (x != "") {
		    return "The string was empty.";
		} else {
		    return "";
		}
	    }
	};

	/// @brief
	/// @todo Doc me!
	/// @tparam
	/// @param
	/// @return
	template<class Requirement1, class Requirement2>
	struct ParameterRequirementAnd {
	    ParameterRequirementAnd(const Requirement1& r1, const Requirement2& r2) :
	    r1_(r1), r2_(r2) { }

	    template<typename T>
	    std::string operator()(const T& t) const {
		std::string e1 = r1_(t);
		std::string e2 = r2_(t);
		if (e1 == "") {
		    return e2;
		} else if (e2 == "") {
		    return e1;
		} else {
		    return e1 + " AND " + e2;
		}
	    }
	private:
	    const Requirement1 r1_;
	    const Requirement2 r2_;
	};

	/// @brief
	/// @todo Doc me!
	/// @param
        struct ParameterRequirementMemberOf {
            explicit ParameterRequirementMemberOf(const std::vector<std::string>& elements);

            /// @brief
            /// @todo Doc me!
            /// @param
            /// @return
            std::string operator()(const std::string& x) const;

	private:
	    const std::vector<std::string> elements_;
	};
} // namespace Opm

#endif // OPM_PARAMETERREQUIREMENT_HEADER
