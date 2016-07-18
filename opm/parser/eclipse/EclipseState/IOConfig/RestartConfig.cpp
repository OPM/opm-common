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

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Deck/SCHEDULESection.hpp>
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

    }

    RestartConfig::restart RestartConfig::rptrst( const DeckKeyword& kw, size_t step ) {

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

        for( const auto& mnemonic : items ) {

            const auto freq_pos = mnemonic.find( "FREQ=" );
            if( freq_pos != std::string::npos ) {
                freq = std::stoul( mnemonic.substr( freq_pos + 5 ) );
            }

            const auto basic_pos = mnemonic.find( "BASIC=" );
            if( basic_pos != std::string::npos ) {
                basic = std::stoul( mnemonic.substr( basic_pos + 6 ) );
                found_basic = true;
            }
        }

        if( found_basic ) return restart( step, basic, freq );

        /* If no BASIC mnemonic is found, either it is not present or we might
         * have an old data set containing integer controls instead of mnemonics.
         * BASIC integer switch is integer control nr 1, FREQUENCY is integer
         * control nr 6.
         */

        /* mnemonics, but without basic and freq. Effectively ignored */
        if( !ints ) return {};

        const int BASIC_index = 0;
        const int FREQ_index = 5;

        if( items.size() > BASIC_index )
            basic = std::stoul( items[ BASIC_index ] );

        // Peculiar special case in eclipse, - not documented
        // This ignore of basic = 0 for the integer mnemonics case
        // is done to make flow write restart file at the same intervals
        // as eclipse for the Norne data set. There might be some rules
        // we are missing here.
        if( 0 == basic ) return {};

        if( items.size() > FREQ_index ) // if frequency is set
            freq = std::stoul( items[ FREQ_index ] );

        return restart( step, basic, freq );
    }

    RestartConfig::restart RestartConfig::rptsched( const DeckKeyword& keyword ) {
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

        if( restart_found ) return restart( step );


        /* No RESTART or NOTHING found, but it is not an integer list */
        if( strs ) return {};

        /* We might have an old data set containing integer controls instead of
         * mnemonics. Restart integer switch is integer control nr 7
         */

        const int RESTART_index = 6;

        if( items.size() <= RESTART_index ) return {};

        return restart( std::stoul( items[ RESTART_index ] ) );
    }

    DynamicState< RestartConfig::restart > RestartConfig::rstconf(
            const SCHEDULESection& schedule,
            std::shared_ptr< const TimeMap > timemap ) {

        size_t current_step = 1;
        bool ignore_RPTSCHED_restart = false;
        restart unset;

        DynamicState< RestartConfig::restart >
            restart_config( timemap, restart( 0, 0, 1 ) );

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

            if( timemap->size() <= current_step ) continue;

            const bool is_RPTRST = name == "RPTRST";

            if( !is_RPTRST && ignore_RPTSCHED_restart ) continue;

            const auto rs = is_RPTRST
                          ? rptrst( keyword, current_step - 1 )
                          : rptsched( keyword );

            if( is_RPTRST ) ignore_RPTSCHED_restart = rs.basic > 2;

            /* we're using the default state of restart to signal "no
             * update". The default state is non-sensical
             */
            if( rs == unset ) continue;

            if( 6 == rs.rptsched_restart || 6 == rs.basic )
                throw std::runtime_error(
                    "OPM does not support the RESTART=6 setting"
                    "(write restart file every timestep)"
                );

            restart_config.update( current_step, rs );
        }

        return restart_config;
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
        m_first_restart_step( -1 ),
        m_restart_output_config( std::make_shared< DynamicState< restart > >(rstconf( schedule, timemap ) ) )
    {
        handleSolutionSection( solution ) ;
        initFirstOutput( );
    }


    bool RestartConfig::getWriteRestartFile(size_t timestep) const {
        bool write_restart_ts = false;

        if (0 == timestep) {
            write_restart_ts = m_write_initial_RST_file;
        } else if (m_restart_output_config) {
            restart ts_restart_config = m_restart_output_config->get(timestep);

            //Look at rptsched restart setting
            if (ts_restart_config.rptsched_restart_set) {
                if (ts_restart_config.rptsched_restart > 0) {
                    write_restart_ts = true;
                }
            } else { //Look at rptrst basic setting
                switch (ts_restart_config.basic) {
                    case 0: //Do not write restart files
                        write_restart_ts = false;
                        break;
                    case 1: //Write restart file every report time
                        write_restart_ts = true;
                        break;
                    case 2: //Write restart file every report time
                        write_restart_ts = true;
                        break;
                    case 3: //Every n'th report time
                        write_restart_ts = getWriteRestartFileFrequency(timestep, ts_restart_config.timestep, ts_restart_config.frequency);
                        break;
                    case 4: //First reportstep of every year, or if n > 1, n'th years
                        write_restart_ts = getWriteRestartFileFrequency(timestep, ts_restart_config.timestep, ts_restart_config.frequency, true);
                        break;
                    case 5: //First reportstep of every month, or if n > 1, n'th months
                        write_restart_ts = getWriteRestartFileFrequency(timestep, ts_restart_config.timestep, ts_restart_config.frequency, false, true);
                        break;
                    default:
                        // do nothing
                        break;
                }
            }
        }

        return write_restart_ts;
    }


    bool RestartConfig::getWriteRestartFileFrequency(size_t timestep,
                                                size_t start_timestep,
                                                size_t frequency,
                                                bool   years,
                                                bool   months) const {
        bool write_restart_file = false;
        if ((!years && !months) && (timestep >= start_timestep)) {
            write_restart_file = ((timestep % frequency) == 0) ? true : false;
        } else {
              write_restart_file = m_timemap->isTimestepInFirstOfMonthsYearsSequence(timestep, years, start_timestep, frequency);

        }
        return write_restart_file;
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

            auto rs = rptrst( rptrstkeyword, 0 );
            if( rs != restart() )
                m_restart_output_config->updateInitial( rs );

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

        restart rs( step, basic, interval );
        m_restart_output_config->globalReset( rs );

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
