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

#include <cstdlib>
#include <iostream>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/common/OpmLog/KeywordLocation.hpp>
#include <opm/common/utility/String.hpp>
#include <opm/common/utility/shmatch.hpp>
#include <opm/common/utility/OpmInputError.hpp>

namespace Opm {


    /*
      A set of predefined error modes are added, with the default
      setting 'InputErrorAction::IGNORE, then afterwards the environment
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

    ParseContext::ParseContext(const std::vector<std::pair<std::string , InputErrorAction>>& initial) {
        initDefault();

        for (const auto& pair : initial)
            update( pair.first , pair.second );

        initEnv();
    }


    /*
      This constructor will initialize all actions to @default_action. The
      environment variables will be used.
    */

    ParseContext::ParseContext(InputErrorAction default_action) {
        initDefault();
        update(default_action);
        initEnv();
    }

    void ParseContext::initDefault() {
        addKey(PARSE_EXTRA_RECORDS, InputErrorAction::THROW_EXCEPTION);
        addKey(PARSE_UNKNOWN_KEYWORD, InputErrorAction::THROW_EXCEPTION);
        addKey(PARSE_RANDOM_TEXT, InputErrorAction::THROW_EXCEPTION);
        addKey(PARSE_RANDOM_SLASH, InputErrorAction::THROW_EXCEPTION);
        addKey(PARSE_MISSING_DIMS_KEYWORD, InputErrorAction::THROW_EXCEPTION);
        addKey(PARSE_EXTRA_DATA, InputErrorAction::THROW_EXCEPTION);
        addKey(PARSE_MISSING_INCLUDE, InputErrorAction::EXIT1);
        addKey(PARSE_LONG_KEYWORD, InputErrorAction::WARN);
        addKey(PARSE_WGNAME_SPACE, InputErrorAction::THROW_EXCEPTION);
        addKey(PARSE_INVALID_KEYWORD_COMBINATION, InputErrorAction::THROW_EXCEPTION);

        addKey(UNIT_SYSTEM_MISMATCH, InputErrorAction::THROW_EXCEPTION);

        // WELLDIMS actions
        this->addKey(RUNSPEC_NUMWELLS_TOO_LARGE, InputErrorAction::THROW_EXCEPTION);
        this->addKey(RUNSPEC_CONNS_PER_WELL_TOO_LARGE, InputErrorAction::THROW_EXCEPTION);
        this->addKey(RUNSPEC_NUMGROUPS_TOO_LARGE, InputErrorAction::THROW_EXCEPTION);
        this->addKey(RUNSPEC_GROUPSIZE_TOO_LARGE, InputErrorAction::THROW_EXCEPTION);

        // WSEGDIMS actions (default is to emit a warning and continue)
        this->addKey(RUNSPEC_NUMMSW_TOO_LARGE, InputErrorAction::WARN);
        this->addKey(RUNSPEC_NUMSEG_PER_WELL_TOO_LARGE, InputErrorAction::WARN);
        this->addKey(RUNSPEC_NUMBRANCH_TOO_LARGE, InputErrorAction::WARN);

        addKey(UNSUPPORTED_INITIAL_THPRES, InputErrorAction::THROW_EXCEPTION);
        addKey(UNSUPPORTED_TERMINATE_IF_BHP, InputErrorAction::THROW_EXCEPTION);

        addKey(INTERNAL_ERROR_UNINITIALIZED_THPRES, InputErrorAction::THROW_EXCEPTION);

        addKey(SUMMARY_UNKNOWN_WELL, InputErrorAction::THROW_EXCEPTION);
        addKey(SUMMARY_UNKNOWN_GROUP, InputErrorAction::THROW_EXCEPTION);
        addKey(SUMMARY_UNKNOWN_NODE, InputErrorAction::WARN);
        addKey(SUMMARY_UNKNOWN_AQUIFER, InputErrorAction::THROW_EXCEPTION);
        addKey(SUMMARY_UNHANDLED_KEYWORD, InputErrorAction::WARN);
        addKey(SUMMARY_UNDEFINED_UDQ, InputErrorAction::WARN);
        addKey(SUMMARY_UDQ_MISSING_UNIT, InputErrorAction::WARN);
        this->addKey(SUMMARY_INVALID_FIPNUM, InputErrorAction::WARN);
        this->addKey(SUMMARY_EMPTY_REGION, InputErrorAction::WARN);
        this->addKey(SUMMARY_REGION_TOO_LARGE, InputErrorAction::WARN);

        addKey(ACTIONX_ILLEGAL_KEYWORD, InputErrorAction::THROW_EXCEPTION);

        addKey(RPT_MIXED_STYLE, InputErrorAction::WARN);
        addKey(RPT_UNKNOWN_MNEMONIC, InputErrorAction::WARN);

        addKey(SIMULATOR_KEYWORD_NOT_SUPPORTED, InputErrorAction::WARN);
        addKey(SIMULATOR_KEYWORD_NOT_SUPPORTED_CRITICAL, InputErrorAction::THROW_EXCEPTION);
        addKey(SIMULATOR_KEYWORD_ITEM_NOT_SUPPORTED, InputErrorAction::WARN);
        addKey(SIMULATOR_KEYWORD_ITEM_NOT_SUPPORTED_CRITICAL, InputErrorAction::THROW_EXCEPTION);

        this->addKey(UDQ_PARSE_ERROR, InputErrorAction::THROW_EXCEPTION);
        this->addKey(UDQ_TYPE_ERROR, InputErrorAction::THROW_EXCEPTION);
        this->addKey(SCHEDULE_GROUP_ERROR, InputErrorAction::THROW_EXCEPTION);
        this->addKey(SCHEDULE_IGNORED_GUIDE_RATE, InputErrorAction::WARN);
        this->addKey(SCHEDULE_WELL_IN_FIELD_GROUP, InputErrorAction::WARN);
        this->addKey(SCHEDULE_COMPSEGS_INVALID, InputErrorAction::THROW_EXCEPTION);
        this->addKey(SCHEDULE_COMPSEGS_NOT_SUPPORTED, InputErrorAction::THROW_EXCEPTION);
        addKey(SCHEDULE_INVALID_NAME, InputErrorAction::THROW_EXCEPTION);
    }

    void ParseContext::initEnv() {
        envUpdate( "OPM_ERRORS_EXCEPTION" , InputErrorAction::THROW_EXCEPTION );
        envUpdate( "OPM_ERRORS_WARN" , InputErrorAction::WARN );
        envUpdate( "OPM_ERRORS_IGNORE" , InputErrorAction::IGNORE );
        envUpdate( "OPM_ERRORS_EXIT1", InputErrorAction::EXIT1);
        envUpdate( "OPM_ERRORS_EXIT", InputErrorAction::EXIT1);
        envUpdate( "OPM_ERRORS_DELAYED_EXIT1", InputErrorAction::DELAYED_EXIT1);
        envUpdate( "OPM_ERRORS_DELAYED_EXIT", InputErrorAction::DELAYED_EXIT1);
    }


    void ParseContext::ignoreKeyword(const std::string& keyword) {
        this->ignore_keywords.insert(keyword);
    }


    void ParseContext::handleError(
            const std::string& errorKey,
            const std::string& msg_fmt,
            const std::optional<KeywordLocation>& location,
            ErrorGuard& errors) const {

        InputErrorAction action = get( errorKey );
        std::string msg = location ? OpmInputError::format(msg_fmt, *location) : msg_fmt;

        if (action == InputErrorAction::IGNORE) {
            errors.addWarning(errorKey, msg);
            return;
        }

        if (action == InputErrorAction::WARN) {
            OpmLog::warning(msg);
            errors.addWarning(errorKey, msg);
            return;
        }

        if (action == InputErrorAction::THROW_EXCEPTION) {
            OpmLog::error(msg);
            // If we decide to throw immediately - we clear the error stack to
            // make sure the error object does not terminate the application
            // when it goes out of scope.
            errors.clear();

            throw OpmInputError(msg, location.value_or(KeywordLocation{}));
        }

        if (action == InputErrorAction::EXIT1) {
            OpmLog::error(msg);
            std::cerr << "A fatal error has occurred and the application will stop." << std::endl;
            std::cerr << msg << std::endl;
            std::exit(1);
        }

        if (action == InputErrorAction::DELAYED_EXIT1) {
            OpmLog::error(msg);
            errors.addError(errorKey, msg);
            return;
        }
    }

    void ParseContext::handleUnknownKeyword(const std::string& keyword, const std::optional<KeywordLocation>& location, ErrorGuard& errors) const {
        if (this->ignore_keywords.find(keyword) == this->ignore_keywords.end()) {
            std::string msg = "Unknown keyword: " + keyword;
            this->handleError(ParseContext::PARSE_UNKNOWN_KEYWORD, msg, location, errors);
        }
    }

    std::map<std::string,InputErrorAction>::const_iterator ParseContext::begin() const {
        return m_errorContexts.begin();
    }


    std::map<std::string,InputErrorAction>::const_iterator ParseContext::end() const {
        return m_errorContexts.end();
    }

    ParseContext ParseContext::withKey(const std::string& key, InputErrorAction action) const {
        ParseContext pc(*this);
        pc.addKey(key, action);
        return pc;
    }

    ParseContext& ParseContext::withKey(const std::string& key, InputErrorAction action) {
        this->addKey(key, action);
        return *this;
    }

    bool ParseContext::hasKey(const std::string& key) const {
        if (m_errorContexts.find( key ) == m_errorContexts.end())
            return false;
        else
            return true;
    }


    void ParseContext::addKey(const std::string& key, InputErrorAction default_action) {
        if (key.find_first_of("|:*") != std::string::npos)
            throw std::invalid_argument("The ParseContext keys can not contain '|', '*' or ':'");

        if (!hasKey(key))
            m_errorContexts.insert( std::pair<std::string , InputErrorAction>( key , default_action));
    }


    InputErrorAction ParseContext::get(const std::string& key) const {
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

        parseContext.updateKey( ParseContext::PARSE_RANDOM_SLASH , InputErrorAction::IGNORE )

    */

    void ParseContext::updateKey(const std::string& key , InputErrorAction action) {
        if (hasKey(key))
            m_errorContexts[key] = action;
        else
            throw std::invalid_argument("The errormode key: " + key + " has not been registered");
    }


    void ParseContext::envUpdate( const std::string& envVariable , InputErrorAction action ) {
        const char * userSetting = getenv(envVariable.c_str());
        if (userSetting )
            update( userSetting , action);
    }


    void ParseContext::update(InputErrorAction action) {
        for (const auto& pair : m_errorContexts) {
            const std::string& key = pair.first;
            updateKey( key , action );
         }
    }


    void ParseContext::patternUpdate( const std::string& pattern , InputErrorAction action) {
        for (const auto& pair : m_errorContexts) {
            const std::string& key = pair.first;
            if (shmatch( pattern , key))
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

    void ParseContext::update(const std::string& keyString , InputErrorAction action) {
        std::vector<std::string> keys = split_string(keyString, ":|");
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
    const std::string ParseContext::PARSE_MISSING_SECTIONS = "PARSE_MISSING_SECTIONS";
    const std::string ParseContext::PARSE_MISSING_INCLUDE = "PARSE_MISSING_INCLUDE";
    const std::string ParseContext::PARSE_LONG_KEYWORD = "PARSE_LONG_KEYWORD";
    const std::string ParseContext::PARSE_WGNAME_SPACE = "PARSE_WGNAME_SPACE";
    const std::string ParseContext::PARSE_INVALID_KEYWORD_COMBINATION = "PARSE_INVALID_KEYWORD_COMBINATION";

    const std::string ParseContext::UNIT_SYSTEM_MISMATCH = "UNIT_SYSTEM_MISMATCH";

    const std::string ParseContext::RUNSPEC_NUMWELLS_TOO_LARGE = "RUNSPEC_NUMWELLS_TOO_LARGE";
    const std::string ParseContext::RUNSPEC_CONNS_PER_WELL_TOO_LARGE = "RUNSPEC_CONNS_PER_WELL_TOO_LARGE";
    const std::string ParseContext::RUNSPEC_NUMGROUPS_TOO_LARGE = "RUNSPEC_NUMGROUPS_TOO_LARGE";
    const std::string ParseContext::RUNSPEC_GROUPSIZE_TOO_LARGE = "RUNSPEC_GROUPSIZE_TOO_LARGE";

    const std::string ParseContext::RUNSPEC_NUMMSW_TOO_LARGE = "RUNSPEC_NUMMSW_TOO_LARGE";
    const std::string ParseContext::RUNSPEC_NUMSEG_PER_WELL_TOO_LARGE = "RUNSPEC_NUMSEG_PER_WELL_TOO_LARGE";
    const std::string ParseContext::RUNSPEC_NUMBRANCH_TOO_LARGE = "RUNSPEC_NUMBRANCH_TOO_LARGE";

    const std::string ParseContext::UNSUPPORTED_INITIAL_THPRES = "UNSUPPORTED_INITIAL_THPRES";
    const std::string ParseContext::UNSUPPORTED_TERMINATE_IF_BHP = "UNSUPPORTED_TERMINATE_IF_BHP";

    const std::string ParseContext::INTERNAL_ERROR_UNINITIALIZED_THPRES = "INTERNAL_ERROR_UNINITIALIZED_THPRES";

    const std::string ParseContext::SUMMARY_UNKNOWN_WELL  = "SUMMARY_UNKNOWN_WELL";
    const std::string ParseContext::SUMMARY_UNKNOWN_GROUP = "SUMMARY_UNKNOWN_GROUP";
    const std::string ParseContext::SUMMARY_UNKNOWN_NODE = "SUMMARY_UNKNOWN_NODE";
    const std::string ParseContext::SUMMARY_UNKNOWN_AQUIFER = "SUMMARY_UNKNOWN_AQUIFER";
    const std::string ParseContext::SUMMARY_UNHANDLED_KEYWORD = "SUMMARY_UNHANDLED_KEYWORD";
    const std::string ParseContext::SUMMARY_UNDEFINED_UDQ = "SUMMARY_UNDEFINED_UDQ";
    const std::string ParseContext::SUMMARY_UDQ_MISSING_UNIT = "SUMMARY_UDQ_MISSING_UNIT";
    const std::string ParseContext::SUMMARY_INVALID_FIPNUM = "SUMMARY_INVALID_FIPNUM";
    const std::string ParseContext::SUMMARY_EMPTY_REGION = "SUMMARY_EMPTY_REGION";
    const std::string ParseContext::SUMMARY_REGION_TOO_LARGE = "SUMMARY_REGION_TOO_LARGE";

    const std::string ParseContext::RPT_MIXED_STYLE = "RPT_MIXED_STYLE";
    const std::string ParseContext::RPT_UNKNOWN_MNEMONIC = "RPT_UNKNOWN_MNEMONIC";

    const std::string ParseContext::SCHEDULE_INVALID_NAME = "SCHEDULE_INVALID_NAME";
    const std::string ParseContext::ACTIONX_ILLEGAL_KEYWORD = "ACTIONX_ILLEGAL_KEYWORD";

    const std::string ParseContext::SIMULATOR_KEYWORD_NOT_SUPPORTED = "SIMULATOR_KEYWORD_NOT_SUPPORTED";
    const std::string ParseContext::SIMULATOR_KEYWORD_NOT_SUPPORTED_CRITICAL = "SIMULATOR_KEYWORD_NOT_SUPPORTED_CRITICAL";
    const std::string ParseContext::SIMULATOR_KEYWORD_ITEM_NOT_SUPPORTED = "SIMULATOR_KEYWORD_ITEM_NOT_SUPPORTED";
    const std::string ParseContext::SIMULATOR_KEYWORD_ITEM_NOT_SUPPORTED_CRITICAL = "SIMULATOR_KEYWORD_ITEM_NOT_SUPPORTED_CRITICAL";

    const std::string ParseContext::UDQ_PARSE_ERROR = "UDQ_PARSE_ERROR";
    const std::string ParseContext::UDQ_TYPE_ERROR = "UDQ_TYPE_ERROR";
    const std::string ParseContext::SCHEDULE_GROUP_ERROR = "SCHEDULE_GROUP_ERROR";
    const std::string ParseContext::SCHEDULE_IGNORED_GUIDE_RATE = "SCHEDULE_IGNORED_GUIDE_RATE";
    const std::string ParseContext::SCHEDULE_WELL_IN_FIELD_GROUP = "SCHEDULE_WELL_IN_FIELD_GROUP";

    const std::string ParseContext::SCHEDULE_COMPSEGS_INVALID = "SCHEDULE_COMPSEG_INVALID";
    const std::string ParseContext::SCHEDULE_COMPSEGS_NOT_SUPPORTED = "SCHEDULE_COMPSEGS_NOT_SUPPORTED";
}
