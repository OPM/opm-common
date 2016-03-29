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

#include <utility>
#include <memory>
#include <set>

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/Parser/MessageContainer.hpp>

namespace Opm {

    template< typename > class GridProperty;
    template< typename > class GridProperties;

    class Box;
    class BoxManager;
    class Deck;
    class DeckItem;
    class DeckKeyword;
    class DeckRecord;
    class EclipseGrid;
    class Fault;
    class FaultCollection;
    class InitConfig;
    class IOConfig;
    class NNC;
    class ParseContext;
    class Schedule;
    class Section;
    class SimulationConfig;
    class TableManager;
    class TransMult;
    class UnitSystem;

    class EclipseState {
    public:
        enum EnabledTypes {
            IntProperties = 0x01,
            DoubleProperties = 0x02,

            AllProperties = IntProperties | DoubleProperties
        };

        EclipseState(std::shared_ptr< const Deck > deck , const ParseContext& parseContext);

        const ParseContext& getParseContext() const;
        std::shared_ptr< const Schedule > getSchedule() const;
        std::shared_ptr< const IOConfig > getIOConfigConst() const;
        std::shared_ptr< IOConfig > getIOConfig() const;
        std::shared_ptr< const InitConfig > getInitConfig() const;
        std::shared_ptr< const SimulationConfig > getSimulationConfig() const;
        std::shared_ptr< const EclipseGrid > getEclipseGrid() const;
        std::shared_ptr< EclipseGrid > getEclipseGridCopy() const;
        const MessageContainer& getMessageContainer() const;
        MessageContainer& getMessageContainer();
        std::string getTitle() const;

        std::shared_ptr<const FaultCollection> getFaults() const;
        std::shared_ptr<const TransMult> getTransMult() const;
        std::shared_ptr<const NNC> getNNC() const;
        bool hasNNC() const;

        const Eclipse3DProperties& getEclipseProperties() const;
        std::shared_ptr<const TableManager> getTableManager() const;

        // the unit system used by the deck. note that it is rarely needed to convert
        // units because internally to opm-parser everything is represented by SI
        // units...
        const UnitSystem& getDeckUnitSystem()  const;
        void applyModifierDeck( std::shared_ptr<const Deck> deck);

    private:
        void initTabdims(std::shared_ptr< const Deck > deck);
        void initIOConfig(std::shared_ptr< const Deck > deck);
        void initIOConfigPostSchedule(std::shared_ptr< const Deck > deck);
        void initTitle(std::shared_ptr< const Deck > deck);
        void initTransMult();
        void initFaults(std::shared_ptr< const Deck > deck);

        void setMULTFLT(std::shared_ptr<const Opm::Section> section) const;
        void initMULTREGT(std::shared_ptr< const Deck > deck);

        void complainAboutAmbiguousKeyword(std::shared_ptr< const Deck > deck, const std::string& keywordName) const;

        std::shared_ptr< IOConfig >               m_ioConfig;
        std::shared_ptr< const InitConfig >       m_initConfig;
        std::shared_ptr< const Schedule >         m_schedule;
        std::shared_ptr< const SimulationConfig > m_simulationConfig;

        std::set<enum Phase::PhaseEnum> phases;
        std::string m_title;
        const UnitSystem& m_deckUnitSystem;
        std::shared_ptr<TransMult> m_transMult;
        std::shared_ptr<FaultCollection> m_faults;
        std::shared_ptr<NNC> m_nnc;
        const ParseContext& m_parseContext;
        std::shared_ptr<const TableManager> m_tables;
        std::shared_ptr<const EclipseGrid> m_eclipseGrid;
        Eclipse3DProperties m_eclipseProperties;
        MessageContainer m_messageContainer;
    };

    typedef std::shared_ptr<EclipseState> EclipseStatePtr;
    typedef std::shared_ptr<const EclipseState> EclipseStateConstPtr;
}

#endif // OPM_ECLIPSE_STATE_HPP
