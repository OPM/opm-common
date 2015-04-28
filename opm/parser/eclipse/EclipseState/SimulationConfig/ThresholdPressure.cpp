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

#include <opm/parser/eclipse/EclipseState/SimulationConfig/ThresholdPressure.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords.hpp>

namespace Opm {

    ThresholdPressure::ThresholdPressure(DeckConstPtr deck, std::shared_ptr<GridProperties<int>> gridProperties) {

        if (Section::hasRUNSPEC(deck) && Section::hasSOLUTION(deck)) {
            std::shared_ptr<const RUNSPECSection> runspecSection = std::make_shared<const RUNSPECSection>(deck);
            std::shared_ptr<const SOLUTIONSection> solutionSection = std::make_shared<const SOLUTIONSection>(deck);
            initThresholdPressure(runspecSection, solutionSection, gridProperties);
        }
    }


    void ThresholdPressure::initThresholdPressure(std::shared_ptr<const RUNSPECSection> runspecSection,
                                                  std::shared_ptr<const SOLUTIONSection> solutionSection,
                                                  std::shared_ptr<GridProperties<int>> gridProperties) {

        bool       thpresOption     = false;
        const bool thpresKeyword    = solutionSection->hasKeyword<ParserKeywords::THPRES>( );
        const bool hasEqlnumKeyword = gridProperties->hasKeyword<ParserKeywords::EQLNUM>( );
        int        maxEqlnum        = 0;


        //Is THPRES option set?
        if (runspecSection->hasKeyword<ParserKeywords::EQLOPTS>( )) {
            auto eqlopts = runspecSection->getKeyword<ParserKeywords::EQLOPTS>( );
            auto rec = eqlopts->getRecord(0);
            for (size_t i = 0; i < rec->size(); ++i) {
                auto item = rec->getItem(i);
                if (item->hasValue(0)) {
                    if (item->getString(0) == "THPRES") {
                        thpresOption = true;
                    } else if (item->getString(0) == "IRREVERS") {
                        throw std::runtime_error("Cannot use IRREVERS version of THPRES option, not implemented");
                    }
                }
            }
        }


        //Option is set and keyword is found
        if (thpresOption && thpresKeyword)
        {
            //Find max of eqlnum
            if (hasEqlnumKeyword) {
                auto eqlnumKeyword = gridProperties->getKeyword<ParserKeywords::EQLNUM>( );
                auto eqlnum = eqlnumKeyword->getData();
                maxEqlnum = *std::max_element(eqlnum.begin(), eqlnum.end());

                if (0 == maxEqlnum) {
                    throw std::runtime_error("Error in EQLNUM data: all values are 0");
                }
            } else {
                throw std::runtime_error("Error when internalizing THPRES: EQLNUM keyword not found in deck");
            }


            // Fill threshold pressure table.
            auto thpres = solutionSection->getKeyword<ParserKeywords::THPRES>( );

            m_thresholdPressureTable.resize(maxEqlnum * maxEqlnum, 0.0);

            const int numRecords = thpres->size();
            for (int rec_ix = 0; rec_ix < numRecords; ++rec_ix) {
                auto rec = thpres->getRecord(rec_ix);
                auto region1Item = rec->getItem<ParserKeywords::THPRES::REGION1>();
                auto region2Item = rec->getItem<ParserKeywords::THPRES::REGION2>();
                auto thpressItem = rec->getItem<ParserKeywords::THPRES::VALUE>();

                if (region1Item->hasValue(0) && region2Item->hasValue(0) && thpressItem->hasValue(0)) {
                    const int r1 = region1Item->getInt(0) - 1;
                    const int r2 = region2Item->getInt(0) - 1;
                    const double p = thpressItem->getSIDouble(0);

                    if (r1 >= maxEqlnum || r2 >= maxEqlnum) {
                        throw std::runtime_error("Too high region numbers in THPRES keyword");
                    }
                    m_thresholdPressureTable[r1 + maxEqlnum*r2] = p;
                    m_thresholdPressureTable[r2 + maxEqlnum*r1] = p;
                } else {
                    throw std::runtime_error("Missing data for use of the THPRES keyword");
                }
            }
        } else if (thpresOption && !thpresKeyword) {
            throw std::runtime_error("Invalid solution section; the EQLOPTS THPRES option is set in RUNSPEC, but no THPRES keyword is found in SOLUTION");

        }
    }



    const std::vector<double>& ThresholdPressure::getThresholdPressureTable() const {
        return m_thresholdPressureTable;
    }


} //namespace Opm





