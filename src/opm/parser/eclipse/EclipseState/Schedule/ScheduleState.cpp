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
#include <iostream>
#include <fmt/format.h>

#include <opm/common/utility/numeric/cmp.hpp>
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
time_point clamp_time(time_point t) {
    return TimeService::from_time_t( TimeService::to_time_t( t ) );
}

std::pair<std::size_t, std::size_t> date_diff(const time_point& t2, const time_point& t1) {
    auto ts1 = TimeStampUTC(TimeService::to_time_t(t1));
    auto ts2 = TimeStampUTC(TimeService::to_time_t(t2));
    auto year_diff  = ts2.year() - ts1.year();
    auto month_diff = year_diff*12 + ts2.month() - ts1.month();
    return { year_diff, month_diff };
}


}



ScheduleState::ScheduleState(const time_point& t1):
    m_start_time(clamp_time(t1)),
    m_first_in_month(true),
    m_first_in_year(true)
{
    auto ts1 = TimeStampUTC(TimeService::to_time_t(this->m_start_time));
    this->m_month_num = ts1.month() - 1;
}

ScheduleState::ScheduleState(const time_point& start_time, const time_point& end_time) :
    ScheduleState(start_time)
{
    this->m_end_time = clamp_time(end_time);
}

void ScheduleState::update_date(const time_point& prev_time) {
    auto [year_diff, month_diff] = date_diff(this->m_start_time, prev_time);
    this->m_year_num += year_diff;
    this->m_first_in_month = (month_diff > 0);
    this->m_first_in_year = (year_diff > 0);

    auto ts1 = TimeStampUTC(TimeService::to_time_t(this->m_start_time));
    this->m_month_num = ts1.month() - 1;
}




ScheduleState::ScheduleState(const ScheduleState& src, const time_point& start_time) :
    ScheduleState(src)
{
    this->m_start_time = clamp_time(start_time);
    this->m_end_time = std::nullopt;
    this->m_sim_step = src.sim_step() + 1;
    this->m_events.reset();
    this->m_wellgroup_events.reset();
    this->m_geo_keywords.clear();
    this->target_wellpi.clear();

    auto next_rft = this->rft_config().next();
    if (next_rft.has_value())
        this->rft_config.update( std::move(*next_rft) );

    this->update_date(src.m_start_time);
    if (this->rst_config().save) {
        auto new_rst = this->rst_config();
        new_rst.save = false;
        this->rst_config.update( std::move(new_rst) );
    }
}


ScheduleState::ScheduleState(const ScheduleState& src, const time_point& start_time, const time_point& end_time) :
    ScheduleState(src, start_time)
{
    this->m_end_time = end_time;
}


time_point ScheduleState::start_time() const {
    return this->m_start_time;
}

time_point ScheduleState::end_time() const {
    return this->m_end_time.value();
}

std::size_t ScheduleState::sim_step() const {
    return this->m_sim_step;
}

std::size_t ScheduleState::month_num() const {
    return this->m_month_num;
}

std::size_t ScheduleState::year_num() const {
    return this->m_year_num;
}

bool ScheduleState::first_in_month() const {
    return this->m_first_in_month;
}

bool ScheduleState::first_in_year() const {
    return this->m_first_in_year;
}

void ScheduleState::init_nupcol(Nupcol nupcol) {
    this->m_nupcol = std::move(nupcol);
}

void ScheduleState::update_nupcol(int nupcol) {
    this->m_nupcol.update(nupcol);
}

int ScheduleState::nupcol() const {
    return this->m_nupcol.value();
}

void ScheduleState::update_oilvap(OilVaporizationProperties oilvap) {
    this->m_oilvap = std::move(oilvap);
}

const OilVaporizationProperties& ScheduleState::oilvap() const {
    return this->m_oilvap;
}

OilVaporizationProperties& ScheduleState::oilvap() {
    return this->m_oilvap;
}

void ScheduleState::update_geo_keywords(std::vector<DeckKeyword> geo_keywords) {
    this->m_geo_keywords = std::move(geo_keywords);
}

std::vector<DeckKeyword>& ScheduleState::geo_keywords() {
    return this->m_geo_keywords;
}

const std::vector<DeckKeyword>& ScheduleState::geo_keywords() const {
    return this->m_geo_keywords;
}

void ScheduleState::update_message_limits(MessageLimits message_limits) {
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

void ScheduleState::update_whistctl(Well::ProducerCMode whistctl) {
    this->m_whistctl_mode = whistctl;
}

bool ScheduleState::operator==(const ScheduleState& other) const {

    return this->m_start_time == other.m_start_time &&
           this->m_oilvap == other.m_oilvap &&
           this->m_sim_step == other.m_sim_step &&
           this->m_month_num == other.m_month_num &&
           this->m_first_in_month == other.m_first_in_month &&
           this->m_first_in_year == other.m_first_in_year &&
           this->m_year_num == other.m_year_num &&
           this->target_wellpi == other.target_wellpi &&
           this->m_tuning == other.m_tuning &&
           this->m_end_time == other.m_end_time &&
           this->m_events == other.m_events &&
           this->m_wellgroup_events == other.m_wellgroup_events &&
           this->m_geo_keywords == other.m_geo_keywords &&
           this->m_message_limits == other.m_message_limits &&
           this->m_whistctl_mode == other.m_whistctl_mode &&
           this->m_nupcol == other.m_nupcol &&
           this->wtest_config.get() == other.wtest_config.get() &&
           this->well_order.get() == other.well_order.get() &&
           this->group_order.get() == other.group_order.get() &&
           this->gconsale.get() == other.gconsale.get() &&
           this->gconsump.get() == other.gconsump.get() &&
           this->wlist_manager.get() == other.wlist_manager.get() &&
           this->rpt_config.get() == other.rpt_config.get() &&
           this->udq_active.get() == other.udq_active.get() &&
           this->glo.get() == other.glo.get() &&
           this->guide_rate.get() == other.guide_rate.get() &&
           this->rft_config.get() == other.rft_config.get() &&
           this->udq.get() == other.udq.get() &&
           this->wells == other.wells &&
           this->groups == other.groups &&
           this->vfpprod == other.vfpprod &&
           this->vfpinj == other.vfpinj;
}



ScheduleState ScheduleState::serializeObject() {
    auto t1 = TimeService::now();
    auto t2 = t1 + std::chrono::hours(48);
    ScheduleState ts(t1, t2);
    ts.m_sim_step = 123;
    ts.m_month_num = 12;
    ts.m_year_num = 66;
    ts.vfpprod = map_member<int, VFPProdTable>::serializeObject();
    ts.vfpinj = map_member<int, VFPInjTable>::serializeObject();
    ts.groups = map_member<std::string, Group>::serializeObject();
    ts.m_events = Events::serializeObject();
    ts.m_nupcol = Nupcol::serializeObject();
    ts.update_oilvap( Opm::OilVaporizationProperties::serializeObject() );
    ts.m_message_limits = MessageLimits::serializeObject();
    ts.m_whistctl_mode = Well::ProducerCMode::THP;
    ts.target_wellpi = {{"WELL1", 1000}, {"WELL2", 2000}};

    ts.pavg.update( PAvg::serializeObject() );
    ts.wtest_config.update( WellTestConfig::serializeObject() );
    ts.gconsump.update( GConSump::serializeObject() );
    ts.gconsale.update( GConSale::serializeObject() );
    ts.wlist_manager.update( WListManager::serializeObject() );
    ts.rpt_config.update( RPTConfig::serializeObject() );
    ts.actions.update( Action::Actions::serializeObject() );
    ts.udq_active.update( UDQActive::serializeObject() );
    ts.network.update( Network::ExtNetwork::serializeObject() );
    ts.well_order.update( NameOrder::serializeObject() );
    ts.group_order.update( GroupOrder::serializeObject() );
    ts.udq.update( UDQConfig::serializeObject() );
    ts.guide_rate.update( GuideRateConfig::serializeObject() );
    ts.glo.update( GasLiftOpt::serializeObject() );
    ts.rft_config.update( RFTConfig::serializeObject() );
    ts.rst_config.update( RSTConfig::serializeObject() );

    return ts;
}

void ScheduleState::update_tuning(Tuning tuning) {
    this->m_tuning = std::move(tuning);
}

const Tuning& ScheduleState::tuning() const {
    return this->m_tuning;
}

Tuning& ScheduleState::tuning() {
    return this->m_tuning;
}

void ScheduleState::update_events(Events events) {
    this->m_events = events;
}

Events& ScheduleState::events() {
    return this->m_events;
}


const Events& ScheduleState::events() const {
    return this->m_events;
}

void ScheduleState::update_wellgroup_events(WellGroupEvents wgevents) {
    this->m_wellgroup_events = std::move(wgevents);
}

WellGroupEvents& ScheduleState::wellgroup_events() {
    return this->m_wellgroup_events;
}

const WellGroupEvents& ScheduleState::wellgroup_events() const {
    return this->m_wellgroup_events;
}


/*
  Observe that the decision to write a restart file will typically be a
  combination of the RST configuration from the previous report step, and the
  first_in_year++ attributes of this report step. That is the reason the
  function takes a RSTConfig argument - instead of using the rst_config member.

*/

bool ScheduleState::rst_file(const RSTConfig& rst) const {
    if (rst.save)
        return true;

    if (rst.write_rst_file.has_value())
        return rst.write_rst_file.value();

    auto freq = rst.freq.value_or(1);
    auto basic = rst.basic.value();

    if (basic == 3)
        return (this->sim_step() % freq) == 0;

    if (basic == 4) {
        if (!this->first_in_year())
            return false;

        return (this->m_year_num % freq) == 0;
    }

    if (basic == 5) {
        if (!this->first_in_month())
            return false;

        return (this->m_month_num % freq) == 0;
    }

    throw std::logic_error(fmt::format("Unsupported BASIC={} value", basic));
}

namespace {
/*
  The insane trickery here (thank you Stackoverflow!) is to be able to provide a
  simple templated comparison function

     template <typename T>
     int not_equal(const T& arg1, const T& arg2, const std::string& msg);

  which will print arg1 and arg2 on stderr *if* T supports operator<<, otherwise
  it will just print the typename of T.
*/


template<typename T, typename = int>
struct cmpx
{
    int neq(const T& arg1, const T& arg2, const std::string& msg) {
        if (arg1 == arg2)
            return 0;

        std::cerr << "Error when comparing <" << typeid(arg1).name() << ">: " << msg << std::endl;
        return 1;
    }
};

template <typename T>
struct cmpx<T, decltype(std::cout << T(), 0)>
{
    int neq(const T& arg1, const T& arg2, const std::string& msg) {
        if (arg1 == arg2)
            return 0;

        std::cerr << "Error when comparing: " << msg << " " << arg1 << " != " << arg2 << std::endl;
        return 1;
    }
};


template <typename T>
int not_equal(const T& arg1, const T& arg2, const std::string& msg) {
    return cmpx<T>().neq(arg1, arg2, msg);
}

template <typename T>
int not_equal(const std::optional<T>& arg1, const std::optional<T>& arg2, const std::string& msg) {
    if (arg1.has_value() && arg2.has_value())
        return cmpx<T>().neq(*arg1, *arg2, msg);

    if (arg1 == arg2)
        return 0;

    std::cerr << "Error when comparing optional<" << typeid(T).name() << ">  has<1>: " << arg1.has_value() << " has<2>: " << arg2.has_value() << " :" << msg << std::endl;
    return 1;
}

template <>
int not_equal(const double& arg1, const double& arg2, const std::string& msg) {
    if (Opm::cmp::scalar_equal(arg1, arg2))
        return 0;

    std::cerr << "Error when comparing: " << msg << " " << arg1 << " != " << arg2 << std::endl;
    return 1;
}

template <>
int not_equal(const UDAValue& arg1, const UDAValue& arg2, const std::string& msg) {
    if (arg1.is<double>())
        return not_equal( arg1.get<double>(), arg2.get<double>(), msg);
    else
        return not_equal( arg1.get<std::string>(), arg2.get<std::string>(), msg);
}

std::string well_msg(const std::string& well, const std::string& msg) {
    return "Well: " + well + " " + msg;
}

std::string well_segment_msg(const std::string& well, int segment_number, const std::string& msg) {
    return "Well: " + well + " Segment: " + std::to_string(segment_number) + " " + msg;
}

std::string well_connection_msg(const std::string& well, const Connection& conn, const std::string& msg) {
    return "Well: " + well + " Connection: " + std::to_string(conn.getI()) + ", " + std::to_string(conn.getJ()) + ", " + std::to_string(conn.getK()) + "  " + msg;
}

}

bool ScheduleState::rst_cmp(const ScheduleState& state1, const ScheduleState& state2) {
    int count = not_equal(state1.well_order(), state2.well_order(), "Well order");
    if (count != 0)
        return false;

    int group_count = 0;
    group_count += not_equal(state1.group_order(), state2.group_order(), "Group order");

    for (const auto& gname : state1.group_order().names()) {
        const auto& group1 = state1.groups.get(gname);
        const auto& group2 = state2.groups.get(gname);

        auto group_msg = [group1](const std::string& msg) {
            return "Group:" + group1.name() + " : " + msg;
        };


        group_count += not_equal(group1.insert_index(), group2.insert_index(), group_msg("Insert index"));
        group_count += not_equal(group1.parent(), group2.parent(), group_msg("Parent"));
        group_count += not_equal(group1.wells(), group2.wells(), group_msg("Wells"));
        group_count += not_equal(group1.groups(), group2.groups(), group_msg("Groups"));
        group_count += not_equal(group1.getGroupEfficiencyFactor(), group2.getGroupEfficiencyFactor(), group_msg("GEFAC"));
        group_count += not_equal(group1.getTransferGroupEfficiencyFactor(), group2.getTransferGroupEfficiencyFactor(), group_msg("Transfer_GEFAC"));
        group_count += not_equal(group1.getGroupNetVFPTable(), group2.getGroupNetVFPTable(), group_msg("VFP Table"));
        group_count += not_equal(group1.topup_phase(), group2.topup_phase(), group_msg("topup_phase"));
        {
            const auto& prod1 = group1.productionProperties();
            const auto& prod2 = group2.productionProperties();

            group_count += not_equal(prod1.name, prod2.name, group_msg("Prod name"));
            group_count += not_equal(static_cast<int>(prod1.cmode), static_cast<int>(prod2.cmode), group_msg("prod CMode"));
            group_count += not_equal(static_cast<int>(prod1.exceed_action), static_cast<int>(prod2.exceed_action), group_msg("ExceedAction"));
            group_count += not_equal(prod1.oil_target, prod2.oil_target, group_msg("Oil target"));
            group_count += not_equal(prod1.gas_target, prod2.gas_target, group_msg("Gas target"));
            group_count += not_equal(prod1.water_target, prod2.water_target, group_msg("Water target"));
            group_count += not_equal(prod1.liquid_target, prod2.liquid_target, group_msg("Liquid target"));
            group_count += not_equal(prod1.resv_target, prod2.resv_target, group_msg("RESV target"));
            group_count += not_equal(prod1.guide_rate, prod2.guide_rate, group_msg("Guide rate"));
            group_count += not_equal(prod1.guide_rate_def, prod2.guide_rate_def, group_msg("Guide rate definition"));
            group_count += not_equal(prod1.available_group_control, prod2.available_group_control, group_msg("Prod: Available for group control"));
            group_count += not_equal(prod1.production_controls, prod2.production_controls, group_msg("Production controls"));
        }

        group_count += not_equal(group1.injectionProperties().size(), group2.injectionProperties().size(), group_msg("Injection: number of phases"));
        for (const auto& [phase,inj1] : group1.injectionProperties()) {
            const auto& inj2 = group2.injectionProperties().at(phase);
            group_count += not_equal(inj1.phase, inj2.phase, group_msg("Injection phase"));
            group_count += not_equal(inj1.cmode, inj2.cmode, group_msg("CMode"));
            group_count += not_equal(inj1.surface_max_rate, inj2.surface_max_rate, group_msg("Surface rate"));
            group_count += not_equal(inj1.resv_max_rate, inj2.resv_max_rate, group_msg("RESV rate"));
            group_count += not_equal(inj1.target_reinj_fraction, inj2.target_reinj_fraction, group_msg("reinj fraction"));
            group_count += not_equal(inj1.target_void_fraction, inj2.target_void_fraction, group_msg("void_fraction"));
            group_count += not_equal(inj1.reinj_group, inj2.reinj_group, group_msg("reinj_group"));
            group_count += not_equal(inj1.voidage_group, inj2.voidage_group, group_msg("voidage_group"));
            group_count += not_equal(inj1.guide_rate, inj2.guide_rate, group_msg("Guide rate"));
            group_count += not_equal(inj1.guide_rate_def, inj2.guide_rate_def, group_msg("Guide rate definition"));
            group_count += not_equal(inj1.available_group_control, inj2.available_group_control, group_msg("Inj: Available for group control"));
            group_count += not_equal(inj1.injection_controls, inj2.injection_controls, group_msg("Injection controls"));
        }

        group_count += not_equal(static_cast<int>(group1.getGroupType()), static_cast<int>(group2.getGroupType()), group_msg("GroupType"));
    }
    count += group_count;

    group_count += not_equal(state1.gconsale(), state2.gconsale(), "GConSale");
    group_count += not_equal(state1.gconsump(), state2.gconsump(), "GConSump");
    group_count += not_equal(state1.guide_rate(), state2.guide_rate(), "Guide rate");
    group_count += not_equal(state1.glo(), state2.glo(), "Gas Lift Optimization");
    group_count += not_equal(state1.wtest_config(), state2.wtest_config(), "WTest config");

    group_count += not_equal(state1.m_start_time, state2.m_start_time, "Start time");
    group_count += not_equal(state1.m_end_time, state2.m_end_time, "End time");
    group_count += not_equal(state1.m_tuning, state2.m_tuning, "Tuning");
    group_count += not_equal(state1.m_nupcol, state2.m_nupcol, "Nupcol");
    group_count += not_equal(state1.m_oilvap, state2.m_oilvap, "oilvap");
    group_count += not_equal(state1.m_events, state2.m_events, "Events");
    group_count += not_equal(state1.m_wellgroup_events, state2.m_wellgroup_events, "WellGroupEvents");
    group_count += not_equal(state1.m_geo_keywords, state2.m_geo_keywords, "Geo keywords");
    group_count += not_equal(state1.m_message_limits, state2.m_message_limits, "Message limits");
    group_count += not_equal(state1.m_whistctl_mode, state2.m_whistctl_mode, "WHist CTLMode");
    group_count += not_equal(state1.target_wellpi, state2.target_wellpi, "Target WELLPI");
    count += group_count;

    count += not_equal(state1.vfpprod.size(), state2.vfpprod.size(), "VFPPROD size");
    for (const auto& vfp_wrapper : state1.vfpprod()) {
        const auto& vfp1 = vfp_wrapper.get();
        const auto& vfp2 = state2.vfpprod.get( vfp1.name() );
        count += not_equal(vfp1, vfp2, "VFPPROD");
    }

    count += not_equal(state1.vfpinj.size(), state2.vfpinj.size(), "VFPINJ size");
    for (const auto& vfp_wrapper : state1.vfpinj()) {
        const auto& vfp1 = vfp_wrapper.get();
        const auto& vfp2 = state2.vfpinj.get( vfp1.name() );
        count += not_equal(vfp1, vfp2, "VFPINJ");
    }


    for (const auto& wname : state1.well_order().names()) {
        const auto& well1 = state1.wells.get(wname);
        const auto& well2 = state2.wells.get(wname);
        int well_count = 0;
        {
            const auto& connections2 = well2.getConnections();
            const auto& connections1 = well1.getConnections();

            well_count += not_equal( connections1.ordering(), connections2.ordering(), well_msg(well1.name(), "Connection: ordering"));
            for (std::size_t icon = 0; icon < connections1.size(); icon++) {
                const auto& conn1 = connections1[icon];
                const auto& conn2 = connections2[icon];
                well_count += not_equal( conn1.getI(), conn2.getI(), well_connection_msg(well1.name(), conn1, "I"));
                well_count += not_equal( conn1.getJ() , conn2.getJ() , well_connection_msg(well1.name(), conn1, "J"));
                well_count += not_equal( conn1.getK() , conn2.getK() , well_connection_msg(well1.name(), conn1, "K"));
                well_count += not_equal( conn1.state() , conn2.state(), well_connection_msg(well1.name(), conn1, "State"));
                well_count += not_equal( conn1.dir() , conn2.dir(), well_connection_msg(well1.name(), conn1, "dir"));
                well_count += not_equal( conn1.complnum() , conn2.complnum(), well_connection_msg(well1.name(), conn1, "complnum"));
                well_count += not_equal( conn1.segment() , conn2.segment(), well_connection_msg(well1.name(), conn1, "segment"));
                well_count += not_equal( conn1.kind() , conn2.kind(), well_connection_msg(well1.name(), conn1, "CFKind"));
                well_count += not_equal( conn1.sort_value(), conn2.sort_value(), well_connection_msg(well1.name(), conn1, "sort_value"));


                well_count += not_equal( conn1.CF(), conn2.CF(), well_connection_msg(well1.name(), conn1, "CF"));
                well_count += not_equal( conn1.Kh(), conn2.Kh(), well_connection_msg(well1.name(), conn1, "Kh"));
                well_count += not_equal( conn1.rw(), conn2.rw(), well_connection_msg(well1.name(), conn1, "rw"));
                well_count += not_equal( conn1.depth(), conn2.depth(), well_connection_msg(well1.name(), conn1, "depth"));

                well_count += not_equal( conn1.r0(), conn2.r0(), well_connection_msg(well1.name(), conn1, "r0"));
                well_count += not_equal( conn1.skinFactor(), conn2.skinFactor(), well_connection_msg(well1.name(), conn1, "skinFactor"));

            }
        }

        if (not_equal(well1.isMultiSegment(), well2.isMultiSegment(), well_msg(well1.name(), "Is MSW")))
            return false;

        if (well1.isMultiSegment()) {
            const auto& segments1 = well1.getSegments();
            const auto& segments2 = well2.getSegments();
            if (not_equal(segments1.size(), segments2.size(), "Segments: size"))
                return false;

            for (std::size_t iseg=0; iseg < segments1.size(); iseg++) {
                const auto& segment1 = segments1[iseg];
                const auto& segment2 = segments2.getFromSegmentNumber(segment1.segmentNumber());
                well_count += not_equal(segment1.segmentNumber(), segment2.segmentNumber(), well_segment_msg(well1.name(), segment1.segmentNumber(), "segmentNumber"));
                well_count += not_equal(segment1.branchNumber(), segment2.branchNumber(), well_segment_msg(well1.name(), segment1.segmentNumber(), "branchNumber"));
                well_count += not_equal(segment1.outletSegment(), segment2.outletSegment(), well_segment_msg(well1.name(), segment1.segmentNumber(), "outletSegment"));
                well_count += not_equal(segment1.totalLength(), segment2.totalLength(), well_segment_msg(well1.name(), segment1.segmentNumber(), "totalLength"));
                well_count += not_equal(segment1.depth(), segment2.depth(), well_segment_msg(well1.name(), segment1.segmentNumber(), "depth"));
                well_count += not_equal(segment1.internalDiameter(), segment2.internalDiameter(), well_segment_msg(well1.name(), segment1.segmentNumber(), "internalDiameter"));
                well_count += not_equal(segment1.roughness(), segment2.roughness(), well_segment_msg(well1.name(), segment1.segmentNumber(), "roughness"));
                well_count += not_equal(segment1.crossArea(), segment2.crossArea(), well_segment_msg(well1.name(), segment1.segmentNumber(), "crossArea"));
                well_count += not_equal(segment1.volume(), segment2.volume(), well_segment_msg(well1.name(), segment1.segmentNumber(), "volume"));
            }
        }

        well_count += not_equal(well1.getStatus(), well2.getStatus(), well_msg(well1.name(), "status"));
        {
            const auto& prod1 = well1.getProductionProperties();
            const auto& prod2 = well2.getProductionProperties();
            well_count += not_equal(prod1.name, prod2.name , well_msg(well1.name(), "Prod: name"));
            well_count += not_equal(prod1.OilRate, prod2.OilRate, well_msg(well1.name(), "Prod: OilRate"));
            well_count += not_equal(prod1.GasRate, prod2.GasRate, well_msg(well1.name(), "Prod: GasRate"));
            well_count += not_equal(prod1.WaterRate, prod2.WaterRate, well_msg(well1.name(), "Prod: WaterRate"));
            well_count += not_equal(prod1.LiquidRate, prod2.LiquidRate, well_msg(well1.name(), "Prod: LiquidRate"));
            well_count += not_equal(prod1.ResVRate, prod2.ResVRate, well_msg(well1.name(), "Prod: ResVRate"));
            well_count += not_equal(prod1.BHPTarget, prod2.BHPTarget, well_msg(well1.name(), "Prod: BHPTarget"));
            well_count += not_equal(prod1.THPTarget, prod2.THPTarget, well_msg(well1.name(), "Prod: THPTarget"));
            well_count += not_equal(prod1.VFPTableNumber, prod2.VFPTableNumber, well_msg(well1.name(), "Prod: VFPTableNumber"));
            well_count += not_equal(prod1.ALQValue, prod2.ALQValue, well_msg(well1.name(), "Prod: ALQValue"));
            if (!prod1.predictionMode) {
                well_count += not_equal(prod1.bhp_hist_limit, prod2.bhp_hist_limit, well_msg(well1.name(), "Prod: bhp_hist_limit"));
                well_count += not_equal(prod1.thp_hist_limit, prod2.thp_hist_limit, well_msg(well1.name(), "Prod: thp_hist_limit"));
                well_count += not_equal(prod1.BHPH, prod2.BHPH, well_msg(well1.name(), "Prod: BHPH"));
                well_count += not_equal(prod1.THPH, prod2.THPH, well_msg(well1.name(), "Prod: THPH"));
            }
            well_count += not_equal(prod1.productionControls(), prod2.productionControls(), well_msg(well1.name(), "Prod: productionControls"));
            if (well1.getStatus() == Well::Status::OPEN) {
                // This means that the active control mode read from the restart
                // file is different from the control mode prescribed in the
                // schedule file. This *might* be an indication of a bug, but in
                // general this is perfectly legitimate.
                if (prod1.controlMode != prod2.controlMode)
                    fmt::print("Difference in production controlMode for well:{}  Schedule input: {}   restart file: {}\n",
                               well1.name(),
                               Well::ProducerCMode2String(prod1.controlMode),
                               Well::ProducerCMode2String(prod2.controlMode));
                well_count += not_equal(prod1.predictionMode, prod2.predictionMode, well_msg(well1.name(), "Prod: predictionMode"));
            }
            well_count += not_equal(prod1.whistctl_cmode, prod2.whistctl_cmode, well_msg(well1.name(), "Prod: whistctl_cmode"));
        }
        {
            const auto& inj1 = well1.getInjectionProperties();
            const auto& inj2 = well2.getInjectionProperties();

            well_count += not_equal(inj1.name, inj2.name, well_msg(well1.name(), "Well::Inj: name"));
            well_count += not_equal(inj1.surfaceInjectionRate, inj2.surfaceInjectionRate, well_msg(well1.name(), "Well::Inj: surfaceInjectionRate"));
            well_count += not_equal(inj1.reservoirInjectionRate, inj2.reservoirInjectionRate, well_msg(well1.name(), "Well::Inj: reservoirInjectionRate"));
            well_count += not_equal(inj1.BHPTarget, inj2.BHPTarget, well_msg(well1.name(), "Well::Inj: BHPTarget"));
            well_count += not_equal(inj1.THPTarget, inj2.THPTarget, well_msg(well1.name(), "Well::Inj: THPTarget"));
            well_count += not_equal(inj1.bhp_hist_limit, inj2.bhp_hist_limit, well_msg(well1.name(), "Well::Inj: bhp_hist_limit"));
            well_count += not_equal(inj1.thp_hist_limit, inj2.thp_hist_limit, well_msg(well1.name(), "Well::Inj: thp_hist_limit"));
            well_count += not_equal(inj1.BHPH, inj2.BHPH, well_msg(well1.name(), "Well::Inj: BHPH"));
            well_count += not_equal(inj1.THPH, inj2.THPH, well_msg(well1.name(), "Well::Inj: THPH"));
            well_count += not_equal(inj1.VFPTableNumber, inj2.VFPTableNumber, well_msg(well1.name(), "Well::Inj: VFPTableNumber"));
            well_count += not_equal(inj1.injectionControls, inj2.injectionControls, well_msg(well1.name(), "Well::Inj: injectionControls"));
            well_count += not_equal(inj1.injectorType, inj2.injectorType, well_msg(well1.name(), "Well::Inj: injectorType"));
            if (well1.getStatus() == Well::Status::OPEN) {
                // This means that the active control mode read from the restart
                // file is different from the control mode prescribed in the
                // schedule file. This *might* be an indication of a bug, but in
                // general this is perfectly legitimate.
                if (inj1.controlMode != inj2.controlMode)
                    fmt::print("Difference in injection controlMode for well:{}  Schedule input: {}   restart file: {}\n",
                               well1.name(),
                               Well::InjectorCMode2String(inj1.controlMode),
                               Well::InjectorCMode2String(inj2.controlMode));
                well_count += not_equal(inj1.predictionMode, inj2.predictionMode, well_msg(well1.name(), "Well::Inj: predictionMode"));
            } else
                well_count += not_equal(inj1.controlMode, inj2.controlMode, well_msg(well1.name(), "Well::Inj: controlMode"));
        }

        {
            well_count += not_equal( well1.groupName(), well2.groupName(), well_msg(well1.name(), "Well: groupName"));
            well_count += not_equal( well1.getHeadI(), well2.getHeadI(), well_msg(well1.name(), "Well: getHeadI"));
            well_count += not_equal( well1.getHeadJ(), well2.getHeadJ(), well_msg(well1.name(), "Well: getHeadJ"));
            well_count += not_equal( well1.getRefDepth(), well2.getRefDepth(), well_msg(well1.name(), "Well: getRefDepth"));
            well_count += not_equal( well1.isMultiSegment(), well2.isMultiSegment() , well_msg(well1.name(), "Well: isMultiSegment"));
            well_count += not_equal( well1.isAvailableForGroupControl(), well2.isAvailableForGroupControl() , well_msg(well1.name(), "Well: isAvailableForGroupControl"));
            well_count += not_equal( well1.getGuideRate(), well2.getGuideRate(), well_msg(well1.name(), "Well: getGuideRate"));
            well_count += not_equal( well1.getGuideRatePhase(), well2.getGuideRatePhase(), well_msg(well1.name(), "Well: getGuideRatePhase"));
            well_count += not_equal( well1.getGuideRateScalingFactor(), well2.getGuideRateScalingFactor(), well_msg(well1.name(), "Well: getGuideRateScalingFactor"));
            well_count += not_equal( well1.canOpen(), well2.canOpen(), well_msg(well1.name(), "Well: canOpen"));
            well_count += not_equal( well1.isProducer(), well2.isProducer(), well_msg(well1.name(), "Well: isProducer"));
            well_count += not_equal( well1.isInjector(), well2.isInjector(), well_msg(well1.name(), "Well: isInjector"));
            if (well1.isInjector())
                well_count += not_equal( well1.injectorType(), well2.injectorType(), well_msg(well1.name(), "Well1: injectorType"));
            well_count += not_equal( well1.seqIndex(), well2.seqIndex(), well_msg(well1.name(), "Well: seqIndex"));
            well_count += not_equal( well1.getAutomaticShutIn(), well2.getAutomaticShutIn(), well_msg(well1.name(), "Well: getAutomaticShutIn"));
            well_count += not_equal( well1.getAllowCrossFlow(), well2.getAllowCrossFlow(), well_msg(well1.name(), "Well: getAllowCrossFlow"));
            well_count += not_equal( well1.getSolventFraction(), well2.getSolventFraction(), well_msg(well1.name(), "Well: getSolventFraction"));
            well_count += not_equal( well1.getStatus(), well2.getStatus(), well_msg(well1.name(), "Well: getStatus"));
            well_count += not_equal( well1.getInjectionProperties(), well2.getInjectionProperties(), "Well: getInjectionProperties");


            if (well1.isProducer())
                well_count += not_equal( well1.getPreferredPhase(), well2.getPreferredPhase(), well_msg(well1.name(), "Well: getPreferredPhase"));
            well_count += not_equal( well1.getDrainageRadius(), well2.getDrainageRadius(), well_msg(well1.name(), "Well: getDrainageRadius"));
            well_count += not_equal( well1.getEfficiencyFactor(), well2.getEfficiencyFactor(), well_msg(well1.name(), "Well: getEfficiencyFactor"));

            if (well1.getStatus() == Well::Status::OPEN)
                well_count += not_equal(well1.predictionMode(), well2.predictionMode(), well_msg(well1.name(), "Well: predictionMode"));
        }
        count += well_count;
        if (well_count > 0)
            std::cerr << std::endl;
    }
    return (count == 0);
}

}
