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

#include <deque>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>

namespace Opm {

    class Dimension;

    class DeckItem {
    public:
        DeckItem(const std::string& name , bool m_scalar = true);
        const std::string& name() const;

        // return true if the default value was used for a given data point
        bool defaultApplied(size_t index) const;

        // Return true if the item has a value for the current index;
        // does not differentiate between default values from the
        // config and values which have been set in the deck.
        bool hasValue(size_t index) const;

        // if the number returned by this method is less than what is semantically
        // expected (e.g. size() is less than the number of cells in the grid for
        // keywords like e.g. SGL), then the remaining values are defaulted. The deck
        // creates the defaulted items if all their sizes are fully specified by the
        // keyword, though...
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

        virtual const std::string& getString(size_t /* index */) const {
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
        void assertSize(size_t index) const;

        std::vector<bool> m_dataPointDefaulted;

    private:
        std::string m_name;
        bool m_scalar;
    };

    typedef std::shared_ptr< DeckItem > DeckItemPtr;
    typedef std::shared_ptr< const DeckItem > DeckItemConstPtr;

    template< typename T >
    class DeckTypeItem : public DeckItem {
        public:
            using DeckItem::DeckItem;

            void push_back( T value );
            // trying to access the data of a "dummy default item" will raise an exception
            void push_backDummyDefault();
            void push_backMultiple( T value, size_t numValues );
            void push_backDefault( T value);

            size_t size() const;

            const T& get( size_t ) const;
            const std::vector< T >& getData() const;

        private:
            std::vector< T > data;
    };

    template< typename T >
    class DeckSIItem : public DeckTypeItem< T > {
        public:
            using DeckTypeItem< T >::DeckTypeItem;

        const T& getSI( size_t ) const;
        const std::vector< T >& getSIData() const;

        void push_backDimension(
                std::shared_ptr< const Dimension > activeDimension,
                std::shared_ptr< const Dimension > defaultDimension ) override;

        private:
            void assertSIData() const;

            // mutable is required because the data is "lazily" converted
            // to SI units in asserSIData() which needs to be callable by
            // 'const'-decorated methods
            mutable std::vector< T > SIdata;
            std::vector< std::shared_ptr< const Dimension > > dimensions;
    };

}
#endif  /* DECKITEM_HPP */

