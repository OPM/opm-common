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

#ifndef SCHEDULE_TSTEP_HPP
#define SCHEDULE_TSTEP_HPP

#include <chrono>
#include <memory>
#include <optional>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/RPTConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/PAvg.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/OilVaporizationProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Events.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WListManager.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MessageLimits.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GConSump.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GConSale.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Network/ExtNetwork.hpp>

namespace Opm {

    /*
      The purpose of the ScheduleState class is to hold the entire Schedule
      information, i.e. wells and groups and so on, at exactly one point in
      time. The ScheduleState class itself has no dynamic behavior, the dynamics
      is handled by the Schedule instance owning the ScheduleState instance.
    */

    class WellTestConfig;

    class ScheduleState {
    public:
        ScheduleState() = default;
        explicit ScheduleState(const std::chrono::system_clock::time_point& start_time);
        ScheduleState(const std::chrono::system_clock::time_point& start_time, const std::chrono::system_clock::time_point& end_time);
        ScheduleState(const ScheduleState& src, const std::chrono::system_clock::time_point& start_time);
        ScheduleState(const ScheduleState& src, const std::chrono::system_clock::time_point& start_time, const std::chrono::system_clock::time_point& end_time);


        std::chrono::system_clock::time_point start_time() const;
        std::chrono::system_clock::time_point end_time() const;
        ScheduleState next(const std::chrono::system_clock::time_point& next_start);

        bool operator==(const ScheduleState& other) const;
        static ScheduleState serializeObject();

        void pavg(PAvg pavg);
        const PAvg& pavg() const;

        void tuning(Tuning tuning);
        Tuning& tuning();
        const Tuning& tuning() const;

        void nupcol(int nupcol);
        int nupcol() const;

        void oilvap(OilVaporizationProperties oilvap);
        const OilVaporizationProperties& oilvap() const;
        OilVaporizationProperties& oilvap();

        void events(Events events);
        Events& events();
        const Events& events() const;

        void wellgroup_events(WellGroupEvents wgevents);
        WellGroupEvents& wellgroup_events();
        const WellGroupEvents& wellgroup_events() const;

        void geo_keywords(std::vector<DeckKeyword> geo_keywords);
        std::vector<DeckKeyword>& geo_keywords();
        const std::vector<DeckKeyword>& geo_keywords() const;

        void message_limits(MessageLimits message_limits);
        MessageLimits& message_limits();
        const MessageLimits& message_limits() const;

        Well::ProducerCMode whistctl() const;
        void whistctl(Well::ProducerCMode whistctl);

        const WellTestConfig& wtest_config() const;
        void wtest_config(WellTestConfig wtest_config);

        const WListManager& wlist_manager() const;
        void wlist_manager(WListManager wlist_manager);

        const GConSale& gconsale() const;
        void gconsale(GConSale gconsale);

        const GConSump& gconsump() const;
        void gconsump(GConSump gconsump);

        const Network::ExtNetwork& network() const;
        void network(Network::ExtNetwork network);

        const RPTConfig& rpt_config() const;
        void rpt_config(RPTConfig rpt_config);

        template<class Serializer>
        void serializeOp(Serializer& serializer) {
            serializer(m_start_time);
            serializer(m_end_time);
            serializer(m_pavg);
            m_tuning.serializeOp(serializer);
            serializer(m_nupcol);
            m_oilvap.serializeOp(serializer);
            m_events.serializeOp(serializer);
            m_wellgroup_events.serializeOp(serializer);
            serializer.vector(m_geo_keywords);
            m_message_limits.serializeOp(serializer);
            serializer(m_whistctl_mode);
            serializer(m_wtest_config);
            serializer(m_gconsale);
            serializer(m_gconsump);
            serializer(m_wlist_manager);
            serializer(m_network);
            serializer(m_rptconfig);
        }

    private:
        std::chrono::system_clock::time_point m_start_time;
        std::optional<std::chrono::system_clock::time_point> m_end_time;

        std::shared_ptr<PAvg> m_pavg;
        Tuning m_tuning;
        int m_nupcol;
        OilVaporizationProperties m_oilvap;
        Events m_events;
        WellGroupEvents m_wellgroup_events;
        std::vector<DeckKeyword> m_geo_keywords;
        MessageLimits m_message_limits;
        Well::ProducerCMode m_whistctl_mode = Well::ProducerCMode::CMODE_UNDEFINED;
        std::shared_ptr<WellTestConfig> m_wtest_config;
        std::shared_ptr<GConSale> m_gconsale;
        std::shared_ptr<GConSump> m_gconsump;
        std::shared_ptr<WListManager> m_wlist_manager;
        std::shared_ptr<Network::ExtNetwork> m_network;
        std::shared_ptr<RPTConfig> m_rptconfig;
    };
}

#endif
