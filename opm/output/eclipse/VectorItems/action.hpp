/*
  Copyright (c) 2021 Equinor ASA

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

#ifndef OPM_OUTPUT_ECLIPSE_VECTOR_ACTION_HPP
#define OPM_OUTPUT_ECLIPSE_VECTOR_ACTION_HPP

#include <vector>

namespace Opm::RestartIO::Helpers::VectorItems {

    namespace IACN {
        enum index : std::vector<int>::size_type {
            /// Region ID of LHS region quantity.
            LHSRegionID = 0,

            /// Region ID of RHS region quantity.
            RHSRegionID = 1,

            /// Segment ID of LHS segment quantity.
            LHSSegmentID = 2,

            /// Segment ID of RHS segment quantity.
            RHSSegmentID = 3,

            /// I location of LHS connection quantity
            LHSConnLocation_I = 4,

            /// J location of LHS connection quantity
            LHSConnLocation_J = 5,

            /// K location of LHS connection quantity
            LHSConnLocation_K = 6,

            /// I location of RHS connection quantity
            RHSConnLocation_I = 7,

            /// J location of RHS connection quantity
            RHSConnLocation_J = 8,

            /// K location of RHS connection quantity
            RHSConnLocation_K = 9,

            /// Quantity type (level) of LHS quantity.
            LHSQuantityType = 10,

            /// Quantity type (level) of RHS quantity.
            RHSQuantityType = 11,

            FirstGreater    = 12,
            TerminalLogic   = 13,
            Paren           = 15,
            Comparator      = 16,
            BoolLink        = 17,

            /// I location of LHS block/cell quantity
            LHSBlockLocation_I = 20,

            /// J location of LHS block/cell quantity
            LHSBlockLocation_J = 21,

            /// K location of LHS block/cell quantity
            LHSBlockLocation_K = 22,

            /// I location of RHS block/cell quantity
            RHSBlockLocation_I = 23,

            /// J location of RHS block/cell quantity
            RHSBlockLocation_J = 24,

            /// K location of RHS block/cell quantity
            RHSBlockLocation_K = 25,
        };

        // The same enum is used for both left-hand side and right-hand side
        // quantities.  Note that not all values are eligible for both
        // sides.
        namespace Value {
            enum QuantityType {
                /// Quantity is defined at the field level (F* vector).
                Field = 1,

                /// Quantity is defined at the well level (W* vector).
                Well = 2,

                /// Quantity is defined at the group level (G* vector).
                Group = 3,

                /// Quantity is defined at the region level (R* vector).
                Region = 4,

                /// Quantity is defined at the segment level (S* vector).
                Segment = 5,

                /// Quantity is defined at the connection level (C* vector).
                Connection = 6,

                /// Quantity is a constant--i.e., a number.
                Const = 8,

                /// Quantity is a day (in a month).
                Day = 10,

                /// Quantity is a month.
                Month = 11,

                /// Quantity is a year.
                Year = 12,

                /// Quantity is defined a the block/cell level (B* vector).
                Block = 15,
            };

            enum ParenType {
                None = 0,
                Open = 1,
                Close = 2,
            };
        } // namespace Value

        constexpr std::size_t ConditionSize = 26;
    } // namespace IACN

    namespace SACN {
        enum index : std::vector<int>::size_type {
            LHSValue0 = 0,
            RHSValue0 = 2,
            LHSValue1 = 4,
            RHSValue1 = 5,
            LHSValue2 = 6,
            RHSValue2 = 7,
            LHSValue3 = 8,
            RHSValue3 = 9,
        };

        constexpr std::size_t ConditionSize = 16;
    } // namespace SACN

    namespace ZACN {
        enum index : std::vector<int>::size_type {
            Quantity = 0,

            /// Summary vector keyword of LHS quantity.
            LHSQuantity = 0,

            /// Summary vector keyword of RHS quantity.
            RHSQuantity = 1,

            Comparator = 2,

            Well = 3,

            /// Well name of LHS quantity (Well, Connection, Segment).
            LHSWell = 3,

            /// Well name of RHS quantity (Well, Connection, Segment).
            RHSWell = 4,

            Group = 5,

            /// Group name of LHS quantity (Group).
            LHSGroup = 5,

            /// Group name of RHS quantity (Group).
            RHSGroup = 6,

            /// Region set name for LHS region quantity.  "NUM" for default FIPNUM set.
            LHSRegionSet = 7,

            /// Region set name for RHS region quantity.  "NUM" for default FIPNUM set.
            RHSRegionSet = 8,
        };

        constexpr std::size_t RHSOffset = 1;
        constexpr std::size_t ConditionSize = 13;
    } // namespace ZACN

    namespace ZLACT {
        constexpr std::size_t max_line_length = 128;
    } // namespace ZLACT

} // namespace Opm::RestartIO::Helpers::VectorItems

#endif
