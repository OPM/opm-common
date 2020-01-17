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

#include <opm/io/eclipse/rst/header.hpp>
#include <opm/io/eclipse/rst/connection.hpp>
#include <opm/io/eclipse/rst/well.hpp>
#include <opm/io/eclipse/rst/state.hpp>

#include <opm/output/eclipse/VectorItems/connection.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>

#include <opm/io/eclipse/PaddedOutputString.hpp>


namespace VI = ::Opm::RestartIO::Helpers::VectorItems;

namespace Opm {
namespace RestartIO {

state::state(const std::vector<int>& intehead,
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
             const std::vector<double>& xcon)
{
    Header header(intehead, logihead, doubhead);
    this->add_groups(header, zgrp, igrp, sgrp, xgrp);

    for (int iw = 0; iw < header.num_wells; iw++) {
        std::size_t zwel_offset = iw * header.nzwelz;
        std::size_t iwel_offset = iw * header.niwelz;
        std::size_t swel_offset = iw * header.nswelz;
        std::size_t xwel_offset = iw * header.nxwelz;
        std::size_t icon_offset = iw * header.niconz * header.ncwmax;
        std::size_t scon_offset = iw * header.nsconz * header.ncwmax;
        std::size_t xcon_offset = iw * header.nxconz * header.ncwmax;

        this->wells.emplace_back(header,
                                 zwel.data() + zwel_offset,
                                 iwel.data() + iwel_offset,
                                 swel.data() + swel_offset,
                                 xwel.data() + xwel_offset,
                                 icon.data() + icon_offset,
                                 scon.data() + scon_offset,
                                 xcon.data() + xcon_offset);

        if (this->wells.back().msw_index)
            throw std::logic_error("MSW data not accounted for in this constructor");
    }
}

state::state(const std::vector<int>& intehead,
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
             const std::vector<int>& ilbr)
{
    Header header(intehead, logihead, doubhead);
    this->add_groups(header, zgrp, igrp, sgrp, xgrp);

    for (int iw = 0; iw < header.num_wells; iw++) {
        std::size_t zwel_offset = iw * header.nzwelz;
        std::size_t iwel_offset = iw * header.niwelz;
        std::size_t swel_offset = iw * header.nswelz;
        std::size_t xwel_offset = iw * header.nxwelz;
        std::size_t icon_offset = iw * header.niconz * header.ncwmax;
        std::size_t scon_offset = iw * header.nsconz * header.ncwmax;
        std::size_t xcon_offset = iw * header.nxconz * header.ncwmax;

        this->wells.emplace_back(header,
                                 zwel.data() + zwel_offset,
                                 iwel.data() + iwel_offset,
                                 swel.data() + swel_offset,
                                 xwel.data() + xwel_offset,
                                 icon.data() + icon_offset,
                                 scon.data() + scon_offset,
                                 xcon.data() + xcon_offset,
                                 iseg,
                                 rseg,
                                 ilbs,
                                 ilbr);
    }
}

void state::add_groups(const Header& header,
                       const std::vector<EclIO::PaddedOutputString<8>>& zgrp,
                       const std::vector<int>& igrp,
                       const std::vector<float>& sgrp,
                       const std::vector<double>& xgrp)
{
    for (int ig=0; ig < header.ngroup; ig++) {
        std::size_t zgrp_offset = ig * header.nzgrpz;
        std::size_t igrp_offset = ig * header.nigrpz;
        std::size_t sgrp_offset = ig * header.nsgrpz;
        std::size_t xgrp_offset = ig * header.nxgrpz;

        this->groups.emplace_back(header,
                                  zgrp.data() + zgrp_offset,
                                  igrp.data() + igrp_offset,
                                  sgrp.data() + sgrp_offset,
                                  xgrp.data() + xgrp_offset);
    }
}


}
}
