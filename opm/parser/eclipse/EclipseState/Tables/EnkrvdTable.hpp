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
#ifndef OPM_PARSER_ENKRVD_TABLE_HPP
#define	OPM_PARSER_ENKRVD_TABLE_HPP

#include "SimpleTable.hpp"
#include <opm/parser/eclipse/EclipseState/Tables/TableEnums.hpp>



namespace Opm {
    class EnkrvdTable : public SimpleTable {
    public:

        EnkrvdTable(Opm::DeckItemConstPtr item)
        {
            m_schema = std::make_shared<TableSchema>( );

            m_schema->addColumn( ColumnSchema( "DEPTH" ,  Table::STRICTLY_INCREASING , Table::DEFAULT_NONE ) );
            m_schema->addColumn( ColumnSchema( "KRWMAX",  Table::RANDOM , Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "KRGMAX",  Table::RANDOM , Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "KROMAX",  Table::RANDOM , Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "KRWCRIT", Table::RANDOM , Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "KRGCRIT", Table::RANDOM , Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "KROCRITG",Table::RANDOM , Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "KROCRITW",Table::RANDOM , Table::DEFAULT_LINEAR ) );

            SimpleTable::init(item);
        }

        // using this method is strongly discouraged but the current endpoint scaling
        // code makes it hard to avoid
        using SimpleTable::getColumn;

        /*!
         * \brief The datum depth for the remaining columns
         */
        const TableColumn& getDepthColumn() const
        { return SimpleTable::getColumn(0); }

        /*!
         * \brief Maximum relative permeability of water
         */
        const TableColumn& getKrwmaxColumn() const
        { return SimpleTable::getColumn(1); }

        /*!
         * \brief Maximum relative permeability of gas
         */
        const TableColumn& getKrgmaxColumn() const
        { return SimpleTable::getColumn(2); }

        /*!
         * \brief Maximum relative permeability of oil
         */
        const TableColumn& getKromaxColumn() const
        { return SimpleTable::getColumn(3); }

        /*!
         * \brief Relative permeability of water at the critical oil (or gas) saturation
         */
        const TableColumn& getKrwcritColumn() const
        { return SimpleTable::getColumn(4); }

        /*!
         * \brief Relative permeability of gas at the critical oil (or water) saturation
         */
        const TableColumn& getKrgcritColumn() const
        { return SimpleTable::getColumn(5); }

        /*!
         * \brief Oil relative permeability of oil at the critical gas saturation
         */
        const TableColumn& getKrocritgColumn() const
        { return SimpleTable::getColumn(6); }

        /*!
         * \brief Oil relative permeability of oil at the critical water saturation
         */
        const TableColumn& getKrocritwColumn() const
        { return SimpleTable::getColumn(7); }
    };
}

#endif

