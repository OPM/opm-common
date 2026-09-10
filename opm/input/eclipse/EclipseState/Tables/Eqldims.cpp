/*
  Copyright (C) 2015 Statoil ASA

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

#include <opm/input/eclipse/EclipseState/Tables/Eqldims.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/E.hpp>

namespace Opm {

Eqldims::Eqldims() :
    m_ntequl( ParserKeywords::EQLDIMS::NTEQUL::defaultValue ),
    m_depth_nodes_p( ParserKeywords::EQLDIMS::DEPTH_NODES_P::defaultValue ),
    m_depth_nodes_tab( ParserKeywords::EQLDIMS::DEPTH_NODES_TAB::defaultValue ),
    m_nttrvd(  ParserKeywords::EQLDIMS::NTTRVD::defaultValue ),
    m_nstrvd(  ParserKeywords::EQLDIMS::NSTRVD::defaultValue )
{ }

Eqldims::Eqldims(const Deck& deck)
    : Eqldims()
{
    using EQLDIMS = ParserKeywords::EQLDIMS;

    if (!deck.hasKeyword<EQLDIMS>()) {
        return;
    }

    const auto& record = deck.get<EQLDIMS>().back().getRecord(0);

    this->m_ntequl          = record.getItem<EQLDIMS::NTEQUL>().get<int>(0);
    this->m_depth_nodes_p   = record.getItem<EQLDIMS::DEPTH_NODES_P>().get<int>(0);
    this->m_depth_nodes_tab = record.getItem<EQLDIMS::DEPTH_NODES_TAB>().get<int>(0);
    this->m_nttrvd          = record.getItem<EQLDIMS::NTTRVD>().get<int>(0);
    this->m_nstrvd          = record.getItem<EQLDIMS::NSTRVD>().get<int>(0);
}

}
