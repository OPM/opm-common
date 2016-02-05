/*
  Copyright 2016 Statoil ASA.

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


#include <opm/common/data/SimulationDataContainer.hpp>

namespace Opm {

    SimulationDataContainer::SimulationDataContainer(size_t num_cells, size_t num_faces , size_t num_phases) :
        m_num_cells( num_cells),
        m_num_faces( num_faces),
        m_num_phases( num_phases )
    {
        addDefaultFields( );
    }


    size_t SimulationDataContainer::numPhases() const {
        return m_num_phases;
    }


    size_t SimulationDataContainer::numFaces() const {
        return m_num_faces;
    }

    size_t SimulationDataContainer::numCells() const {
        return m_num_cells;
    }


    bool SimulationDataContainer::hasCellData( const std::string& name ) const {
        return ( m_cell_data.find( name ) == m_cell_data.end() ? false : true );
    }


    std::vector<double>& SimulationDataContainer::getCellData( const std::string& name ) {
        auto iter = m_cell_data.find( name );
        if (iter == m_cell_data.end()) {
            throw std::invalid_argument("The cell data with name: " + name + " does not exist");
        } else
            return iter->second;
    }


    const std::vector<double>& SimulationDataContainer::getCellData( const std::string& name ) const {
        auto iter = m_cell_data.find( name );
        if (iter == m_cell_data.end()) {
            throw std::invalid_argument("The cell data with name: " + name + " does not exist");
        } else
            return iter->second;
    }


    void SimulationDataContainer::registerCellData( const std::string& name , size_t components , double initialValue) {
        if (!hasCellData( name )) {
            m_cell_data.insert( std::pair<std::string , std::vector<double>>( name , std::vector<double>(components * m_num_cells , initialValue )));
        }
    }


    bool SimulationDataContainer::hasFaceData( const std::string& name ) const {
        return ( m_face_data.find( name ) == m_face_data.end() ? false : true );
    }


    std::vector<double>& SimulationDataContainer::getFaceData( const std::string& name ) {
        auto iter = m_face_data.find( name );
        if (iter == m_face_data.end()) {
            throw std::invalid_argument("The face data with name: " + name + " does not exist");
        } else
            return iter->second;
    }

    const std::vector<double>& SimulationDataContainer::getFaceData( const std::string& name ) const {
        auto iter = m_face_data.find( name );
        if (iter == m_face_data.end()) {
            throw std::invalid_argument("The Face data with name: " + name + " does not exist");
        } else
            return iter->second;
    }

    void SimulationDataContainer::registerFaceData( const std::string& name , size_t components , double initialValue) {
        if (!hasFaceData( name )) {
            m_face_data.insert( std::pair<std::string , std::vector<double>>( name , std::vector<double>(components * m_num_faces , initialValue )));
        }
    }


    /* This is very deprecated. */
    void SimulationDataContainer::addDefaultFields() {
        registerCellData("PRESSURE" , 1 , 0.0);
        registerCellData("SATURATION" , m_num_phases , 0.0);
        registerCellData("TEMPERATURE" , 1 , 273.15 + 20);

        registerFaceData("FACEPRESSURE" , 1 , 0.0 );
        registerFaceData("FACEFLUX" , 1 , 0.0 );
    }

    std::vector<double>& SimulationDataContainer::pressure( ) {
        return getCellData("PRESSURE");
    }

    std::vector<double>& SimulationDataContainer::temperature() {
        return getCellData("TEMPERATURE");
    }

    std::vector<double>& SimulationDataContainer::saturation() {
        return getCellData("SATURATION");
    }

    std::vector<double>& SimulationDataContainer::facepressure() {
        return getFaceData("FACEPRESSURE");
    }

    std::vector<double>& SimulationDataContainer::faceflux() {
        return getFaceData("FACEFLUX");
    }

}
