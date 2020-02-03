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
#include <opm/output/eclipse/VectorItems/connection.hpp>


namespace VI = ::Opm::RestartIO::Helpers::VectorItems;

namespace Opm {
namespace RestartIO {

RstConnection::RstConnection(const int* icon, const float* scon, const double* xcon) :
    insert_index(icon[VI::IConn::SeqIndex] - 1),
    ijk({icon[VI::IConn::CellI] - 1, icon[VI::IConn::CellJ] - 1, icon[VI::IConn::CellK] - 1}),
    status(icon[VI::IConn::ConnStat]),
    drain_sat_table(icon[VI::IConn::Drainage]),
    imb_sat_table(icon[VI::IConn::Imbibition]),
    completion(icon[VI::IConn::ComplNum] - 1),
    dir(icon[VI::IConn::ConnDir]),
    segment(icon[VI::IConn::Segment] - 1),
    tran(scon[VI::SConn::ConnTrans]),
    depth(scon[VI::SConn::Depth]),
    diameter(scon[VI::SConn::Diameter]),
    kh(scon[VI::SConn::EffectiveKH]),
    segdist_end(scon[VI::SConn::SegDistEnd]),
    segdist_start(scon[VI::SConn::SegDistStart]),
    oil_rate(xcon[VI::XConn::OilRate]),
    water_rate(xcon[VI::XConn::WaterRate]),
    gas_rate(xcon[VI::XConn::GasRate]),
    pressure(xcon[VI::XConn::Pressure]),
    resv_rate(xcon[VI::XConn::ResVRate])
{}

}
}
