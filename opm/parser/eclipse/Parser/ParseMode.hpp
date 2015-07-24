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


#ifndef OPM_PARSE_MODE_HPP
#define OPM_PARSE_MODE_HPP

#include <opm/parser/eclipse/Parser/InputErrorAction.hpp>

namespace Opm {


    /*
       The ParseMode struct is meant to control the behavior of the
       parsing and EclipseState construction phase when
       errors/inconsistencies/... are encountered in the input.

       The ParseMode struct should be used as a simple value object,
       i.e. apart from the constructor there are no methods - the
       object can not 'do anything'. It is perfectly legitimate for
       calling scope to manipulate the fields of a parsemode instance
       directly.

       For each of the possible problems encountered the possible
       actions are goverened by the InputError::Action enum:

          InputError::THROW_EXCEPTION
          InputError::WARN
          InputError::IGNORE

    */

    struct ParseMode {
        ParseMode();
        InputError::Action unknownKeyword;
    };
}


#endif
