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
    void addNNC(const int NNC1, const int NNC2, const double trans);
    const std::vector<int>& nnc1() const { return nnc1_; }
    const std::vector<int>& nnc2() const { return nnc2_; }
    const std::vector<double>& trans() const { return trans_; }
    int numNNC();
    bool hasNNC();

private:
    std::vector<int> nnc1_;
    std::vector<int> nnc2_;
    std::vector<double> trans_;
    bool hasNNC_;
};


} // namespace Opm


#endif // NNC_HPP
