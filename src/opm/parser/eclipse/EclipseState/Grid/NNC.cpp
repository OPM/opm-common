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
#include <array>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridDims.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/NNC.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/N.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/E.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>

#include <sstream>

namespace Opm
{

template<typename T, typename F1, typename F2>
    void processNncs(const T& nncs, const GridDims& gridDims, F1 f1, F2 f2);

    NNC::NNC(const Deck& deck) {
        GridDims gridDims(deck);
        const auto& nncs = deck.getKeywordList<ParserKeywords::NNC>();
        const auto& tmpEditNncs = deck.getKeywordList<ParserKeywords::EDITNNC>();
        std::vector<NNCdata> editNncs;
        editNncs.reserve(tmpEditNncs.size());
        processNncs(tmpEditNncs, gridDims,
                    [&editNncs](size_t i, size_t j, double t) { editNncs.emplace_back(i,j,t); },
                    [](const DeckItem& i) {return i.get<double>(0); });
        auto compare = [](const NNCdata& d1, const NNCdata& d2)
            { return d1.cell1 < d2.cell1 ||
              ( d1.cell1 == d2.cell1 && d1.cell2 < d2.cell2 );};
        std::sort(editNncs.begin(), editNncs.end(), compare);
        processNncs(nncs, gridDims,
                    [this](size_t i, size_t j, double t) { this->addNNC(i, j, t); },
                    [](const DeckItem& i) {return i.getSIDouble(0); });
        std::sort(m_nnc.begin(), m_nnc.end(), compare);

        std::ostringstream warning;
        warning<<"The following NNC entries in EDITNNC have been ignored: ";
        bool        ignored = false;
        std::size_t counter = 0;
        // Assuming that we edit less nncs than we have
        auto nncIt = m_nnc.begin();
        for( const auto& edit: editNncs )
        {
            auto candidate = std::lower_bound(nncIt, m_nnc.end(), edit, compare);
            bool processed = false;

            for( auto next=candidate; next != m_nnc.end() && 
                     ( next->cell1 == edit.cell1 && next->cell2 == edit.cell2 );
                 ++next)
            {
                next->trans *= edit.trans;
                processed    = true;
            }

            if ( ! processed )
            {
                warning << edit.cell1 << "->" << edit.cell2 << " ";
                ignored = true;
            }
            ++counter;

            if ( candidate != m_nnc.end() )
            {
                // There could be another edit for the same cell, start over.
                nncIt = candidate;
            }
            else
            {
                // for all other editnnc there will be no nnc.
                break;
            }
        }
        if ( counter < editNncs.size() )
        {
            ignored = true;
            for(auto nnc = editNncs.begin() + counter, end = editNncs.end(); nnc != end; ++nnc)
            {
                warning << nnc->cell1 << "->" << nnc->cell2 << " ";
            }
        }
        if ( ignored )
        {
            OpmLog::warning(warning.str());
        }
    }

template<typename T, typename F1, typename F2>
void processNncs(const T& nncs, const GridDims& gridDims, F1 f1, F2 f2)
    {
        for (size_t idx_nnc = 0; idx_nnc<nncs.size(); ++idx_nnc) {
            const auto& nnc = *nncs[idx_nnc];
            for (size_t i = 0; i < nnc.size(); ++i) {
                std::array<size_t, 3> ijk1;
                ijk1[0] = static_cast<size_t>(nnc.getRecord(i).getItem(0).template get< int >(0)-1);
                ijk1[1] = static_cast<size_t>(nnc.getRecord(i).getItem(1).template get< int >(0)-1);
                ijk1[2] = static_cast<size_t>(nnc.getRecord(i).getItem(2).template get< int >(0)-1);
                size_t global_index1 = gridDims.getGlobalIndex(ijk1[0],ijk1[1],ijk1[2]);
                
                std::array<size_t, 3> ijk2;
                ijk2[0] = static_cast<size_t>(nnc.getRecord(i).getItem(3).template get< int >(0)-1);
                ijk2[1] = static_cast<size_t>(nnc.getRecord(i).getItem(4).template get< int >(0)-1);
                ijk2[2] = static_cast<size_t>(nnc.getRecord(i).getItem(5).template get< int >(0)-1);
                size_t global_index2 = gridDims.getGlobalIndex(ijk2[0],ijk2[1],ijk2[2]);
                
                const double trans = f2(nnc.getRecord(i).getItem(6));
                
                f1(global_index1, global_index2, trans);
            }
        }
    }
    void NNC::addNNC(const size_t cell1, const size_t cell2, const double trans) {
        NNCdata tmp;
        tmp.cell1 = cell1;
        tmp.cell2 = cell2;
        tmp.trans = trans;
        m_nnc.push_back(tmp);
    }

    size_t NNC::numNNC() const {
        return(m_nnc.size());
    }

    bool NNC::hasNNC() const {
        return m_nnc.size()>0;
    }

} // namespace Opm

