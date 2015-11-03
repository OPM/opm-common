/*
  Copyright 2015 SINTEF ICT, Applied Mathematics.

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

#include <opm/parser/eclipse/EclipseState/Schedule/Compsegs.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>

namespace Opm {


    Compsegs::Compsegs(int i_in, int j_in, int k_in, int branch_number_in, double distance_start_in, double distance_end_in,
                       WellCompletion::DirectionEnum dir_in, double center_depth_in, double thermal_length_in, int segment_number_in)
    : m_i(i_in),
      m_j(j_in),
      m_k(k_in),
      m_branch_number(branch_number_in),
      m_distance_start(distance_start_in),
      m_distance_end(distance_end_in),
      m_dir(dir_in),
      m_center_depth(center_depth_in),
      m_thermal_length(thermal_length_in),
      m_segment_number(segment_number_in)
    {
    }

    std::vector<std::shared_ptr<Compsegs>> Compsegs::compsegsFromCOMPSEGSKeyword(DeckKeywordConstPtr compsegsKeyword,
                                                                                 EclipseGridConstPtr grid) {
        // the thickness of grid cells will be required in the future for more complete support.
        // Silence warning about unused argument
        static_cast<void>(grid);

        // only handle the second record here
        // The first record here only contains the well name
        std::vector<std::shared_ptr<Compsegs>> compsegs;

        for (size_t recordIndex = 1; recordIndex < compsegsKeyword->size(); ++recordIndex) {
            DeckRecordConstPtr record = compsegsKeyword->getRecord(recordIndex);
            // following the coordinate rule for completions
            const int I = record->getItem("I")->getInt(0) - 1;
            const int J = record->getItem("J")->getInt(0) - 1;
            const int K = record->getItem("K")->getInt(0) - 1;
            const int branch = record->getItem("BRANCH")->getInt(0);

            double distance_start;
            double distance_end;
            if (record->getItem("DISTANCE_START")->hasValue(0)) {
                distance_start = record->getItem("DISTANCE_START")->getSIDouble(0);
            } else if (recordIndex == 1) {
                distance_start = 0.;
            } else {
                // TODO: the end of the previous connection or range
                // 'previous' should be in term of the input order
                // since basically no specific order for the completions
                throw std::runtime_error("this way to obtain DISTANCE_START not implemented yet!");
            }
            if (record->getItem("DISTANCE_END")->hasValue(0)) {
                distance_end = record->getItem("DISTANCE_END")->getSIDouble(0);
            } else {
                // TODO: the distance_start plus the thickness of the grid block
                throw std::runtime_error("this way to obtain DISTANCE_END not implemented yet!");
            }

            WellCompletion::DirectionEnum direction;
            if (record->getItem("DIRECTION")->hasValue(0)) {
                direction = WellCompletion::DirectionEnumFromString(record->getItem("DIRECTION")->getString(0));
            } else if (!record->getItem("DISTANCE_END")->hasValue(0)) {
                throw std::runtime_error("the direction has to be specified when DISTANCE_END in the record is not specified");
            }

            int end_IJK;
            if (record->getItem("END_IJK")->hasValue(0)) {
                // following the coordinate rule for completions
                end_IJK = record->getItem("END_IJK")->getInt(0) - 1;
                if (!record->getItem("DIRECTION")->hasValue(0)) {
                    throw std::runtime_error("the direction has to be specified when END_IJK in the record is specified");
                }
            } else {
                // only one completion is specified here
                end_IJK = -1;
            }

            double center_depth;
            if (!record->getItem("CENTER_DEPTH")->defaultApplied(0)) {
                center_depth = record->getItem("CENTER_DEPTH")->getSIDouble(0);
            } else {
                // 0.0 is also the defaulted value
                // which is used to indicate to obtain the final value through related segment
                center_depth = 0.;
            }

            if (center_depth < 0.) {
                //TODO: get the depth from COMPDAT data.
                throw std::runtime_error("this way to obtain CENTER_DISTANCE not implemented yet either!");
            }

            double thermal_length;
            if (!record->getItem("THERMAL_LENGTH")->defaultApplied(0)) {
                thermal_length = record->getItem("THERMAL_LENGTH")->getSIDouble(0);
            } else {
                //TODO: get the thickness of the grid block in the direction of penetration
                throw std::runtime_error("this way to obtain THERMAL_LENGTH not implemented yet!");
            }

            int segment_number;
            if (record->getItem("SEGMENT_NUMBER")->hasValue(0)) {
                segment_number = record->getItem("SEGMENT_NUMBER")->getInt(0);
            } else {
                segment_number = 0;
                // will decide the segment number based on the distance in a process later.
            }

            if (end_IJK < 0) { // only one compsegs
                CompsegsPtr new_compsegs = std::make_shared<Compsegs>(I, J, K, branch, distance_start, distance_end,
                                                                      direction, center_depth, thermal_length, segment_number);
                compsegs.push_back(new_compsegs);
            } else { // a range is defined. genrate a range of Compsegs
                throw std::runtime_error("entering COMPSEGS entries with a range is not supported yet!");
            }
        }

        return compsegs;
    }

}
