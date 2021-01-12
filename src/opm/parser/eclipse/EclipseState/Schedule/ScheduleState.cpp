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

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellTestConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GConSump.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GConSale.hpp>

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

}
