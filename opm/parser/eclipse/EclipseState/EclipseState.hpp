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

#ifndef ECLIPSESTATE_H
#define ECLIPSESTATE_H

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/BoxManager.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/TransMult.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaultCollection.hpp>

#include <set>
#include <memory>

namespace Opm {
    
    class EclipseState {
    public:
        EclipseState(DeckConstPtr deck, bool beStrict = false);
        ScheduleConstPtr getSchedule() const;
        EclipseGridConstPtr getEclipseGrid() const;
        EclipseGridPtr getEclipseGridCopy() const;
        bool hasPhase(enum Phase::PhaseEnum phase) const;
        std::string getTitle() const;
        bool supportsGridProperty(const std::string& keyword) const;

        std::shared_ptr<GridProperty<int> > getIntGridProperty( const std::string& keyword ) const;
        std::shared_ptr<GridProperty<double> > getDoubleGridProperty( const std::string& keyword ) const;
        bool hasIntGridProperty(const std::string& keyword) const;
        bool hasDoubleGridProperty(const std::string& keyword) const;

        void loadGridPropertyFromDeckKeyword(std::shared_ptr<const Box> inputBox , DeckKeywordConstPtr deckKeyword);

        std::shared_ptr<const FaultCollection> getFaults() const;
        std::shared_ptr<const TransMult> getTransMult() const;
    private:
        void initSchedule(DeckConstPtr deck);
        void initEclipseGrid(DeckConstPtr deck);
        void initPhases(DeckConstPtr deck);
        void initTitle(DeckConstPtr deck);
        void initProperties(DeckConstPtr deck);
        void initTransMult();
        void initFaults(DeckConstPtr deck);
        void setMULTFLT(std::shared_ptr<const Section> section) const;

        double getSIScaling(const std::string &dimensionString) const;

        void scanSection(std::shared_ptr<Opm::Section> section , BoxManager& boxManager);
        void handleADDKeyword(DeckKeywordConstPtr deckKeyword , BoxManager& boxManager);
        void handleBOXKeyword(DeckKeywordConstPtr deckKeyword , BoxManager& boxManager);
        void handleCOPYKeyword(DeckKeywordConstPtr deckKeyword , BoxManager& boxManager);
        void handleENDBOXKeyword(BoxManager& boxManager);
        void handleEQUALSKeyword(DeckKeywordConstPtr deckKeyword , BoxManager& boxManager);
        void handleMULTIPLYKeyword(DeckKeywordConstPtr deckKeyword , BoxManager& boxManager);
        
        void setKeywordBox(DeckRecordConstPtr deckRecord , BoxManager& boxManager);

        void copyIntKeyword(const std::string& srcField , const std::string& targetField , std::shared_ptr<const Box> inputBox);
        void copyDoubleKeyword(const std::string& srcField , const std::string& targetField , std::shared_ptr<const Box> inputBox);

        EclipseGridConstPtr m_eclipseGrid;
        ScheduleConstPtr schedule;
        std::set<enum Phase::PhaseEnum> phases;
        std::string m_title;
        std::shared_ptr<UnitSystem> m_unitSystem;
        std::shared_ptr<GridProperties<int> > m_intGridProperties;
        std::shared_ptr<GridProperties<double> > m_doubleGridProperties;
        std::shared_ptr<TransMult> m_transMult;
        std::shared_ptr<FaultCollection> m_faults;
    };

    typedef std::shared_ptr<EclipseState> EclipseStatePtr;
    typedef std::shared_ptr<const EclipseState> EclipseStateConstPtr;
}

#endif
