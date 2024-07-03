/*
  Copyright (c) 2021 Equinor ASA

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

#ifndef OPM_UDQDIMS_HPP
#define OPM_UDQDIMS_HPP

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace Opm {

class UDQConfig;

} // namespace Opm

namespace Opm {

class UDQDims
{
public:
    explicit UDQDims(const UDQConfig& config, const std::vector<int>& intehead);

    static std::size_t entriesPerIUDQ() { return  3; }
    static std::size_t entriesPerIUAD() { return  5; }
    static std::size_t entriesPerZUDN() { return  2; }
    static std::size_t entriesPerZUDL() { return 16; }

    std::size_t totalNumUDQs() const;
    std::size_t numIUAD() const;
    std::size_t numIGPH() const;
    std::size_t numIUAP() const;

    std::size_t numFieldUDQs() const;

    std::size_t maxNumGroups() const;
    std::size_t numGroupUDQs() const;

    std::size_t maxNumWells() const;
    std::size_t numWellUDQs() const;

    const std::vector<int>& data() const
    {
        if (! this->dimensionData_.has_value()) {
            this->collectDimensions();
        }

        return *this->dimensionData_;
    }

private:
    std::size_t totalNumUDQs_{};
    std::reference_wrapper<const std::vector<int>> intehead_;

    mutable std::optional<std::vector<int>> dimensionData_;

    void collectDimensions() const;

    std::size_t intehead(const std::vector<int>::size_type i) const;
};

} // namespace Opm

#endif // OPM_UDQDIMS_HPP
