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
        size_t aquifer_id;
        size_t I, J, K;
        size_t global_index;
        FaceDir::DirEnum face_dir;
        double trans_multipler;
        int trans_option;
        bool connect_active_cell;
        // The following are options related to VE simulation
        double ve_frac_relperm;
        double ve_frac_cappress;

        static std::vector<NumAquiferCon> generateConnections(const EclipseGrid&, const DeckRecord&, const std::vector<int>& actnum);
    };

    class NumericalAquiferConnections {
    public:
        NumericalAquiferConnections(const Deck& deck, const EclipseGrid& grid, const std::vector<int>& actnum);
        const std::map<size_t, NumAquiferCon>& getConnections(const size_t aqu_id) const;

    private:
        std::map<size_t, std::map<size_t, NumAquiferCon>> connections_;
    };
}
#endif //OPM_AQUCON_HPP
