/*
  Copyright 2015 IRIS
  
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

#ifndef NNC_HPP
#define NNC_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <vector>

namespace Opm
{
class NNC
{
public:
    /// Construct from input deck.
    NNC(Opm::DeckConstPtr deck, EclipseGridConstPtr eclipseGrid);
    void addNNC(const size_t NNC1, const size_t NNC2, const double trans);
    const std::vector<size_t>& nnc1() const { return m_nnc1; }
    const std::vector<size_t>& nnc2() const { return m_nnc2; }
    const std::vector<double>& trans() const { return m_trans; }
    size_t numNNC() const;
    bool hasNNC() const;

private:
    std::vector<size_t> m_nnc1;
    std::vector<size_t> m_nnc2;
    std::vector<double> m_trans;
};


} // namespace Opm


#endif // NNC_HPP
