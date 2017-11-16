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
#include <algorithm>
#include <stdexcept>
#include <string>
#include <type_traits>
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
            Rates() = default;
            enum class opt : uint32_t {
                wat           = (1 << 0),
                oil           = (1 << 1),
                gas           = (1 << 2),
                polymer       = (1 << 3),
                solvent       = (1 << 4),
                dissolved_gas = (1 << 5),
                vaporized_oil = (1 << 6),
            };

            using enum_size = std::underlying_type< opt >::type;

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

            /// true if any option is set; false otherwise
            inline bool any() const noexcept;

        private:
            double& get_ref( opt );
            const double& get_ref( opt ) const;

            opt mask = static_cast< opt >( 0 );

            double wat = 0.0;
            double oil = 0.0;
            double gas = 0.0;
            double polymer = 0.0;
            double solvent = 0.0;
            double dissolved_gas = 0.0;
            double vaporized_oil = 0.0;
    };

    struct Completion {
        using active_index = size_t;
        static const constexpr int restart_size = 2;

        active_index index;
        Rates rates;
        double pressure;
        double reservoir_rate;
    };

    struct Well {
        Rates rates;
        double bhp;
        double thp;
        double temperature;
        int control;
        std::vector< Completion > completions;

        inline bool flowing() const noexcept;
    };


    class WellRates : public std::map<std::string , Well> {
    public:

        double get(const std::string& well_name , Rates::opt m) const {
            const auto& well = this->find( well_name );
            if( well == this->end() ) return 0.0;

            return well->second.rates.get( m, 0.0 );
        }


        double get(const std::string& well_name , Completion::active_index completion_grid_index, Rates::opt m) const {
            const auto& witr = this->find( well_name );
            if( witr == this->end() ) return 0.0;

            const auto& well = witr->second;
            const auto& completion = std::find_if( well.completions.begin() ,
                                                   well.completions.end() ,
                                                   [=]( const Completion& c ) {
                                                        return c.index == completion_grid_index; });

            if( completion == well.completions.end() )
                return 0.0;

            return completion->rates.get( m, 0.0 );
        }

    };

    using Wells = WellRates;

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
            case opt::dissolved_gas: return this->dissolved_gas;
            case opt::vaporized_oil: return this->vaporized_oil;
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

    inline bool Rates::any() const noexcept {
        return static_cast< enum_size >( this->mask ) != 0;
    }

    inline bool Well::flowing() const noexcept {
        return this->rates.any();
    }

    }
}

#endif //OPM_OUTPUT_WELLS_HPP
