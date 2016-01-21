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
#ifndef OPM_PARSER_EQUIL_WRAPPER_HPP
#define	OPM_PARSER_EQUIL_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

namespace Opm {
    class EquilWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data of the 'EQUIL' keyword.
         */
        EquilWrapper(std::shared_ptr< const DeckKeyword > keyword)
            : m_keyword(keyword)
        {
        }

        /*!
         * \brief Return the number of regions used for initialization
         */
        int numRegions() const
        { return m_keyword->size(); }

        /*!
         * \brief The reference depth
         */
        double datumDepth(int regionIdx) const
        { return m_keyword->getRecord(regionIdx)->getItem(0)->getSIDouble(0); }

        /*!
         * \brief The pressure at reference depth
         */
        double datumDepthPressure(int regionIdx) const
        { return m_keyword->getRecord(regionIdx)->getItem(1)->getSIDouble(0); }

        /*!
         * \brief The depth of the water-oil contact
         */
        double waterOilContactDepth(int regionIdx) const
        { return m_keyword->getRecord(regionIdx)->getItem(2)->getSIDouble(0); }

        /*!
         * \brief The capillary pressure at the water-oil contact
         */
        double waterOilContactCapillaryPressure(int regionIdx) const
        { return m_keyword->getRecord(regionIdx)->getItem(3)->getSIDouble(0); }

        /*!
         * \brief The depth of the gas-oil contact
         */
        double gasOilContactDepth(int regionIdx) const
        { return m_keyword->getRecord(regionIdx)->getItem(4)->getSIDouble(0); }

        /*!
         * \brief The capillary pressure at the gas-oil contact
         */
        double gasOilContactCapillaryPressure(int regionIdx) const
        { return m_keyword->getRecord(regionIdx)->getItem(5)->getSIDouble(0); }

        /*!
         * \brief Integer number specifying the initialization proceedure for live oil
         *
         * Much fun with eclipse...
         */
        int liveOilInitProceedure(int regionIdx) const
        { return m_keyword->getRecord(regionIdx)->getItem(6)->getInt(0); }

        /*!
         * \brief Integer number specifying the initialization proceedure for wet gas
         */
        int wetGasInitProceedure(int regionIdx) const
        { return m_keyword->getRecord(regionIdx)->getItem(7)->getInt(0); }

        /*!
         * \brief Integer number specifying the desired accuracy of the initialization
         */
        int initializationTargetAccuracy(int regionIdx) const
        { return m_keyword->getRecord(regionIdx)->getItem(8)->getInt(0); }

        /*!
         * \brief Integer number specifying the type of the initialization
         *
         * This is only relevant for fully-compositional models
         */
        int compositionalInitializationProceedure(int regionIdx) const
        { return m_keyword->getRecord(regionIdx)->getItem(9)->getInt(0); }

        /*!
         * \brief Use the saturation pressure at the of the gas for
         *        the gas-oil contact.
         *
         * This is only relevant for fully-compositional models and if
         * the initialization proceedure is either '2' or '3'.
         */
        bool useSaturationPressure(int regionIdx) const
        { return m_keyword->getRecord(regionIdx)->getItem(10)->getInt(0) == 0; }

    private:
        std::shared_ptr< const DeckKeyword > m_keyword;
    };
}

#endif	// OPM_PARSER_EQUIL_WRAPPER_HPP

