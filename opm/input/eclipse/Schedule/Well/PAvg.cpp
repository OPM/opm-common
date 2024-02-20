/*
  Copyright 2020 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/Well/PAvg.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <string>
#include <stdexcept>

#include <fmt/format.h>

namespace {

Opm::PAvg::DepthCorrection
depthCorrectionFromString(const std::string& s)
{
    if (s == "WELL") {
        return Opm::PAvg::DepthCorrection::WELL;
    }

    if (s == "RES") {
        return Opm::PAvg::DepthCorrection::RES;
    }

    if (s == "NONE") {
        return Opm::PAvg::DepthCorrection::NONE;
    }

    throw std::invalid_argument {
        fmt::format("{} not recognized as depth correction mode", s)
    };
}

bool openConnectionsFromString(const std::string& s)
{
    if (s == "OPEN") {
        return true;
    }

    if (s == "ALL") {
        return false;
    }

    throw std::invalid_argument {
        fmt::format("{} not recognized as connection indicator", s)
    };
}

} // Anonymous namespace

namespace Opm {

PAvg::PAvg()
    : m_inner_weight(ParserKeywords::WPAVE::F1::defaultValue)
    , m_conn_weight(ParserKeywords::WPAVE::F2::defaultValue)
    , m_depth_correction(depthCorrectionFromString(ParserKeywords::WPAVE::DEPTH_CORRECTION::defaultValue))
    , m_open_connections(openConnectionsFromString(ParserKeywords::WPAVE::CONNECTION::defaultValue))
{}

PAvg::PAvg(const double          inner_weight,
           const double          conn_weight,
           const DepthCorrection depth_correction,
           const bool            use_open_connections)
    : m_inner_weight(inner_weight)
    , m_conn_weight(conn_weight)
    , m_depth_correction(depth_correction)
    , m_open_connections(use_open_connections)
{}

PAvg::PAvg(const DeckRecord& record)
    : PAvg()
{
    // This code uses the WPAVE keyword to access the content of the record,
    // but the record can equally well come from a WWPAVE keyword--i.e., it
    // is a HARD assumption that the same item names is used both for WPAVE
    // and WWPAVE.
    using WPAVE = ParserKeywords::WPAVE;

    this->m_inner_weight = record.getItem<WPAVE::F1>().get<double>(0);
    this->m_conn_weight  = record.getItem<WPAVE::F2>().get<double>(0);

    if (const auto& item_depth_correction = record.getItem<WPAVE::DEPTH_CORRECTION>();
        !item_depth_correction.defaultApplied(0))
    {
        this->m_depth_correction = depthCorrectionFromString(item_depth_correction.get<std::string>(0));
    }

    if (const auto& item_connections = record.getItem<WPAVE::CONNECTION>();
        !item_connections.defaultApplied(0))
    {
        this->m_open_connections = openConnectionsFromString(item_connections.get<std::string>(0));
    }
}

PAvg PAvg::serializationTestObject()
{
    return PAvg(0.10, 0.30, PAvg::DepthCorrection::NONE, false);
}

bool PAvg::use_porv() const
{
    return (this->m_conn_weight != 1)
        || (this->m_inner_weight < 0);
}

bool PAvg::operator==(const PAvg& other) const
{
    return (this->m_inner_weight == other.m_inner_weight)
        && (this->m_conn_weight == other.m_conn_weight)
        && (this->m_depth_correction == other.m_depth_correction)
        && (this->m_open_connections == other.m_open_connections)
        ;
}

bool PAvg::operator!=(const PAvg& other) const
{
    return !(*this == other);
}

}
