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

#ifndef OPM_COMMON_VECTOR_WITH_DEFAULT_ALLOCATOR_HPP
#define OPM_COMMON_VECTOR_WITH_DEFAULT_ALLOCATOR_HPP
namespace Opm
{
// NVCC being weird about std::vector, so we need this workaround.
// In essence, one can not use std::vector as a default template<typename> typename argument.
template <class T>
using VectorWithDefaultAllocator = std::vector<T, std::allocator<T>>;
} // namespace Opm
#endif
