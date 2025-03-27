/*
  Copyright (C) 2025 NORCE Norwegian Research Centre AS.

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
#ifndef OPM_PARSER_BIOFILM_TABLE_HPP
#define OPM_PARSER_BIOFILM_TABLE_HPP

#include "SimpleTable.hpp"

namespace Opm {

class DeckItem;

class BiofilmTable : public SimpleTable
{
public:
    BiofilmTable( const DeckItem& item, const int tableID );

    const TableColumn& getDensityBiofilm() const;
    const TableColumn& getMicrobialDeathRate() const;
    const TableColumn& getMaximumGrowthRate() const;
    const TableColumn& getHalfVelocityOxygen() const;
    const TableColumn& getYieldGrowthCoefficient() const;
    const TableColumn& getOxygenConsumptionFactor() const;
    const TableColumn& getMicrobialAttachmentRate() const;
    const TableColumn& getDetachmentRate() const;
    const TableColumn& getDetachmentExponent() const;
    const TableColumn& getMaximumUreaUtilization() const;
    const TableColumn& getHalfVelocityUrea() const;
    const TableColumn& getDensityCalcite() const;
    const TableColumn& getYieldUreaToCalciteCoefficient() const;
};

} // end of namespace Opm

#endif //OPM_PARSER_BIOFILM_TABLE_HPP
