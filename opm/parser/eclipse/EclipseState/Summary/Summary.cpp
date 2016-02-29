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

#include <ert/ecl/ecl_smspec.h>

#include <numeric>
#include <array>

namespace Opm {

    namespace fun {

        /*
         * map :: (a -> b) -> [a] -> [b]
         *
         * C can be any foreach-compatible container (that supports .begin,
         * .end), but will always return a vector.
         */
        template< typename F, typename C >
        std::vector< typename std::result_of< F( typename C::const_iterator::value_type& ) >::type >
        map( F&& f, const C& src ) {
            using A = typename C::const_iterator::value_type;
            using B = typename std::result_of< F( A& ) >::type;
            std::vector< B > ret;
            ret.reserve( src.size() );

            std::transform( src.begin(), src.end(), std::back_inserter( ret ), f );
            return ret;
        }

        template< typename A >
        std::vector< A > concat( std::vector< std::vector< A > >&& src ) {

            const auto size = std::accumulate( src.begin(), src.end(), 0,
                []( std::size_t acc, const std::vector< A >& x ) {
                    return acc + x.size();
                }
            );

            std::vector< A > dst;
            dst.reserve( size );

            for( auto& x : src )
                std::move( x.begin(), x.end(), std::back_inserter( dst ) );

            return dst;
        }

    }

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
            const EclipseState& es ) {

        std::vector< ERT::smspec_node > res;
        res.push_back( ERT::smspec_node( keyword.name() ) );
        return res;
    }

    static inline std::vector< ERT::smspec_node > keywordB(
            const DeckKeyword& keyword,
            const EclipseState& es ) {

        std::array< int, 3 > dims = {
            int( es.getEclipseGrid()->getNX() ),
            int( es.getEclipseGrid()->getNY() ),
            int( es.getEclipseGrid()->getNZ() )
        };

        const auto mkrecord = [&dims,&keyword]( const DeckRecord& record ) {

            std::array< int , 3 > ijk = {
                record.getItem( 0 ).get< int >( 0 ) - 1,
                record.getItem( 1 ).get< int >( 0 ) - 1,
                record.getItem( 2 ).get< int >( 0 ) - 1
            };

            return ERT::smspec_node( keyword.name(), dims.data(), ijk.data() );
        };

        return fun::map( mkrecord, keyword );
    }

    std::vector< ERT::smspec_node > handleKW( const DeckKeyword& keyword, const EclipseState& es ) {
        const auto var_type = ecl_smspec_identify_var_type( keyword.name().c_str() );

        switch( var_type ) {
            case ECL_SMSPEC_WELL_VAR: /* intentional fall-through */
            case ECL_SMSPEC_GROUP_VAR: return keywordWG( var_type, keyword, es );
            case ECL_SMSPEC_FIELD_VAR: return keywordF( keyword, es );
            case ECL_SMSPEC_BLOCK_VAR: return keywordB( keyword, es );

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
