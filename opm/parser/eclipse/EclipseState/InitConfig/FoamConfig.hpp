#ifndef OPM_FOAM_HPP
#define OPM_FOAM_HPP

#include <vector>

namespace Opm {

    class DeckKeyword;
    class DeckRecord;

    class FoamRecord {
        public:
            explicit FoamRecord( const DeckRecord& );

        private:
    };

    class Foam {
        public:
            using const_iterator = std::vector< FoamRecord >::const_iterator;

            Foam() = default;
            explicit Foam( const DeckKeyword& );

            const FoamRecord& getRecord( size_t id ) const;

            size_t size() const;
            bool empty() const;

            const_iterator begin() const;
            const_iterator end() const;

        private:
            std::vector< FoamRecord > records;
    };

}

#endif //OPM_FOAM_HPP
