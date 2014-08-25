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

#ifndef DECKITEM_HPP
#define DECKITEM_HPP

#include <opm/parser/eclipse/Units/Dimension.hpp>

#include <stdexcept>
#include <string>
#include <vector>
#include <memory>

namespace Opm {

    namespace DeckValue {
        enum StatusEnum {
            // These values are used as a bitmask.
            NOT_SET = 0 ,
            SET_IN_DECK = 1,  
            DEFAULT = 2
        };
    }


    class DeckItem {
    public:
        DeckItem(const std::string& name , bool m_scalar = true);
        const std::string& name() const;
        
        bool setInDeck() const;
        bool defaultApplied() const;
        bool hasData() const;

        virtual size_t size() const = 0;
        
        virtual int getInt(size_t /* index */) const {
            throw std::logic_error("This implementation of DeckItem does not support int");
        }

        virtual float getSIFloat(size_t /* index */) const {
            throw std::logic_error("This implementation of DeckItem does not support float");
        }

        virtual float getRawFloat(size_t /* index */) const {
            throw std::logic_error("This implementation of DeckItem does not support float");
        }

        virtual double getSIDouble(size_t /* index */) const {
            throw std::logic_error("This implementation of DeckItem does not support double");
        }

        virtual double getRawDouble(size_t /* index */) const {
            throw std::logic_error("This implementation of DeckItem does not support double");
        }

        virtual bool getBool(size_t /* index */) const {
            throw std::logic_error("This implementation of DeckItem does not support bool");
        }

        virtual std::string getString(size_t /* index */) const {
            throw std::logic_error("This implementation of DeckItem does not support string");
        }

        virtual std::string getTrimmedString(size_t /* index */) const  {
            throw std::logic_error("This implementation of DeckItem does not support trimmed strings");
        }

        virtual const std::vector<int>& getIntData( ) const {
            throw std::logic_error("This implementation of DeckItem does not support int");
        }

        virtual const std::vector<double>& getSIDoubleData() const {
            throw std::logic_error("This implementation of DeckItem does not support double");
        }

        virtual const std::vector<double>& getRawDoubleData() const {
            throw std::logic_error("This implementation of DeckItem does not support double");
        }

        virtual const std::vector<float>& getSIFloatData() const {
            throw std::logic_error("This implementation of DeckItem does not support float");
        }

        virtual const std::vector<float>& getRawFloatData() const {
            throw std::logic_error("This implementation of DeckItem does not support float");
        }


        virtual const std::vector<std::string>& getStringData() const {
            throw std::logic_error("This implementation of DeckItem does not support string");
        }

        virtual void push_backDimension(std::shared_ptr<const Dimension> /* activeDimension */,
                                        std::shared_ptr<const Dimension> /* defaultDimension */) {
            throw std::invalid_argument("Should not be here - internal error ...");
        }

        virtual ~DeckItem() {
        }

    protected:
        void assertValueSet() const;
        int m_valueStatus;

    private:
        std::string m_name;
        bool m_scalar;
    };

    typedef std::shared_ptr<DeckItem> DeckItemPtr;
    typedef std::shared_ptr<const DeckItem> DeckItemConstPtr;
}
#endif  /* DECKITEM_HPP */

