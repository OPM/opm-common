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
#ifndef OPM_BLACK_OIL_FLUID_SYSTEM_NONSTATIC_HPP
#define OPM_BLACK_OIL_FLUID_SYSTEM_NONSTATIC_HPP

#include <opm/common/utility/gpuDecorators.hpp>


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

// Nonstatic class name
#define FLUIDSYSTEM_CLASSNAME_NONSTATIC BlackOilFluidSystemNonStatic

// Static class name
#define FLUIDSYSTEM_CLASSNAME_STATIC BlackOilFluidSystem

// Fluid system class name for the fluid system we are defining now, ie the nonstatic version
#define FLUIDSYSTEM_CLASSNAME BlackOilFluidSystemNonStatic

// We need to decorate the nonstatic member functions with the gpu decorators
#define STATIC_OR_DEVICE OPM_HOST_DEVICE

// Member variables need no decorators for the nonstatic version
#define STATIC_OR_NOTHING


// We need to forward-declare the static version of the fluid system, since we will
// add it as a friend to the nonstatic version.
namespace Opm
{
template <class Scalar,
          class IndexTraits,
          template <typename> typename Storage,
          template <typename> typename SmartPointer>
class FLUIDSYSTEM_CLASSNAME_STATIC;
}

// Include the macrotemplate file
#include <opm/material/fluidsystems/BlackOilFluidSystem_macrotemplate.hpp>

// Undefine the macros we defined above
#undef STATIC_OR_DEVICE
#undef STATIC_OR_NOTHING
#undef FLUIDSYSTEM_CLASSNAME_NONSTATIC
#undef FLUIDSYSTEM_CLASSNAME_STATIC
#undef FLUIDSYSTEM_CLASSNAME
#endif
