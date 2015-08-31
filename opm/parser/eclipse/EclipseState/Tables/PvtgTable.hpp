/*
  Copyright (C) 2013 by Andreas Lauser

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
#ifndef OPM_PARSER_PVTG_TABLE_HPP
#define	OPM_PARSER_PVTG_TABLE_HPP

#include "FullTable.hpp"
#include "PvtgInnerTable.hpp"
#include "PvtgOuterTable.hpp"

namespace Opm {
    // forward declaration
    class Tables;

    /*!
     * \brief Read the table for the PVTG and provide convenient access to it.
     */
    class PvtgTable : public Opm::FullTable<Opm::PvtgOuterTable, Opm::PvtgInnerTable>
    {
        typedef Opm::FullTable<Opm::PvtgOuterTable, Opm::PvtgInnerTable> ParentType;

        friend class Tables;

        using ParentType::init;

    public:
        PvtgTable() = default;

#ifdef BOOST_TEST_MODULE
        // DO NOT TRY TO CALL THIS METHOD! it is only for the unit tests!
        void initFORUNITTESTONLY(Opm::DeckKeywordConstPtr keyword, size_t tableIdx)
        { init(keyword, tableIdx); }
#endif
    };

    typedef std::shared_ptr<PvtgTable> PvtgTablePtr;
    typedef std::shared_ptr<const PvtgTable> PvtgConstTablePtr;
}

#endif
