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

#include <opm/io/eclipse/EclUtil.hpp>
#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/LODSmryOutput.hpp>
#include <opm/io/eclipse/EclOutput.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <iostream>
#include <iomanip>
#include <string>


namespace Opm { namespace EclIO {


LODSmryOutput::LODSmryOutput(const std::vector<std::string>& valueKeys, const std::vector<std::string>& valueUnits,
                 const EclipseState& es, const time_t start_time)
{
    m_nVect = valueKeys.size();
    m_nTimeSteps = 0;
    m_maxTimeSteps = 3;

    //m_maxTimeSteps = 50;


    IOConfig ioconf = es.getIOConfig();

    auto dims = es.gridDims();

    m_outputFileName = ioconf.getOutputDir() + "/" + ioconf.getBaseName() + ".LODSMRY";

    std::vector<std::string> mod_keys = this->make_modified_keys(valueKeys, dims);

    Opm::time_point startdat = Opm::TimeService::from_time_t(start_time);

    Opm::TimeStampUTC ts( std::chrono::system_clock::to_time_t( startdat ));

    std::vector<int> start_date_vect = {ts.day(), ts.month(), ts.year(),
        ts.hour(), ts.minutes(), ts.seconds(), 0 };

    Opm::EclIO::EclOutput outFile(m_outputFileName, false, std::ios::out);

    outFile.write<int>("START", start_date_vect);

    outFile.write("KEYCHECK", mod_keys, 24);
    outFile.write("UNITS", valueUnits);

    uint64_t rstep_pos = outFile.file_pos();

    std::vector<int> rstep(m_maxTimeSteps, -1);
    std::vector<float> zero_vect(m_maxTimeSteps, 0.0);

    outFile.write<int>("RSTEP", rstep);

    std::vector<uint64_t> vect_pos;

    for (size_t n = 0; n < static_cast<size_t>(m_nVect); n++ ) {
        std::string vect_name="V" + std::to_string(n);
        vect_pos.push_back(outFile.file_pos());
        outFile.write<float>(vect_name, zero_vect);
    }

    m_lodmsryPos = make_tuple(rstep_pos, vect_pos);
}

void LODSmryOutput::write(const std::vector<float>& ts_data, int report_step)
{
    std::cout << " Writing LODSMRY: ";

    auto lap0 = std::chrono::system_clock::now();

    if (ts_data.size() != static_cast<size_t>(m_nVect))
        throw std::invalid_argument("size of ts_data vector not same as number of smry vectors");

    if (m_nTimeSteps == m_maxTimeSteps) {
        m_maxTimeSteps = this->expand_and_rewrite(m_nTimeSteps*2);
    }

    int max_num_block = Opm::EclIO::MaxBlockSizeReal / Opm::EclIO::sizeOfReal;
    int n_blocks =  m_nTimeSteps  / max_num_block;

    //int rest = (m_nTimeSteps + 1) % 1000;

    uint64_t block_size_flags = Opm::EclIO::sizeOfInte * n_blocks * 2;
    auto rstep_pos = std::get<0>(m_lodmsryPos);
    auto vect_pos = std::get<1>(m_lodmsryPos);

    std::ofstream ofileH;

    ofileH.open(m_outputFileName, std::ios::binary|std::ios::out|std::ios::in);

    ofileH.seekp(rstep_pos + 28 + m_nTimeSteps*4 + block_size_flags);

    int rvalue = Opm::EclIO::flipEndianInt(report_step);
    ofileH.write(reinterpret_cast<char*>(&rvalue),4);

    for (size_t v = 0; v < ts_data.size(); v++){

        if (ts_data[v] != 0.0) {
            ofileH.seekp(vect_pos[v] + 28 + m_nTimeSteps*4 + block_size_flags);

            float value = Opm::EclIO::flipEndianFloat(ts_data[v]);
            ofileH.write(reinterpret_cast<char*>(&value), 4);
        }
    }

    ofileH.close();

    std::cout << " step " << m_nTimeSteps << "/" << m_maxTimeSteps;

    auto lap1 = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_wts = lap1-lap0;

    std::cout << "   + " << std::setw(8) << std::setprecision(5) << std::fixed << elapsed_wts.count();

    m_elapsed_writing = m_elapsed_writing + static_cast<double>(elapsed_wts.count());

    std::cout << " s,  total writing : " << m_elapsed_writing << " s \n";

    m_nTimeSteps++;
}


int LODSmryOutput::expand_and_rewrite(int new_size)
{
    Opm::EclIO::EclFile file1(m_outputFileName);

    file1.loadData();

    auto start_date_vect = file1.get<int>("START");
    auto keylist = file1.get<std::string>("KEYCHECK");
    auto unitlist = file1.get<std::string>("UNITS");
    auto rstep = file1.get<int>("RSTEP");

    size_t incSize = new_size - rstep.size();

    if (incSize > 100)
        incSize = 100;

    std::vector<int> inc_rstep(incSize, -1);
    std::vector<float> inc_values(incSize, 0.0);

    rstep.insert(rstep.end(), std::begin(inc_rstep), std::end(inc_rstep));

    std::vector<std::vector<float>> smryData;
    smryData.resize(m_nVect,{});

    for (size_t n=0; n < static_cast<size_t>(m_nVect); n++){
        std::string key = "V" + std::to_string(n);
        smryData[n] = file1.get<float>(key);
        smryData[n].insert(smryData[n].end(), std::begin(inc_values), std::end(inc_values));
    }

    bool isFormatted = false;

    uint64_t rstep_pos;
    std::vector<uint64_t> vect_pos;

    {
        Opm::EclIO::EclOutput newfile(m_outputFileName, isFormatted);

        newfile.write<int>("START", start_date_vect);
        newfile.write("KEYCHECK", keylist, 24);
        newfile.write("UNITS", unitlist);

        rstep_pos = newfile.file_pos();
        newfile.write("RSTEP", rstep);

        for (size_t n=0; n < smryData.size(); n++){
            std::string key = "V" + std::to_string(n);
            vect_pos.push_back(newfile.file_pos());
            newfile.write(key, smryData[n]);
        }
    }

    m_lodmsryPos = std::make_tuple(rstep_pos, vect_pos);


    return new_size;
}


std::vector<std::string> LODSmryOutput::make_modified_keys(const std::vector<std::string> valueKeys, const GridDims& dims)
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

std::array<int, 3> LODSmryOutput::ijk_from_global_index(const GridDims& dims, int globInd) const
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

/*
H5SmryOutput::~H5SmryOutput()
{
    H5Fclose(m_file_id);
}
*/

}} // namespace Opm::EclIO
