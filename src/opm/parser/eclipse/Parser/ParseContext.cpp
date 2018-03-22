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

#include <ert/util/util.h>
#include <cstdlib>

#include <boost/algorithm/string.hpp>

#include <opm/parser/eclipse/Parser/InputErrorAction.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>

namespace Opm {


    /*
      A set of predefined error modes are added, with the default
      setting 'InputError::IGNORE, then afterwards the environment
      variables 'OPM_ERRORS_EXCEPTION', 'OPM_ERRORS_WARN' and
      'OPM_ERRORS_IGNORE' are consulted
    */

    ParseContext::ParseContext() {
        initDefault();
        initEnv();
    }

    /*
      If you intend to hardwire settings you should use this
      constructor, as that way the environment variables are applied
      after the hawrdwired settings.
    */

    ParseContext::ParseContext(const std::vector<std::pair<std::string , InputError::Action>>& initial) {
        initDefault();

        for (const auto& pair : initial)
            update( pair.first , pair.second );

        initEnv();
    }


    /*
      This constructor will initialize all actions to @default_action. The
      environment variables will be used.
    */

    ParseContext::ParseContext(InputError::Action default_action) {
        initDefault();
        update(default_action);
        initEnv();
    }

    void ParseContext::initDefault() {
        addKey(PARSE_EXTRA_RECORDS, InputError::THROW_EXCEPTION);
        addKey(PARSE_UNKNOWN_KEYWORD, InputError::THROW_EXCEPTION);
        addKey(PARSE_RANDOM_TEXT, InputError::THROW_EXCEPTION);
        addKey(PARSE_RANDOM_SLASH, InputError::THROW_EXCEPTION);
        addKey(PARSE_MISSING_DIMS_KEYWORD, InputError::THROW_EXCEPTION);
        addKey(PARSE_EXTRA_DATA, InputError::THROW_EXCEPTION);
        addKey(PARSE_MISSING_INCLUDE, InputError::THROW_EXCEPTION);

        addKey(UNSUPPORTED_SCHEDULE_GEO_MODIFIER, InputError::THROW_EXCEPTION);
        addKey(UNSUPPORTED_COMPORD_TYPE, InputError::THROW_EXCEPTION);
        addKey(UNSUPPORTED_INITIAL_THPRES, InputError::THROW_EXCEPTION);
        addKey(UNSUPPORTED_TERMINATE_IF_BHP, InputError::THROW_EXCEPTION);

        addKey(INTERNAL_ERROR_UNINITIALIZED_THPRES, InputError::THROW_EXCEPTION);

        addKey(SUMMARY_UNKNOWN_WELL, InputError::THROW_EXCEPTION);
        addKey(SUMMARY_UNKNOWN_GROUP, InputError::THROW_EXCEPTION);
    }

    void ParseContext::initEnv() {
        envUpdate( "OPM_ERRORS_EXCEPTION" , InputError::THROW_EXCEPTION );
        envUpdate( "OPM_ERRORS_WARN" , InputError::WARN );
        envUpdate( "OPM_ERRORS_IGNORE" , InputError::IGNORE );
    }


    void ParseContext::handleError(
            const std::string& errorKey,
            const std::string& msg ) const {

        InputError::Action action = get( errorKey );

        if (action == InputError::WARN) {
            OpmLog::warning(msg);
            return;
        }

        else if (action == InputError::THROW_EXCEPTION) {
            OpmLog::error(msg);
            throw std::invalid_argument(errorKey + ": " + msg);
        }
    }

    std::map<std::string,InputError::Action>::const_iterator ParseContext::begin() const {
        return m_errorContexts.begin();
    }


    std::map<std::string,InputError::Action>::const_iterator ParseContext::end() const {
        return m_errorContexts.end();
    }

    ParseContext ParseContext::withKey(const std::string& key, InputError::Action action) const {
        ParseContext pc(*this);
        pc.addKey(key, action);
        return pc;
    }

    ParseContext& ParseContext::withKey(const std::string& key, InputError::Action action) {
        this->addKey(key, action);
        return *this;
    }

    bool ParseContext::hasKey(const std::string& key) const {
        if (m_errorContexts.find( key ) == m_errorContexts.end())
            return false;
        else
            return true;
    }


    void ParseContext::addKey(const std::string& key, InputError::Action default_action) {
        if (key.find_first_of("|:*") != std::string::npos)
            throw std::invalid_argument("The ParseContext keys can not contain '|', '*' or ':'");

        if (!hasKey(key))
            m_errorContexts.insert( std::pair<std::string , InputError::Action>( key , default_action));
    }


    InputError::Action ParseContext::get(const std::string& key) const {
        if (hasKey( key ))
            return m_errorContexts.find( key )->second;
        else
            throw std::invalid_argument("The errormode key: " + key + " has not been registered");
    }

    /*****************************************************************/

    /*
      This is the 'strict' update function, it will throw an exception
      if the input string is not a defined error mode. This should
      typically be used in a downstream module where the policy
      regarding an error mode is hardcoded. When using this method the
      static string constanst for the different error modes should be
      used as arguments:

        parseContext.updateKey( ParseContext::PARSE_RANDOM_SLASH , InputError::IGNORE )

    */

    void ParseContext::updateKey(const std::string& key , InputError::Action action) {
        if (hasKey(key))
            m_errorContexts[key] = action;
        else
            throw std::invalid_argument("The errormode key: " + key + " has not been registered");
    }


    void ParseContext::envUpdate( const std::string& envVariable , InputError::Action action ) {
        const char * userSetting = getenv(envVariable.c_str());
        if (userSetting )
            update( userSetting , action);
    }


    void ParseContext::update(InputError::Action action) {
        for (const auto& pair : m_errorContexts) {
            const std::string& key = pair.first;
            updateKey( key , action );
         }
    }


    void ParseContext::patternUpdate( const std::string& pattern , InputError::Action action) {
        const char * c_pattern = pattern.c_str();
        for (const auto& pair : m_errorContexts) {
            const std::string& key = pair.first;
            if (util_fnmatch( c_pattern , key.c_str()) == 0)
                updateKey( key , action );
         }
    }


    /*
      This is the most general update function. The input keyString is
      "selector string", and all matching error modes will be set to
      @action. The algorithm for decoding the @keyString is:

        1. The input string is split into several tokens on occurences
           of ':' or ':' - and then each element is treated
           seperately.

        2. For each element in the list from 1):

           a) If it contains at least one '*' - update all error modes
              matching the input string.

           b) If it is exactly equal to recognized error mode - update
              that.

           c) Otherwise - silently ignore.
    */

    void ParseContext::update(const std::string& keyString , InputError::Action action) {
        std::vector<std::string> keys;
        boost::split( keys , keyString , boost::is_any_of(":|"));
        for (const auto& input_key : keys) {
            std::vector<std::string> matching_keys;
            size_t wildcard_pos = input_key.find("*");

            if (wildcard_pos == std::string::npos) {
                if (hasKey( input_key ))
                    updateKey( input_key , action );
            } else
                patternUpdate( input_key , action );

        }
    }

    const std::string ParseContext::PARSE_EXTRA_RECORDS = "PARSE_EXTRA_RECORDS";
    const std::string ParseContext::PARSE_UNKNOWN_KEYWORD = "PARSE_UNKNOWN_KEYWORD";
    const std::string ParseContext::PARSE_RANDOM_TEXT = "PARSE_RANDOM_TEXT";
    const std::string ParseContext::PARSE_RANDOM_SLASH = "PARSE_RANDOM_SLASH";
    const std::string ParseContext::PARSE_MISSING_DIMS_KEYWORD = "PARSE_MISSING_DIMS_KEYWORD";
    const std::string ParseContext::PARSE_EXTRA_DATA = "PARSE_EXTRA_DATA";
    const std::string ParseContext::PARSE_MISSING_INCLUDE = "PARSE_MISSING_INCLUDE";

    const std::string ParseContext::UNSUPPORTED_SCHEDULE_GEO_MODIFIER = "UNSUPPORTED_SCHEDULE_GEO_MODIFIER";
    const std::string ParseContext::UNSUPPORTED_COMPORD_TYPE = "UNSUPPORTED_COMPORD_TYPE";
    const std::string ParseContext::UNSUPPORTED_INITIAL_THPRES = "UNSUPPORTED_INITIAL_THPRES";
    const std::string ParseContext::UNSUPPORTED_TERMINATE_IF_BHP = "UNSUPPORTED_TERMINATE_IF_BHP";

    const std::string ParseContext::INTERNAL_ERROR_UNINITIALIZED_THPRES = "INTERNAL_ERROR_UNINITIALIZED_THPRES";

    const std::string ParseContext::PARSE_MISSING_SECTIONS = "PARSE_MISSING_SECTIONS";

    const std::string ParseContext::SUMMARY_UNKNOWN_WELL  = "SUMMARY_UNKNOWN_WELL";
    const std::string ParseContext::SUMMARY_UNKNOWN_GROUP = "SUMMARY_UNKNOWN_GROUP";
}


