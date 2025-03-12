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

#include <opm/io/eclipse/rst/group.hpp>
#include <opm/io/eclipse/rst/well.hpp>

#include <cstddef>
#include <map>
#include <optional>
#include <string>

namespace Opm {

/// Gas lift optimisation parameters at the group level.
class GasLiftGroup
{
public:
    /// Default constructor.
    ///
    /// Resulting object mostly usable as a target for a deserialisation
    /// operation.
    GasLiftGroup() = default;

    /// Construct gas lift optimisation parameter collection for a single
    /// group.
    ///
    /// \param[in] name Group name.
    explicit GasLiftGroup(const std::string& name)
        : m_name(name)
    {}

    /// Construct gas lift optimisation parameter collection for a single
    /// group from restart file representation.
    ///
    /// \param[in] rst_group Restart file representation of group object.
    explicit GasLiftGroup(const RestartIO::RstGroup& rst_group);

    /// Predicate for whether or not gas lift optimisation applies to a
    /// group at simulation restart time.
    ///
    /// \param[in] rst_group Restart file representation of group object.
    ///
    /// \return Whether or not gas lift optimisation applies to this
    /// particular group.
    static bool active(const RestartIO::RstGroup& rst_group);

    /// Maximum lift gas limit for this group.
    ///
    /// Nullopt for no limit.
    const std::optional<double>& max_lift_gas() const
    {
        return this->m_max_lift_gas;
    }

    /// Assign maximum lift gas limit for this group.
    ///
    /// \param[in] value Maximum lift gas limit.  Used only if non-negative.
    void max_lift_gas(const double value)
    {
        if (! (value < 0.0)) {
            this->m_max_lift_gas = value;
        }
    }

    /// Maximum total gas limit for this group.
    ///
    /// Sum of lift gas and produced gas.
    ///
    /// Nullopt if not limit.
    const std::optional<double>& max_total_gas() const
    {
        return this->m_max_total_gas;
    }

    /// Assign maximum total gas limit for this group.
    ///
    /// \param[in] value Maximum total gas limit.  Used only if
    /// non-negative.
    void max_total_gas(const double value)
    {
        if (! (value < 0.0)) {
            this->m_max_total_gas = value;
        }
    }

    /// Group name.
    ///
    /// Mostly for convenience.
    const std::string& name() const
    {
        return this->m_name;
    }

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_name);
        serializer(m_max_lift_gas);
        serializer(m_max_total_gas);
    }

    /// Create a serialisation test object.
    static GasLiftGroup serializationTestObject();

    /// Equality predicate.
    ///
    /// \param[in] other Object against which \code *this \endcode will be
    /// tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p
    /// other.
    bool operator==(const GasLiftGroup& other) const;

private:
    /// Group name.
    std::string m_name{};

    /// Maximum lift gas limit for this group.  Nullopt for no limit.
    std::optional<double> m_max_lift_gas{};

    /// Maximum total gas (lift + produced) limit for this group.  Nullopt
    /// for no limit.
    std::optional<double> m_max_total_gas{};
};

// ---------------------------------------------------------------------------

/// Gas lift and gas lift optimisation parameters at the well level.
class GasLiftWell
{
public:
    /// Default constructor.
    ///
    /// Resulting object mostly usable as a target for a deserialisation
    /// operation.
    GasLiftWell() = default;

    /// Construct gas lift optimisation parameter collection for a single
    /// well.
    ///
    /// \param[in] name Group name.
    ///
    /// \param[in] use_glo Whether or not to apply gas lift optimisation to
    /// this well.
    GasLiftWell(const std::string& name, const bool use_glo)
        : m_name    { name }
        , m_use_glo { use_glo }
    {}

    /// Construct gas lift optimisation parameter collection for a single
    /// well from restart file representation.
    ///
    /// \param[in] rst_well Restart file representation of well object.
    explicit GasLiftWell(const RestartIO::RstWell& rst_well);

    /// Predicate for whether or not gas lift optimisation applies to a
    /// group at simulation restart time.
    ///
    /// \param[in] rst_well Restart file representation of well object.
    ///
    /// \return Whether or not gas lift optimisation applies to this
    /// particular well.
    static bool active(const RestartIO::RstWell& rst_well);

    /// Well name.
    ///
    /// Mostly for convenience.
    const std::string& name() const
    {
        return this->m_name;
    }

    /// Whether or not this well is subject to gas lift optimisation.
    bool use_glo() const
    {
        return this->m_use_glo;
    }

    /// Assign maximum gas lift rate for this well.
    ///
    /// \param[in] value Maximum gas lift rate for this well.
    void max_rate(const double value)
    {
        this->m_max_rate = value;
    }

    /// Retrieve maximum gas lift rate for this well.
    ///
    /// The semantics of the max_rate are as follows
    ///
    ///  1. If the optional has a value, then that value is the maximum gas
    ///     lift rate.
    ///
    ///  2. Otherwise, the maximum gas lift rate depends on use_glo().  If
    ///     gas lift optimisation does not apply to this well--i.e., when
    ///     use_glo() is false--then the maximum gas lift rate is the well's
    ///     artificial lift quantity (ALQ).  Conversely, when the well is
    ///     subject to gas lift optimisation, the maximum gas lift rate
    ///     should be the largest ALQ value in the well's VFP table.
    const std::optional<double>& max_rate() const
    {
        return this->m_max_rate;
    }

    /// Assign weighting factor for preferential allocation of lift gas.
    ///
    /// \param[in] value Weighting factor.
    void weight_factor(const double value)
    {
        if (this->m_use_glo) {
            this->m_weight = value;
        }
    }

    /// Retrieve weighting factor for preferential allocation of lift gas.
    double weight_factor() const
    {
        return this->m_weight;
    }

    /// Assign incremental gas rate weighting factor for this well.
    ///
    /// \param[in] value Incremental gas rate weighting factor.
    void inc_weight_factor(const double value)
    {
        if (this->m_use_glo) {
            this->m_inc_weight = value;
        }
    }

    /// Retrieve incremental gas rate weighting factor for this well.
    double inc_weight_factor() const
    {
        return this->m_inc_weight;
    }

    /// Assign minimum rate of lift gas injection for this well.
    ///
    /// \param[in] value Minimum rate of lift gas injection.  Used only if
    /// the well is subject to gas lift optimisation--i.e., when use_glo()
    /// is true.
    void min_rate(const double value)
    {
        if (this->m_use_glo) {
            this->m_min_rate = value;
        }
    }

    /// Retrieve this well's minimum lift gas injection rate.
    double min_rate() const
    {
        return this->m_min_rate;
    }

    /// Assign flag for whether or not to allocate extra lift gas if
    /// available, even if group target is or would be exceeded.
    ///
    /// \param[in] value Flag for whether or not to allocate extra lift gas.
    void alloc_extra_gas(const bool value)
    {
        if (this->m_use_glo) {
            this->m_alloc_extra_gas = value;
        }
    }

    /// Whether or not to allocate extra lift gas if available, even if
    /// group target is or would be exceeded.
    bool alloc_extra_gas() const
    {
        return this->m_alloc_extra_gas;
    }

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
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

    /// Create a serialisation test object.
    static GasLiftWell serializationTestObject();

    /// Equality predicate.
    ///
    /// \param[in] other Object against which \code *this \endcode will be
    /// tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p
    /// other.
    bool operator==(const GasLiftWell& other) const;

private:
    /// Well name.
    std::string m_name{};

    /// Maximum lift gas injection limit.
    ///
    /// Nullopt value interpretation subject to value of m_use_glo.
    std::optional<double> m_max_rate{};

    /// Minimum lift gas injection rate.
    double m_min_rate { 0.0 };

    /// Whether or not gas lift optimisation applies to this well.
    bool m_use_glo { false };

    /// Weighting factor for preferential allocation of lift gas.
    double m_weight { 1.0 };

    /// Weighting factor for incremental gas rate.
    double m_inc_weight { 0.0 };

    /// Whether or not to allocate extra lift gas to this well even if group
    /// target is or would be exceeded.
    bool m_alloc_extra_gas { false };
};

// ---------------------------------------------------------------------------

/// Gas lift optimisation parameters for all wells and groups.
class GasLiftOpt
{
public:
    /// Retrieve gas lift optimisation parameters for a single named group.
    ///
    /// Throws an exception of type std::out_of_range if no parameters have
    /// been defined for the named group.
    ///
    /// \param[in] gname Group name.
    ///
    /// \return Gas lift optimisation parameters for named group \p gname.
    const GasLiftGroup& group(const std::string& gname) const;

    /// Retrieve gas lift and gas lift optimisation parameters for a single
    /// named well.
    ///
    /// Throws an exception of type std::out_of_range if no parameters have
    /// been defined for the named well.
    ///
    /// \param[in] qname Well name.
    ///
    /// \return Gas lift and gas lift optimisation parameters for named
    /// well \p wname.
    const GasLiftWell& well(const std::string& wname) const;

    /// Lift gas rate increment.
    double gaslift_increment() const;

    /// Assign lift gas rate increment.
    ///
    /// \param[in] gaslift_increment Lift gas rate increment value.
    void gaslift_increment(double gaslift_increment);

    /// Retrieve minimum economical gradient threshold to continue
    /// increasing lift gas injection rate.
    double min_eco_gradient() const;

    /// Assign minimum economical gradient threshold to continue increasing
    /// lift gas injection rate.
    void min_eco_gradient(double min_eco_gradient);

    /// Retrieve minimum wait time between gas lift optimisation runs.
    double min_wait() const;

    /// Assign minimum wait time between gas lift optimisation runs.
    ///
    /// \param[in] min_wait Minimum wait time (seconds) between gas lift
    /// optimisation runs.
    void min_wait(double min_wait);

    /// Whether or not to include gas lift optimisation in all of the first
    /// "NUPCOL" non-linear iterations.
    ///
    /// If not, gas lift optimisation should be included only in the first
    /// non-linear iteration of a time step.
    bool all_newton() const;

    /// Assign flag for whether or not to include gas lift optimisation in
    /// all of the first "NUPCOL" non-linear iterations.
    void all_newton(bool all_newton);

    /// Incorporate gas lift optimisation parameters for a single group into
    /// collection.
    ///
    /// \param[in] group Gas lift optimisation parameters for a single named
    /// group.
    void add_group(const GasLiftGroup& group);

    /// Incorporate gas lift and gas lift optimisation parameters for a
    /// single well into collection.
    ///
    /// \param[in] well Gas lift and gas lift optimisation parameters for a
    /// single named well.
    void add_well(const GasLiftWell& well);

    /// Whether or not gas lift optimisation is currently enabled in the run.
    bool active() const;

    /// Whether or not gas lift parameters exists for single named well.
    ///
    /// \param[in] well Well name.
    ///
    /// \return Whether or not gas lift parameters exist for named well \p
    /// well.
    bool has_well(const std::string& well) const;

    /// Whether or not gas lift optimisation parameters exists for single
    /// named group.
    ///
    /// \param[in] group Group name.
    ///
    /// \return Whether or not gas lift optimisation parameters exist for
    /// named group \p group.
    bool has_group(const std::string& group) const;

    /// Number of wells currently known to gas lift optimisation facility.
    std::size_t num_wells() const
    {
        return this->m_wells.size();
    }

    /// Create a serialisation test object.
    static GasLiftOpt serializationTestObject();

    /// Equality predicate.
    ///
    /// \param[in] other Object against which \code *this \endcode will be
    /// tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p
    /// other.
    bool operator==(const GasLiftOpt& other) const;

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
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
    /// Lift gas injection rate increment.
    double m_increment { 0.0 };

    /// Minimum economic gradient threshold to continue increasing lift gas
    /// injection rate.
    double m_min_eco_gradient { 0.0 };

    /// Minimum wait time (seconds) between gas lift optimisation runs.
    double m_min_wait { 0.0 };

    /// Whether or not to include gas lift optimisation in every "NUPCOL"
    /// non-linear iteration.
    ///
    /// If not, gas lift optimisation is performed only in the first
    /// non-linear iteration.
    bool m_all_newton { true };

    /// Gas lift optimisation parameters for all applicable groups in
    /// simulation run.
    std::map<std::string, GasLiftGroup> m_groups{};

    /// Gas lift and gas lift optimisation parameters for all applicable
    /// wells in simulation run.
    std::map<std::string, GasLiftWell> m_wells{};
};

} // namespace Opm

#endif // GAS_LIFT_OPT_HPP
