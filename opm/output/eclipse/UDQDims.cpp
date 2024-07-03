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

#include <opm/output/eclipse/UDQDims.hpp>

#include <opm/output/eclipse/VectorItems/intehead.hpp>

#include <opm/input/eclipse/Schedule/UDQ/UDQConfig.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace VI = ::Opm::RestartIO::Helpers::VectorItems;

Opm::UDQDims::UDQDims(const UDQConfig&        config,
                      const std::vector<int>& inteHead)
    : totalNumUDQs_ { config.size() }
    , intehead_     { std::cref(inteHead) }
{}

std::size_t Opm::UDQDims::totalNumUDQs() const
{
    return this->totalNumUDQs_;
}

std::size_t Opm::UDQDims::numIUAD() const
{
    return this->intehead(VI::intehead::NO_IUADS);
}

std::size_t Opm::UDQDims::numIGPH() const
{
    return (this->numIUAD() > 0)
        ? this->intehead(VI::intehead::NGMAXZ)
        : std::size_t{0};
}

std::size_t Opm::UDQDims::numIUAP() const
{
    return this->intehead(VI::intehead::NO_IUAPS);
}

std::size_t Opm::UDQDims::numFieldUDQs() const
{
    return this->intehead(VI::intehead::NO_FIELD_UDQS);
}

std::size_t Opm::UDQDims::maxNumGroups() const
{
    return this->intehead(VI::intehead::NGMAXZ);
}

std::size_t Opm::UDQDims::numGroupUDQs() const
{
    return this->intehead(VI::intehead::NO_GROUP_UDQS);
}

std::size_t Opm::UDQDims::maxNumWells() const
{
    return this->intehead(VI::intehead::NWMAXZ);
}

std::size_t Opm::UDQDims::numWellUDQs() const
{
    return this->intehead(VI::intehead::NO_WELL_UDQS);
}

void Opm::UDQDims::collectDimensions() const
{
    this->dimensionData_.emplace(13, 0);

    (*this->dimensionData_)[0] = this->totalNumUDQs();
    (*this->dimensionData_)[1] = entriesPerIUDQ();
    (*this->dimensionData_)[2] = this->numIUAD();
    (*this->dimensionData_)[3] = entriesPerIUAD();
    (*this->dimensionData_)[4] = entriesPerZUDN();
    (*this->dimensionData_)[5] = entriesPerZUDL();

    (*this->dimensionData_)[6] = this->numIGPH();
    (*this->dimensionData_)[7] = this->numIUAP();

    (*this->dimensionData_)[8] = this->maxNumWells();
    (*this->dimensionData_)[9] = this->numWellUDQs();

    (*this->dimensionData_)[10] = this->maxNumGroups();
    (*this->dimensionData_)[11] = this->numGroupUDQs();

    (*this->dimensionData_)[12] = this->numFieldUDQs();
}

std::size_t Opm::UDQDims::intehead(const std::vector<int>::size_type i) const
{
    return this->intehead_.get()[i];
}
