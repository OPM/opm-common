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


#ifndef WELL_HPP_
#define WELL_HPP_

#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>

#include <boost/shared_ptr.hpp>
#include <string>

namespace Opm {

    class Well {
    public:
        Well(const std::string& name, TimeMapConstPtr timeMap);
        const std::string& name() const;

        double getOilRate(size_t timeStep) const;
        void setOilRate(size_t timeStep, double oilRate);
        bool isInPredictionMode(size_t timeStep) const;
        void setInPredictionMode(size_t timeStep, bool isInPredictionMode);
        void addWELSPECS(DeckRecordConstPtr deckRecord);

    private:
        std::string m_name;
        boost::shared_ptr<DynamicState<double> > m_oilRate;
        boost::shared_ptr<DynamicState<bool> > m_inPredictionMode;

    };
    typedef boost::shared_ptr<Well> WellPtr;
    typedef boost::shared_ptr<const Well> WellConstPtr;
}



#endif /* WELL_HPP_ */
