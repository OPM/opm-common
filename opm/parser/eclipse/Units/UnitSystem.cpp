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


#include <iostream>
#include <stdexcept>
#include <boost/algorithm/string.hpp>

#include <opm/parser/eclipse/Units/ConversionFactors.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <vector>


namespace Opm {

    UnitSystem::UnitSystem(const std::string& unitSystem) : 
        m_name( unitSystem )
    {
        
    }

    bool UnitSystem::hasDimension(const std::string& dimension) const {
        return (m_dimensions.find( dimension ) != m_dimensions.end());
    }


    std::shared_ptr<const Dimension> UnitSystem::getNewDimension(const std::string& dimension) {
        if (!hasDimension( dimension )) {
            std::shared_ptr<const Dimension> newDimension = parse( dimension );
            addDimension( newDimension );
        }
        return getDimension( dimension );
    }


    std::shared_ptr<const Dimension> UnitSystem::getDimension(const std::string& dimension) const {
        if (hasDimension( dimension )) 
            return m_dimensions.at( dimension );
        else
            throw std::invalid_argument("Dimension: " + dimension + " not recognized ");
    }

    
    void UnitSystem::addDimension(std::shared_ptr<const Dimension> dimension) {
        if (hasDimension(dimension->getName()))
            m_dimensions.erase( dimension->getName() );
        
        m_dimensions.insert( std::make_pair(dimension->getName() , dimension));
    }

    void UnitSystem::addDimension(const std::string& dimension , double SI_factor) {
        std::shared_ptr<const Dimension> dim( new Dimension(dimension , SI_factor) );
        addDimension(dim);
    }

    const std::string& UnitSystem::getName() const {
        return m_name;
    }


    std::shared_ptr<const Dimension> UnitSystem::parseFactor(const std::string& dimension) const {
        std::vector<std::string> dimensionList;
        boost::split(dimensionList , dimension , boost::is_any_of("*"));
        double SIfactor = 1.0;
        for (auto iter = dimensionList.begin(); iter != dimensionList.end(); ++iter) {
            std::shared_ptr<const Dimension> dim = getDimension( *iter );
            SIfactor *= dim->getSIScaling();
        }
        return std::shared_ptr<Dimension>(Dimension::newComposite( dimension , SIfactor ));
    }
    


    std::shared_ptr<const Dimension> UnitSystem::parse(const std::string& dimension) const {
        bool haveDivisor;
        {
            size_t divCount = std::count( dimension.begin() , dimension.end() , '/' );
            if (divCount == 0)
                haveDivisor = false;
            else if (divCount == 1)
                haveDivisor = true;
            else
                throw std::invalid_argument("Dimension string can only have one division sign /");
        }

        if (haveDivisor) {
            std::vector<std::string> parts;
            boost::split(parts , dimension , boost::is_any_of("/"));
            std::shared_ptr<const Dimension> dividend = parseFactor( parts[0] );
            std::shared_ptr<const Dimension> divisor = parseFactor( parts[1] );
        
            return std::shared_ptr<Dimension>( Dimension::newComposite( dimension , dividend->getSIScaling() / divisor->getSIScaling() ));
        } else {
            return parseFactor( dimension );
        }
    }


    bool UnitSystem::equal(const UnitSystem& other) const {
        bool equal = (m_dimensions.size() == other.m_dimensions.size());
        
        if (equal) {
            for (auto iter = m_dimensions.begin(); iter != m_dimensions.end(); ++iter) {
                std::shared_ptr<const Dimension> dim = getDimension( iter->first );

                if (other.hasDimension( iter->first )) {
                    std::shared_ptr<const Dimension> otherDim = other.getDimension( iter->first );
                    if (!dim->equal(*otherDim))
                        equal = false;
                } else
                    equal = false;
                
            }
        }
        return equal;
    }


    UnitSystem * UnitSystem::newMETRIC() {
        UnitSystem * system = new UnitSystem("Metric");
        
        system->addDimension("1"         , 1.0);
        system->addDimension("P"         , Metric::Pressure );
        system->addDimension("L"         , Metric::Length);
        system->addDimension("t"         , Metric::Time );
        system->addDimension("m"         , Metric::Mass );
        system->addDimension("K"         , Metric::Permeability );
        system->addDimension("Rs"        , Metric::DissolvedGasRaito);
        system->addDimension("FlowVolume", Metric::FlowVolume );
        system->addDimension("Rho"       , Metric::Density );
        system->addDimension("mu"        , Metric::Viscosity);
        return system;
    }


    
    UnitSystem * UnitSystem::newFIELD() {
        UnitSystem * system = new UnitSystem("Field");
        
        system->addDimension("1"    , 1.0);
        system->addDimension("P"    , Field::Pressure );
        system->addDimension("L"    , Field::Length);
        system->addDimension("t"    , Field::Time);
        system->addDimension("m"    , Field::Mass);
        system->addDimension("K"    , Field::Permeability );
        system->addDimension("Rs"   , Field::DissolvedGasRaito );
        system->addDimension("FlowVolume"    , Field::FlowVolume );
        system->addDimension("Rho"  , Field::Density );
        system->addDimension("mu"   , Field::Viscosity);
        return system;
    }

}



