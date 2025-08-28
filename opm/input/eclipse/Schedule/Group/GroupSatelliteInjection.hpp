/*
  Copyright 2025 Equinor ASA.

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

#ifndef OPM_GROUP_SATELLITE_INJECTION_HPP_INCLUDED
#define OPM_GROUP_SATELLITE_INJECTION_HPP_INCLUDED

#include <opm/input/eclipse/EclipseState/Phase.hpp>

#include <map>
#include <optional>
#include <vector>

namespace Opm {

    /// Group level satellite production
    class GroupSatelliteInjection
    {
    public:
        /// Satellite injection rates for a single phase.
        class Rate
        {
        public:
            /// Assign surface injection rate for this phase.
            ///
            /// \param[in] q Surface injection rate [sm3/s].
            ///
            /// \return *this
            Rate& surface(const double q);

            /// Assign reservoir injection rate for this phase.
            ///
            /// \param[in] q Reservoir injection rate [rm3/s].
            ///
            /// \return *this
            Rate& reservoir(const double q);

            /// Assign mean calorific value for this phase.
            ///
            /// \param[in] c Mean calorific value [kJ/sm3].
            ///
            /// \return *this
            Rate& calorific(const double c);

            /// Surface injection rate for this phase.
            ///
            /// Nullopt if there is no defined surface injection rate.
            const std::optional<double>& surface() const
            {
                return this->surface_;
            }

            /// Reservoir injection rate for this phase.
            ///
            /// Nullopt if there is no defined reservoir injection rate.
            const std::optional<double>& reservoir() const
            {
                return this->resv_;
            }

            /// Mean calorific value of injected gas.
            ///
            /// Nullopt if there is no defined mean calorific value.
            const std::optional<double>& calorific() const
            {
                return this->calorific_;
            }

            /// Create a serialisation test object.
            static Rate serializationTestObject();

            /// Equality predicate.
            ///
            /// \param[in] that Object against which \code *this \endcode
            /// will be tested for equality.
            ///
            /// \return Whether or not \code *this \endcode is the same as
            /// \p that.
            bool operator==(const Rate& that) const
            {
                return (this->surface_ == that.surface_)
                    && (this->resv_ == that.resv_)
                    && (this->calorific_ == that.calorific_)
                    ;
            }

            /// Convert between byte array and object representation.
            ///
            /// \tparam Serializer Byte array conversion protocol.
            ///
            /// \param[in,out] serializer Byte array conversion object.
            template <class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(this->surface_);
                serializer(this->resv_);
                serializer(this->calorific_);
            }

        private:
            /// Surface injection rate for this phase.
            ///
            /// Nullopt if there is no surface injection rate defined for
            /// this phase.
            std::optional<double> surface_{};

            /// Reservoir injection rate for this phase.
            ///
            /// Nullopt if there is no reservoir injection rate defined for
            /// this phase.
            std::optional<double> resv_{};

            /// Mean calorific injection rate for this phase.
            ///
            /// Nullopt if there is no mean calorific injection rate defined
            /// for this phase.
            std::optional<double> calorific_{};
        };

        /// Index type for looking up phase rate objects.
        using RateIx = std::vector<Rate>::size_type;

        /// Create a serialisation test object.
        static GroupSatelliteInjection serializationTestObject();

        /// Default constructor.
        ///
        /// Forms an object that's mostly suitable as a target in a
        /// deserialisation operation.
        GroupSatelliteInjection() = default;

        /// Constructor.
        ///
        /// Creates a container for a particular named group, but without
        /// any satellite injection rates defined for any phase.
        ///
        /// \param[in] group Group name.
        explicit GroupSatelliteInjection(const std::string& group);

        /// Read/write access to injection rate object for particular phase.
        ///
        /// Will create a \c Rate object as needed.
        ///
        /// \param[in] phase Injection phase.
        ///
        /// \return Mutable \c Rate object corresponding to injection phase
        /// \p phase.
        Rate& rate(const Phase phase);

        /// Group name.
        ///
        /// This is needed to meet interface requirements of class
        /// ScheduleState::map_member<>, but is otherwise redundant.
        const std::string& name() const { return this->group_; }

        /// Compute lookup index for particular phase
        ///
        /// \param[in] phase Injection phase.
        ///
        /// \return Lookup index for injection phase \p phase.  Nullopt if
        /// there are no satellite injection rates recorded for phase \p
        /// phase in this group.
        std::optional<RateIx> rateIndex(const Phase phase) const;

        /// Read only satellite injection rates.
        ///
        /// \param[in] i Lookup index.  Typically computed in a previous
        /// call to member function rateIndex().
        ///
        /// \return Satellite injection rate object at position \p i.
        const Rate& operator[](const RateIx i) const
        {
            return this->rates_[i];
        }

        /// Equality predicate.
        ///
        /// \param[in] data Object against which \code *this \endcode will
        /// be tested for equality.
        ///
        /// \return Whether or not \code *this \endcode is the same as \p
        /// that.
        bool operator==(const GroupSatelliteInjection& that) const;

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->group_);
            serializer(this->i_);
            serializer(this->rates_);
        }

    private:
        /// Group name.
        ///
        /// Mostly to satisfy interface requirements of class
        /// ScheduleState::map_member<>.
        std::string group_{};

        /// Indirection map for recorded phase injection rates.
        std::map<Phase, RateIx> i_{};

        /// Satellite injection rates for all recorded phases.
        std::vector<Rate> rates_{};
    };

} // namespace Opm

#endif // OPM_GROUP_SATELLITE_INJECTION_HPP_INCLUDED
