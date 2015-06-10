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
#include "NNC.hpp"
#include <array>

namespace Opm
{
    NNC::NNC(Opm::DeckConstPtr deck, EclipseGridConstPtr eclipseGrid)
    {
        if (deck->hasKeyword("NNC")) {
            const std::vector<DeckKeywordConstPtr>& nncs = deck->getKeywordList("NNC");
            for (size_t idx_nnc = 0; idx_nnc<nncs.size(); ++idx_nnc) {
                Opm::DeckKeywordConstPtr nnc = nncs[idx_nnc];
                size_t numNNC = nnc->size();
                nnc1_.reserve(numNNC);
                nnc2_.reserve(numNNC);
                trans_.reserve(numNNC);

                for (size_t i = 0; i<numNNC; ++i) {
                    std::array<int, 3> xyz1;
                    xyz1[0] = nnc->getRecord(i)->getItem(0)->getInt(0)-1;
                    xyz1[1] = nnc->getRecord(i)->getItem(1)->getInt(0)-1;
                    xyz1[2] = nnc->getRecord(i)->getItem(2)->getInt(0)-1;
                    size_t global_index1 = eclipseGrid->getGlobalIndex(xyz1[0],xyz1[1],xyz1[2]);
                    nnc1_.push_back(global_index1);

                    std::array<int, 3> xyz2;
                    xyz2[0] = nnc->getRecord(i)->getItem(3)->getInt(0)-1;
                    xyz2[1] = nnc->getRecord(i)->getItem(4)->getInt(0)-1;
                    xyz2[2] = nnc->getRecord(i)->getItem(5)->getInt(0)-1;
                    size_t global_index2 = eclipseGrid->getGlobalIndex(xyz2[0],xyz2[1],xyz2[2]);
                    nnc2_.push_back(global_index2);

                    const double trans = nnc->getRecord(i)->getItem(6)->getSIDouble(0);

                    trans_.push_back(trans);
                    //std::cout << trans << std::endl;
                }
            }
        }
    }

    void NNC::addNNC(const int NNC1, const int NNC2, const double trans) {
        nnc1_.push_back(NNC1);
        nnc2_.push_back(NNC2);
        trans_.push_back(trans);
    }

    size_t NNC::numNNC() const {
        return(nnc1_.size());
    }

    bool NNC::hasNNC() const {
        return nnc1_.size()>0;
    }


} // namespace Opm

