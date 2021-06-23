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

#include <algorithm>
#include <iterator>
#include <numeric>


#include <opm/io/eclipse/rst/header.hpp>
#include <opm/io/eclipse/rst/connection.hpp>
#include <opm/io/eclipse/rst/well.hpp>
#include <opm/io/eclipse/rst/state.hpp>

#include <opm/output/eclipse/UDQDims.hpp>
#include <opm/output/eclipse/VectorItems/connection.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>
#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/VectorItems/doubhead.hpp>


namespace VI = ::Opm::RestartIO::Helpers::VectorItems;

namespace Opm {
namespace RestartIO {

RstState::RstState(const ::Opm::UnitSystem& unit_system_,
                   const std::vector<int>& intehead,
                   const std::vector<bool>& logihead,
                   const std::vector<double>& doubhead):
    unit_system(unit_system_),
    header(unit_system_, intehead, logihead, doubhead)
{
    this->load_tuning(intehead, doubhead);
}


void RstState::load_tuning(const std::vector<int>& intehead,
                           const std::vector<double>& doubhead)
{
    using M  = ::Opm::UnitSystem::measure;

    this->tuning.NEWTMX  = intehead[ VI::intehead::NEWTMX ];
    this->tuning.NEWTMN  = intehead[ VI::intehead::NEWTMN ];
    this->tuning.LITMAX  = intehead[ VI::intehead::LITMAX ];
    this->tuning.LITMIN  = intehead[ VI::intehead::LITMIN ];
    this->tuning.MXWSIT  = intehead[ VI::intehead::MXWSIT ];
    this->tuning.MXWPIT  = intehead[ VI::intehead::MXWPIT ];

    tuning.TSINIT = this->unit_system.to_si(M::time, doubhead[VI::doubhead::TsInit]);
    tuning.TSMAXZ = this->unit_system.to_si(M::time, doubhead[VI::doubhead::TsMaxz]);
    tuning.TSMINZ = this->unit_system.to_si(M::time, doubhead[VI::doubhead::TsMinz]);
    tuning.TSMCHP = this->unit_system.to_si(M::time, doubhead[VI::doubhead::TsMchp]);
    tuning.TSFMAX = doubhead[VI::doubhead::TsFMax];
    tuning.TSFMIN = doubhead[VI::doubhead::TsFMin];
    tuning.TSFCNV = doubhead[VI::doubhead::TsFcnv];
    tuning.THRUPT = doubhead[VI::doubhead::ThrUPT];
    tuning.TFDIFF = doubhead[VI::doubhead::TfDiff];
    tuning.TRGTTE = doubhead[VI::doubhead::TrgTTE];
    tuning.TRGCNV = doubhead[VI::doubhead::TrgCNV];
    tuning.TRGMBE = doubhead[VI::doubhead::TrgMBE];
    tuning.TRGLCV = doubhead[VI::doubhead::TrgLCV];
    tuning.XXXTTE = doubhead[VI::doubhead::XxxTTE];
    tuning.XXXCNV = doubhead[VI::doubhead::XxxCNV];
    tuning.XXXMBE = doubhead[VI::doubhead::XxxMBE];
    tuning.XXXLCV = doubhead[VI::doubhead::XxxLCV];
    tuning.XXXWFL = doubhead[VI::doubhead::XxxWFL];
    tuning.TRGFIP = doubhead[VI::doubhead::TrgFIP];
    tuning.TRGSFT = doubhead[VI::doubhead::TrgSFT];
    tuning.TRGDPR = doubhead[VI::doubhead::TrgDPR];
    tuning.XXXDPR = doubhead[VI::doubhead::XxxDPR];
    tuning.DDPLIM = doubhead[VI::doubhead::DdpLim];
    tuning.DDSLIM = doubhead[VI::doubhead::DdsLim];
}

void RstState::add_groups(const std::vector<std::string>& zgrp,
                          const std::vector<int>& igrp,
                          const std::vector<float>& sgrp,
                          const std::vector<double>& xgrp)
{
    for (int ig=0; ig < this->header.ngroup; ig++) {
        std::size_t zgrp_offset = ig * this->header.nzgrpz;
        std::size_t igrp_offset = ig * this->header.nigrpz;
        std::size_t sgrp_offset = ig * this->header.nsgrpz;
        std::size_t xgrp_offset = ig * this->header.nxgrpz;

        this->groups.emplace_back(this->unit_system,
                                  this->header,
                                  zgrp.data() + zgrp_offset,
                                  igrp.data() + igrp_offset,
                                  sgrp.data() + sgrp_offset,
                                  xgrp.data() + xgrp_offset);
    }
}

void RstState::add_wells(const std::vector<std::string>& zwel,
                         const std::vector<int>& iwel,
                         const std::vector<float>& swel,
                         const std::vector<double>& xwel,
                         const std::vector<int>& icon,
                         const std::vector<float>& scon,
                         const std::vector<double>& xcon) {

    for (int iw = 0; iw < this->header.num_wells; iw++) {
        std::size_t zwel_offset = iw * this->header.nzwelz;
        std::size_t iwel_offset = iw * this->header.niwelz;
        std::size_t swel_offset = iw * this->header.nswelz;
        std::size_t xwel_offset = iw * this->header.nxwelz;
        std::size_t icon_offset = iw * this->header.niconz * this->header.ncwmax;
        std::size_t scon_offset = iw * this->header.nsconz * this->header.ncwmax;
        std::size_t xcon_offset = iw * this->header.nxconz * this->header.ncwmax;
        int group_index = iwel[ iwel_offset + VI::IWell::Group ] - 1;
        const std::string group = this->groups[group_index].name;

        this->wells.emplace_back(this->unit_system,
                                 this->header,
                                 group,
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

void RstState::add_msw(const std::vector<std::string>& zwel,
                       const std::vector<int>& iwel,
                       const std::vector<float>& swel,
                       const std::vector<double>& xwel,
                       const std::vector<int>& icon,
                       const std::vector<float>& scon,
                       const std::vector<double>& xcon,
                       const std::vector<int>& iseg,
                       const std::vector<double>& rseg) {

    for (int iw = 0; iw < this->header.num_wells; iw++) {
        std::size_t zwel_offset = iw * this->header.nzwelz;
        std::size_t iwel_offset = iw * this->header.niwelz;
        std::size_t swel_offset = iw * this->header.nswelz;
        std::size_t xwel_offset = iw * this->header.nxwelz;
        std::size_t icon_offset = iw * this->header.niconz * this->header.ncwmax;
        std::size_t scon_offset = iw * this->header.nsconz * this->header.ncwmax;
        std::size_t xcon_offset = iw * this->header.nxconz * this->header.ncwmax;
        int group_index = iwel[ iwel_offset + VI::IWell::Group ] - 1;
        const std::string group = this->groups[group_index].name;

        this->wells.emplace_back(this->unit_system,
                                 this->header,
                                 group,
                                 zwel.data() + zwel_offset,
                                 iwel.data() + iwel_offset,
                                 swel.data() + swel_offset,
                                 xwel.data() + xwel_offset,
                                 icon.data() + icon_offset,
                                 scon.data() + scon_offset,
                                 xcon.data() + xcon_offset,
                                 iseg,
                                 rseg);
    }
}

void RstState::add_udqs(const std::vector<int>& iudq,
                        const std::vector<std::string>& zudn,
                        const std::vector<std::string>& zudl,
                        const std::vector<double>& dudw,
                        const std::vector<double>& dudg,
                        const std::vector<double>& dudf) {

    for (auto udq_index = 0; udq_index < this->header.num_udq(); udq_index++) {
        const auto& name = zudn[udq_index * UDQDims::entriesPerZUDN()];
        const auto& unit = zudn[udq_index * UDQDims::entriesPerZUDN() + 1];

        auto zudl_begin = zudl.begin();
        auto zudl_end = zudl.begin();
        std::advance( zudl_begin, udq_index * UDQDims::entriesPerZUDL() );
        std::advance( zudl_end, (udq_index + 1) * UDQDims::entriesPerZUDL() );
        auto udq_define = std::accumulate(zudl_begin, zudl_end, std::string{}, std::plus<std::string>());
        if (udq_define.empty())
            this->udqs.emplace_back(name, unit);
        else {
            auto status_int = iudq[udq_index * UDQDims::entriesPerIUDQ()];
            auto status = UDQ::updateType(status_int);
            if (udq_define[0] == '~')
                udq_define[0] = '-';

            this->udqs.emplace_back(name, unit, udq_define, status);
        }

        auto& udq = this->udqs.back();
        if (udq.var_type == UDQVarType::WELL_VAR) {
            for (std::size_t well_index = 0; well_index < this->wells.size(); well_index++) {
                auto well_value = dudw[ udq_index * this->header.num_wells + well_index];
                const auto& well_name = this->wells[well_index].name;
                udq.add_well_value( well_name, well_value );
            }
        }

        if (udq.var_type == UDQVarType::GROUP_VAR) {
            for (std::size_t group_index = 0; group_index < this->groups.size(); group_index++) {
                auto group_value = dudg[ udq_index * this->header.ngroup + group_index];
                const auto& group_name = this->groups[group_index].name;
                udq.add_group_value( group_name, group_value );
            }
        }

        if (udq.var_type == UDQVarType::FIELD_VAR) {
            auto field_value = dudf[ udq_index ];
            udq.add_field_value( field_value );
        }
    }
}


const RstWell& RstState::get_well(const std::string& wname) const {
    const auto well_iter = std::find_if(this->wells.begin(),
                                        this->wells.end(),
                                        [&wname] (const auto& well) {
                                            return well.name == wname;
                                        });
    if (well_iter == this->wells.end())
        throw std::out_of_range("No such well: " + wname);

    return *well_iter;
}

RstState RstState::load(EclIO::ERst& rst_file, int report_step) {
    rst_file.loadReportStepNumber(report_step);
    const auto& intehead = rst_file.getRestartData<int>("INTEHEAD", report_step, 0);
    const auto& logihead = rst_file.getRestartData<bool>("LOGIHEAD", report_step, 0);
    const auto& doubhead = rst_file.getRestartData<double>("DOUBHEAD", report_step, 0);

    auto unit_id = intehead[VI::intehead::UNIT];
    ::Opm::UnitSystem unit_system(unit_id);

    RstState state(unit_system, intehead, logihead, doubhead);
    if (state.header.ngroup > 0) {
        const auto& zgrp = rst_file.getRestartData<std::string>("ZGRP", report_step, 0);
        const auto& igrp = rst_file.getRestartData<int>("IGRP", report_step, 0);
        const auto& sgrp = rst_file.getRestartData<float>("SGRP", report_step, 0);
        const auto& xgrp = rst_file.getRestartData<double>("XGRP", report_step, 0);
        state.add_groups(zgrp, igrp, sgrp, xgrp);
    }

    if (state.header.num_wells > 0) {
        const auto& zwel = rst_file.getRestartData<std::string>("ZWEL", report_step, 0);
        const auto& iwel = rst_file.getRestartData<int>("IWEL", report_step, 0);
        const auto& swel = rst_file.getRestartData<float>("SWEL", report_step, 0);
        const auto& xwel = rst_file.getRestartData<double>("XWEL", report_step, 0);

        const auto& icon = rst_file.getRestartData<int>("ICON", report_step, 0);
        const auto& scon = rst_file.getRestartData<float>("SCON", report_step, 0);
        const auto& xcon = rst_file.getRestartData<double>("XCON", report_step, 0);

        if (rst_file.hasKey("ISEG")) {
            const auto& iseg = rst_file.getRestartData<int>("ISEG", report_step, 0);
            const auto& rseg = rst_file.getRestartData<double>("RSEG", report_step, 0);

            state.add_msw(zwel, iwel, swel, xwel,
                          icon, scon, xcon,
                          iseg, rseg);
        } else
            state.add_wells(zwel, iwel, swel, xwel,
                            icon, scon, xcon);
    }

    if (state.header.num_udq() > 0) {
        const auto& iudq = rst_file.getRestartData<int>("IUDQ", report_step, 0);
        const auto& zudn = rst_file.getRestartData<std::string>("ZUDN", report_step, 0);
        const auto& zudl = rst_file.getRestartData<std::string>("ZUDL", report_step, 0);

        const auto& dudw = state.header.nwell_udq  > 0 ? rst_file.getRestartData<double>("DUDW", report_step, 0) : std::vector<double>{};
        const auto& dudg = state.header.ngroup_udq > 0 ? rst_file.getRestartData<double>("DUDG", report_step, 0) : std::vector<double>{};
        const auto& dudf = state.header.nfield_udq > 0 ? rst_file.getRestartData<double>("DUDF", report_step, 0) : std::vector<double>{};

        state.add_udqs(iudq, zudn, zudl, dudw, dudg, dudf);
    }
    return state;
}

}
}


