/*
  Copyright 2013 Statoil ASA.

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

#include <opm/input/eclipse/Parser/ParserEnums.hpp>

#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace Opm {

    /*****************************************************************/

    std::string ParserKeywordSizeEnum2String(ParserKeywordSizeEnum enumValue)
    {
        const auto enumMap = std::map<ParserKeywordSizeEnum, std::string> {
            { ParserKeywordSizeEnum::SLASH_TERMINATED       , "SLASH_TERMINATED"        },
            { ParserKeywordSizeEnum::FIXED                  , "FIXED"                   },
            { ParserKeywordSizeEnum::OTHER_KEYWORD_IN_DECK  , "OTHER_KEYWORD_IN_DECK"   },
            { ParserKeywordSizeEnum::UNKNOWN                , "UNKNOWN"                 },
            { ParserKeywordSizeEnum::FIXED_CODE             , "FIXED_CODE"              },
            { ParserKeywordSizeEnum::DOUBLE_SLASH_TERMINATED, "DOUBLE_SLASH_TERMINATED" },
        };

        auto enumPos = enumMap.find(enumValue);
        if (enumPos == enumMap.end()) {
            throw std::invalid_argument("Implementation error - should NOT be here");
        }

        return enumPos->second;
    }

    ParserKeywordSizeEnum ParserKeywordSizeEnumFromString(const std::string& stringValue)
    {
        const auto enumMap = std::unordered_map<std::string, ParserKeywordSizeEnum> {
            { "SLASH_TERMINATED"       , ParserKeywordSizeEnum::SLASH_TERMINATED        },
            { "FIXED"                  , ParserKeywordSizeEnum::FIXED                   },
            { "OTHER_KEYWORD_IN_DECK"  , ParserKeywordSizeEnum::OTHER_KEYWORD_IN_DECK   },
            { "UNKNOWN"                , ParserKeywordSizeEnum::UNKNOWN                 },
            { "FIXED_CODE"             , ParserKeywordSizeEnum::FIXED_CODE              },
            { "DOUBLE_SLASH_TERMINATED", ParserKeywordSizeEnum::DOUBLE_SLASH_TERMINATED },
        };

        auto enumPos = enumMap.find(stringValue);
        if (enumPos == enumMap.end()) {
            throw std::invalid_argument {
                "String: " + stringValue + " is not a supported size-type enumerator"
            };
        }

        return enumPos->second;
    }

    /*****************************************************************/

    std::string ParserKeywordActionEnum2String(ParserKeywordActionEnum enumValue)
    {
        const auto enumMap = std::map <ParserKeywordActionEnum, std::string> {
            { ParserKeywordActionEnum::INTERNALIZE    , "INTERNALIZE"     },
            { ParserKeywordActionEnum::IGNORE         , "IGNORE"          },
            { ParserKeywordActionEnum::IGNORE_WARNING , "IGNORE_WARNING"  },
            { ParserKeywordActionEnum::THROW_EXCEPTION, "THROW_EXCEPTION" },
        };

        auto enumPos = enumMap.find(enumValue);
        if (enumPos == enumMap.end()) {
            throw std::invalid_argument("Implementation error - should NOT be here");
        }

        return enumPos->second;
    }

    ParserKeywordActionEnum ParserKeywordActionEnumFromString(const std::string& stringValue)
    {
        const auto enumMap = std::unordered_map <std::string, ParserKeywordActionEnum> {
            { "INTERNALIZE"    , ParserKeywordActionEnum::INTERNALIZE     },
            { "IGNORE"         , ParserKeywordActionEnum::IGNORE          },
            { "IGNORE_WARNING" , ParserKeywordActionEnum::IGNORE_WARNING  },
            { "THROW_EXCEPTION", ParserKeywordActionEnum::THROW_EXCEPTION },
        };

        auto enumPos = enumMap.find(stringValue);
        if (enumPos == enumMap.end()) {
            throw std::invalid_argument {
                "String: " + stringValue + " is not a supported action-type enumerator"
            };
        }

        return enumPos->second;
    }


}
