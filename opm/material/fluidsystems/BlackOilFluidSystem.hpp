/*
  Copyright 2025 Equinor ASA

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
#ifndef OPM_BLACK_OIL_FLUID_SYSTEM_HPP
#define OPM_BLACK_OIL_FLUID_SYSTEM_HPP

// Here we need to define certain macros before including the macrotemplate file.
// 
// The idea is in essence the following:
//   1) In the macrotemplate file, we have method declarations of the form
//      
//        `STATIC_OR_DEVICE void foo();`
//      
//      and member variable declarations of the form
//
//        `STATIC_OR_NOTHING int bar;`
//
//   2) We want to be able to compile the same code for both the dynamic (non-static) and static
//      versions of the fluid system. The dynamic version is used when the fluid system is accessed
//      from the GPU, while the static version is used when the fluid system is accessed from the CPU.
//   3) We want to be able to compile the same code for both the dynamic and static versions of the
//      fluid system, but with different method and member variable declarations.
//
// Furthermore, we need to specify the class name of the fluid system, which is different for the
// nonstatic and static versions of the fluid system. We also need to specify if we are compiling
// the static version of the fluid system, since we will define certain constructors and singleton
// functions only in the static or nonstatic case. 

// Is defined for the static version of the fluid system.
#define COMPILING_STATIC_FLUID_SYSTEM

// The static version does not need any gpu decorators, simply static
#define STATIC_OR_DEVICE static

// Make sure member variables are declared as static
#define STATIC_OR_NOTHING static

// Define the class names for the static and nonstatic versions of the fluid system
#define FLUIDSYSTEM_CLASSNAME_NONSTATIC BlackOilFluidSystemNonStatic
#define FLUIDSYSTEM_CLASSNAME_STATIC BlackOilFluidSystem

// Define the class name for the fluid system
#define FLUIDSYSTEM_CLASSNAME BlackOilFluidSystem


// We need to forward-declare the nonstatic version of the fluid system, since we will 
// make the nonstatic version a friend of the static version being defined here.
namespace Opm
{
template <class Scalar,
          class IndexTraits,
          template <typename> typename Storage,
          template <typename> typename SmartPointer>
class FLUIDSYSTEM_CLASSNAME_NONSTATIC;
}

// Include the macrotemplate file
#include <opm/material/fluidsystems/BlackOilFluidSystem_macrotemplate.hpp>

// Undefine the macros we defined above
#undef STATIC_OR_DEVICE
#undef COMPILING_STATIC_FLUID_SYSTEM
#undef STATIC_OR_NOTHING
#undef FLUIDSYSTEM_CLASSNAME_STATIC
#undef FLUIDSYSTEM_CLASSNAME_NONSTATIC
#undef FLUIDSYSTEM_CLASSNAME

#endif // OPM_BLACK_OIL_FLUID_SYSTEM_HPP
