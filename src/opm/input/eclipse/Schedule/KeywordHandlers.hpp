/*
  Copyright 2020 Statoil ASA.

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
#ifndef KEYWORD_HANDLERS_HPP
#define KEYWORD_HANDLERS_HPP

#include <functional>
#include <string>
#include <unordered_map>

namespace Opm {

class HandlerContext;

//! \brief Singleton class for Keyword handlers in Schedule.
class KeywordHandlers
{
public:
    using handler_function = std::function<void(HandlerContext&)>; //!< Handler function type

    //! \brief Obtain singleton instance.
    static const KeywordHandlers& getInstance();

    //! \brief Handle a keyword.
    bool handleKeyword(HandlerContext& handlerContext) const;

private:
    //! \brief The constructor creates the list of keyword handler functions.
    KeywordHandlers();

    using HandlerFunctionMap = std::unordered_map<std::string,handler_function>; //!< Map of handler functions
    HandlerFunctionMap handler_functions; //!< Registered handler functions
};

}

#endif
