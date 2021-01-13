/*
  Copyright 2021 Equinor ASA.

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
#include <fmt/format.h>

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellTestConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GConSump.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GConSale.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/VFPProdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/VFPInjTable.hpp>


namespace Opm {

namespace {


/*
  This is to ensure that only time_points which can be represented with
  std::time_t are used. The reason for clamping to std::time_t resolution is
  that the serialization code in
  opm-simulators:opm/simulators/utils/ParallelRestart.cpp goes via std::time_t.
*/
std::chrono::system_clock::time_point clamp_time(std::chrono::system_clock::time_point t) {
    return std::chrono::system_clock::from_time_t( std::chrono::system_clock::to_time_t( t ) );
}

}



ScheduleState::ScheduleState(const std::chrono::system_clock::time_point& t1):
    m_start_time(clamp_time(t1)),
    m_pavg( std::make_shared<PAvg>())
{
}

ScheduleState::ScheduleState(const std::chrono::system_clock::time_point& start_time, const std::chrono::system_clock::time_point& end_time) :
    ScheduleState(start_time)
{
    this->m_end_time = clamp_time(end_time);
}

ScheduleState::ScheduleState(const ScheduleState& src, const std::chrono::system_clock::time_point& start_time) :
    ScheduleState(src)
{
    this->m_start_time = clamp_time(start_time);
    this->m_end_time = std::nullopt;

    this->m_events.reset();
    this->m_wellgroup_events.reset();
    this->m_geo_keywords.clear();
}

ScheduleState::ScheduleState(const ScheduleState& src, const std::chrono::system_clock::time_point& start_time, const std::chrono::system_clock::time_point& end_time) :
    ScheduleState(src, start_time)
{
    this->m_end_time = end_time;
}


std::chrono::system_clock::time_point ScheduleState::start_time() const {
    return this->m_start_time;
}

std::chrono::system_clock::time_point ScheduleState::end_time() const {
    return this->m_end_time.value();
}


void ScheduleState::pavg(PAvg arg) {
    this->m_pavg = std::make_shared<PAvg>( std::move(arg) );
}

const PAvg& ScheduleState::pavg() const {
    return *this->m_pavg;
}

void ScheduleState::nupcol(int nupcol) {
    this->m_nupcol = nupcol;
}

int ScheduleState::nupcol() const {
    return this->m_nupcol;
}

void ScheduleState::oilvap(OilVaporizationProperties oilvap) {
    this->m_oilvap = std::move(oilvap);
}

const OilVaporizationProperties& ScheduleState::oilvap() const {
    return this->m_oilvap;
}

OilVaporizationProperties& ScheduleState::oilvap() {
    return this->m_oilvap;
}

void ScheduleState::geo_keywords(std::vector<DeckKeyword> geo_keywords) {
    this->m_geo_keywords = std::move(geo_keywords);
}

std::vector<DeckKeyword>& ScheduleState::geo_keywords() {
    return this->m_geo_keywords;
}

const std::vector<DeckKeyword>& ScheduleState::geo_keywords() const {
    return this->m_geo_keywords;
}

void ScheduleState::message_limits(MessageLimits message_limits) {
    this->m_message_limits = std::move(message_limits);
}

const MessageLimits& ScheduleState::message_limits() const {
    return this->m_message_limits;
}

MessageLimits& ScheduleState::message_limits() {
    return this->m_message_limits;
}

Well::ProducerCMode ScheduleState::whistctl() const {
    return this->m_whistctl_mode;
}

void ScheduleState::whistctl(Well::ProducerCMode whistctl) {
    this->m_whistctl_mode = whistctl;
}

bool ScheduleState::operator==(const ScheduleState& other) const {
    auto&& map_equal = [](const auto& map1, const auto& map2) {
        if (map1.size() != map2.size())
            return false;

        auto it2 = map2.begin();
        for (const auto& it1 : map1) {
            if (it1.first != it2->first)
                return false;

            if (!(*it1.second == *it2->second))
                return false;

            ++it2;
        }

        return true;
    };

    if (!map_equal(this->m_vfpprod, other.m_vfpprod))
        return false;

    if (!map_equal(this->m_vfpinj, other.m_vfpinj))
        return false;

    return this->m_start_time == other.m_start_time &&
           this->m_oilvap == other.m_oilvap &&
           this->m_tuning == other.m_tuning &&
           this->m_end_time == other.m_end_time &&
           this->m_events == other.m_events &&
           this->m_wellgroup_events == other.m_wellgroup_events &&
           this->m_geo_keywords == other.m_geo_keywords &&
           this->m_message_limits == other.m_message_limits &&
           this->m_whistctl_mode == other.m_whistctl_mode &&
           *this->m_wtest_config == *other.m_wtest_config &&
           *this->m_gconsale == *other.m_gconsale &&
           *this->m_gconsump == *other.m_gconsump &&
           *this->m_wlist_manager == *other.m_wlist_manager &&
           *this->m_rptconfig == *other.m_rptconfig &&
           this->m_nupcol == other.m_nupcol;
}

ScheduleState ScheduleState::serializeObject() {
    auto t1 = std::chrono::system_clock::now();
    auto t2 = t1 + std::chrono::hours(48);
    ScheduleState ts(t1, t2);
    ts.m_events = Events::serializeObject();
    ts.nupcol(77);
    ts.oilvap( Opm::OilVaporizationProperties::serializeObject() );
    ts.m_message_limits = MessageLimits::serializeObject();
    ts.m_whistctl_mode = Well::ProducerCMode::THP;
    ts.m_wtest_config = std::make_shared<WellTestConfig>( WellTestConfig::serializeObject() );
    ts.m_gconsump = std::make_shared<GConSump>( GConSump::serializeObject() );
    ts.m_gconsale = std::make_shared<GConSale>( GConSale::serializeObject() );
    ts.m_wlist_manager = std::make_shared<WListManager>( WListManager::serializeObject() );
    ts.m_rptconfig = std::make_shared<RPTConfig>( RPTConfig::serializeObject() );
    ts.m_vfpprod.emplace( std::make_pair(77, std::make_shared<VFPProdTable>(VFPProdTable::serializeObject() )));
    ts.m_vfpprod.emplace( std::make_pair(78, std::make_shared<VFPProdTable>(VFPProdTable::serializeObject() )));
    ts.m_vfpinj.emplace( std::make_pair(177, std::make_shared<VFPInjTable>(VFPInjTable::serializeObject() )));
    ts.m_vfpinj.emplace( std::make_pair(178, std::make_shared<VFPInjTable>(VFPInjTable::serializeObject() )));

    return ts;
}

void ScheduleState::tuning(Tuning tuning) {
    this->m_tuning = std::move(tuning);
}

const Tuning& ScheduleState::tuning() const {
    return this->m_tuning;
}

Tuning& ScheduleState::tuning() {
    return this->m_tuning;
}

void ScheduleState::events(Events events) {
    this->m_events = events;
}

Events& ScheduleState::events() {
    return this->m_events;
}


const Events& ScheduleState::events() const {
    return this->m_events;
}

void ScheduleState::wellgroup_events(WellGroupEvents wgevents) {
    this->m_wellgroup_events = std::move(wgevents);
}

WellGroupEvents& ScheduleState::wellgroup_events() {
    return this->m_wellgroup_events;
}

const WellGroupEvents& ScheduleState::wellgroup_events() const {
    return this->m_wellgroup_events;
}

const WellTestConfig& ScheduleState::wtest_config() const {
    return *this->m_wtest_config;
}

void ScheduleState::wtest_config(WellTestConfig wtest_config) {
    this->m_wtest_config = std::make_shared<WellTestConfig>( std::move(wtest_config) );
}

const GConSale& ScheduleState::gconsale() const {
    return *this->m_gconsale;
}

void ScheduleState::gconsale(GConSale gconsale) {
    this->m_gconsale = std::make_shared<GConSale>( std::move(gconsale) );
}

const GConSump& ScheduleState::gconsump() const {
    return *this->m_gconsump;
}

void ScheduleState::gconsump(GConSump gconsump) {
    this->m_gconsump = std::make_shared<GConSump>( std::move(gconsump) );
}

const WListManager& ScheduleState::wlist_manager() const {
    return *this->m_wlist_manager;
}

void ScheduleState::wlist_manager(WListManager wlist_manager) {
    this->m_wlist_manager = std::make_shared<WListManager>( std::move(wlist_manager) );
}

const Network::ExtNetwork& ScheduleState::network() const {
    return *this->m_network;
}

void ScheduleState::network(Network::ExtNetwork network) {
    this->m_network = std::make_shared<Network::ExtNetwork>( std::move(network) );
}

const RPTConfig& ScheduleState::rpt_config() const {
    return *this->m_rptconfig;
}


void ScheduleState::rpt_config(RPTConfig rpt_config) {
    this->m_rptconfig = std::make_shared<RPTConfig>(std::move(rpt_config));
}

std::vector<const VFPProdTable*> ScheduleState::vfpprod() const {
    std::vector<const VFPProdTable*> tables;
    for (const auto& [_, table] : this->m_vfpprod) {
        (void)_;
        tables.push_back( table.get() );
    }
    return tables;
}

const VFPProdTable& ScheduleState::vfpprod(int table_id) const {
    auto vfp_iter = this->m_vfpprod.find(table_id);
    if (vfp_iter == this->m_vfpprod.end())
        throw std::logic_error(fmt::format("No VFPPROD table with id: {} has been registered", table_id));

    return *vfp_iter->second;
}

void ScheduleState::vfpprod(VFPProdTable vfpprod) {
    int table_id = vfpprod.getTableNum();
    this->m_vfpprod[table_id] = std::make_shared<VFPProdTable>( std::move(vfpprod) );
}

std::vector<const VFPInjTable*> ScheduleState::vfpinj() const {
    std::vector<const VFPInjTable*> tables;
    for (const auto& [_, table] : this->m_vfpinj) {
        (void)_;
        tables.push_back( table.get() );
    }
    return tables;
}

const VFPInjTable& ScheduleState::vfpinj(int table_id) const {
    auto vfp_iter = this->m_vfpinj.find(table_id);
    if (vfp_iter == this->m_vfpinj.end())
        throw std::logic_error(fmt::format("No VFPINJ table with id: {} has been registered", table_id));

    return *vfp_iter->second;
}

void ScheduleState::vfpinj(VFPInjTable vfpinj) {
    int table_id = vfpinj.getTableNum();
    this->m_vfpinj[table_id] = std::make_shared<VFPInjTable>( std::move(vfpinj) );
}
}
