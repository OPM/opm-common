/*
  Copyright 2015 Statoil ASA.

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

#include <stdio.h>
#include <iostream>
#include <iterator>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <opm/parser/eclipse/Utility/Functional.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/RestartConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <ert/ecl/ecl_util.h>




namespace Opm {

    namespace {

        inline bool is_int( const std::string& x ) {
            auto is_digit = []( char c ) { return std::isdigit( c ); };

            return !x.empty()
                && ( x.front() == '-' || is_digit( x.front() ) )
                && std::all_of( x.begin() + 1, x.end(), is_digit );
        }


        std::set<std::string> notSupported = {"NORST", "SFREQ" , "STREAM"};

        constexpr const char*  integerKeywords[] = {""     ,      //  1 BASIC=?? - ignored
                                                    "FLOWS",      //  2
                                                    "FIP",        //  3
                                                    "POT",        //  4
                                                    "PBPD",       //  5
                                                    "",           //  6 FREQ=?? - ignored
                                                    "PRES",       //  7
                                                    "VISC",       //  8
                                                    "DEN",        //  9
                                                    "DRAIN",      // 10
                                                    "KRO",        // 11
                                                    "KRW",        // 12
                                                    "KRG",        // 13
                                                    "PORO",       // 14
                                                    "NOGRAD",     // 15
                                                    "NORST",      // 16 NORST - not supported
                                                    "SAVE",       // 17
                                                    "SFREQ",      // 18 SFREQ=?? - not supported
                                                    "ALLPROPS",   // 19
                                                    "ROCKC",      // 20
                                                    "SGTRAP",     // 21
                                                    "",           // 22 - Blank - ignored.
                                                    "RSSAT",      // 23
                                                    "RVSAT",      // 24
                                                    "GIMULT",     // 25
                                                    "SURFBLK",    // 26
                                                    "PCOW,PCOG",  // 27 - Multiple, split on "," [Special cased]
                                                    "STREAM",     // 28 STREAM=?? - not supported
                                                    "RK",         // 29
                                                    "VELOCITY",   // 30
                                                    "COMPRESS" }; // 31


        static void insertKeyword(std::set<std::string>& keywords, const std::string& keyword) {
            const auto pos = keyword.find( "=" );
            if (pos != std::string::npos) {
                std::string base_keyword( keyword , 0 , pos );
                insertKeyword( keywords, base_keyword );
            } else {
                /*
                  We have an explicit list of not supported keywords;
                  those are keywords requesting special output
                  behavior which is not supported. However - the list
                  of keywords is long, and not appearing on this list
                  does *not* imply that the keyword is actually
                  supported by the simulator.
                */
                if (notSupported.find( keyword ) == notSupported.end())
                    keywords.insert( keyword );
            }
        }

    }


    RestartSchedule::RestartSchedule( size_t sched_restart) :
        rptsched_restart_set( true ),
        rptsched_restart( sched_restart )
    {
    }

    RestartSchedule::RestartSchedule( size_t step, size_t b, size_t freq) :
        timestep( step ),
        basic( b ),
        frequency( freq )
    {
    }

    bool RestartSchedule::operator!=(const RestartSchedule & rhs) const {
        return !( *this == rhs );
    }

    bool RestartSchedule::operator==( const RestartSchedule& rhs ) const {
        if( this->rptsched_restart_set ) {
            return rhs.rptsched_restart_set
                && this->rptsched_restart == rhs.rptsched_restart;
        }

        return this->timestep == rhs.timestep &&
            this->basic == rhs.basic &&
            this->frequency == rhs.frequency;
    }



    void RestartConfig::handleRPTRST( const DeckKeyword& kw, size_t step ) {
        RestartSchedule rs;
        RestartSchedule unset;
        const auto& items = kw.getStringData();

        /* if any of the values are pure integers we assume this is meant to be
         * the slash-terminated list of integers way of configuring. If
         * integers and non-integers are mixed, this is an error.
         */
        const auto ints = std::any_of( items.begin(), items.end(), is_int );
        const auto strs = !std::all_of( items.begin(), items.end(), is_int );

        if( ints && strs ) throw std::runtime_error(
                "RPTRST does not support mixed mnemonics and integer list."
            );

        size_t basic = 1;
        size_t freq = 0;
        bool found_basic = false;
        if (strs) {
            std::vector<std::string> keywords;
            for( const auto& mnemonic : items ) {

                const auto freq_pos = mnemonic.find( "FREQ=" );
                if( freq_pos != std::string::npos ) {
                    freq = std::stoul( mnemonic.substr( freq_pos + 5 ) );
                    continue;
                }

                const auto basic_pos = mnemonic.find( "BASIC=" );
                if( basic_pos != std::string::npos ) {
                    basic = std::stoul( mnemonic.substr( basic_pos + 6 ) );
                    found_basic = true;
                    continue;
                }

                keywords.push_back( std::string( mnemonic ));
            }
            addKeywords( step , keywords );

            if( found_basic )
                rs = RestartSchedule( step, basic, freq );
        } else {
            /* If no BASIC mnemonic is found, either it is not present or we might
             * have an old data set containing integer controls instead of mnemonics.
             * BASIC integer switch is integer control nr 1, FREQUENCY is integer
             * control nr 6.
             */

            const int BASIC_index = 0;
            const int FREQ_index = 5;

            if( items.size() > BASIC_index )
                basic = std::stoul( items[ BASIC_index ] );

            // Peculiar special case in eclipse, - not documented
            // This ignore of basic = 0 for the integer mnemonics case
            // is done to make flow write restart file at the same intervals
            // as eclipse for the Norne data set. There might be some rules
            // we are missing here.
            if (basic == 0)
                rs = unset;
            else {
                if( items.size() > FREQ_index ) // if frequency is set
                    freq = std::stoul( items[ FREQ_index ] );

                rs = RestartSchedule( step, basic, freq );
            }

            updateKeywords( step , fun::map( []( const std::string& s) { return std::stoi(s); } , items ));
        }

        if (rs != unset)
            update( step , rs);
    }


    RestartSchedule RestartConfig::rptsched( const DeckKeyword& keyword ) {
        size_t step = 0;
        bool restart_found = false;

        const auto& items = keyword.getStringData();
        const auto ints = std::any_of( items.begin(), items.end(), is_int );
        const auto strs = !std::all_of( items.begin(), items.end(), is_int );

        if( ints && strs ) throw std::runtime_error(
                "RPTSCHED does not support mixed mnemonics and integer list."
            );

        for( const auto& mnemonic : items  ) {
            const auto restart_pos = mnemonic.find( "RESTART=" );
            if( restart_pos != std::string::npos ) {
                step = std::stoul( mnemonic.substr( restart_pos + 8 ) );
                restart_found = true;
            }

            const auto nothing_pos = mnemonic.find( "NOTHING" );
            if( nothing_pos != std::string::npos ) {
                step = 0;
                restart_found = true;
            }
        }

        if( restart_found ) return RestartSchedule( step );


        /* No RESTART or NOTHING found, but it is not an integer list */
        if( strs ) return {};

        /* We might have an old data set containing integer controls instead of
         * mnemonics. Restart integer switch is integer control nr 7
         */

        const int RESTART_index = 6;

        if( items.size() <= RESTART_index ) return {};

        return RestartSchedule( std::stoul( items[ RESTART_index ] ) );
    }


    void RestartConfig::update( size_t step, const RestartSchedule& rs) {
        if (step == 0)
            this->restart_schedule->updateInitial( rs );
        else
            this->restart_schedule->update( step, rs );
    }




    /*
      The time related mnemonics BASIC= and FREQ= have been stripped
      out before calling this method, the other keywords just come
      verbatim from the deck; in particular we do not split KEY=int
      keywords - there are some of them.
    */

    void RestartConfig::addKeywords( size_t step, const std::vector<std::string>& keywords) {
        if (keywords.size() > 0) {
            std::set<std::string> kw_set = this->restart_keywords->back();
            for (const auto& kw : keywords )
                insertKeyword( kw_set , kw );

            if (step == 0)
                this->restart_keywords->updateInitial( kw_set );
            else
                this->restart_keywords->update( step , kw_set );
        }
    }


    /*
      When handling the RPTRST keyword based on integer controls we
      can actually remove keywords from the write set by passing in a
      control value of 0. With our implementation it is not possible
      to remove keywords with the string based menonics.
    */

    void RestartConfig::updateKeywords( size_t step, const std::vector<int>& integer_controls) {
        std::set<std::string> keywords = this->restart_keywords->back();

        for (size_t index = 0; index < integer_controls.size(); index++) {
            if (index == 26) {
                if (integer_controls[index] > 0) {
                    insertKeyword( keywords , "PCOW" );
                    insertKeyword( keywords , "PCOG" );
                } else {
                    keywords.erase( "PCOW" );
                    keywords.erase( "PCOG" );
                }
                continue;
            }

            {
                const std::string& keyword = integerKeywords[index];
                if (keyword != "") {
                    if (integer_controls[index] > 0)
                        insertKeyword( keywords , keyword );
                    else
                        keywords.erase( keyword );
                }
            }
        }

        if (step == 0)
            this->restart_keywords->updateInitial( keywords );
        else
            this->restart_keywords->update( step , keywords );
    }


    void RestartConfig::handleScheduleSection(const SCHEDULESection& schedule) {

        size_t current_step = 1;
        bool ignore_RPTSCHED_restart = false;
        RestartSchedule unset;

        for( const auto& keyword : schedule ) {
            const auto& name = keyword.name();

            if( name == "DATES" ) {
                current_step += keyword.size();
                continue;
            }

            if( name == "TSTEP" ) {
                current_step += keyword.getRecord( 0 ).getItem( 0 ).size();
                continue;
            }

            if( !( name == "RPTRST" || name == "RPTSCHED" ) ) continue;

            if( this->m_timemap->size() <= current_step ) continue;

            const bool is_RPTRST = name == "RPTRST";

            if( !is_RPTRST && ignore_RPTSCHED_restart ) continue;

            if (is_RPTRST) {
                handleRPTRST( keyword, current_step );  // Was -1??
                const auto& rs = this->restart_schedule->back();
                if (rs.basic > 2)
                    ignore_RPTSCHED_restart = true;
            } else {
                const auto rs = rptsched( keyword );

                /* we're using the default state of restart to signal "no
                 * update". The default state is non-sensical
                 */
                if( rs == unset ) continue;

                if( 6 == rs.rptsched_restart || 6 == rs.basic )
                    throw std::runtime_error(
                                             "OPM does not support the RESTART=6 setting"
                                             "(write restart file every timestep)"
                                             );

                update( current_step , rs );
            }
        }
    }

    bool RestartSchedule::writeRestartFile( size_t input_timestep , const TimeMap& timemap) const {
        if (this->rptsched_restart_set && (this->rptsched_restart > 0))
            return true;

        switch (basic) {
         //Do not write restart files
        case 0:  return false;

        //Write restart file every report time
        case 1: return true;

        //Write restart file every report time
        case 2: return true;

        //Every n'th report time
        case 3: return  ((input_timestep % this->frequency) == 0) ? true : false;

        //First reportstep of every year, or if n > 1, n'th years
        case 4: return timemap.isTimestepInFirstOfMonthsYearsSequence(input_timestep, true , this->timestep, this->frequency);

         //First reportstep of every month, or if n > 1, n'th months
        case 5: return timemap.isTimestepInFirstOfMonthsYearsSequence(input_timestep, false , this->timestep, this->frequency);

        default: return false;
        }
    }



    RestartConfig::RestartConfig( const Deck& deck ) :
        RestartConfig( SCHEDULESection( deck ),
                       SOLUTIONSection( deck ),
                       std::make_shared< const TimeMap >( deck ))
    {}


    RestartConfig::RestartConfig( const SCHEDULESection& schedule,
                                  const SOLUTIONSection& solution,
                                  std::shared_ptr< const TimeMap > timemap) :
        m_timemap( timemap ),
        m_first_restart_step( -1 )
    {
        restart_schedule.reset( new DynamicState<RestartSchedule>(timemap , RestartSchedule(0 ,0 ,1)) );
        restart_keywords.reset( new DynamicState<std::set<std::string>>( timemap, {}));

        handleSolutionSection( solution ) ;
        handleScheduleSection( schedule ) ;

        initFirstOutput( );
    }


    RestartSchedule RestartConfig::getNode( size_t timestep ) const{
        return restart_schedule->get(timestep);
    }


    bool RestartConfig::getWriteRestartFile(size_t timestep) const {
        if (0 == timestep)
            return m_write_initial_RST_file;

        {
            RestartSchedule ts_restart_config = getNode( timestep );
            return ts_restart_config.writeRestartFile( timestep , *m_timemap );
        }
    }


    const std::set<std::string>& RestartConfig::getRestartKeywords( size_t timestep ) const {
        return restart_keywords->at( timestep );
    }


    /*
      Will initialize the internal variable holding the first report
      step when restart output is queried.

      The reason we are interested in this report step is that when we
      reach this step the output files should be opened with mode 'w'
      - whereas for subsequent steps they should be opened with mode
      'a'.
    */

    void RestartConfig::initFirstOutput( ) {
        size_t report_step = 0;
        while (true) {
            if (getWriteRestartFile(report_step)) {
                m_first_restart_step = report_step;
                break;
            }
            report_step++;
            if (report_step == m_timemap->size())
                break;
        }
    }


    void RestartConfig::handleSolutionSection(const SOLUTIONSection& solutionSection) {
        if (solutionSection.hasKeyword("RPTRST")) {
            const auto& rptrstkeyword        = solutionSection.getKeyword("RPTRST");

            handleRPTRST( rptrstkeyword, 0 );
            setWriteInitialRestartFile(true); // Guessing on eclipse rules for write of initial RESTART file (at time 0):
                                              // Write of initial restart file is (due to the eclipse reference manual)
                                              // governed by RPTSOL RESTART in solution section,
                                              // if RPTSOL RESTART > 1 initial restart file is written.
                                              // but - due to initial restart file written from Eclipse
                                              // for data where RPTSOL RESTART not set - guessing that
                                              // when RPTRST is set in SOLUTION (no basic though...) -> write inital restart.
        } //RPTRST


        if (solutionSection.hasKeyword("RPTSOL") && (m_timemap->size() > 0)) {
            handleRPTSOL(solutionSection.getKeyword("RPTSOL"));
        } //RPTSOL
    }


    void RestartConfig::overrideRestartWriteInterval(size_t interval) {
        size_t step = 0;
        /* write restart files if the interval is non-zero. The restart
         * mnemonic (setting) that governs restart-on-interval is BASIC=3
         */
        size_t basic = interval > 0 ? 3 : 0;

        RestartSchedule rs( step, basic, interval );
        restart_schedule->globalReset( rs );

        setWriteInitialRestartFile( interval > 0 );
    }


    void RestartConfig::setWriteInitialRestartFile(bool writeInitialRestartFile) {
        m_write_initial_RST_file = writeInitialRestartFile;
    }


    void RestartConfig::handleRPTSOL( const DeckKeyword& keyword) {
        const auto& record = keyword.getRecord(0);

        size_t restart = 0;
        size_t found_mnemonic_RESTART = 0;
        bool handle_RPTSOL_RESTART = false;

        const auto& item = record.getItem(0);


        for (size_t index = 0; index < item.size(); ++index) {
            const std::string& mnemonic = item.get< std::string >(index);

            found_mnemonic_RESTART = mnemonic.find("RESTART=");
            if (found_mnemonic_RESTART != std::string::npos) {
                std::string restart_no = mnemonic.substr(found_mnemonic_RESTART+8, mnemonic.size());
                restart = boost::lexical_cast<size_t>(restart_no);
                handle_RPTSOL_RESTART = true;
            }
        }


        /* If no RESTART mnemonic is found, either it is not present or we might
           have an old data set containing integer controls instead of mnemonics.
           Restart integer switch is integer control nr 7 */

        if (found_mnemonic_RESTART == std::string::npos) {
            if (item.size() >= 7)  {
                const std::string& integer_control = item.get< std::string >(6);
                try {
                    restart = boost::lexical_cast<size_t>(integer_control);
                    handle_RPTSOL_RESTART = true;
                } catch (boost::bad_lexical_cast &) {
                    //do nothing
                }
            }
        }

        if (handle_RPTSOL_RESTART) {
            if (restart > 1) {
                setWriteInitialRestartFile(true);
            } else {
                setWriteInitialRestartFile(false);
            }
        }
    }



    std::string RestartConfig::getRestartFileName(const std::string& restart_base, int report_step, bool unified , bool fmt_file) {

        ecl_file_enum file_type = (unified) ? ECL_UNIFIED_RESTART_FILE : ECL_RESTART_FILE;
        char * c_str = ecl_util_alloc_filename( NULL , restart_base.c_str() , file_type, fmt_file , report_step);
        std::string restart_filename = c_str;
        free( c_str );

        return restart_filename;
    }


    int RestartConfig::getFirstRestartStep() const {
        return m_first_restart_step;
    }

}
