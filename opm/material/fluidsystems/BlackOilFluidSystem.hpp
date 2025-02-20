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
#define COMPILING_STATIC_FLUID_SYSTEM
#define STATIC_OR_DEVICE static
#define STATIC_OR_NOTHING static
#define FLUIDSYSTEM_CLASSNAME_NONSTATIC BlackOilFluidSystemNonStatic
#define FLUIDSYSTEM_CLASSNAME_STATIC BlackOilFluidSystem
#define FLUIDSYSTEM_CLASSNAME BlackOilFluidSystem
namespace Opm
{
template <class Scalar,
          class IndexTraits,
          template <typename> typename Storage,
          template <typename> typename SmartPointer>
class FLUIDSYSTEM_CLASSNAME_NONSTATIC;
}
#include <opm/material/fluidsystems/BlackOilFluidSystem_macrotemplate.hpp>
#undef STATIC_OR_DEVICE
#undef COMPILING_STATIC_FLUID_SYSTEM
#undef STATIC_OR_NOTHING
#undef FLUIDSYSTEM_CLASSNAME_STATIC
#undef FLUIDSYSTEM_CLASSNAME_NONSTATIC
#undef FLUIDSYSTEM_CLASSNAME
#endif // OPM_BLACK_OIL_FLUID_SYSTEM_HPP
