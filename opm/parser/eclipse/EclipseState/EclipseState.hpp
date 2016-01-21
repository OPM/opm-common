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

#ifndef OPM_ECLIPSE_STATE_HPP
#define OPM_ECLIPSE_STATE_HPP

#include <opm/parser/eclipse/EclipseState/Grid/BoxManager.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/NNC.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/TransMult.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>

#include <set>

namespace Opm {

    template< typename > class GridProperty;
    template< typename > class GridProperties;

    class Deck;
    class DeckItem;
    class Fault;
    class FaultCollection;
    class InitConfig;
    class IOConfig;
    class ParseMode;
    class Schedule;
    class Section;
    class SimulationConfig;
    class TableManager;

    class EclipseState {
    public:
        enum EnabledTypes {
            IntProperties = 0x01,
            DoubleProperties = 0x02,

            AllProperties = IntProperties | DoubleProperties
        };

        EclipseState(std::shared_ptr< const Deck > deck , const ParseMode& parseMode);

        const ParseMode& getParseMode() const;
        std::shared_ptr< const Schedule > getSchedule() const;
        std::shared_ptr< const IOConfig > getIOConfigConst() const;
        std::shared_ptr< IOConfig > getIOConfig() const;
        std::shared_ptr< const InitConfig > getInitConfig() const;
        std::shared_ptr< const SimulationConfig > getSimulationConfig() const;
        std::shared_ptr< const EclipseGrid > getEclipseGrid() const;
        std::shared_ptr< EclipseGrid > getEclipseGridCopy() const;
        bool hasPhase(enum Phase::PhaseEnum phase) const;
        std::string getTitle() const;
        bool supportsGridProperty(const std::string& keyword, int enabledTypes=AllProperties) const;

        std::shared_ptr<GridProperty<int> > getRegion(std::shared_ptr< const DeckItem > regionItem) const;
        std::shared_ptr<GridProperty<int> > getDefaultRegion() const;
        std::shared_ptr<GridProperty<int> > getIntGridProperty( const std::string& keyword ) const;
        std::shared_ptr<GridProperty<double> > getDoubleGridProperty( const std::string& keyword ) const;
        bool hasIntGridProperty(const std::string& keyword) const;
        bool hasDoubleGridProperty(const std::string& keyword) const;

        void loadGridPropertyFromDeckKeyword(std::shared_ptr<const Box> inputBox,
                                             std::shared_ptr< const DeckKeyword > deckKeyword,
                                             int enabledTypes = AllProperties);

        std::shared_ptr<const FaultCollection> getFaults() const;
        std::shared_ptr<const TransMult> getTransMult() const;
        std::shared_ptr<const NNC> getNNC() const;
        bool hasNNC() const;

        std::shared_ptr<const TableManager> getTableManager() const;
        size_t getNumPhases() const;

        // the unit system used by the deck. note that it is rarely needed to convert
        // units because internally to opm-parser everything is represented by SI
        // units...
        std::shared_ptr<const UnitSystem> getDeckUnitSystem()  const;
        void applyModifierDeck( std::shared_ptr<const Deck> deck);

    private:
        void initTabdims(std::shared_ptr< const Deck > deck);
        void initTables(std::shared_ptr< const Deck > deck);
        void initIOConfig(std::shared_ptr< const Deck > deck);
        void initSchedule(std::shared_ptr< const Deck > deck);
        void initIOConfigPostSchedule(std::shared_ptr< const Deck > deck);
        void initInitConfig(std::shared_ptr< const Deck > deck);
        void initSimulationConfig(std::shared_ptr< const Deck > deck);
        void initEclipseGrid(std::shared_ptr< const Deck > deck);
        void initGridopts(std::shared_ptr< const Deck > deck);
        void initPhases(std::shared_ptr< const Deck > deck);
        void initTitle(std::shared_ptr< const Deck > deck);
        void initProperties(std::shared_ptr< const Deck > deck);
        void initTransMult();
        void initFaults(std::shared_ptr< const Deck > deck);
        void initNNC(std::shared_ptr< const Deck > deck);


        void setMULTFLT(std::shared_ptr<const Opm::Section> section) const;
        void initMULTREGT(std::shared_ptr< const Deck > deck);

        double getSIScaling(const std::string &dimensionString) const;

        void processGridProperties(std::shared_ptr< const Deck > deck, int enabledTypes);
        void scanSection(std::shared_ptr<Opm::Section> section , int enabledTypes);
        void handleADDKeyword(std::shared_ptr< const DeckKeyword > deckKeyword  , BoxManager& boxManager, int enabledTypes);
        void handleBOXKeyword(std::shared_ptr< const DeckKeyword > deckKeyword  , BoxManager& boxManager);
        void handleCOPYKeyword(std::shared_ptr< const DeckKeyword > deckKeyword , BoxManager& boxManager, int enabledTypes);
        void handleENDBOXKeyword(BoxManager& boxManager);
        void handleEQUALSKeyword(std::shared_ptr< const DeckKeyword > deckKeyword   , BoxManager& boxManager, int enabledTypes);
        void handleMULTIPLYKeyword(std::shared_ptr< const DeckKeyword > deckKeyword , BoxManager& boxManager, int enabledTypes);

        void handleEQUALREGKeyword(std::shared_ptr< const DeckKeyword > deckKeyword, int enabledTypes);
        void handleMULTIREGKeyword(std::shared_ptr< const DeckKeyword > deckKeyword, int enabledTypes);
        void handleADDREGKeyword(std::shared_ptr< const DeckKeyword > deckKeyword  , int enabledTypes);
        void handleCOPYREGKeyword(std::shared_ptr< const DeckKeyword > deckKeyword , int enabledTypes);

        void setKeywordBox(std::shared_ptr< const DeckKeyword > deckKeyword, size_t recordIdx, BoxManager& boxManager);

        void copyIntKeyword(const std::string& srcField , const std::string& targetField , std::shared_ptr<const Box> inputBox);
        void copyDoubleKeyword(const std::string& srcField , const std::string& targetField , std::shared_ptr<const Box> inputBox);

        void complainAboutAmbiguousKeyword(std::shared_ptr< const Deck > deck, const std::string& keywordName) const;

        std::shared_ptr< const EclipseGrid >      m_eclipseGrid;
        std::shared_ptr< IOConfig >              m_ioConfig;
        std::shared_ptr< const InitConfig >       m_initConfig;
        std::shared_ptr< const Schedule >         schedule;
        std::shared_ptr< const SimulationConfig > m_simulationConfig;

        std::shared_ptr<const TableManager> m_tables;

        std::set<enum Phase::PhaseEnum> phases;
        std::string m_title;
        std::shared_ptr<const UnitSystem> m_deckUnitSystem;
        std::shared_ptr<GridProperties<int> > m_intGridProperties;
        std::shared_ptr<GridProperties<double> > m_doubleGridProperties;
        std::shared_ptr<TransMult> m_transMult;
        std::shared_ptr<FaultCollection> m_faults;
        std::shared_ptr<NNC> m_nnc;
        std::string m_defaultRegion;
        const ParseMode& m_parseMode;
    };

    typedef std::shared_ptr<EclipseState> EclipseStatePtr;
    typedef std::shared_ptr<const EclipseState> EclipseStateConstPtr;
}

#endif
