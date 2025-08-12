/*
  Copyright 2025 Equinor
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
#include <opm/input/eclipse/EclipseState/Grid/AutoRefinement.hpp>
#include <opm/input/eclipse/EclipseState/Grid/AutoRefManager.hpp>

#include <memory>

namespace Opm {

AutoRefManager::AutoRefManager()
{}

const AutoRefinement& AutoRefManager::getAutoRef() const
{   // Add check nullptr
    return *m_keywordAutoRef;
}

void AutoRefManager::readKeywordAutoRef(int nx,
                                        int ny,
                                        int nz,
                                        double option_trans_mult)
{
    this->m_keywordAutoRef = std::make_unique<AutoRefinement>(nx, ny, nz, option_trans_mult);
}
} // namespace Opm
