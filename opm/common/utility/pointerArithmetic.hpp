/*
  Copyright 2025 Equinor ASA.

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

namespace Opm {

    // Utility function motivated by for instance computing GPU pointers from the cpu without entering a kernel
    // Given a Buffer A with a starting pointer and a pointer into the buffer, compute the pointer with the same offset from the start of a buffer B
    template <class PtrType>
    PtrType* ComputePtrBasedOnOffsetInOtherBuffer(PtrType* bufBStart, size_t bufBLength, PtrType* bufAStart, size_t bufALength, PtrType* ptrInA)
    {
        if (bufAStart == nullptr || bufBStart == nullptr || ptrInA == nullptr) {
            throw std::invalid_argument("ComputePtrBasedOnOffsetInOtherBuffer: One or more input pointers are null.");
        }

        auto offset = ptrInA - bufAStart;

        if (offset < 0 || static_cast<size_t>(offset) >= bufALength) {
            throw std::out_of_range("ComputePtrBasedOnOffsetInOtherBuffer: Pointer into A is out of range.");
        }

        auto res = bufBStart + offset;
        return res;
    }

} // namespace Opm
