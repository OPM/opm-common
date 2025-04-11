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

#include <opm/input/eclipse/Schedule/Well/WellFractureSeeds.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <numeric>
#include <vector>

bool Opm::WellFractureSeeds::updateSeed(const std::size_t   seedCellGlobal,
                                        const NormalVector& seedNormal)
{
    const auto ix = this->seedIndex(seedCellGlobal);

    if (ix == this->seedCell_.size()) {
        return this->insertNewSeed(seedCellGlobal, seedNormal);
    }
    else {
        return this->updateExistingSeed(ix, seedNormal);
    }
}

void Opm::WellFractureSeeds::finalizeSeeds()
{
    this->establishLookup();
}

const Opm::WellFractureSeeds::NormalVector*
Opm::WellFractureSeeds::getNormal(const SeedCell& c) const
{
    const auto ix = this->seedIndex(c.c);

    if (ix == this->seedNormal_.size()) {
        return nullptr;
    }
    else {
        return &this->seedNormal_[ix];
    }
}

bool Opm::WellFractureSeeds::operator==(const WellFractureSeeds& that) const
{
    return (this->wellName_ == that.wellName_)
        && (this->seedCell_ == that.seedCell_)
        && (this->seedNormal_ == that.seedNormal_)
        && (this->lookup_ == that.lookup_)
        ;
}

Opm::WellFractureSeeds Opm::WellFractureSeeds::serializationTestObject()
{
    auto s = WellFractureSeeds { "testwell" };

    s.seedCell_.push_back(1729);
    s.seedNormal_.push_back({ 1.1, -2.2, 3.3 });
    s.lookup_.push_back(0);

    return s;
}

// ===========================================================================
// Private member functions of class WellFractureSeeds below separator
// ===========================================================================

void Opm::WellFractureSeeds::establishLookup()
{
    this->lookup_.assign(this->seedNormal_.size(), NormalVectorIx{});
    std::iota(this->lookup_.begin(), this->lookup_.end(), NormalVectorIx{});

    std::sort(this->lookup_.begin(), this->lookup_.end(),
              [this](const auto i1, const auto i2)
              { return this->seedCell_[i1] < this->seedCell_[i2]; });
}

Opm::WellFractureSeeds::NormalVectorIx
Opm::WellFractureSeeds::seedIndex(const std::size_t seedCellGlobal) const
{
    assert (this->seedCell_.size() == this->seedNormal_.size());

    if (this->lookup_.size() == this->seedNormal_.size()) {
        return this->seedIndexBinarySearch(seedCellGlobal);
    }
    else {
        return this->seedIndexLinearSearch(seedCellGlobal);
    }
}

Opm::WellFractureSeeds::NormalVectorIx
Opm::WellFractureSeeds::seedIndexBinarySearch(const std::size_t seedCellGlobal) const
{
    assert (this->lookup_.size() == this->seedCell_.size());

    auto ixPos = std::lower_bound(this->lookup_.begin(),
                                  this->lookup_.end(),
                                  seedCellGlobal,
                                  [this](const auto ix, const std::size_t search)
                                  { return this->seedCell_[ix] < search; });

    if ((ixPos == this->lookup_.end()) ||
        (this->seedCell_[*ixPos] != seedCellGlobal))
    {
        return this->seedCell_.size();
    }
    else {
        return *ixPos;
    }
}

Opm::WellFractureSeeds::NormalVectorIx
Opm::WellFractureSeeds::seedIndexLinearSearch(const std::size_t seedCellGlobal) const
{
    auto cellPos = std::find(this->seedCell_.begin(),
                             this->seedCell_.end(),
                             seedCellGlobal);

    return (cellPos == this->seedCell_.end())
        ? this->seedCell_.size()
        : std::distance(this->seedCell_.begin(), cellPos);
}

bool Opm::WellFractureSeeds::insertNewSeed(const std::size_t   seedCellGlobal,
                                           const NormalVector& seedNormal)
{
    this->seedCell_.push_back(seedCellGlobal);
    this->seedNormal_.push_back(seedNormal);
    this->lookup_.clear();

    return true;
}

bool Opm::WellFractureSeeds::updateExistingSeed(const NormalVectorIx ix,
                                                const NormalVector&  seedNormal)
{
    const auto isDifferent = this->seedNormal_[ix] != seedNormal;

    this->seedNormal_[ix] = seedNormal;

    return isDifferent;
}
