/*
  Copyright (C) 2017 TNO

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
#include <opm/parser/eclipse/EclipseState/Grid/FaceDir.hpp>
#include <opm/parser/eclipse/EclipseState/Aquancon.hpp>
#include <utility>
#include <algorithm>
#include <iterator>
#include <iostream>

namespace Opm {
    namespace{
        struct AquanconRecord{
            std::vector<size_t> global_index_per_record;

            // Variables constants
            std::vector<std::shared_ptr<double>>  influx_coeff_per_record;  //Aquifer influx coefficient
            std::vector<double>  influx_mult_per_record;   //Aquifer influx coefficient Multiplier
            // Cell face to connect aquifer to
            std::vector<int> face_per_record;
            std::vector<int> record_index_per_record;
        };
    }

    Aquancon::Aquancon(const EclipseGrid& grid, const Deck& deck)
    {
        if (!deck.hasKeyword("AQUANCON"))
            return;

        std::vector<Opm::AquanconRecord> aqurecords;
        // Aquifer ID per record
        std::vector<int> aquiferID_per_record;
        int m_maxAquID = 0;

        const auto& aquanconKeyword = deck.getKeyword("AQUANCON");
        // Resize the parameter vector container based on row entries in aquancon
        aqurecords.resize(aquanconKeyword.size());
        aquiferID_per_record.resize(aquanconKeyword.size());

        for (size_t aquanconRecordIdx = 0; aquanconRecordIdx < aquanconKeyword.size(); ++aquanconRecordIdx)
        {
            const auto& aquanconRecord = aquanconKeyword.getRecord(aquanconRecordIdx);
            aquiferID_per_record[aquanconRecordIdx] = aquanconRecord.getItem("AQUIFER_ID").template get<int>(0);

            // offset the indices
            const int i1 = aquanconRecord.getItem("I1").template get<int>(0) - 1;
            const int i2 = aquanconRecord.getItem("I2").template get<int>(0) - 1;
            const int j1 = aquanconRecord.getItem("J1").template get<int>(0) - 1;
            const int j2 = aquanconRecord.getItem("J2").template get<int>(0) - 1;
            const int k1 = aquanconRecord.getItem("K1").template get<int>(0) - 1;
            const int k2 = aquanconRecord.getItem("K2").template get<int>(0) - 1;

            m_maxAquID = (m_maxAquID < aquiferID_per_record[aquanconRecordIdx] )?
                            aquiferID_per_record[aquanconRecordIdx] : m_maxAquID;

            const FaceDir::DirEnum faceDir = FaceDir::FromString(aquanconRecord.getItem("FACE").getTrimmedString(0));

            // whether allow aquifer connections to locate inside reservoir
            const std::string& str_inside_reservoir  = aquanconRecord.getItem("CONNECT_ADJOINING_ACTIVE_CELL").getTrimmedString(0);
            // not sure whether we should give a warning when input other than "YES" or "NO"
            const bool allow_aquifer_inside_reservoir = str_inside_reservoir == "YES" ? true : false;

            auto& aqurecord = aqurecords[aquanconRecordIdx];
            // Loop over the cartesian indices to convert to the global grid index
            for (int k = k1; k <= k2; k++) {
                for (int j = j1; j <= j2; j++) {
                    for (int i = i1; i <= i2; i++) {
                        if ( grid.cellActive(i, j, k) ) { // the cell itself needs to be active
                            if (  allow_aquifer_inside_reservoir
                               || !neighborCellInsideReservoirAndActive(grid, i, j, k, faceDir) ) {
                                     aqurecord.global_index_per_record.push_back(grid.getGlobalIndex(i, j, k));
                             }
                        }
                    }
                }
            }
            const size_t global_index_per_record_size = aqurecord.global_index_per_record.size();

            aqurecord.influx_coeff_per_record.resize(global_index_per_record_size, nullptr);

            if (aquanconRecord.getItem("INFLUX_COEFF").hasValue(0))
            {
                const double influx_coeff = aquanconRecord.getItem("INFLUX_COEFF").getSIDouble(0);

                for (auto& influx: aqurecord.influx_coeff_per_record)
                {
                    influx.reset(new double(influx_coeff));
                }
            }


            const double influx_mult = aquanconRecord.getItem("INFLUX_MULT").getSIDouble(0);

            aqurecord.influx_mult_per_record.resize(global_index_per_record_size, influx_mult);
            aqurecord.face_per_record.resize(global_index_per_record_size, faceDir);
            aqurecord.record_index_per_record.resize(global_index_per_record_size, aquanconRecordIdx);
        }

        // Collate_function
        collate_function(m_aquoutput, aqurecords, aquiferID_per_record, m_maxAquID);

        // Logic for grid connection applied here
        m_aquoutput = logic_application(m_aquoutput);
    }


    bool Aquancon::cellInsideReservoirAndActive(const Opm::EclipseGrid& grid, const int i, const int j, const int k)
    {
        if ( i < 0 || j < 0 || k < 0
            || size_t(i) > grid.getNX() - 1
            || size_t(j) > grid.getNY() - 1
            || size_t(k) > grid.getNZ() - 1 )
        {
            return false;
        }

        return grid.cellActive(i, j, k );
    }

    bool Aquancon::neighborCellInsideReservoirAndActive(const Opm::EclipseGrid& grid,
           const int i, const int j, const int k, const Opm::FaceDir::DirEnum faceDir)
    {
        switch(faceDir) {
        case FaceDir::XMinus:
            return cellInsideReservoirAndActive(grid, i - 1, j, k);
        case FaceDir::XPlus:
            return cellInsideReservoirAndActive(grid, i + 1, j, k);
        case FaceDir::YMinus:
            return cellInsideReservoirAndActive(grid, i, j - 1, k);
        case FaceDir::YPlus:
            return cellInsideReservoirAndActive(grid, i, j + 1, k);
        case FaceDir::ZMinus:
            return cellInsideReservoirAndActive(grid, i, j, k - 1);
        case FaceDir::ZPlus:
            return cellInsideReservoirAndActive(grid, i, j, k + 1);
        default:
            throw std::runtime_error("Unknown FaceDir enum " + std::to_string(faceDir));
        }
    }

    // This function is used to convert from a per record vector to a per aquifer ID vector.
    void Aquancon::collate_function(std::vector<Aquancon::AquanconOutput>& output_vector,
                                    std::vector<Opm::AquanconRecord>& aqurecords,
                                    const std::vector<int>& aquiferID_per_record,
                                    const int m_maxAquID)
    {
        output_vector.resize(m_maxAquID);
        // Find record indices at which the aquifer ids are located in
        for (int i = 1; i <= m_maxAquID; ++i)
        {
            std::vector<int> result_id;

            convert_record_id_to_aquifer_id(result_id, i, aquiferID_per_record);

            // We add the aquifer id into each element of output_vector
            output_vector.at(i - 1).aquiferID = i;
            for ( auto record_index_matching_id : result_id)
            {
                // This is for the global indices
                output_vector.at(i - 1).global_index.insert(
                                                             output_vector.at(i - 1).global_index.end(),
                                                             aqurecords.at(record_index_matching_id).global_index_per_record.begin(),
                                                             aqurecords.at(record_index_matching_id).global_index_per_record.end()
                                                           );
                // This is for the influx_coeff
                output_vector.at(i - 1).influx_coeff.insert(
                                                             output_vector.at(i - 1).influx_coeff.end(),
                                                             aqurecords.at(record_index_matching_id).influx_coeff_per_record.begin(),
                                                             aqurecords.at(record_index_matching_id).influx_coeff_per_record.end()
                                                           );
                // This is for the influx_multiplier
                output_vector.at(i - 1).influx_multiplier.insert(
                                                                  output_vector.at(i - 1).influx_multiplier.end(),
                                                                  aqurecords.at(record_index_matching_id).influx_mult_per_record.begin(),
                                                                  aqurecords.at(record_index_matching_id).influx_mult_per_record.end()
                                                                );
                // This is for the reservoir_face_dir
                output_vector.at(i - 1).reservoir_face_dir.insert(
                                                                   output_vector.at(i - 1).reservoir_face_dir.end(),
                                                                   aqurecords.at(record_index_matching_id).face_per_record.begin(),
                                                                   aqurecords.at(record_index_matching_id).face_per_record.end()
                                                                 );
                // This is for the record index in order for us to know which one is updated
                output_vector.at(i - 1).record_index.insert(
                                                             output_vector.at(i - 1).record_index.end(),
                                                             aqurecords.at(record_index_matching_id).record_index_per_record.begin(),
                                                             aqurecords.at(record_index_matching_id).record_index_per_record.end()
                                                           );
            }
        }
    }

    std::vector<Aquancon::AquanconOutput> Aquancon::logic_application(const std::vector<Aquancon::AquanconOutput>& original_vector)
    {
        std::vector<Aquancon::AquanconOutput> output_vector = original_vector;

        // Create a local struct to couple each element for easy sorting
        struct pair_elements
        {
          size_t global_index;
          std::shared_ptr<double> influx_coeff;
          double influx_multiplier;
          int reservoir_face_dir;
          int record_index;
        };


        // Iterate through each aquifer IDs (This is because each element in the original vector represents an aquifer ID)
        for (auto aquconvec = output_vector.begin(); aquconvec != output_vector.end(); ++aquconvec)
        {
            // Create a working buffer
            std::vector<pair_elements> working_buffer;
            for (size_t i = 0; i < aquconvec->global_index.size(); ++i )
                working_buffer.push_back( { aquconvec->global_index[i],
                                            aquconvec->influx_coeff[i],
                                            aquconvec->influx_multiplier[i],
                                            aquconvec->reservoir_face_dir[i],
                                            aquconvec->record_index[i]});


            // Sort by ascending order the working_buffer vector in order of priority:
            // 1) global_index, then 2) record_index

            std::sort(  working_buffer.begin(),
                        working_buffer.end(),
                        [](pair_elements& element1, pair_elements& element2) -> bool
                        {
                            if (element1.global_index == element2.global_index)
                            {
                                return element1.record_index < element2.record_index;
                            }
                            else
                                return element1.global_index < element2.global_index;
                        }
                     );

            // We then proceed to obtain unique elements of the global_index, and we apply the
            // following behaviour (as mentioned in the Eclipse 2014.1 Reference Manual p.345):
            // If a reservoir cell is defined more than once, its previous value for the
            // aquifer influx coefficient is added to the present value.

            auto i2 = std::unique(  working_buffer.begin(),
                                    working_buffer.end(),
                                    [](pair_elements& element1, pair_elements& element2) -> bool
                                    {
                                        // Not entirely clear for the manual; but it would seem
                                        // natural that this equality check also included the face?
                                        if (element1.global_index == element2.global_index)
                                        {
                                            if (element1.influx_coeff && element2.influx_coeff)
                                                *(element1.influx_coeff) += *(element2.influx_coeff);
                                            else {
                                                if (element1.influx_coeff || element2.influx_coeff)
                                                    throw std::invalid_argument("Sorry - can not combine defaulted and not default AQUANCON records");
                                            }

                                            return true;
                                        }

                                        return false;
                                    }
                                 );
            working_buffer.resize( std::distance(working_buffer.begin(),i2) );

            // We now resize and fill the output_vector elements
            aquconvec->global_index.resize(working_buffer.size());
            aquconvec->influx_coeff.resize(working_buffer.size());
            aquconvec->influx_multiplier.resize(working_buffer.size());
            aquconvec->reservoir_face_dir.resize(working_buffer.size());
            aquconvec->record_index.resize(working_buffer.size());
            for (size_t i = 0; i < working_buffer.size(); ++i)
            {
                aquconvec->global_index.at(i) = working_buffer.at(i).global_index;
                aquconvec->influx_coeff.at(i) = working_buffer.at(i).influx_coeff;
                aquconvec->influx_multiplier.at(i) = working_buffer.at(i).influx_multiplier;
                aquconvec->reservoir_face_dir.at(i) = working_buffer.at(i).reservoir_face_dir;
                aquconvec->record_index.at(i) = working_buffer.at(i).record_index;
            }

        }

        return output_vector;
    }

    void Aquancon::convert_record_id_to_aquifer_id(std::vector<int>& record_indices_matching_id,
                                                   const int i, const std::vector<int>& aquiferID_per_record)
    {
        auto it = std::find_if( aquiferID_per_record.begin(), aquiferID_per_record.end(),
                                    [&](int id) {
                                        return id == i;
                                    }
                                );
        while (it != aquiferID_per_record.end()) {
            record_indices_matching_id.emplace_back(std::distance(aquiferID_per_record.begin(), it));
            it = std::find_if(std::next(it), aquiferID_per_record.end(), [&](int id){return id == i;});
        }
    }

    const std::vector<Aquancon::AquanconOutput>& Aquancon::getAquOutput() const
    {
        return m_aquoutput;
    }

}
