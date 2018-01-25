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
#include <algorithm>
#include <iterator>

namespace Opm {
    namespace{
        struct AquanconRecord{
            // Grid cell box definition to connect aquifer
            int i1, i2, j1, j2, k1, k2;

            std::vector<size_t> global_index_per_record;

            // Variables constants
            std::vector<double>  influx_coeff_per_record,  //Aquifer influx coefficient
                                 influx_mult_per_record;   //Aquifer influx coefficient Multiplier       
            // Cell face to connect aquifer to        
            std::vector<int> face_per_record;           
        };
    }

    Aquancon::Aquancon(const EclipseGrid& grid, const Deck& deck)
    { 
        if (!deck.hasKeyword("AQUANCON"))
            return;  

        std::vector<Opm::AquanconRecord> m_aqurecord;
        // Aquifer ID per record
        std::vector<int> m_aquiferID_per_record;
        int m_maxAquID = 0;

        const auto& aquanconKeyword = deck.getKeyword("AQUANCON");
        // Resize the parameter vector container based on row entries in aquancon
        m_aqurecord.resize(aquanconKeyword.size());
        m_aquiferID_per_record.resize(aquanconKeyword.size());

        // We now do a loop over each record entry in aquancon
        for (size_t aquanconRecordIdx = 0; aquanconRecordIdx < aquanconKeyword.size(); ++ aquanconRecordIdx) 
            {
            const auto& aquanconRecord = aquanconKeyword.getRecord(aquanconRecordIdx);
            m_aquiferID_per_record.at(aquanconRecordIdx) = aquanconRecord.getItem("AQUIFER_ID").template get<int>(0);

            m_aqurecord.at(aquanconRecordIdx).i1 = aquanconRecord.getItem("I1").template get<int>(0);
            m_aqurecord.at(aquanconRecordIdx).i2 = aquanconRecord.getItem("I2").template get<int>(0);
            m_aqurecord.at(aquanconRecordIdx).j1 = aquanconRecord.getItem("J1").template get<int>(0);
            m_aqurecord.at(aquanconRecordIdx).j2 = aquanconRecord.getItem("J2").template get<int>(0);
            m_aqurecord.at(aquanconRecordIdx).k1 = aquanconRecord.getItem("K1").template get<int>(0);
            m_aqurecord.at(aquanconRecordIdx).k2 = aquanconRecord.getItem("K2").template get<int>(0);

            m_aquiferID_per_record.at(aquanconRecordIdx) = aquanconRecord.getItem("AQUIFER_ID").template get<int>(0);
            m_maxAquID = (m_maxAquID < m_aquiferID_per_record.at(aquanconRecordIdx) )?
                            m_aquiferID_per_record.at(aquanconRecordIdx) : m_maxAquID;
              
            double influx_coeff = aquanconRecord.getItem("INFLUX_COEFF").getSIDouble(0);

            double influx_mult = aquanconRecord.getItem("INFLUX_MULT").getSIDouble(0);

            FaceDir::DirEnum faceDir = FaceDir::FromString(aquanconRecord.getItem("FACE").getTrimmedString(0));
              
            // Loop over the cartesian indices to convert to the global grid index
            for (int k=m_aqurecord.at(aquanconRecordIdx).k1; k <= m_aqurecord.at(aquanconRecordIdx).k2; k++) {
                for (int j=m_aqurecord.at(aquanconRecordIdx).j1; j <= m_aqurecord.at(aquanconRecordIdx).j2; j++)
                    for (int i=m_aqurecord.at(aquanconRecordIdx).i1; i <= m_aqurecord.at(aquanconRecordIdx).i2; i++)
                        m_aqurecord.at(aquanconRecordIdx).global_index_per_record.push_back
                                                            (grid.getGlobalIndex(i-1 ,j-1 ,k- 1)
                                                            );
            }
            size_t global_index_per_record_size = m_aqurecord.at(aquanconRecordIdx).global_index_per_record.size();
            m_aqurecord.at(aquanconRecordIdx).influx_coeff_per_record.resize(global_index_per_record_size,influx_coeff);
            m_aqurecord.at(aquanconRecordIdx).influx_mult_per_record.resize(global_index_per_record_size,influx_mult);
            m_aqurecord.at(aquanconRecordIdx).face_per_record.resize(global_index_per_record_size,faceDir);
        }

        // Collate_function
        collate_function(m_aquoutput, m_aqurecord, m_aquiferID_per_record, m_maxAquID);

        // Logic for grid connection applied here
        logic_application(m_aquoutput);
    }
                                         
    
    // This function is used to convert from a per record vector to a per aquifer ID vector.
    void Aquancon::collate_function(std::vector<Aquancon::AquanconOutput>& output_vector, 
                                    std::vector<Opm::AquanconRecord>& m_aqurecord, 
                                    std::vector<int> m_aquiferID_per_record, int m_maxAquID)
    {
        output_vector.resize(m_maxAquID);
        // Find record indices at which the aquifer ids are located in
        for (int i = 1; i <= m_maxAquID; ++i)
        {
            std::vector<int> result_id;

            convert_record_id_to_aquifer_id(result_id, i, m_aquiferID_per_record);

            // We add the aquifer id into each element of output_vector
            output_vector.at(i - 1).aquiferID = i;
            for ( auto record_index_matching_id : result_id)
            {
                // This is for the global indices
                output_vector.at(i - 1).global_index.insert(
                                                             output_vector.at(i - 1).global_index.end(),
                                                             m_aqurecord.at(record_index_matching_id).global_index_per_record.begin(),
                                                             m_aqurecord.at(record_index_matching_id).global_index_per_record.end()
                                                           );
                // This is for the influx_coeff
                output_vector.at(i - 1).influx_coeff.insert(
                                                             output_vector.at(i - 1).influx_coeff.end(),
                                                             m_aqurecord.at(record_index_matching_id).influx_coeff_per_record.begin(),
                                                             m_aqurecord.at(record_index_matching_id).influx_coeff_per_record.end()
                                                           );
                // This is for the influx_multiplier
                output_vector.at(i - 1).influx_multiplier.insert(
                                                                  output_vector.at(i - 1).influx_multiplier.end(),
                                                                  m_aqurecord.at(record_index_matching_id).influx_mult_per_record.begin(),
                                                                  m_aqurecord.at(record_index_matching_id).influx_mult_per_record.end()
                                                                );
                // This is for the reservoir_face_dir
                output_vector.at(i - 1).reservoir_face_dir.insert(
                                                                   output_vector.at(i - 1).reservoir_face_dir.end(),
                                                                   m_aqurecord.at(record_index_matching_id).face_per_record.begin(),
                                                                   m_aqurecord.at(record_index_matching_id).face_per_record.end()
                                                                 );
            }
        }
    }

    void Aquancon::logic_application(std::vector<Aquancon::AquanconOutput>& output_vector)
    {
        // Find if Global index is repeated for each aquifer, if so, select only the first one
        for (auto aquconvec = output_vector.begin(); aquconvec != output_vector.end(); ++aquconvec)
        {
            std::sort(aquconvec->global_index.begin(), aquconvec->global_index.end());
            auto it = std::unique ( aquconvec->global_index.begin(), aquconvec->global_index.end() );
            aquconvec->global_index.resize( std::distance(aquconvec->global_index.begin(),it) );
        }

        //TODO: Find if face on outside of reservoir or adjoins an inactive cell 
        //TODO: Total number of grid blocks connected to aquifer must not exceed item 6 of AQUDIMS
    }

    void Aquancon::convert_record_id_to_aquifer_id(std::vector<int>& record_indices_matching_id,
                                                   int i, std::vector<int> m_aquiferID_per_record)
    {
        auto it = std::find_if( m_aquiferID_per_record.begin(), m_aquiferID_per_record.end(), 
                                    [&](int id) {
                                        return id == i;
                                    } 
                                );
        while (it != m_aquiferID_per_record.end()) {
            record_indices_matching_id.emplace_back(std::distance(m_aquiferID_per_record.begin(), it));
            it = std::find_if(std::next(it), m_aquiferID_per_record.end(), [&](int id){return id == i;});
        }
    }

    const std::vector<Aquancon::AquanconOutput>& Aquancon::getAquOutput() const
    {
        return m_aquoutput;
    }

}
