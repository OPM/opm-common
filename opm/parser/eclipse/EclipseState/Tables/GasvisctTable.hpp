/*
  Copyright (C) 2015 by Andreas Lauser

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
#ifndef OPM_PARSER_GASVISCT_TABLE_HPP
#define	OPM_PARSER_GASVISCT_TABLE_HPP

#include "SingleRecordTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class GasvisctTable : protected SingleRecordTable {
        typedef SingleRecordTable ParentType;

        friend class TableManager;
        GasvisctTable() = default;

        /*!
         * \brief Read the GASVISCT keyword and provide some convenience
         *        methods for it.
         */
        void init(const Deck& deck, Opm::DeckKeywordConstPtr keyword, int recordIdx)
        {
            int numComponents = deck.getKeyword("COMPS")->getRecord(0)->getItem(0)->getInt(0);

            auto temperatureDimension = deck.getActiveUnitSystem()->getDimension("Temperature");
            auto viscosityDimension = deck.getActiveUnitSystem()->getDimension("Viscosity");

            // create the columns: temperature plus one viscosity column per component
            std::vector<std::string> columnNames;
            columnNames.push_back("Temperature");

            for (int compIdx = 0; compIdx < numComponents; ++ compIdx)
                columnNames.push_back("Viscosity" + std::to_string(static_cast<long long>(compIdx)));

            ParentType::createColumns(columnNames);

            // extract the actual data from the deck
            Opm::DeckRecordConstPtr deckRecord =
                keyword->getRecord(recordIdx);

            size_t numFlatItems = getNumFlatItems(deckRecord);
            if ( numFlatItems % numColumns() != 0)
                throw std::runtime_error("Number of columns in the data file is inconsistent "
                                         "with the expected number for keyword GASVISCT");

            for (size_t rowIdx = 0; rowIdx*numColumns() < numFlatItems; ++rowIdx) {
                // add the current temperature
                int deckItemIdx = rowIdx*numColumns();

                bool isDefaulted = this->getFlatIsDefaulted(deckRecord, deckItemIdx);
                this->m_valueDefaulted[0].push_back(isDefaulted);

                if (!isDefaulted) {
                    double T = this->getFlatRawDoubleData(deckRecord, deckItemIdx);
                    this->m_columns[0].push_back(temperatureDimension->convertRawToSi(T));
                }

                // deal with the component viscosities
                for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                    deckItemIdx = rowIdx*numColumns() + compIdx + 1;
                    size_t columnIdx = compIdx + 1;

                    isDefaulted = this->getFlatIsDefaulted(deckRecord, deckItemIdx);
                    this->m_valueDefaulted[columnIdx].push_back(isDefaulted);

                    if (!isDefaulted) {
                        double mu = this->getFlatRawDoubleData(deckRecord, deckItemIdx);
                        this->m_columns[columnIdx].push_back(viscosityDimension->convertRawToSi(mu));
                    }
                }
            }

            // make sure that the columns agree with the keyword specification of the
            // reference manual. (actually, the documentation does not say anyting about
            // whether items of these columns are defaultable or not, so we assume here
            // that they are not.)
            ParentType::checkNonDefaultable("Temperature");
            ParentType::checkMonotonic("Temperature", /*isAscending=*/true);

            for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                std::string columnName = "Viscosity" + std::to_string(static_cast<long long>(compIdx));
                ParentType::checkNonDefaultable(columnName);
                ParentType::checkMonotonic(columnName,
                                           /*isAscending=*/true,
                                           /*strictlyMonotonic=*/false);
            }
        }

    public:
        using ParentType::numTables;
        using ParentType::numRows;
        using ParentType::numColumns;
        using ParentType::evaluate;

        const std::vector<double> &getTemperatureColumn() const
        { return ParentType::getColumn(0); }

        const std::vector<double> &getGasViscosityColumn(size_t compIdx) const
        { return ParentType::getColumn(1 + compIdx); }
    };
}

#endif
