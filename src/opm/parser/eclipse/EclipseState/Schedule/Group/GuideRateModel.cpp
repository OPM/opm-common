/*
  Copyright 2019 Equinor ASA.

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
#include <stdexcept>
#include <cmath>

#include <opm/parser/eclipse/EclipseState/Schedule/Group/GuideRateModel.hpp>

namespace Opm {

GuideRateModel::GuideRateModel(double time_interval_arg,
                               Target target_arg,
                               double A_arg,
                               double B_arg,
                               double C_arg,
                               double D_arg,
                               double E_arg,
                               double F_arg,
                               bool allow_increase_arg,
                               double damping_factor_arg,
                               bool use_free_gas_arg) :
    time_interval(time_interval_arg),
    target(target_arg),
    A(A_arg),
    B(B_arg),
    C(C_arg),
    D(D_arg),
    E(E_arg),
    F(F_arg),
    allow_increase(allow_increase_arg),
    damping_factor(damping_factor_arg),
    use_free_gas(use_free_gas_arg),
    default_model(false)
{
    if (this->A > 3 || this->A < -3)
        throw std::invalid_argument("Invalid value for A must be in interval [-3,3]");

    if (this->B < 0)
        throw std::invalid_argument("Invalid value for B must be > 0");

    if (this->D > 3 || this->D < -3)
        throw std::invalid_argument("Invalid value for D must be in interval [-3,3]");

    if (this->F > 3 || this->F < -3)
        throw std::invalid_argument("Invalid value for F must be in interval [-3,3]");
}


double GuideRateModel::update_delay() const {
    return this->time_interval;
}


double GuideRateModel::eval(double pot, double R1, double R2) const {
    if (this->default_model)
        throw std::invalid_argument("The default GuideRateModel can not be evaluated - must enter GUIDERAT information explicitly.");

    double denom = this->B + this->C*std::pow(R1, this->D) + this->E*std::pow(R2, this->F);
    /*
      The values pot, R1 and R2 are runtime simulation results, so here
      basically anything could happen. Quite dangerous to have hard error
      handling here?
    */
    if (denom <= 0)
        throw std::range_error("Invalid denominator: " + std::to_string(denom));

    return std::pow(pot, this->A) / denom;
}

bool GuideRateModel::operator==(const GuideRateModel& other) const {
    return (this->time_interval == other.time_interval) &&
        (this->target == other.target) &&
        (this->A == other.A) &&
        (this->B == other.B) &&
        (this->C == other.C) &&
        (this->D == other.D) &&
        (this->E == other.E) &&
        (this->F == other.F) &&
        (this->allow_increase == other.allow_increase) &&
        (this->damping_factor == other.damping_factor) &&
        (this->use_free_gas == other.use_free_gas);
}

bool GuideRateModel::operator!=(const GuideRateModel& other) const {
    return !(*this == other);
}


GuideRateModel::Target GuideRateModel::TargetFromString(const std::string& s) {
    if (s == "OIL")
        return Target::OIL;

    if (s == "LIQ")
        return Target::LIQ;

    if (s == "GAS")
        return Target::GAS;

    if (s == "RES")
        return Target::RES;

    if (s == "COMB")
        return Target::COMB;

    if (s == "NONE")
        return Target::NONE;

    throw std::invalid_argument("Could not convert: " + s + " to a valid Target enum value");
}

}
