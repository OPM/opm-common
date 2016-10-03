/*
  Copyright 2016 Statoil ASA.

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

#ifndef OPM_OUTPUT_WELLS_HPP
#define OPM_OUTPUT_WELLS_HPP

#include <initializer_list>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace Opm {

    namespace data {

    class Rates {
        /* Methods are defined inline for performance, as the actual *work* done
         * is trivial, but somewhat frequent (typically once per time step per
         * completion per well).
         *
         * To add a new rate type, add an entry in the enum with the correct
         * shift, and if needed, increase the size type. Add a member variable
         * and a new case in get_ref.
         */

        public:
            using enum_size = uint16_t;

            enum class opt : enum_size {
                wat     = (1 << 0),
                oil     = (1 << 1),
                gas     = (1 << 2),
                polymer = (1 << 3),
                solvent = (1 << 4),
            };

            /// Query if a value is set.
            inline bool has( opt ) const;

            /// Read the value indicated by m. Throws an exception if
            /// if the requested value is unset.
            inline double get( opt m ) const;
            /// Read the value indicated by m. Returns a default value if
            /// the requested value is unset.
            inline double get( opt m, double default_value ) const;
            /// Set the value specified by m. Throws an exception if multiple
            /// values are requested. Returns a self-reference to support
            /// chaining.
            inline Rates& set( opt m, double value );

        private:
            double& get_ref( opt );
            const double& get_ref( opt ) const;

            opt mask = static_cast< opt >( 0 );

            double wat;
            double oil;
            double gas;
            double polymer;
            double solvent;
    };

    struct Completion {
        using active_index = size_t;
        active_index index;
        Rates rates;
    };

    struct Well {
        Rates rates;
        double bhp;
        double thp;
        std::map< Completion::active_index, Completion > completions;
    };

    struct Wells {
        using value_type = std::map< std::string, Well >::value_type;
        using iterator = std::map< std::string, Well >::iterator;

        inline Well& operator[]( const std::string& );
        inline Well& at( const std::string& );
        inline const Well& at( const std::string& ) const;
        template< typename... Args >
        inline std::pair< iterator, bool > emplace( Args&&... );

        inline Wells() = default;
        inline Wells( std::initializer_list< value_type > );
        inline Wells( std::initializer_list< value_type >,
                      std::vector< double > bhp,
                      std::vector< double > temperature,
                      std::vector< double > wellrates,
                      std::vector< double > perf_pressure,
                      std::vector< double > perf_rates );

        std::map< std::string, Well > wells;
        std::vector< double > bhp;
        std::vector< double > temperature;
        std::vector< double > well_rate;
        std::vector< double > perf_pressure;
        std::vector< double > perf_rate;
    };

    /* IMPLEMENTATIONS */

    inline bool Rates::has( opt m ) const {
        const auto mand = static_cast< enum_size >( this->mask )
                        & static_cast< enum_size >( m );

        return static_cast< opt >( mand ) == m;
    }

    inline double Rates::get( opt m ) const {
        if( !this->has( m ) )
            throw std::invalid_argument( "Uninitialized value." );

        return this->get_ref( m );
    }

    inline double Rates::get( opt m, double default_value ) const {
        if( !this->has( m ) ) return default_value;

        return this->get_ref( m );
    }

    inline Rates& Rates::set( opt m, double value ) {
        this->get_ref( m ) = value;
        /* mask |= m */
        this->mask = static_cast< opt >(
                        static_cast< enum_size >( this->mask ) |
                        static_cast< enum_size >( m )
                    );

        return *this;
    }

    /*
     * To avoid error-prone and repetitve work when extending rates with new
     * values, the get+set methods use this helper get_ref to determine what
     * member to manipulate. To add a new option, just add another case
     * corresponding to the enum entry in Rates to this function.
     *
     * This is an implementation detail and understanding this has no
     * significant impact on correct use of the class.
     */
    inline const double& Rates::get_ref( opt m ) const {
        switch( m ) {
            case opt::wat: return this->wat;
            case opt::oil: return this->oil;
            case opt::gas: return this->gas;
            case opt::polymer: return this->polymer;
            case opt::solvent: return this->solvent;
        }

        throw std::invalid_argument(
                "Unknown value type '"
                + std::to_string( static_cast< enum_size >( m ) )
                + "'" );

    }

    inline double& Rates::get_ref( opt m ) {
        return const_cast< double& >(
                static_cast< const Rates* >( this )->get_ref( m )
                );
    }

    inline Well& Wells::operator[]( const std::string& k ) {
        return this->wells[ k ];
    }

    inline Well& Wells::at( const std::string& k ) {
        return this->wells.at( k );
    }

    inline const Well& Wells::at( const std::string& k ) const {
        return this->wells.at( k );
    }

    template< typename... Args >
    inline std::pair< Wells::iterator, bool > Wells::emplace( Args&&... args ) {
        return this->wells.emplace( std::forward< Args >( args )... );
    }

    inline Wells::Wells( std::initializer_list< Wells::value_type > l ) :
        wells( l )
    {}

    inline Wells::Wells( std::initializer_list< value_type > l,
                         std::vector< double > b,
                         std::vector< double > t,
                         std::vector< double > w,
                         std::vector< double > pp,
                         std::vector< double > pr ) :
        wells( l ),
        bhp( b ),
        temperature( t ),
        well_rate( w ),
        perf_pressure( pp ),
        perf_rate( pr ) {
        // TODO: size asserts and sanity checks in debug mode
    }

    }
}

#endif //OPM_OUTPUT_WELLS_HPP
