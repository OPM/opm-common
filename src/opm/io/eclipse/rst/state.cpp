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
#include <memory>
#include <numeric>
#include <optional>

#include <opm/io/eclipse/RestartFileView.hpp>

#include <opm/io/eclipse/rst/header.hpp>
#include <opm/io/eclipse/rst/connection.hpp>
#include <opm/io/eclipse/rst/well.hpp>
#include <opm/io/eclipse/rst/state.hpp>

#include <opm/output/eclipse/UDQDims.hpp>
#include <opm/output/eclipse/VectorItems/connection.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>
#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/VectorItems/doubhead.hpp>


namespace {
    std::string
    udq_define(const std::vector<std::string>& zudl,
               const std::size_t               udq_index)
    {
        auto zudl_begin = zudl.begin();
        auto zudl_end = zudl.begin();
        std::advance( zudl_begin, (udq_index + 0) * Opm::UDQDims::entriesPerZUDL() );
        std::advance( zudl_end  , (udq_index + 1) * Opm::UDQDims::entriesPerZUDL() );

        auto define = std::accumulate(zudl_begin, zudl_end, std::string{}, std::plus<>());
        if (!define.empty() && (define[0] == '~')) {
            define[0] = '-';
        }

        return define;
    }

    Opm::UDQUpdate udq_update(const std::vector<int>& iudq,
                              const std::size_t       udq_index)
    {
        return Opm::UDQ::updateType(iudq[udq_index * Opm::UDQDims::entriesPerIUDQ()]);
    }
}

namespace VI = ::Opm::RestartIO::Helpers::VectorItems;

namespace Opm { namespace RestartIO {

RstState::RstState(std::shared_ptr<EclIO::RestartFileView> rstView,
                   const ::Opm::EclipseGrid*               grid)
    : unit_system(rstView->intehead()[VI::intehead::UNIT])
    , header(unit_system, rstView->intehead(), rstView->logihead(), rstView->doubhead())
    , aquifers(rstView, grid, unit_system)
{
    this->load_tuning(rstView->intehead(), rstView->doubhead());
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
                        const std::vector<double>& dudf)
{
    auto well_var  = 0*this->header.num_wells;
    auto group_var = 0*this->header.ngroup;
    auto field_var = 0*this->header.num_udq();

    for (auto udq_index = 0*this->header.num_udq(); udq_index < this->header.num_udq(); ++udq_index) {
        const auto& name = zudn[udq_index*UDQDims::entriesPerZUDN() + 0];
        const auto& unit = zudn[udq_index*UDQDims::entriesPerZUDN() + 1];

        const auto define = udq_define(zudl, udq_index);
        auto& udq = define.empty()
            ? this->udqs.emplace_back(name, unit)
            : this->udqs.emplace_back(name, unit, define, udq_update(iudq, udq_index));

        if (udq.var_type == UDQVarType::WELL_VAR) {
            for (std::size_t well_index = 0; well_index < this->wells.size(); well_index++) {
                auto well_value = dudw[ well_var * this->header.max_wells_in_field + well_index ];
                if (well_value == UDQ::restart_default)
                    continue;

                const auto& well_name = this->wells[well_index].name;
                udq.add_value(well_name, well_value);
            }

            ++well_var;
        }

        if (udq.var_type == UDQVarType::GROUP_VAR) {
            for (std::size_t group_index = 0; group_index < this->groups.size(); group_index++) {
                auto group_value = dudg[ group_var * this->header.max_groups_in_field + group_index ];
                if (group_value == UDQ::restart_default)
                    continue;

                const auto& group_name = this->groups[group_index].name;
                udq.add_value(group_name, group_value);
            }

            ++group_var;
        }

        if (udq.var_type == UDQVarType::FIELD_VAR) {
            auto field_value = dudf[ field_var ];
            if (field_value != UDQ::restart_default)
                udq.add_value(field_value);

            ++field_var;
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

RstState RstState::load(std::shared_ptr<EclIO::RestartFileView> rstView,
                        const Runspec&,
                        const ::Opm::EclipseGrid*               grid)
{
    RstState state(rstView, grid);

    if (state.header.ngroup > 0) {
        const auto& zgrp = rstView->getKeyword<std::string>("ZGRP");
        const auto& igrp = rstView->getKeyword<int>("IGRP");
        const auto& sgrp = rstView->getKeyword<float>("SGRP");
        const auto& xgrp = rstView->getKeyword<double>("XGRP");
        state.add_groups(zgrp, igrp, sgrp, xgrp);
    }

    if (state.header.num_wells > 0) {
        const auto& zwel = rstView->getKeyword<std::string>("ZWEL");
        const auto& iwel = rstView->getKeyword<int>("IWEL");
        const auto& swel = rstView->getKeyword<float>("SWEL");
        const auto& xwel = rstView->getKeyword<double>("XWEL");

        const auto& icon = rstView->getKeyword<int>("ICON");
        const auto& scon = rstView->getKeyword<float>("SCON");
        const auto& xcon = rstView->getKeyword<double>("XCON");

        if (rstView->hasKeyword<int>("ISEG")) {
            const auto& iseg = rstView->getKeyword<int>("ISEG");
            const auto& rseg = rstView->getKeyword<double>("RSEG");

            state.add_msw(zwel, iwel, swel, xwel,
                          icon, scon, xcon,
                          iseg, rseg);
        } else
            state.add_wells(zwel, iwel, swel, xwel,
                            icon, scon, xcon);
    }

    if (state.header.num_udq() > 0) {
        const auto& iudq = rstView->getKeyword<int>("IUDQ");
        const auto& zudn = rstView->getKeyword<std::string>("ZUDN");
        const auto& zudl = rstView->getKeyword<std::string>("ZUDL");

        const auto& dudw = state.header.nwell_udq  > 0 ? rstView->getKeyword<double>("DUDW") : std::vector<double>{};
        const auto& dudg = state.header.ngroup_udq > 0 ? rstView->getKeyword<double>("DUDG") : std::vector<double>{};
        const auto& dudf = state.header.nfield_udq > 0 ? rstView->getKeyword<double>("DUDF") : std::vector<double>{};

        state.add_udqs(iudq, zudn, zudl, dudw, dudg, dudf);

        if (rstView->hasKeyword<int>("IUAD")) {
            const auto& iuad = rstView->getKeyword<int>("IUAD");
            const auto& iuap = rstView->getKeyword<int>("IUAP");
            const auto& igph = rstView->getKeyword<int>("IGPH");
            state.udq_active = RstUDQActive(iuad, iuap, igph);
        }
    }

    return state;
}

}} // namespace Opm::RestartIO
