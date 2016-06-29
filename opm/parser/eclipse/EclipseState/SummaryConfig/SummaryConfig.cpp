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


#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/MessageContainer.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/Utility/Functional.hpp>

#include <ert/ecl/ecl_smspec.h>

#include <iostream>
#include <algorithm>
#include <array>
#include <functional>

namespace Opm {

    const std::vector <std::string> SummaryConfig::__ALL_expands_keywords ({
        "FAQR", "FAQRG", "FAQT", "FAQTG", "FGIP", "FGIPG", "FGIPL",
        "FGIR", "FGIT", "FGOR", "FGPR", "FGPT", "FOIP", "FOIPG",
        "FOIPL", "FOIR", "FOIT", "FOPR", "FOPT", "FPR", "FVIR",
        "FVIT", "FVPR", "FVPT", "FWCT", "FWGR", "FWIP", "FWIR",
        "FWIT", "FWPR", "FWPT",
        "GGIR", "GGIT", "GGOR", "GGPR", "GGPT", "GOIR", "GOIT",
        "GOPR", "GOPT", "GVIR", "GVIT", "GVPR", "GVPT", "GWCT",
        "GWGR", "GWIR", "GWIT", "GWPR", "GWPT",
        "WBHP", "WGIR", "WGIT", "WGOR", "WGPR", "WGPT", "WOIR",
        "WOIT", "WOPR", "WOPT", "WPI", "WTHP", "WVIR", "WVIT",
        "WVPR", "WVPT", "WWCT", "WWGR", "WWIR", "WWIT", "WWPR",
        "WWPT",
        // ALL will not expand to these keywords yet
        "AAQR", "AAQRG", "AAQT", "AAQTG"
    });

	std::vector <std::string> SummaryConfig::getAllExpandedKeywords() {
		return __ALL_expands_keywords;
	}


    /*
      When the error handling config says that the error should be
      logged, the handleMissingWell and handleMissingGroup routines
      cheat. Ideally we should have a MessageContainer instance around
      and pass that to the parseContext::handlError() routine. Instead
      we:

        1. We instantiate new MessageContainer() which is just
           immediately dropped to floor, leaving the messages behind.

        2. Print a message on stderr.

      The case of incorrectly/missing well/group names in the SUMMARY
      section did just not seem important enough to warrant the
      refactoring required to pass a mutable proper MessageContainer
      all the way down here.
    */

    static void handleMissingWell( const ParseContext& parseContext , const std::string& keyword, const std::string& well) {
        std::string msg = std::string("Error in keyword:") + keyword + std::string(" No such well: ") + well;
        MessageContainer msgContainer;
        if (parseContext.get( ParseContext::SUMMARY_UNKNOWN_WELL) == InputError::WARN)
            std::cerr << "ERROR: " << msg << std::endl;

        parseContext.handleError( ParseContext::SUMMARY_UNKNOWN_WELL , msgContainer , msg );
    }


    static void handleMissingGroup( const ParseContext& parseContext , const std::string& keyword, const std::string& group) {
        std::string msg = std::string("Error in keyword:") + keyword + std::string(" No such group: ") + group;
        MessageContainer msgContainer;
        if (parseContext.get( ParseContext::SUMMARY_UNKNOWN_GROUP) == InputError::WARN)
            std::cerr << "ERROR: " << msg << std::endl;

        parseContext.handleError( ParseContext::SUMMARY_UNKNOWN_GROUP , msgContainer , msg );
    }



    template< typename T >
    static std::string name( const T* x ) {
        return x->name();
    }

    static inline std::vector< ERT::smspec_node > defaultW(
            const std::string& keyword,
            const Schedule& schedule ) {

        const auto mknode = [&keyword]( const std::string& wname ) {
            return ERT::smspec_node( ECL_SMSPEC_WELL_VAR, wname, keyword );
        };

        return fun::map( mknode, fun::map( name< Well >, schedule.getWells() ) );
    }


    static inline std::vector< ERT::smspec_node > keywordW(const ParseContext& parseContext,
                                                           const DeckKeyword& keyword,
                                                           const Schedule& schedule ) {

        const auto& item = keyword.getDataRecord().getDataItem();
        if (item.hasValue( 0 )) {
            std::vector<ERT::smspec_node> nodes;
            for (const std::string& well : item.getData< std::string >()) {
                if (schedule.hasWell( well ))
                    nodes.emplace_back( ECL_SMSPEC_WELL_VAR , well , keyword.name());
                else
                    handleMissingWell( parseContext , keyword.name() , well );
            }
            return nodes;
        } else
            return defaultW( keyword.name() , schedule );
    }



    static inline std::vector< ERT::smspec_node > defaultG(const std::string& keyword,
                                                           const Schedule& schedule ) {

        const auto mknode = [&keyword]( const std::string& gname ) {
            return ERT::smspec_node( ECL_SMSPEC_GROUP_VAR, gname, keyword );
        };

        return fun::map( mknode, fun::map( name< Group  >, schedule.getGroups() ) );
    }

    static inline std::vector< ERT::smspec_node > keywordG(const ParseContext& parseContext,
                                                           const DeckKeyword& keyword,
                                                           const Schedule& schedule ) {

        const auto& item = keyword.getDataRecord().getDataItem();
        if (item.hasValue( 0 )) {
            std::vector<ERT::smspec_node> nodes;
            for (const std::string& group : item.getData< std::string >()) {
                if (schedule.hasGroup( group ))
                    nodes.emplace_back( ECL_SMSPEC_GROUP_VAR , group , keyword.name());
                else
                    handleMissingGroup( parseContext , keyword.name() , group );
            }
            return nodes;
        } else
            return defaultG( keyword.name() , schedule );
    }



    static inline std::vector< ERT::smspec_node > keywordF(
            const DeckKeyword& keyword ) {

        std::vector< ERT::smspec_node > res;
        res.push_back( ERT::smspec_node( keyword.name() ) );
        return res;
    }

    static inline std::vector< ERT::smspec_node > keywordF(
            const std::string& keyword ) {

        std::vector< ERT::smspec_node > res;
        res.push_back( ERT::smspec_node( keyword ) );
        return res;
    }

    static inline std::array< int, 3 > dimensions( const EclipseGrid& grid ) {
        return {{
            int( grid.getNX() ),
            int( grid.getNY() ),
            int( grid.getNZ() )
        }};
    }

    static inline std::array< int, 3 > getijk( const DeckRecord& record,
                                               int offset = 0 )
    {
        return {{
            record.getItem( offset + 0 ).get< int >( 0 ) - 1,
            record.getItem( offset + 1 ).get< int >( 0 ) - 1,
            record.getItem( offset + 2 ).get< int >( 0 ) - 1
        }};
    }

    static inline std::array< int, 3 > getijk( const Completion& completion ) {
        return {{ completion.getI(), completion.getJ(), completion.getK() }};
    }

    static inline std::vector< ERT::smspec_node > keywordB(
            const DeckKeyword& keyword,
            std::array< int, 3 > dims ) {

        const auto mkrecord = [dims,&keyword]( const DeckRecord& record ) {
            auto ijk = getijk( record );
            return ERT::smspec_node( keyword.name(), dims.data(), ijk.data() );
        };

        return fun::map( mkrecord, keyword );
    }

    static inline std::vector< ERT::smspec_node > keywordR(
            const DeckKeyword& keyword,
            const Eclipse3DProperties& props,
            std::array< int, 3 > dims ) {

        /* RUNSUM is not a region keyword but a directive for how to format and
         * print output. Unfortunately its *recognised* as a region keyword
         * because of its structure and position. Hence the special handling of ignoring it.
         */
        if( keyword.name() == "RUNSUM" ) return {};
        if( keyword.name() == "RPTONLY" ) return {};

        const auto mknode = [dims,&keyword]( int region ) {
            return ERT::smspec_node( keyword.name(), dims.data(), region );
        };

        const auto& item = keyword.getDataRecord().getDataItem();
        const auto regions = item.size() > 0 && item.hasValue( 0 )
            ? item.getData< int >()
            : props.getRegions( "FIPNUM" );

        return fun::map( mknode, regions );
    }

   static inline std::vector< ERT::smspec_node > keywordC(const ParseContext& parseContext,
                                                           const DeckKeyword& keyword,
                                                           const Schedule& schedule,
                                                           std::array< int, 3 > dims ) {

       std::vector< ERT::smspec_node > nodes;
       const auto& keywordstring = keyword.name();
       const auto last_timestep = schedule.getTimeMap()->last();

       for( const auto& record : keyword ) {

           if( record.getItem( 0 ).defaultApplied( 0 ) ) {
               for( const auto& well : schedule.getWells() ) {

                   const auto& name = well->name();

                   for( const auto& completion : *well->getCompletions( last_timestep ) ) {
                       auto cijk = getijk( *completion );

                       /* well defaulted, block coordinates defaulted */
                       if( record.getItem( 1 ).defaultApplied( 0 ) ) {
                           nodes.emplace_back( keywordstring, name, dims.data(), cijk.data() );
                       }
                       /* well defaulted, block coordinates specified */
                       else {
                           auto recijk = getijk( record, 1 );
                           if( std::equal( recijk.begin(), recijk.end(), cijk.begin() ) )
                               nodes.emplace_back( keywordstring, name, dims.data(), cijk.data() );
                       }
                   }
               }

           } else {
                const auto& name = record.getItem( 0 ).get< std::string >( 0 );
                if (!schedule.hasWell( name ))
                    handleMissingWell( parseContext , keyword.name() , name );
                else {
                    /* all specified */
                    if( !record.getItem( 1 ).defaultApplied( 0 ) ) {
                        auto ijk = getijk( record, 1 );
                        nodes.emplace_back( keywordstring, name, dims.data(), ijk.data() );
                    }
                    else {
                        /* well specified, block coordinates defaulted */
                        for( const auto& completion : *schedule.getWell( name )->getCompletions( last_timestep ) ) {
                            auto ijk = getijk( *completion );
                            nodes.emplace_back( keywordstring, name, dims.data(), ijk.data() );
                        }
                    }
                }
           }
       }

       return nodes;
   }

    std::vector< ERT::smspec_node > handleKW( const DeckKeyword& keyword,
                                              const Schedule& schedule,
                                              const Eclipse3DProperties& props,
                                              const ParseContext& parseContext,
                                              std::array< int, 3 > n_xyz ) {
        const auto var_type = ecl_smspec_identify_var_type( keyword.name().c_str() );

        switch( var_type ) {
            case ECL_SMSPEC_WELL_VAR: return keywordW( parseContext , keyword, schedule );
            case ECL_SMSPEC_GROUP_VAR: return keywordG( parseContext , keyword, schedule );
            case ECL_SMSPEC_FIELD_VAR: return keywordF( keyword );
            case ECL_SMSPEC_BLOCK_VAR: return keywordB( keyword, n_xyz );
            case ECL_SMSPEC_REGION_VAR: return keywordR( keyword, props, n_xyz );
            case ECL_SMSPEC_COMPLETION_VAR: return keywordC( parseContext , keyword, schedule, n_xyz );

            default: return {};
        }
    }

    std::vector< ERT::smspec_node > handleALL( const Schedule& schedule) {


        std::vector< ERT::smspec_node > all;

        for(const std::string& keyword: SummaryConfig::getAllExpandedKeywords()) {
            const auto var_type = ecl_smspec_identify_var_type(keyword.c_str());
            switch (var_type) {
                case ECL_SMSPEC_WELL_VAR:
                    for(auto&k :defaultW(keyword, schedule)) all.push_back(k);
                    break;
                case ECL_SMSPEC_GROUP_VAR:
                    for(auto& k:defaultG(keyword, schedule)) all.push_back(k);
                    break;
                case ECL_SMSPEC_FIELD_VAR:
                    for(auto& k:keywordF(keyword)) all.push_back(k);
                    break;
                case ECL_SMSPEC_AQUIFER_VAR:
                    {}
                    break;
                default:
                	throw std::runtime_error("Unrecognized keyword type: " + keyword);
            }
        }

        return all;
    }

    SummaryConfig::SummaryConfig( const Deck& deck, const EclipseState& es , const ParseContext& parseContext)
        : SummaryConfig( deck,
                         *es.getSchedule(),
                         es.get3DProperties(),
                         parseContext,
                         dimensions( *es.getInputGrid() ) )
    {}

    SummaryConfig::SummaryConfig( const Deck& deck,
                                  const Schedule& schedule,
                                  const Eclipse3DProperties& props,
                                  const ParseContext& parseContext,
                                  std::array< int, 3 > n_xyz ) {

        SUMMARYSection section( deck );

        using namespace std::placeholders;
        const auto handler = std::bind( handleKW, _1, schedule, props, parseContext , n_xyz );

        /* This line of code does not compile on VS2015
         *   this->keywords = fun::concat( fun::map( handler, section ) );
         * The following code is a workaround for this compiler bug */
        for (auto& x : section) {

            if (x.name().compare("ALL") == 0)
            {
                for (auto& keyword : handleALL(schedule))
                    this->keywords.push_back(keyword);
            }
            else {
                for (auto &keyword : handler(x))
                    this->keywords.push_back(keyword);
            }
        }
    }

    SummaryConfig::const_iterator SummaryConfig::begin() const {
        return this->keywords.cbegin();
    }

    SummaryConfig::const_iterator SummaryConfig::end() const {
        return this->keywords.cend();
    }

}
