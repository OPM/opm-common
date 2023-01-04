//===========================================================================
//
// File: Parameter.cpp
//
// Created: Tue Jun  2 19:18:25 2009
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

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <opm/common/utility/parameters/Parameter.hpp>

#include <sstream>
#include <string>

namespace Opm {

std::string
correct_parameter_tag(const ParameterMapItem& item)
{
    std::string tag = item.getTag();
    if (tag != ID_xmltag__param) {
        std::string error = "The XML tag was '" +
                            tag + "' but should be '" +
                            ID_xmltag__param + "'.\n";
        return error;
    } else {
        return "";
    }
}

std::string
correct_type(const Parameter& parameter,
             const std::string& param_type)
{
    std::string type = parameter.getType();
    if (type != param_type && type != ID_param_type__cmdline) {
        std::string error = "The data was of type '" + type +
                            "' but should be of type '" +
                            param_type + "'.\n";
        return error;
    } else {
        return "";
    }
}

int ParameterMapItemTrait<int>::
convert(const ParameterMapItem& item,
        std::string& conversion_error,
        const bool)
{
    conversion_error = correct_parameter_tag(item);
    if (!conversion_error.empty()) {
        return 0;
    }
    const Parameter& parameter = dynamic_cast<const Parameter&>(item);
    conversion_error = correct_type(parameter, ID_param_type__int);
    if (!conversion_error.empty()) {
        return 0;
    }
    std::stringstream stream;
    stream << parameter.getValue();
    int value;
    stream >> value;
    if (stream.fail()) {
        conversion_error = "Conversion to '" +
                           ID_param_type__int +
                           "' failed. Data was '" +
                           parameter.getValue() + "'.\n";
        return 0;
    }
    return value;
}

double ParameterMapItemTrait<double>::
convert(const ParameterMapItem& item,
        std::string& conversion_error,
        const bool)
{
    conversion_error = correct_parameter_tag(item);
    if (!conversion_error.empty()) {
        return 0.0;
    }
    const Parameter& parameter = dynamic_cast<const Parameter&>(item);
    conversion_error = correct_type(parameter, ID_param_type__float);
    if (!conversion_error.empty()) {
        return 0.0;
    }
    std::stringstream stream;
    stream << parameter.getValue();
    double value;
    stream >> value;
    if (stream.fail()) {
        conversion_error = "Conversion to '" +
                           ID_param_type__float +
                           "' failed. Data was '" +
                           parameter.getValue() + "'.\n";
        return 0.0;
    }
    return value;
}

bool ParameterMapItemTrait<bool>::
convert(const ParameterMapItem& item,
        std::string& conversion_error,
        const bool)
{
    conversion_error = correct_parameter_tag(item);
    if (!conversion_error.empty()) {
        return false;
    }
    const Parameter& parameter = dynamic_cast<const Parameter&>(item);
    conversion_error = correct_type(parameter, ID_param_type__bool);
    if (!conversion_error.empty()) {
        return false;
    }
    if (parameter.getValue() == ID_true) {
        return true;
    } else if (parameter.getValue() == ID_false) {
        return false;
    } else {
        conversion_error = "Conversion failed. Data was '" +
                           parameter.getValue() +
                           "', but should be one of '" +
                           ID_true + "' or '" + ID_false + "'.\n";
        return false;
    }
}

std::string ParameterMapItemTrait<std::string>::
convert(const ParameterMapItem& item,
        std::string& conversion_error,
        const bool)
{
    conversion_error = correct_parameter_tag(item);
    if (!conversion_error.empty()) {
        return "";
    }
    const Parameter& parameter = dynamic_cast<const Parameter&>(item);
    conversion_error = correct_type(parameter, ID_param_type__string);
    if (!conversion_error.empty()) {
        return "";
    }
    return parameter.getValue();
}

} // namespace Opm
