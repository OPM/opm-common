/*
  Copyright 2020 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  OPM is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.

  You should have received a copy of the GNU General Public License along
  with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RST_STATE
#define RST_STATE

#include <opm/io/eclipse/rst/action.hpp>
#include <opm/io/eclipse/rst/aquifer.hpp>
#include <opm/io/eclipse/rst/group.hpp>
#include <opm/io/eclipse/rst/header.hpp>
#include <opm/io/eclipse/rst/netbalan.hpp>
#include <opm/io/eclipse/rst/network.hpp>
#include <opm/io/eclipse/rst/udq.hpp>
#include <opm/io/eclipse/rst/well.hpp>

#include <opm/input/eclipse/Schedule/OilVaporizationProperties.hpp>
#include <opm/input/eclipse/Schedule/Tuning.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <ctime>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace Opm {
    class EclipseGrid;
    class Parser;
    class Runspec;
} // namespace Opm

namespace Opm { namespace EclIO {
    class RestartFileView;
}} // namespace Opm::EclIO

namespace Opm { namespace RestartIO {

struct RstState
{
    RstState(std::shared_ptr<EclIO::RestartFileView> rstView,
             const ::Opm::EclipseGrid*               grid,
             std::optional<int>                      numPVTTables = std::nullopt);

    static RstState load(std::shared_ptr<EclIO::RestartFileView> rstView,
                         const Parser&                           parser,
                         std::optional<int>                      numPVTTables = std::nullopt,
                         const ::Opm::EclipseGrid*               grid = nullptr);

    static RstState load(std::shared_ptr<EclIO::RestartFileView> rstView,
                         const Runspec&                          runspec,
                         const Parser&                           parser,
                         const ::Opm::EclipseGrid*               grid = nullptr);

    const RstWell& get_well(const std::string& wname) const;

    ::Opm::UnitSystem unit_system;
    RstHeader header;
    RstAquifer aquifers;
    RstNetbalan netbalan;
    RstNetwork network;
    std::vector<RstWell> wells;
    std::vector<RstGroup> groups;
    std::vector<RstUDQ> udqs;
    std::optional<RstUDQActive> udq_active;
    std::vector<RstAction> actions;
    Tuning tuning;
    OilVaporizationProperties oilvap;
    std::unordered_map<std::string, std::vector<std::string>> wlists;

private:
    /// Encapsulation of restart file information pertaining to actions or conditions.
    ///
    /// \tparam FloatType The floating point type used for the action data.
    /// Typically \c float or \c double.
    template <typename FloatType>
    struct ActionData
    {
        /// Integer restart file information pertaining to all actions or conditions.
        ///
        /// Typically wraps IACT for actions and IACN for conditions.
        std::span<const int> i;

        /// Floating point restart file information pertaining to all actions or conditions.
        ///
        /// Typically wraps SACT (\c float) for actions and SACN (\c double) for conditions.
        std::span<const FloatType> s;

        /// String restart file information pertaining to all actions or conditions.
        ///
        /// Typically wraps ZACT for actions and ZACN for conditions.
        std::span<const std::string> z;
    };

    void load_oil_vaporization(const std::vector<int>&    intehead,
                               const std::vector<bool>&   logihead,
                               const std::vector<double>& doubhead);

    void load_tuning(const std::vector<int>& intehead,
                     const std::vector<double>& doubhead);

    void add_groups(const std::vector<std::string>& zgrp,
                    const std::vector<int>& igrp,
                    const std::vector<float>& sgrp,
                    const std::vector<double>& xgrp);

    void add_wells(const std::vector<std::string>& zwel,
                   const std::vector<int>& iwel,
                   const std::vector<float>& swel,
                   const std::vector<double>& xwel,
                   const std::vector<int>& icon,
                   const std::vector<float>& scon,
                   const std::vector<double>& xcon);

    void add_msw(const std::vector<std::string>& zwel,
                 const std::vector<int>& iwel,
                 const std::vector<float>& swel,
                 const std::vector<double>& xwel,
                 const std::vector<int>& icon,
                 const std::vector<float>& scon,
                 const std::vector<double>& xcon,
                 const std::vector<int>& iseg,
                 const std::vector<double>& rseg);

    void add_udqs(std::shared_ptr<EclIO::RestartFileView> rstView);

    /// Restore all applicable dynamic actions from the restart file.
    ///
    /// Populates the \c actions vector of this \c RstState object with all applicable actions.
    ///
    /// \param[in] parser The parser used to interpret action block keywords.
    ///
    /// \param[in] runspec Run's specification including dimensions and active phases.
    ///
    /// \param[in] sim_time Simulated time (i.e., time point) at simulation restart.
    ///
    /// \param[in] actions The action data (i.e., the [ISZ]ACT arrays).
    ///
    /// \param[in] conditions The condition data (i.e., the [ISZ]ACN arrays).
    ///
    /// \param[in] zlact The action block keywords.
    void add_actions(const Parser&                parser,
                     ActionData<float>            actions,
                     ActionData<double>           conditions,
                     std::span<const std::string> zlact);

    void add_wlist(const std::vector<std::string>& zwls,
                   const std::vector<int>& iwls);

    /// Restore conditions for single action from restart file information.
    ///
    /// \param[in] conditions The condition data (i.e., the [ISZ]ACN arrays).
    ///
    /// \param[in] index The action index.
    ///
    /// \return The restored conditions for the \p index-th action.
    std::vector<RstAction::Condition>
    restore_conditions(ActionData<double> conditions,
                       std::size_t        index) const;

    /// Restore a single action from restart file information.
    ///
    /// Appends an action object to the \c actions vector of this \c RstState object.
    ///
    /// \param[in] runspec The run's specification including dimensions and active phases.
    ///
    /// \param[in] sim_time Simulated time (i.e., time point) at simulation restart.
    ///
    /// \param[in] actionArrays The action data (i.e., the [ISZ]ACT arrays).
    ///
    /// \param[in] index The action index.
    ///
    /// \param[in] conditions The restored conditions for the \p index-th action.
    void create_action(ActionData<float>                   actionArrays,
                       std::size_t                         index,
                       std::vector<RstAction::Condition>&& conditions);

    /// Restore action keywords from restart file information.
    ///
    /// Populates the \c keywords container of \code actions.back() \endcode
    /// with the pertinent action block keywords.
    ///
    /// \param[in] parser The parser used to interpret action block keywords.
    ///
    /// \param[in] zlact Linearised collection of action block keyword strings.
    /// Assumed to encompass only those strings that pertain to \code actions.back() \endcode.
    void restore_action_keywords(const Parser&                parser,
                                 std::span<const std::string> zlact);

};

}} // namespace Opm::RestartIO

#endif
