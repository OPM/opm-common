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

#ifndef COMMON_WELLRESULTS_HPP
#define COMMON_WELLRESULTS_HPP

#include <map>
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
            using enum_size = int16_t;

            enum class opt : enum_size {
                wat     = (1 << 0),
                oil     = (1 << 1),
                gas     = (1 << 2),
                polymer = (1 << 3),
            };

            /// Query if a set of values are set. Returns true if all values
            /// specified by the optmask are set, false if any are missing.
            inline bool has( opt );

            /// Read the value indicated by m. Throws an exception if multiple
            /// values are requested or if the value is unset.
            inline double get( opt m );
            /// Read the value indicated by m. Returns errval if multiple
            /// values are requested or the value is unset.
            inline double get( opt m, double errval );
            /// Set the value specified by m. Throws an exception if multiple
            /// values are requested
            inline Rates& set( opt m, double value );

        private:
            double& get_ref( opt, double& );

            opt mask = static_cast< opt >( 0 );

            double wat;
            double oil;
            double gas;
            double polymer;
    };

    struct Completion {
        size_t logical_cartesian_index;
        Rates rates;
    };

    struct Well {
        Rates rates;
        double bhp;
        std::map< size_t, Completion > completions;
    };

    struct Wells {
        size_t step_length;
        std::map< std::string, Well > wells;
    };

    /* IMPLEMENTATIONS */

    inline Rates::opt operator|( Rates::opt x, Rates::opt y ) {
        using sz = Rates::enum_size;
        return static_cast< Rates::opt >
            ( static_cast< sz >( x ) | static_cast< sz >( y ) );
    }

    inline Rates::opt operator&( Rates::opt x, Rates::opt y ) {
        using sz = Rates::enum_size;
        return static_cast< Rates::opt >
            ( static_cast< sz >( x ) & static_cast< sz >( y ) );
    }

    inline Rates::opt& operator|=( Rates::opt& x, Rates::opt y ) {
        return x = x | y;
    }

    inline bool Rates::has( opt m ) {
        return (this->mask & m) == m;
    }

    inline double Rates::get( opt m ) {
        double errval;
        auto ret = this->get_ref( m, errval );

        if( std::addressof( ret ) == std::addressof( errval ) )
            throw std::invalid_argument( "Invalid bitmask" );

        return ret;
    }

    inline double Rates::get( opt m, double errval ) {
        return this->get_ref( this->mask & m, errval );
    }

    inline Rates& Rates::set( opt m, double value ) {
        double errval;
        auto& x = this->get_ref( m, errval );

        if( std::addressof( errval ) == std::addressof( x ) )
            throw std::invalid_argument( "Invalid bitmask" );

        this->mask |= m;
        x = value;
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
    inline double& Rates::get_ref( opt m, double& errval ) {
        switch( m ) {
            case opt::wat: return this->wat;
            case opt::oil: return this->oil;
            case opt::gas: return this->gas;
            case opt::polymer: return this->polymer;
        }

        return errval;
    }

    }
}

#endif //COMMON_WELLRESULTS_HPP
