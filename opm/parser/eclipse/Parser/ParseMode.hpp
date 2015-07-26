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

        /*
          The unknownKeyword field regulates how the parser should
          react when it encounters an unknwon keyword. Observe that
          'keyword' in this context means:

             o A string of 8 characters or less - starting in column
               0.

             o A string consisiting of UPPERCASE characters and
               numerals, staring with an UPPERCASE character [Hmmm -
               actually lowercase is also accepted?!]

           Observe that unknownKeyword does *not* consult any global
           collection of keywords to see if a particular string
           corresponds to a known valid keyword which we just happen
           to ignore for this particualar parse operation.

           The 'unknownkeyword' and 'randomText' error situations are
           not fully orthogonal, and in particualar if a unknown
           keyword has been encountered - without halting the parser, a
           subsequent piece of 'random text' might not be identified
           correctly as such.
        */
        InputError::Action unknownKeyword;


        /*
          With random text we mean a string in the input deck is not
          correctly formatted as a keyword heading.
        */
        InputError::Action randomText;


        /*
          For some keywords the number of records (i.e. size) is given
          as an item in another keyword. A typical example is the
          EQUIL keyword where the number of records is given by the
          NTEQUL item of the EQLDIMS keyword. If the size defining
          XXXDIMS keyword is not in the deck, we can use the default
          values of the XXXDIMS keyword; this is regulated by the
          'missingDIMskeyword' field.

          Observe that a fully defaulted XXXDIMS keyword does not
          trigger this behavior.
        */
        InputError::Action missingDIMSKeyword;

        /*
          Some property modfiers can be modified in the Schedule
          section; this effectively means that Eclipse supports time
          dependent geology. This is marked as an exocit special
          feature in Eclipse, and not supported at all in the
          EclipseState object of opm-parser. If these modifiers are
          encountered in the Schedule section the behavior is
          regulated by this setting.
        */
        InputError::Action unsupportedScheduleGeoModifiers;
    };
}


#endif
