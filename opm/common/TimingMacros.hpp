/*
  Copyright 2023 SINTEF Digital

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
#ifndef OPM_TIMINGMACROS_HPP
#define OPM_TIMINGMACROS_HPP

// macros used to time blocks for example with tracy
// time block of main part of codes which do not effect performance
#ifndef OPM_TIMEBLOCK
#define OPM_TIMEBLOCK(x)\
    do { /* nothing */ } while (false)
#endif

// detailed timing which may effect performance
#ifndef OPM_TIMEBLOCK_LOCAL
#define OPM_TIMEBLOCK_LOCAL(x)\
    do { /* nothing */ } while (false)
#endif

#endif // OPM_TIMINGMACROS_HPP
