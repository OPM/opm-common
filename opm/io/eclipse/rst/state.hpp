/*
  Copyright 2020 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RST_STATE
#define RST_STATE

#include <vector>

#include <opm/io/eclipse/rst/group.hpp>
#include <opm/io/eclipse/rst/well.hpp>
#include <opm/io/eclipse/PaddedOutputString.hpp>

namespace Opm {
namespace RestartIO {
struct state {
    state(const std::vector<int>& intehead,
          const std::vector<bool>& logihead,
          const std::vector<double>& doubhead,
          const std::vector<EclIO::PaddedOutputString<8>>& zgrp,
          const std::vector<int>& igrp,
          const std::vector<float>& sgrp,
          const std::vector<double>& xgrp,
          const std::vector<EclIO::PaddedOutputString<8>>& zwel,
          const std::vector<int>& iwel,
          const std::vector<float>& swel,
          const std::vector<double>& xwel,
          const std::vector<int>& icon,
          const std::vector<float>& scon,
          const std::vector<double>& xcon);

    state(const std::vector<int>& intehead,
          const std::vector<bool>& logihead,
          const std::vector<double>& doubhead,
          const std::vector<EclIO::PaddedOutputString<8>>& zgrp,
          const std::vector<int>& igrp,
          const std::vector<float>& sgrp,
          const std::vector<double>& xgrp,
          const std::vector<EclIO::PaddedOutputString<8>>& zwel,
          const std::vector<int>& iwel,
          const std::vector<float>& swel,
          const std::vector<double>& xwel,
          const std::vector<int>& icon,
          const std::vector<float>& scon,
          const std::vector<double>& xcon,
          const std::vector<int>& iseg,
          const std::vector<double>& rseg,
          const std::vector<int>& ilbs,
          const std::vector<int>& ilbr);

    void add_groups(const Header& header,
                    const std::vector<EclIO::PaddedOutputString<8>>& zgrp,
                    const std::vector<int>& igrp,
                    const std::vector<float>& sgrp,
                    const std::vector<double>& xgrp);

    std::vector<Well> wells;
    std::vector<Group> groups;
};
}
}




#endif
