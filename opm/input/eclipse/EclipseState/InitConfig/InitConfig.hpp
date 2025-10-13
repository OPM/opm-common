/*
  Copyright 2015 Statoil ASA.

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

#ifndef OPM_INIT_CONFIG_HPP
#define OPM_INIT_CONFIG_HPP

#include <opm/input/eclipse/EclipseState/InitConfig/Equil.hpp>
#include <opm/input/eclipse/EclipseState/InitConfig/FoamConfig.hpp>

#include <string>

namespace Opm {

    class Deck;
    class Phases;

} // namespace Opm

namespace Opm {

    /// Settings for model initialisation.
    class InitConfig
    {
    public:
        /// Default constructor.
        ///
        /// Resulting object is mostly usable as a target in a
        /// deserialisation operation.
        InitConfig() = default;

        /// Constructor.
        ///
        /// Internalises the run's initialisation-related information.
        ///
        /// \param[in] deck Run's model description.
        /// \param[in] phases Run's active phase description.
        InitConfig(const Deck& deck, const Phases& phases);

        /// Create a serialisation test object.
        static InitConfig serializationTestObject();

        /// Assign simulation restart information.
        ///
        /// Mostly provided to construct a meaningful InitConfig object
        /// during simulation restart.
        ///
        /// \param[in] root Full path to run's restart input file containing
        /// the run's initial pressure and mass distributions.
        ///
        /// \param[in] step One-based report step from which to restart the
        /// simulation run.
        void setRestart(const std::string& root, int step);

        /// Whether or not this is a restarted simulation run (input uses
        /// the RESTART keyword).
        bool restartRequested() const;

        /// Report step from which to restart the simulation.
        ///
        /// Only meaningful if restartRequested() returns true.
        int getRestartStep() const;

        /// Full path to run's restart input (i.e., run's initial pressures,
        /// saturations, Rs, &c).
        ///
        /// Only meaningful if restartRequested() returns true.
        const std::string& getRestartRootName() const;

        /// Relative path to run's restart input (i.e., run's initial
        /// pressures, saturations, Rs, &c).
        ///
        /// Copy of item 1 from the RESTART keyword.
        ///
        /// Only meaningful if restartRequested() returns true.
        const std::string& getRestartRootNameInput() const;

        /// Whether or not run uses initialisation by equilibration
        bool hasEquil() const;

        /// Equilibration specification.
        ///
        /// Only meaningful if hasEquil() returns true.
        const Equil& getEquil() const;

        /// Whether or not run initialises its mechanical stresses by an
        /// equilibration procedure (STREQUIL keyword).
        ///
        /// Only relevant for runs with geo-mechanical effects.
        bool hasStressEquil() const;

        /// Mechanical stress equilibration specification.
        ///
        /// Only meaningful if hasStressEquil() returns true.
        const StressEquil& getStressEquil() const;

        /// Whether or not run includes gravity effects.
        ///
        /// Will be true unless run specifies the NOGRAV keyword.
        bool hasGravity() const
        {
            return this->m_gravity;
        }

        /// Whether or not run includes foam effects.
        bool hasFoamConfig() const;

        /// Run's foam configuration.
        ///
        /// Only meaningful if hasFoamConfig() returns true.
        const FoamConfig& getFoamConfig() const;

        /// Whether or not the run specifies the FILLEPS keyword that
        /// requests expanded end-point scaling arrays be output to the
        /// run's INIT file.
        bool filleps() const
        {
            return this->m_filleps;
        }

        /// Equality predicate.
        ///
        /// \param[in] config Object against which \code *this \endcode will
        /// be tested for equality.
        ///
        /// \return Whether or not \code *this \endcode is the same as \p
        /// config.
        bool operator==(const InitConfig& config) const;

        /// Equality predicate for objects created from restart file
        /// information.
        ///
        /// Exists mostly to support simulation restart development and may
        /// be removed in the future.
        ///
        /// \param[in] full_config Initialisation information from a
        /// complete model description.
        ///
        /// \param[in] rst_config Initialisation information formed from
        /// restart file information.
        ///
        /// \return Whether or not the restart information in \p rst_config
        /// matches the retart information in \p full_config.
        static bool rst_cmp(const InitConfig& full_config,
                            const InitConfig& rst_config);

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(equil);
            serializer(stress_equil);
            serializer(foamconfig);
            serializer(m_filleps);
            serializer(m_gravity);
            serializer(m_restartRequested);
            serializer(m_restartStep);
            serializer(m_restartRootName);
            serializer(m_restartRootNameInput);
        }

    private:
        /// Run's gravity equilibration specification.
        Equil equil{};

        /// Run's mechanical stress equilibration specification.
        StressEquil stress_equil{};

        /// Run's foam specification.
        FoamConfig foamconfig{};

        /// Whether or not run specifies FILLEPS keyword.
        bool m_filleps{false};

        /// Whether or not run includes gravity effects.
        ///
        /// Typically true.
        bool m_gravity{true};

        /// Whether or not this is a restarted simulation run.
        bool m_restartRequested{false};

        /// Report step from which to restart simulation.
        ///
        /// Meaningful only if m_restartRequested is true.
        int m_restartStep{0};

        /// Full path to run's restart input (initial pressures, saturation,
        /// Rs, Rv...).
        ///
        /// Meaningful only if m_restartRequested is true.
        std::string m_restartRootName{};

        /// Relative path to run's restart input (initial pressures,
        /// saturation, Rs, Rv...).
        ///
        /// Meaningful only if m_restartRequested is true.
        std::string m_restartRootNameInput{};

        /// Internalise run's RESTART keyword data.
        ///
        /// No effect unless run specifies RESTART keyword.
        ///
        /// \param[in] deck Run's model description.
        void parseRestartKeyword(const Deck& deck);
    };

} // namespace Opm

#endif // OPM_INIT_CONFIG_HPP
