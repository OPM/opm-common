/*
  Copyright (C) 2020 Equinor ASA
  Copyright (C) 2020 SINTEF Digital

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  ?You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPM_AQUCON_HPP
#define OPM_AQUCON_HPP

#include <opm/parser/eclipse/EclipseState/Grid/FaceDir.hpp>
// #include <unordered_map>
#include <map>
#include <array>

namespace Opm {

    class EclipseGrid;
    class Deck;
    class DeckRecord;

    struct NumAquiferCon {
        int aquifer_id;
        // TODO: maybe it is okay to use global_index
        int I, J, K;
        FaceDir::DirEnum face_dir;
        double trans_multipler = 1.0;
        int trans_option = 0;
        bool connect_active_cell = false;
        // The following are
        double ve_frac_relperm = 1.0;
        double ve_frac_cappress = 1.0;

        static std::vector<NumAquiferCon> generateConnections(const EclipseGrid&, const DeckRecord&);
    };

    class NumericalAquiferConnections {
    public:
        // TODO: maybe just easier to use a global_index here
        using CellIndex = std::array<int, 3>;
        NumericalAquiferConnections(const Deck& deck, const EclipseGrid& grid);
        // TODO: might be removed later.
        // It is possible when without this function, unordered_map problem can be fixed
        NumericalAquiferConnections() = default;

        const std::map<CellIndex, NumAquiferCon>& getConnections(const int aqu_id) const;

    private:
        // TODO: compilation failure with_unordered_map
        // basically due to  implicitly-deleted default constructor of 'std::unordered_map
        // std::unordered_map<int, std::unordered_map<CellIndex, NumAquiferCon>> connections_;
        std::map<int, std::map<CellIndex, NumAquiferCon>> connections_;
    };
}
#endif //OPM_AQUCON_HPP
