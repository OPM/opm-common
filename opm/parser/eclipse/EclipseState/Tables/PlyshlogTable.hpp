/*
  Copyright 2015 Statoil ASA.

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
#ifndef OPM_PARSER_PLYSHLOG_TABLE_HPP
#define	OPM_PARSER_PLYSHLOG_TABLE_HPP

#include <opm/parser/eclipse/Parser/ParserKeywords.hpp>
#include "SimpleTable.hpp"
#include <opm/parser/eclipse/EclipseState/Tables/TableEnums.hpp>


namespace Opm {
    // forward declaration
    class TableManager;

    class PlyshlogTable : public SimpleTable {
    public:
        friend class TableManager;

        PlyshlogTable(Opm::DeckRecordConstPtr indexRecord, Opm::DeckRecordConstPtr dataRecord) {
            {
                const auto item = indexRecord->getItem<ParserKeywords::PLYSHLOG::REF_POLYMER_CONCENTRATION>();
                setRefPolymerConcentration(item->getRawDouble(0));
            }

            {
                const auto item = indexRecord->getItem<ParserKeywords::PLYSHLOG::REF_SALINITY>();
                if (item->hasValue(0)) {
                    setHasRefSalinity(true);
                    setRefSalinity(item->getRawDouble(0));
                } else
                    setHasRefSalinity(false);
            }

            {
                const auto item = indexRecord->getItem<ParserKeywords::PLYSHLOG::REF_TEMPERATURE>();
                if (item->hasValue(0)) {
                    setHasRefTemperature(true);
                    setRefTemperature(item->getRawDouble(0));
                } else
                    setHasRefTemperature(false);
            }

            m_schema = std::make_shared<TableSchema>();
            m_schema->addColumn( ColumnSchema("WaterVelocity"   , Table::STRICTLY_INCREASING , Table::DEFAULT_NONE));
            m_schema->addColumn( ColumnSchema("ShearMultiplier" , Table::RANDOM , Table::DEFAULT_NONE));

            SimpleTable::init( dataRecord->getItem<ParserKeywords::PLYSHLOG::DATA>() );
        }

    public:

        double getRefPolymerConcentration() const {
            return m_refPolymerConcentration;
        }
        double getRefSalinity() const {
            return m_refSalinity;
        }

        double getRefTemperature() const{
            return m_refTemperature;
        }

        void setRefPolymerConcentration(const double refPlymerConcentration) {
            m_refPolymerConcentration = refPlymerConcentration;
        }

        void setRefSalinity(const double refSalinity) {
            m_refSalinity = refSalinity;
        }

        void setRefTemperature(const double refTemperature) {
            m_refTemperature = refTemperature;
        }

        bool hasRefSalinity() const {
            return m_hasRefSalinity;
        }

        bool hasRefTemperature() const {
            return m_hasRefTemperature;
        }

        void setHasRefSalinity(const bool has) {
            m_hasRefSalinity = has;
        }

        void setHasRefTemperature(const bool has) {
            m_refTemperature = has;
        }

        const TableColumn& getWaterVelocityColumn() const {
            return getColumn(0);
        }

        const TableColumn& getShearMultiplierColumn() const {
            return getColumn(1);
        }


    private:
        double m_refPolymerConcentration;
        double m_refSalinity;
        double m_refTemperature;

        bool m_hasRefSalinity;
        bool m_hasRefTemperature;
    };

}

#endif
