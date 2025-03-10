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

#ifndef COMPLETION_HPP_
#define COMPLETION_HPP_

#include <opm/input/eclipse/Schedule/Well/FilterCake.hpp>
#include <opm/input/eclipse/Schedule/Well/WINJMULT.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Opm {
    class DeckKeyword;
    class DeckRecord;
    class ScheduleGrid;
    class FieldPropsManager;
} // namespace Opm

namespace Opm { namespace RestartIO {
    struct RstConnection;
}} // namespace Opm::RestartIO

namespace Opm {

    enum class ConnectionOrder {
        DEPTH,
        INPUT,
        TRACK,
    };

    class Connection
    {
    public:
        enum class State {
            OPEN = 1,
            SHUT = 2,
            AUTO = 3, // Seems like the AUTO state can not be serialized to restart files.
        };

        static std::string State2String(State enumValue);
        static State StateFromString(std::string_view stringValue);


        enum class Direction {
            X = 1,
            Y = 2,
            Z = 3,
        };

        static std::string Direction2String(const Direction enumValue);
        static Direction   DirectionFromString(std::string_view stringValue);


        using Order = ConnectionOrder;

        static std::string Order2String(Order enumValue);
        static Order OrderFromString(std::string_view comporderStringValue);


        enum class CTFKind {
            DeckValue,
            Defaulted,
        };


        /// Quantities that go into calculating the connection
        /// transmissibility factor.
        struct CTFProperties
        {
            /// Static connection transmissibility factor calculated from
            /// input quantities.
            double CF{};

            /// Static 'Kh' product
            double Kh{};

            /// Effective permeability.
            double Ke{};

            /// Connection's wellbore radius
            double rw{};

            /// Connection's pressure equivalent radius
            double r0{};

            /// Connection's area equivalent radius--mostly for use by the
            /// polymer code.
            double re{};

            /// Length of connection's perfororation interval
            double connection_length{};

            /// Connection's skin factor.
            double skin_factor{};

            /// Connection's D factor-i.e., the flow-dependent skin factor
            /// for gas.
            double d_factor{};

            /// Product of certain static elements of D-factor correlation
            /// law (WDFACCOR keyword).
            double static_dfac_corr_coeff{};

            /// Denominator in peaceman's formula-i.e., log(r0/rw) + skin.
            double peaceman_denom{};

            /// Serialisation test object.
            static CTFProperties serializationTestObject();

            /// Equality operator
            ///
            /// \param[in] that Property object to which \c *this will be compared.
            bool operator==(const CTFProperties& that) const;

            /// Inequality operator
            ///
            /// \param[in] that Property object to which \c *this will be compared.
            bool operator!=(const CTFProperties& that) const
            {
                return ! (*this == that);
            }

            /// Serialisation operator
            ///
            /// \tparam Serializer Protocol for serialising and
            ///   deserialising objects between memory and character
            ///   buffers.
            ///
            /// \param[in,out] serializer Serialisation object.
            template <class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(this->CF);
                serializer(this->Kh);
                serializer(this->Ke);
                serializer(this->rw);
                serializer(this->r0);
                serializer(this->re);
                serializer(this->connection_length);
                serializer(this->skin_factor);
                serializer(this->d_factor);
                serializer(this->static_dfac_corr_coeff);
                serializer(this->peaceman_denom);
            }
        };


        Connection() = default;
        Connection(int i, int j, int k,
                   std::size_t          global_index,
                   int                  complnum,
                   State                state,
                   Direction            direction,
                   CTFKind              ctf_kind,
                   const int            satTableId,
                   double               depth,
                   const CTFProperties& ctf_properties,
                   const std::size_t    sort_value,
                   const bool           defaultSatTabId,
                   int                  lgr_grid_ = 0);

        Connection(const RestartIO::RstConnection& rst_connection,
                   const ScheduleGrid&             grid,
                   const FieldPropsManager&        fp);

        static Connection serializationTestObject();

        bool attachedToSegment() const;
        bool sameCoordinate(const int i, const int j, const int k) const;
        int getI() const;
        int getJ() const;
        int getK() const;
        std::size_t global_index() const;
        State state() const;
        Direction dir() const;
        double depth() const;
        int satTableId() const;
        int complnum() const;
        int segment() const;
        double wpimult() const;
        double CF() const;
        double Kh() const;
        double Ke() const;
        double rw() const;
        double r0() const;
        double re() const;
        double connectionLength() const;
        double skinFactor() const;
        double dFactor() const;
        CTFKind kind() const;
        const InjMult& injmult() const;
        bool activeInjMult() const;
        const FilterCake& getFilterCake() const;
        bool filterCakeActive() const;
        double getFilterCakeRadius() const;
        double getFilterCakeArea() const;

        const CTFProperties& ctfProperties() const
        {
            return this->ctf_properties_;
        }

        std::size_t sort_value() const;
        bool getDefaultSatTabId() const;
        const std::optional<std::pair<double, double>>& perf_range() const;
        std::string str() const;

        bool ctfAssignedFromInput() const
        {
            return this->m_ctfkind == CTFKind::DeckValue;
        }

        bool operator==(const Connection&) const;
        bool operator!=(const Connection& that) const
        {
            return ! (*this == that);
        }

        void setInjMult(const InjMult& inj_mult);
        void setFilterCake(const FilterCake& filter_cake);
        void setState(State state);
        void setComplnum(int compnum);
        void setSkinFactor(double skin_factor);
        void setDFactor(double d_factor);
        void setKe(double Ke);
        void setCF(double CF);
        void setDefaultSatTabId(bool id);
        void setStaticDFacCorrCoeff(const double c);

        void scaleWellPi(double wellPi);
        bool prepareWellPIScaling();
        bool applyWellPIScaling(const double scaleFactor);

        void updateSegmentRST(int segment_number_arg,
                              double center_depth_arg);
        void updateSegment(int segment_number_arg,
                           double center_depth_arg,
                           std::size_t compseg_insert_index,
                           const std::optional<std::pair<double,double>>& perf_range);

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->direction);
            serializer(this->center_depth);
            serializer(this->open_state);
            serializer(this->sat_tableId);
            serializer(this->m_complnum);
            serializer(this->ctf_properties_);
            serializer(this->ijk);
            serializer(this->lgr_grid);
            serializer(this->m_ctfkind);
            serializer(this->m_global_index);
            serializer(this->m_injmult);
            serializer(this->m_sort_value);
            serializer(this->m_perf_range);
            serializer(this->m_defaultSatTabId);
            serializer(this->segment_number);
            serializer(this->m_wpimult);
            serializer(this->m_subject_to_welpi);
            serializer(this->m_filter_cake);
        }

    private:
        // Note to maintainer: If you add new members to this list, then
        // please also update the operator==(), serializeOp(), and
        // serializationTestObject() member functions.
        Direction direction { Direction::Z };
        double center_depth { 0.0 };
        State open_state { State::SHUT };
        int sat_tableId { -1 };
        int m_complnum { -1 };
        CTFProperties ctf_properties_{};

        std::array<int,3> ijk{};
        int lgr_grid{0};
        CTFKind m_ctfkind { CTFKind::DeckValue };
        std::optional<InjMult> m_injmult{};
        std::size_t m_global_index{};

        /*
          The sort_value member is a peculiar quantity. The connections are
          assembled in the WellConnections class. During the lifetime of the
          connections there are three different sort orders which are all
          relevant:

             input: This is the ordering implied be the order of the
                connections in the input deck.

             simulation: This is the ordering the connections have in
                WellConnections container during the simulation and RFT output.

             restart: This is the ordering the connections have when they are
                written out to a restart file.

          Exactly what consitutes input, simulation and restart ordering, and
          how the connections transition between the three during application
          lifetime is different from MSW and normal wells.

          normal wells: For normal wells the simulation order is given by the
          COMPORD keyword, and then when the connections are serialized to the
          restart file they are written in input order; i.e. we have:

               input == restart and simulation given COMPORD

          To recover the input order when creating the restart files the
          sort_value member corresponds to the insert index for normal wells.

          MSW wells: For MSW wells the wells simulator order[*] is given by
          COMPSEGS keyword, the COMPORD keyword is ignored. The connections are
          sorted in WellConnections::order() and then retain that order for all
          eternity i.e.

               input and simulation == restart

          Now the important point is that the COMPSEGS detail used to perform
          this sorting is not available when loading from a restart file, but
          then the connections are already sorted correctly. I.e. *after* a
          restart we will have:

               input(from restart) == simulation == restart

          The sort_value member is used to sort the connections into restart
          ordering. In the case of normal wells this corresponds to recovering
          the input order, whereas for MSW wells this is equivalent to the
          simulation order.

          [*]: For MSW wells the topology is given by the segments and entered
               explicitly, so the truth is probably that the storage order
               during simulation makes no difference?
        */
        std::size_t m_sort_value{};

        std::optional<std::pair<double,double>> m_perf_range{};
        bool m_defaultSatTabId{true};

        // Associate segment number
        //
        // 0 means the connection is not associated to a segment.
        int segment_number { 0 };

        double m_wpimult { 1.0 };

        // Whether or not this Connection is subject to WELPI scaling.
        bool m_subject_to_welpi { false };

        std::optional<FilterCake> m_filter_cake{};

        static std::string CTFKindToString(const CTFKind);
    };

} // namespace Opm

#endif // COMPLETION_HPP_
