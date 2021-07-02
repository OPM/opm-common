/*
   Copyright 2019 Statoil ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#include <opm/io/hdf5/H5SmryOutput.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <iostream>
#include <string>


namespace Opm { namespace Hdf5IO {


H5SmryOutput::H5SmryOutput(const std::vector<std::string>& valueKeys, const std::vector<std::string>& valueUnits,
                 const EclipseState& es, const time_t start_time)
{

    if (valueKeys.size() != valueUnits.size())
        throw std::invalid_argument("length of summary vectors names and units are different ");

    IOConfig ioconf = es.getIOConfig();

    auto dims = es.gridDims();

    std::string outputFileName = ioconf.getOutputDir() + "/" + ioconf.getBaseName() + ".H5SMRY";

    m_nTimeSteps = 0;
    m_maxTimeSteps = 20;

    m_nVect = valueKeys.size();

    std::vector<int> rstep_vect(m_maxTimeSteps, -1);

    std::array<int, 2> chunk_size {m_nVect, 1000};

    std::vector<std::vector<float>> smrydata;
    smrydata.reserve(m_nVect);

    for (size_t n = 0; n < static_cast<size_t>(m_nVect); n++){
        std::vector<float> smry_data(m_maxTimeSteps, 0.0);
        smrydata.push_back(smry_data);
    }


    Opm::time_point startdat = Opm::TimeService::from_time_t(start_time);

    Opm::TimeStampUTC ts( std::chrono::system_clock::to_time_t( startdat ));

    std::vector<int> start_date_vect = {ts.day(), ts.month(), ts.year(),
        ts.hour(), ts.minutes(), ts.seconds(), 0 };

    std::vector<std::string> mod_keys = this->make_modified_keys(valueKeys, dims);

    hid_t fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);

    m_file_id = H5Fcreate(outputFileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);

    Opm::Hdf5IO::write_1d_hdf5<int>(m_file_id, "RSTEP",  rstep_vect, true);

    Opm::Hdf5IO::write_1d_hdf5(m_file_id, "START_DATE", start_date_vect );

    Opm::Hdf5IO::write_1d_hdf5(m_file_id, "KEYS", mod_keys );

    Opm::Hdf5IO::write_1d_hdf5(m_file_id, "UNITS", valueUnits );

    Opm::Hdf5IO::write_2d_hdf5<float>(m_file_id, "SMRYDATA", smrydata, true, chunk_size);

    herr_t testswmr = H5Fstart_swmr_write(m_file_id);

    if (testswmr < 0)
        throw std::runtime_error("H5SMRY, Failed with setting HDF5 SWMR");
}


void H5SmryOutput::write(const std::vector<float>& ts_data, int report_step)
{

    if (ts_data.size() != static_cast<size_t>(m_nVect))
        throw std::runtime_error("invalid time step vector in H5Smry");

    if (m_nTimeSteps == m_maxTimeSteps){

        m_maxTimeSteps = Opm::Hdf5IO::expand_1d_dset_swmr(m_file_id, "RSTEP", 2, -1);
        int check = Opm::Hdf5IO::expand_2d_dset_swmr(m_file_id, "SMRYDATA", 2, -9.999);

        if (check !=m_maxTimeSteps)
            throw std::runtime_error("invalid update, different size for RSTEP and SMRYDATA");
    }

    Opm::Hdf5IO::set_value_for_1d_hdf5(m_file_id, "RSTEP", m_nTimeSteps, report_step);
    Opm::Hdf5IO::set_value_for_2d_hdf5(m_file_id, "SMRYDATA", m_nTimeSteps, ts_data);

    m_nTimeSteps++;
}


std::vector<std::string> H5SmryOutput::make_modified_keys(const std::vector<std::string> valueKeys, const GridDims& dims)
{
    std::vector<std::string> mod_keys;
    mod_keys.reserve(valueKeys.size());

    for (size_t n=0; n < valueKeys.size(); n++){

        if (valueKeys[n].substr(0,15) == "SMSPEC.Internal"){
            std::string mod_key = valueKeys[n].substr(16);
            int p = mod_key.find_first_of(".");
            mod_key=mod_key.substr(0,p);
            mod_keys.push_back(mod_key);

        } else if (valueKeys[n].substr(0,1) == "C"){
            size_t p = valueKeys[n].find_first_of(":");
            p = valueKeys[n].find_first_of(":", p + 1);

            int num = std::stod(valueKeys[n].substr(p + 1)) - 1;

            auto ijk = ijk_from_global_index(dims, num);

            std::string mod_key = valueKeys[n].substr(0, p + 1);
            mod_key = mod_key + std::to_string(ijk[0] + 1) + "," + std::to_string(ijk[1] + 1) + "," + std::to_string(ijk[2] + 1);

            mod_keys.push_back(mod_key);

        } else if (valueKeys[n].substr(0,1) == "B"){

            size_t p = valueKeys[n].find_first_of(":");

            int num = std::stod(valueKeys[n].substr(p + 1)) - 1;

            auto ijk = ijk_from_global_index(dims, num);

            std::string mod_key = valueKeys[n].substr(0, p + 1);
            mod_key = mod_key + std::to_string(ijk[0] + 1) + "," + std::to_string(ijk[1] + 1) + "," + std::to_string(ijk[2] + 1);

            mod_keys.push_back(mod_key);

        } else {
            mod_keys.push_back(valueKeys[n]);
        }
    }

    return mod_keys;

}

std::array<int, 3> H5SmryOutput::ijk_from_global_index(const GridDims& dims, int globInd) const
{

    if (globInd < 0 || static_cast<size_t>(globInd) >= dims[0] * dims[1] * dims[2])
        throw std::invalid_argument("global index out of range");

    std::array<int, 3> result;
    result[2] = globInd / (dims[0] * dims[1]);

    int rest = globInd % (dims[0] * dims[1]);

    result[1] = rest / dims[0];
    result[0] = rest % dims[0];

    return result;
}

H5SmryOutput::~H5SmryOutput()
{
    H5Fclose(m_file_id);
}

}} // namespace Opm::Hdf5IO
