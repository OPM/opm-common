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


#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Summary/Summary.hpp>
#include <opm/parser/eclipse/Utility/Functional.hpp>

#include <ert/ecl/ecl_smspec.h>

#include <numeric>
#include <array>

namespace Opm {

    static std::string wellName( const std::shared_ptr< const Well >& well ) {
        return well->name();
    }

    static std::string groupName( const Group* group ) {
        return group->name();
    }

    static inline std::vector< ERT::smspec_node > keywordWG(
            ecl_smspec_var_type var_type,
            const DeckKeyword& keyword,
            const EclipseState& es ) {

        const auto mknode = [&keyword,var_type]( const std::string& name ) {
            return ERT::smspec_node( var_type, name, keyword.name() );
        };

        const auto find = []( ecl_smspec_var_type v, const EclipseState& est ) {
            if( v == ECL_SMSPEC_WELL_VAR )
                return fun::map( wellName, est.getSchedule()->getWells() );
            else
                return fun::map( groupName, est.getSchedule()->getGroups() );
        };

        const auto& item = keyword.getDataRecord().getDataItem();
        const auto wgnames = item.size() > 0 && item.hasValue( 0 )
            ? item.getData< std::string >()
            : find( var_type, es );

        return fun::map( mknode, wgnames );
    }

    static inline std::vector< ERT::smspec_node > keywordF(
            const DeckKeyword& keyword,
            const EclipseState& /* es */ ) {

        std::vector< ERT::smspec_node > res;
        res.push_back( ERT::smspec_node( keyword.name() ) );
        return res;
    }

    static inline std::array< int, 3 > dimensions( const EclipseGrid& grid ) {
        return {
            int( grid.getNX() ),
            int( grid.getNY() ),
            int( grid.getNZ() )
        };
    }

    static inline std::vector< ERT::smspec_node > keywordB(
            const DeckKeyword& keyword,
            const EclipseState& es ) {

        auto dims = dimensions( *es.getEclipseGrid() );

        const auto mkrecord = [&dims,&keyword]( const DeckRecord& record ) {

            std::array< int , 3 > ijk = {{
                record.getItem( 0 ).get< int >( 0 ) - 1,
                record.getItem( 1 ).get< int >( 0 ) - 1,
                record.getItem( 2 ).get< int >( 0 ) - 1
            }};

            return ERT::smspec_node( keyword.name(), dims.data(), ijk.data() );
        };

        return fun::map( mkrecord, keyword );
    }

    static inline std::vector< ERT::smspec_node > keywordR(
            const DeckKeyword& keyword,
            const EclipseState& es ) {

        auto dims = dimensions( *es.getEclipseGrid() );

        const auto mknode = [&dims,&keyword]( int region ) {
            return ERT::smspec_node( keyword.name(), dims.data(), region );
        };

        const auto& item = keyword.getDataRecord().getDataItem();
        const auto regions = item.size() > 0 && item.hasValue( 0 )
            ? item.getData< int >()
            : es.getRegions();

        return fun::map( mknode, regions );
    }

    std::vector< ERT::smspec_node > handleKW( const DeckKeyword& keyword, const EclipseState& es ) {
        const auto var_type = ecl_smspec_identify_var_type( keyword.name().c_str() );

        switch( var_type ) {
            case ECL_SMSPEC_WELL_VAR: /* intentional fall-through */
            case ECL_SMSPEC_GROUP_VAR: return keywordWG( var_type, keyword, es );
            case ECL_SMSPEC_FIELD_VAR: return keywordF( keyword, es );
            case ECL_SMSPEC_BLOCK_VAR: return keywordB( keyword, es );
            case ECL_SMSPEC_REGION_VAR: return keywordR( keyword, es );

            default: return {};
        }
    }

    Summary::Summary( const Deck& deck, const EclipseState& es ) {

        SUMMARYSection section( deck );
        const auto handler = [&es]( const DeckKeyword& kw ) {
            return handleKW( kw, es );
        };
        this->keywords = fun::concat( fun::map( handler, section ) );
    }

    Summary::const_iterator Summary::begin() const {
        return this->keywords.cbegin();
    }

    Summary::const_iterator Summary::end() const {
        return this->keywords.cend();
    }

}
