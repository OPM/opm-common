/*
  Copyright 2013 Statoil ASA.

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
#ifndef SCHEDULE_STATIC_HPP
#define SCHEDULE_STATIC_HPP

#include <opm/input/eclipse/EclipseState/Runspec.hpp>

#include <opm/input/eclipse/Schedule/MessageLimits.hpp>
#include <opm/input/eclipse/Schedule/OilVaporizationProperties.hpp>
#include <opm/input/eclipse/Schedule/RSTConfig.hpp>
#include <opm/input/eclipse/Schedule/ScheduleRestartInfo.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace Opm {

    class Python;

} // namespace Opm

namespace Opm {

/// Initial state of Schedule object created from information in SOLUTION
/// section
struct ScheduleStatic
{
    // Note to maintainers: When changing this list of data members, please
    // update operator==(), serializationTestObject(), and serializeOp()
    // accordingly.

    /// Run's Python interpreter
    std::shared_ptr<const Python> m_python_handle{};

    /// On-disk location of run's model description (".DATA" file).
    std::string m_input_path{};

    /// How to handle SCHEDULE section in a restarted simulation run.
    ScheduleRestartInfo rst_info{};

    /// Limits on number of messages of each kind (MESSAGES keyword).
    MessageLimits m_deck_message_limits{};

    /// Run's input/output unit system conventions.
    UnitSystem m_unit_system{};

    /// Run's descriptive meta information (RUNSPEC section).
    Runspec m_runspec{};

    /// Initial restart file output requests.
    ///
    /// Keyword RPTRST in SOLUTION section.
    RSTConfig rst_config{};

    /// Not really used and therefore intentionally undocumented.
    std::optional<int> output_interval{};

    /// Sparse summary output interval (SUMTHIN keyword in SUMMARY section).
    ///
    /// Negative default value means to output summary information at every
    /// time step.
    double sumthin{-1.0};

    /// Whether or not to output summary information at report steps only
    /// (RPTONLY keyword in SUMMARY section).
    ///
    /// Default value is to output summary information at every time step.
    bool rptonly{false};

    /// Whether or not run activates the gas-lift optimisation facility.
    bool gaslift_opt_active{false};

    /// Limits on gas re-solution and oil vaporisation rates (e.g., DRSTD in
    /// SOLUTION section).
    std::optional<OilVaporizationProperties> oilVap{};

    /// Whether or not this run is externally controlled by another
    /// simulation run (reservoir coupling facility).
    bool slave_mode{false};

    /// Default constructor.
    ///
    /// Creates an object that's mostly usable as a target in a
    /// deserialisation operation.
    ScheduleStatic() = default;

    /// Constructor.
    ///
    /// Creates an object with everything other than the run's Python
    /// interpreter in its default state.  The object is mostly usable as a
    /// target in a deserialisation operation.
    ///
    /// \param[in] python_handle Run's Python interpreter.
    explicit ScheduleStatic(std::shared_ptr<const Python> python_handle)
        : m_python_handle { std::move(python_handle) }
    {}

    /// Constructor.
    ///
    /// \param[in] python_handle Run's Python interpreter.
    ///
    /// \param[in] restart_info How to handle SCHEDULE section in a
    /// restarted simulation run.
    ///
    /// \param[in] deck Run's model description.  Mostly used for its
    /// SOLUTION and SUMMARY section contents.
    ///
    /// \param[in] runspec Run's descriptive meta information
    ///
    /// \param[in] output_interval_ Not really used and therefore left
    /// undocumented.
    ///
    /// \param[in] parseContext How to handle parse failures.
    ///
    /// \param[in,out] errors Collection of parse failures.
    ///
    /// \param[in] slave_mode Whether or not this run is externally
    /// controlled by another simulation (reservoir coupling facility).
    ScheduleStatic(std::shared_ptr<const Python> python_handle,
                   const ScheduleRestartInfo& restart_info,
                   const Deck& deck,
                   const Runspec& runspec,
                   const std::optional<int>& output_interval_,
                   const ParseContext& parseContext,
                   ErrorGuard& errors,
                   const bool slave_mode);

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->m_input_path);
        serializer(this->rst_info);
        serializer(this->m_deck_message_limits);
        serializer(this->m_unit_system);
        serializer(this->m_runspec);
        serializer(this->rst_config);
        serializer(this->output_interval);
        serializer(this->sumthin);
        serializer(this->rptonly);
        serializer(this->gaslift_opt_active);
        serializer(this->oilVap);
        serializer(this->slave_mode);
    }

    /// Create a serialisation test object.
    static ScheduleStatic serializationTestObject();

    /// Equality predicate.
    ///
    /// \param[in] other Object against which \code *this \endcode will be
    /// tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p other.
    bool operator==(const ScheduleStatic& other) const;
};

} // end namespace Opm

#endif // SCHEDULE_STATIC_HPP
