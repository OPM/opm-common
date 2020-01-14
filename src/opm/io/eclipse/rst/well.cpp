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

#include <opm/output/eclipse/VectorItems/connection.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>


namespace VI = ::Opm::RestartIO::Helpers::VectorItems;

namespace Opm {
namespace RestartIO {

Well::Well(const Header& header,
           const EclIO::PaddedOutputString<8>* zwel,
           const int * iwel,
           const float * swel,
           const double * xwel,
           const int * icon,
           const float * scon,
           const double * xcon) :
    name(zwel[0].trim()),
    ij({iwel[VI::IWell::IHead] - 1, iwel[VI::IWell::JHead] - 1}),
    k1k2(std::make_pair(iwel[VI::IWell::FirstK] - 1, iwel[VI::IWell::LastK] - 1)),
    wtype(iwel[VI::IWell::WType]),
    active_control(iwel[VI::IWell::ActWCtrl]),
    vfp_table(iwel[VI::IWell::VFPTab]),
    pred_requested_control(iwel[VI::IWell::PredReqWCtrl]),
    xflow(iwel[VI::IWell::XFlow]),
    hist_requested_control(iwel[VI::IWell::HistReqWCtrl]),
    msw_index(iwel[VI::IWell::MsWID]),
    completion_ordering(iwel[VI::IWell::CompOrd]),
    orat_target(swel[VI::SWell::OilRateTarget]),
    wrat_target(swel[VI::SWell::WatRateTarget]),
    grat_target(swel[VI::SWell::GasRateTarget]),
    lrat_target(swel[VI::SWell::LiqRateTarget]),
    resv_target(swel[VI::SWell::ResVRateTarget]),
    thp_target(swel[VI::SWell::THPTarget]),
    bhp_target_float(swel[VI::SWell::BHPTarget]),
    hist_lrat_target(swel[VI::SWell::HistLiqRateTarget]),
    hist_grat_target(swel[VI::SWell::HistGasRateTarget]),
    hist_bhp_target(swel[VI::SWell::HistBHPTarget]),
    oil_rate(xwel[VI::XWell::OilPrRate]),
    water_rate(xwel[VI::XWell::WatPrRate]),
    gas_rate(xwel[VI::XWell::GasPrRate]),
    liquid_rate(xwel[VI::XWell::LiqPrRate]),
    void_rate(xwel[VI::XWell::VoidPrRate]),
    flow_bhp(xwel[VI::XWell::FlowBHP]),
    wct(xwel[VI::XWell::WatCut]),
    gor(xwel[VI::XWell::GORatio]),
    oil_total(xwel[VI::XWell::OilPrTotal]),
    water_total(xwel[VI::XWell::WatPrTotal]),
    gas_total(xwel[VI::XWell::GasPrTotal]),
    void_total(xwel[VI::XWell::VoidPrTotal]),
    water_inj_total(xwel[VI::XWell::WatInjTotal]),
    gas_inj_total(xwel[VI::XWell::GasInjTotal]),
    gas_fvf(xwel[VI::XWell::GasFVF]),
    bhp_target_double(xwel[VI::XWell::BHPTarget]),
    hist_oil_total(xwel[VI::XWell::HistOilPrTotal]),
    hist_wat_total(xwel[VI::XWell::HistWatPrTotal]),
    hist_gas_total(xwel[VI::XWell::HistGasPrTotal]),
    hist_water_inj_total(xwel[VI::XWell::HistWatInjTotal]),
    hist_gas_inj_total(xwel[VI::XWell::HistGasInjTotal]),
    water_void_rate(xwel[VI::XWell::WatVoidPrRate]),
    gas_void_rate(xwel[VI::XWell::GasVoidPrRate])
{
    for (int ic = 0; ic < iwel[VI::IWell::NConn]; ic++) {
        std::size_t icon_offset = ic * header.niconz;
        std::size_t scon_offset = ic * header.nsconz;
        std::size_t xcon_offset = ic * header.nxconz;
        this->connections.emplace_back( icon + icon_offset, scon + scon_offset, xcon + xcon_offset);
    }
}

}
}
