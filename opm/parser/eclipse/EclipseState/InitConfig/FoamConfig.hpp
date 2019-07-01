#ifndef OPM_FOAMCONFIG_HPP
#define OPM_FOAMCONFIG_HPP

#include <vector>

namespace Opm {

    class DeckKeyword;
    class DeckRecord;

    class FoamRecord {
        public:
            explicit FoamRecord( const DeckRecord& );

            double referenceSurfactantConcentration() const;
            double exponent() const;
            double minimumSurfactantConcentration() const;

        private:
            double reference_surfactant_concentration_;
            double exponent_;
            double minimum_surfactant_concentration_;
    };

    class FoamConfig {
        public:
            using const_iterator = std::vector< FoamRecord >::const_iterator;

            FoamConfig() = default;
            explicit FoamConfig( const DeckKeyword& );

            const FoamRecord& getRecord( size_t id ) const;

            size_t size() const;
            bool empty() const;

            const_iterator begin() const;
            const_iterator end() const;

        private:
            std::vector< FoamRecord > records;
    };

}

#endif // OPM_FOAMCONFIG_HPP
