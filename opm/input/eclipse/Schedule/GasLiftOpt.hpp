/*
  Copyright 2020 Equinor ASA.

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
#ifndef GAS_LIFT_OPT_HPP
#define GAS_LIFT_OPT_HPP

#include <optional>
#include <string>
#include <map>
#include <opm/io/eclipse/rst/well.hpp>
#include <opm/io/eclipse/rst/group.hpp>

namespace Opm {

class GasLiftGroup {
public:
    GasLiftGroup() = default;

    GasLiftGroup(const std::string& name) :
        m_name(name)
    {}

    static bool active(const RestartIO::RstGroup& rst_group) {
        if ((rst_group.glift_max_rate + rst_group.glift_max_supply) != 0)
            return false;

        return true;
    }

    explicit GasLiftGroup(const RestartIO::RstGroup& rst_group)
        : m_name(rst_group.name)
        , m_max_lift_gas(rst_group.glift_max_supply)
        , m_max_total_gas(rst_group.glift_max_rate)
    {}

    const std::optional<double>& max_lift_gas() const {
        return this->m_max_lift_gas;
    }

    void max_lift_gas(double value) {
        if (value >= 0)
            this->m_max_lift_gas = value;
    }

    const std::optional<double>& max_total_gas() const {
        return this->m_max_total_gas;
    }

    void max_total_gas(double value) {
        if (value >= 0)
            this->m_max_total_gas = value;
    }

    const std::string& name() const {
        return this->m_name;
    }

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_name);
        serializer(m_max_lift_gas);
        serializer(m_max_total_gas);
    }


    static GasLiftGroup serializationTestObject();

    bool operator==(const GasLiftGroup& other) const;

private:
    std::string m_name;
    std::optional<double> m_max_lift_gas;
    std::optional<double> m_max_total_gas;
};

class GasLiftWell {
public:
    GasLiftWell() = default;

    explicit GasLiftWell(const RestartIO::RstWell& rst_well)
        : m_name(rst_well.name)
        , m_max_rate(rst_well.glift_max_rate)
        , m_min_rate(rst_well.glift_min_rate)
        , m_use_glo(rst_well.glift_active)
        , m_weight(rst_well.glift_weight_factor)
        , m_inc_weight(rst_well.glift_inc_weight_factor)
        , m_alloc_extra_gas(rst_well.glift_alloc_extra_gas)
    {}


    GasLiftWell(const std::string& name, bool use_glo) :
        m_name(name),
        m_use_glo(use_glo)
    {}

    // Unfortunately it seems just using the rst_well.glift_active flag is
    // not sufficient to determine whether the well should be included in
    // gas lift optimization or not. The current implementation based on
    // numerical values found in the restart file is pure guesswork.
    static bool active(const RestartIO::RstWell& rst_well) {
        if ((rst_well.glift_max_rate + rst_well.glift_min_rate + rst_well.glift_weight_factor == 0))
            return false;

        return true;
    }

    const std::string& name() const {
        return this->m_name;
    }

    bool use_glo() const {
        return this->m_use_glo;
    }

    void max_rate(double value) {
        this->m_max_rate = value;
    }


    /*
      The semantics of the max_rate is quite complicated:

        1. If the std::optional<double> has a value that value should be
           used as the maximum rate and all is fine.

        2. If the std::optional<double> does not a have well we must check
           the value of Well::use_glo():

           False: The maximum gas lift should have been set with WCONPROD /
              WELTARG - this code does not provide a value in that case.

           True: If the well should be controlled with gas lift optimization
              the value to use should be the largest ALQ value in the wells
              VFP table.
    */
    const std::optional<double>& max_rate() const {
        return this->m_max_rate;
    }

    void weight_factor(double value) {
        if (this->m_use_glo)
            this->m_weight = value;
    }

    double weight_factor() const {
        return this->m_weight;
    }

    void inc_weight_factor(double value) {
        if (this->m_use_glo)
            this->m_inc_weight = value;
    }

    double inc_weight_factor() const {
        return this->m_inc_weight;
    }

    void min_rate(double value) {
        if (this->m_use_glo)
            this->m_min_rate = value;
    }

    double min_rate() const {
        return this->m_min_rate;
    }

    void alloc_extra_gas(bool value) {
        if (this->m_use_glo)
            this->m_alloc_extra_gas = value;
    }

    bool alloc_extra_gas() const {
        return this->m_alloc_extra_gas;
    }

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_name);
        serializer(m_use_glo);
        serializer(m_max_rate);
        serializer(m_min_rate);
        serializer(m_weight);
        serializer(m_inc_weight);
        serializer(m_alloc_extra_gas);
    }

    static GasLiftWell serializationTestObject();

    bool operator==(const GasLiftWell& other) const;

private:
    std::string m_name;
    std::optional<double> m_max_rate;
    double m_min_rate = 0;
    bool m_use_glo = false;
    double m_weight = 1;
    double m_inc_weight = 0;
    bool m_alloc_extra_gas = false;
};

class GasLiftOpt {
public:
    const GasLiftGroup& group(const std::string& gname) const;
    const GasLiftWell& well(const std::string& wname) const;

    double gaslift_increment() const;
    void gaslift_increment(double gaslift_increment);
    double min_eco_gradient() const;
    void min_eco_gradient(double min_eco_gradient);
    double min_wait() const;
    void min_wait(double min_wait);
    void all_newton(double all_newton);
    bool all_newton() const;
    void add_group(const GasLiftGroup& group);
    void add_well(const GasLiftWell& well);
    bool active() const;
    bool has_well(const std::string& well) const;
    bool has_group(const std::string& group) const;
    std::size_t num_wells() const;

    static GasLiftOpt serializationTestObject();
    bool operator==(const GasLiftOpt& other) const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_increment);
        serializer(m_min_eco_gradient);
        serializer(m_min_wait);
        serializer(m_all_newton);
        serializer(m_groups);
        serializer(m_wells);
    }

private:
    double m_increment = 0;
    double m_min_eco_gradient;
    double m_min_wait;
    bool   m_all_newton = true;

    std::map<std::string, GasLiftGroup> m_groups;
    std::map<std::string, GasLiftWell> m_wells;
};

}

#endif
