//===========================================================================
//
// File: Parameter.hpp
//
// Created: Tue Jun  2 16:00:21 2009
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

#ifndef OPM_PARAMETER_HEADER
#define OPM_PARAMETER_HEADER

#include <string>

#include <opm/common/utility/parameters/ParameterMapItem.hpp>
#include <opm/common/utility/parameters/ParameterStrings.hpp>

namespace Opm {
	/// @brief
	/// @todo Doc me!
	class Parameter : public ParameterMapItem {
	public:
	    /// @brief
	    /// @todo Doc me!
	    virtual ~Parameter() {}
	    /// @brief
	    /// @todo Doc me!
	    /// @return
	    virtual std::string getTag() const {return ID_xmltag__param;}
	    /// @brief
	    /// @todo Doc me!
	    /// @param
	    Parameter(const std::string& value, const std::string& type)
                : value_(value), type_(type) {}
	    /// @brief
	    /// @todo Doc me!
	    /// @return
	    std::string getValue() const {return value_;}
	    /// @brief
	    /// @todo Doc me!
	    /// @return
	    std::string getType() const {return type_;}
	private:
	    std::string value_;
	    std::string type_;
	};

	/// @brief
	/// @todo Doc me!
	/// @param
	/// @return
	std::string correct_parameter_tag(const ParameterMapItem& item);
	std::string correct_type(const Parameter& parameter,
                                 const std::string& type);

	/// @brief
	/// @todo Doc me!
	/// @tparam
	/// @param
	/// @return
	template<>
	struct ParameterMapItemTrait<int> {
	    static int convert(const ParameterMapItem& item,
                               std::string& conversion_error,
                               const bool);

	    static std::string type() {return ID_param_type__int;}
	};

	/// @brief
	/// @todo Doc me!
	/// @tparam
	/// @param
	/// @return
	template<>
	struct ParameterMapItemTrait<double> {
	    static double convert(const ParameterMapItem& item,
                                  std::string& conversion_error,
                                  const bool);

	    static std::string type() {return ID_param_type__float;}
	};

	/// @brief
	/// @todo Doc me!
	/// @tparam
	/// @param
	/// @return
	template<>
	struct ParameterMapItemTrait<bool> {
	    static bool convert(const ParameterMapItem& item,
                                std::string& conversion_error,
                                const bool);

	    static std::string type() {return ID_param_type__bool;}
	};

	/// @brief
	/// @todo Doc me!
	/// @tparam
	/// @param
	/// @return
	template<>
	struct ParameterMapItemTrait<std::string> {
	    static std::string convert(const ParameterMapItem& item,
                                       std::string& conversion_error,
                                       const bool);

	    static std::string type() {return ID_param_type__string;}
	};
} // namespace Opm
#endif  // OPM_PARAMETER_HPP
