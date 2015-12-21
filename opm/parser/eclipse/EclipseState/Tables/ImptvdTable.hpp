/*
  Copyright (C) 2014 by Andreas Lauser

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

#ifndef OPM_PARSER_IMPTVD_TABLE_HPP
#define	OPM_PARSER_IMPTVD_TABLE_HPP

#include "SimpleTable.hpp"
#include <opm/parser/eclipse/EclipseState/Tables/TableEnums.hpp>

namespace Opm {
    class ImptvdTable : public SimpleTable {
    public:

        ImptvdTable(Opm::DeckItemConstPtr item)
        {
            m_schema = std::make_shared<TableSchema>( );
            m_schema->addColumn( ColumnSchema( "DEPTH" ,  Table::STRICTLY_INCREASING , Table::DEFAULT_NONE ) );
            m_schema->addColumn( ColumnSchema( "SWCO",    Table::RANDOM , Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "SWCRIT",  Table::RANDOM , Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "SWMAX",   Table::RANDOM , Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "SGCO",    Table::RANDOM , Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "SGCRIT",  Table::RANDOM , Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "SGMAX",   Table::RANDOM , Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "SOWCRIT", Table::RANDOM , Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "SOGCRIT", Table::RANDOM , Table::DEFAULT_LINEAR ) );

            SimpleTable::init(item);
        }

        const TableColumn& getDepthColumn() const
        { return SimpleTable::getColumn(0); }

        /*!
         * \brief Connate water saturation
         */
        const TableColumn& getSwcoColumn() const
        { return SimpleTable::getColumn(1); }

        /*!
         * \brief Critical water saturation
         */
        const TableColumn& getSwcritColumn() const
        { return SimpleTable::getColumn(2); }

        /*!
         * \brief Maximum water saturation
         */
        const TableColumn& getSwmaxColumn() const
        { return SimpleTable::getColumn(3); }

        /*!
         * \brief Connate gas saturation
         */
        const TableColumn& getSgcoColumn() const
        { return SimpleTable::getColumn(4); }

        /*!
         * \brief Critical gas saturation
         */
        const TableColumn& getSgcritColumn() const
        { return SimpleTable::getColumn(5); }

        /*!
         * \brief Maximum gas saturation
         */
        const TableColumn& getSgmaxColumn() const
        { return SimpleTable::getColumn(6); }

        /*!
         * \brief Critical oil-in-water saturation
         */
        const TableColumn& getSowcritColumn() const
        { return SimpleTable::getColumn(7); }

        /*!
         * \brief Critical oil-in-gas saturation
         */
        const TableColumn& getSogcritColumn() const
        { return SimpleTable::getColumn(8); }
    };
}

#endif

