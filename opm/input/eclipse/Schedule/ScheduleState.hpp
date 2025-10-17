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

#include <opm/common/utility/gpuDecorators.hpp>
#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/AquiferFlux.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>

#include <opm/input/eclipse/Schedule/BCProp.hpp>
#include <opm/input/eclipse/Schedule/Events.hpp>
#include <opm/input/eclipse/Schedule/Group/Group.hpp>
#include <opm/input/eclipse/Schedule/MessageLimits.hpp>
#include <opm/input/eclipse/Schedule/OilVaporizationProperties.hpp>
#include <opm/input/eclipse/Schedule/RSTConfig.hpp>
#include <opm/input/eclipse/Schedule/Source.hpp>
#include <opm/input/eclipse/Schedule/Tuning.hpp>
#include <opm/input/eclipse/Schedule/VFPInjTable.hpp>
#include <opm/input/eclipse/Schedule/VFPProdTable.hpp>
#include <opm/input/eclipse/Schedule/Well/PAvg.hpp>
#include <opm/input/eclipse/Schedule/Well/WCYCLE.hpp>
#include <opm/input/eclipse/Schedule/Well/WellEnums.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

[[maybe_unused]] std::string as_string(int value) {
    return std::to_string(value);
}

[[maybe_unused]] std::string as_string(const std::string& value) {
    return value;
}

}

namespace Opm {

    namespace Action {
        class Actions;
    }
    class GasLiftOpt;
    class GConSale;
    class GConSump;
    class GroupEconProductionLimits;
    class GroupOrder;
    class GroupSatelliteInjection;
    class GSatProd;
    class GuideRateConfig;
    class NameOrder;
    namespace Network {
        class Balance;
        class ExtNetwork;
    }
    namespace ReservoirCoupling {
        class CouplingInfo;
    }
    class RFTConfig;
    class RPTConfig;
    class UDQActive;
    class UDQConfig;
    class Well;
    class WellFractureSeeds;
    class WellTestConfig;
    class WListManager;

    /*
      The purpose of the ScheduleState class is to hold the entire Schedule
      information, i.e. wells and groups and so on, at exactly one point in
      time. The ScheduleState class itself has no dynamic behavior, the dynamics
      is handled by the Schedule instance owning the ScheduleState instance.
    */

    class ScheduleState {
    public:
        /*
          In the SCHEDULE section typically all information is a function of
          time, and the ScheduleState class is used to manage a snapshot of
          state at one point in time. Typically a large part of the
          configuration does not change between timesteps and consecutive
          ScheduleState instances are very similar, to handle this many of the
          ScheduleState members are implemented as std::shared_ptr<>s.

          The ptr_member<T> class is a small wrapper around the
          std::shared_ptr<T>. The ptr_member<T> class is meant to be internal to
          the Schedule implementation and downstream should only access this
          indirectly like e.g.

             const auto& gconsum = sched_state.gconsump();

          The remaining details of the ptr_member<T> class are heavily
          influenced by the code used to serialize the Schedule information.
         */



        template <typename T>
        class ptr_member {
        public:
            const T& get() const {
                return *this->m_data;
            }

            /*
              This will allocate new storage and assign @object to the new
              storage.
            */
            void update(T object)
            {
                this->m_data = std::make_shared<T>( std::move(object) );
            }

            /*
              Will reassign the pointer to point to existing shared instance
              @other.
            */
            void update(const ptr_member<T>& other)
            {
                this->m_data = other.m_data;
            }

            const T& operator()() const {
                return *this->m_data;
            }

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(m_data);
            }

        private:
            std::shared_ptr<T> m_data;
        };


        /*
          The map_member class is a quite specialized class used to internalize
          the map variables managed in the ScheduleState. The actual value
          objects will be stored as std::shared_ptr<T>, and only the unique
          objects have dedicated storage. The class T must implement the method:

              const K& T::name() const;

          Which is used to get the storage key for the objects.
         */

        template <typename K, typename T>
        class map_member {
        public:
            std::vector<K> keys() const {
                std::vector<K> key_vector;
                std::transform( this->m_data.begin(), this->m_data.end(), std::back_inserter(key_vector), [](const auto& pair) { return pair.first; });
                return key_vector;
            }


            template <typename Predicate>
            const T* find(Predicate&& predicate) const {
                auto iter = std::find_if( this->m_data.begin(), this->m_data.end(), std::forward<Predicate>(predicate));
                if (iter == this->m_data.end())
                    return nullptr;

                return iter->second.get();
            }


            const std::shared_ptr<T> get_ptr(const K& key) const {
                auto iter = this->m_data.find(key);
                if (iter != this->m_data.end())
                    return iter->second;

                return {};
            }


            bool has(const K& key) const {
                auto ptr = this->get_ptr(key);
                return (ptr != nullptr);
            }

            void update(const K& key, std::shared_ptr<T> value) {
                this->m_data.insert_or_assign(key, std::move(value));
            }

            void update(T object) {
                auto key = object.name();
                this->m_data[key] = std::make_shared<T>( std::move(object) );
            }

            void update(const K& key, const map_member<K,T>& other) {
                auto other_ptr = other.get_ptr(key);
                if (other_ptr)
                    this->m_data[key] = other.get_ptr(key);
                else
                    throw std::logic_error(std::string{"Tried to update member: "} + as_string(key) + std::string{"with uninitialized object"});
            }

            const T& operator()(const K& key) const {
                return this->get(key);
            }

            const T& get(const K& key) const {
                return *this->m_data.at(key);
            }

            T& get(const K& key) {
                return *this->m_data.at(key);
            }


            std::vector<std::reference_wrapper<const T>> operator()() const {
                std::vector<std::reference_wrapper<const T>> as_vector;
                for (const auto& [_, elm_ptr] : this->m_data) {
                    (void)_;
                    as_vector.push_back( std::cref(*elm_ptr));
                }
                return as_vector;
            }


            std::vector<std::reference_wrapper<T>> operator()() {
                std::vector<std::reference_wrapper<T>> as_vector;
                for (const auto& [_, elm_ptr] : this->m_data) {
                    (void)_;
                    as_vector.push_back( std::ref(*elm_ptr));
                }
                return as_vector;
            }


            bool operator==(const map_member<K,T>& other) const {
                if (this->m_data.size() != other.m_data.size())
                    return false;

                for (const auto& [key1, ptr1] : this->m_data) {
                    const auto& ptr2 = other.get_ptr(key1);
                    if (!ptr2)
                        return false;

                    if (!(*ptr1 == *ptr2))
                        return false;
                }
                return true;
            }


            std::size_t size() const {
                return this->m_data.size();
            }

            typename std::unordered_map<K, std::shared_ptr<T>>::const_iterator begin() const {
                return this->m_data.begin();
            }

            typename std::unordered_map<K, std::shared_ptr<T>>::const_iterator end() const {
                return this->m_data.end();
            }


            static map_member<K,T> serializationTestObject() {
                map_member<K,T> map_object;
                T value_object = T::serializationTestObject();
                K key = value_object.name();
                map_object.m_data.emplace( key, std::make_shared<T>( std::move(value_object) ));
                return map_object;
            }

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(m_data);
            }

        private:
            std::unordered_map<K, std::shared_ptr<T>> m_data;
        };

        struct BHPDefaults {
            std::optional<double> prod_target;
            std::optional<double> inj_limit;

            static BHPDefaults serializationTestObject()
            {
                return BHPDefaults{1.0, 2.0};
            }

            bool operator==(const BHPDefaults& rhs) const
            {
                return this->prod_target == rhs.prod_target
                    && this->inj_limit == rhs.inj_limit;
            }

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(prod_target);
                serializer(inj_limit);
            }
        };

        /// Flag for structural changes to the run's well list
        class WellListChangeTracker
        {
        private:
            /// Named flag indices
            enum class Ix : std::size_t {
                /// Flag for static (regular) well list changes.
                Static,

                /// Flag for dynamic, i.e., ACTIONX based, well list changes.
                Action,

                /// Number of flags.  Must be last enumerator.
                Num,
            };

            /// Translate flag to numeric index
            ///
            /// \param[in] Flag index
            ///
            /// \return numeric index corresponding to flag index \p i.
            static constexpr auto index(const Ix i)
            {
                return static_cast<std::underlying_type_t<Ix>>(i);
            }

        public:
            /// Record that one or more well lists have changed structurally
            /// in response to a WLIST keyword entered in the regular input
            /// stream.
            void recordStaticChangedLists()
            {
                this->listsChanged_[ index(Ix::Static) ] = true;
            }

            /// Record that one or more well lists have changed structurally
            /// in response to a WLIST keyword entered in an ACTIONX block.
            void recordActionChangedLists()
            {
                this->listsChanged_[ index(Ix::Action) ] = true;
            }

            /// Report whether or not any well lists have changed since the
            /// previous report step.
            bool changedLists() const
            {
                return this->listsChanged_[ index(Ix::Static) ];
            }

            /// Prepare internal structure to record changes at the next
            /// report step.
            ///
            /// Should typically be called at the end of one report step or
            /// at the very beginning of the next report step, usually as
            /// part of preparing the next ScheduleState object.
            void prepareNextReportStep();

            /// Create a serialisation test object.
            static WellListChangeTracker serializationTestObject();

            /// Equality predicate.
            ///
            /// \param[in] that Object against which \code *this \endcode
            /// will be tested for equality.
            ///
            /// \return Whether or not \code *this \endcode is the same as
            /// \p that.
            bool operator==(const WellListChangeTracker& that) const
            {
                return this->listsChanged_ == that.listsChanged_;
            }

            /// Convert between byte array and object representation.
            ///
            /// \tparam Serializer Byte array conversion protocol.
            ///
            /// \param[in,out] serializer Byte array conversion object.
            template <class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(this->listsChanged_);
            }

        private:
            /// Collection of change flags for run's well lists.
            using ListChangeStatus = std::array
                <bool, static_cast<std::underlying_type_t<Ix>>(Ix::Num)>;

            /// Change flags for run's well lists.
            ///
            /// This could arguably be packed into a single unsigned char
            /// (or std::byte).
            ListChangeStatus listsChanged_{{false, false}};
        };

        ScheduleState() = default;
        explicit ScheduleState(const time_point& start_time);
        ScheduleState(const time_point& start_time, const time_point& end_time);
        ScheduleState(const ScheduleState& src, const time_point& start_time);
        ScheduleState(const ScheduleState& src, const time_point& start_time, const time_point& end_time);


        time_point start_time() const;
        time_point end_time() const;
        ScheduleState next(const time_point& next_start);

        // The sim_step() is the report step we are currently simulating on. The
        // results when we have completed sim_step=N are stored in report_step
        // N+1.
        std::size_t sim_step() const;

        // The month_num and year_num() functions return the accumulated number
        // of full months/years to the start of the current block.
        std::size_t month_num() const;
        std::size_t year_num() const;
        bool first_in_month() const;
        bool first_in_year() const;
        bool well_group_contains_lgr(const Group& grp, const std::string& lgr_tag) const;
        bool group_contains_lgr(const Group& grp, const std::string& lgr_tag) const;

        std::size_t num_lgr_well_in_group(const Group& grp, const std::string& lgr_tag) const;
        std::size_t num_lgr_groups_in_group(const Group& grp, const std::string& lgr_tag) const;


        bool operator==(const ScheduleState& other) const;
        static ScheduleState serializationTestObject();

        void update_tuning(Tuning tuning);
        Tuning& tuning();
        const Tuning& tuning() const;
        double max_next_tstep(const bool enableTUNING = false) const;

        void init_nupcol(Nupcol nupcol);
        void update_nupcol(int nupcol);
        int nupcol() const;

        void update_oilvap(OilVaporizationProperties oilvap);
        const OilVaporizationProperties& oilvap() const;
        OilVaporizationProperties& oilvap();

        void update_events(Events events);
        Events& events();
        const Events& events() const;

        void update_wellgroup_events(WellGroupEvents wgevents);
        WellGroupEvents& wellgroup_events();
        const WellGroupEvents& wellgroup_events() const;

        void update_geo_keywords(std::vector<DeckKeyword> geo_keywords);
        std::vector<DeckKeyword>& geo_keywords();
        const std::vector<DeckKeyword>& geo_keywords() const;

        void update_message_limits(MessageLimits message_limits);
        MessageLimits& message_limits();
        const MessageLimits& message_limits() const;

        WellProducerCMode whistctl() const;
        void update_whistctl(WellProducerCMode whistctl);

        bool rst_file(const RSTConfig& rst_config, const time_point& previous_restart_output_time) const;
        void update_date(const time_point& prev_time);
        void updateSAVE(bool save);
        bool save() const;

        const std::optional<double>& sumthin() const;
        void update_sumthin(double sumthin);

        bool rptonly() const;
        void rptonly(const bool only);

        bool has_gpmaint() const;

        bool hasAnalyticalAquifers() const
        {
            return ! this->aqufluxs.empty();
        }

        /*********************************************************************/

        ptr_member<GConSale> gconsale;
        ptr_member<GConSump> gconsump;
        ptr_member<GSatProd> gsatprod;
        ptr_member<GroupEconProductionLimits> gecon;
        ptr_member<GuideRateConfig> guide_rate;

        ptr_member<WListManager> wlist_manager;
        ptr_member<NameOrder> well_order;
        ptr_member<GroupOrder> group_order;

        ptr_member<Action::Actions> actions;
        ptr_member<UDQConfig> udq;
        ptr_member<UDQActive> udq_active;

        ptr_member<PAvg> pavg;
        ptr_member<WellTestConfig> wtest_config;
        ptr_member<GasLiftOpt> glo;
        ptr_member<Network::ExtNetwork> network;
        ptr_member<Network::Balance> network_balance;
        ptr_member<ReservoirCoupling::CouplingInfo> rescoup;

        ptr_member<RPTConfig> rpt_config;
        ptr_member<RFTConfig> rft_config;
        ptr_member<RSTConfig> rst_config;

        ptr_member<BHPDefaults> bhp_defaults;
        ptr_member<Source> source;
        ptr_member<WCYCLE> wcycle;

        ptr_member<WellListChangeTracker> wlist_tracker;

        template <typename T>
        ptr_member<T>& get() {
            return const_cast<ptr_member<T>&>(std::as_const(*this).template get<T>());
        }

        template <typename T>
        const ptr_member<T>& get() const
        {
            struct always_false1 : std::false_type {};

            if constexpr ( std::is_same_v<T, PAvg> )
                             return this->pavg;
            else if constexpr ( std::is_same_v<T, WellTestConfig> )
                                  return this->wtest_config;
            else if constexpr ( std::is_same_v<T, GConSale> )
                                  return this->gconsale;
            else if constexpr ( std::is_same_v<T, GConSump> )
                                  return this->gconsump;
            else if constexpr ( std::is_same_v<T, GSatProd> )
                                  return this->gsatprod;
            else if constexpr ( std::is_same_v<T, GroupEconProductionLimits> )
                                  return this->gecon;
            else if constexpr ( std::is_same_v<T, WListManager> )
                                  return this->wlist_manager;
            else if constexpr ( std::is_same_v<T, Network::ExtNetwork> )
                                  return this->network;
            else if constexpr ( std::is_same_v<T, Network::Balance> )
                                  return this->network_balance;
            else if constexpr ( std::is_same_v<T, ReservoirCoupling::CouplingInfo> )
                                  return this->rescoup;
            else if constexpr ( std::is_same_v<T, RPTConfig> )
                                  return this->rpt_config;
            else if constexpr ( std::is_same_v<T, Action::Actions> )
                                  return this->actions;
            else if constexpr ( std::is_same_v<T, UDQActive> )
                                  return this->udq_active;
            else if constexpr ( std::is_same_v<T, NameOrder> )
                                  return this->well_order;
            else if constexpr ( std::is_same_v<T, GroupOrder> )
                                  return this->group_order;
            else if constexpr ( std::is_same_v<T, UDQConfig> )
                                  return this->udq;
            else if constexpr ( std::is_same_v<T, GasLiftOpt> )
                                  return this->glo;
            else if constexpr ( std::is_same_v<T, GuideRateConfig> )
                                  return this->guide_rate;
            else if constexpr ( std::is_same_v<T, RFTConfig> )
                                  return this->rft_config;
            else if constexpr ( std::is_same_v<T, RSTConfig> )
                                  return this->rst_config;
            else if constexpr ( std::is_same_v<T, BHPDefaults> )
                                  return this->bhp_defaults;
            else if constexpr ( std::is_same_v<T, Source> )
                                  return this->source;
            else if constexpr ( std::is_same_v<T, WCYCLE> )
                                  return this->wcycle;
            else if constexpr ( std::is_same_v<T, WellListChangeTracker> )
                                  return this->wlist_tracker;
            else {
                #if !OPM_IS_COMPILING_WITH_GPU_COMPILER // NVCC evaluates this branch for some reason
                static_assert(always_false1::value, "Template type <T> not supported in get()");
                #endif
            }
        }

        map_member<int, VFPProdTable> vfpprod;
        map_member<int, VFPInjTable> vfpinj;
        map_member<std::string, Group> groups;
        map_member<std::string, Well> wells;

        /// Group level satellite injection rates.
        map_member<std::string, GroupSatelliteInjection> satelliteInjection;

        /// Well fracturing seed points and associate fracture plane normal
        /// vectors.
        map_member<std::string, WellFractureSeeds> wseed;

        // constant flux aquifers
        std::unordered_map<int, SingleAquiferFlux> aqufluxs;
        BCProp bcprop;
        // injection streams for compostional STREAM injection using WINJGAS
        map_member<std::string, std::vector<double>> inj_streams;

        std::unordered_map<std::string, double> target_wellpi;
        std::optional<NextStep> next_tstep;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(gconsale);
            serializer(gconsump);
            serializer(gsatprod);
            serializer(gecon);
            serializer(guide_rate);
            serializer(wlist_manager);
            serializer(well_order);
            serializer(group_order);
            serializer(actions);
            serializer(udq);
            serializer(udq_active);
            serializer(pavg);
            serializer(wtest_config);
            serializer(glo);
            serializer(network);
            serializer(network_balance);
            serializer(rescoup);
            serializer(rpt_config);
            serializer(rft_config);
            serializer(rst_config);
            serializer(bhp_defaults);
            serializer(source);
            serializer(wcycle);
            serializer(this->wlist_tracker);
            serializer(vfpprod);
            serializer(vfpinj);
            serializer(groups);
            serializer(wells);
            serializer(this->satelliteInjection);
            serializer(wseed);
            serializer(aqufluxs);
            serializer(bcprop);
            serializer(inj_streams);
            serializer(target_wellpi);
            serializer(this->next_tstep);
            serializer(m_start_time);
            serializer(m_end_time);
            serializer(m_sim_step);
            serializer(m_month_num);
            serializer(m_year_num);
            serializer(m_first_in_year);
            serializer(m_first_in_month);
            serializer(m_save_step);
            serializer(m_tuning);
            serializer(m_nupcol);
            serializer(m_oilvap);
            serializer(m_events);
            serializer(m_wellgroup_events);
            serializer(m_geo_keywords);
            serializer(m_message_limits);
            serializer(m_whistctl_mode);
            serializer(m_sumthin);
            serializer(this->m_rptonly);
        }

    private:
        time_point m_start_time{};
        std::optional<time_point> m_end_time{};

        std::size_t m_sim_step = 0;
        std::size_t m_month_num = 0;
        std::size_t m_year_num = 0;
        bool m_first_in_month{false};
        bool m_first_in_year{false};
        bool m_save_step{false};

        Tuning m_tuning{};
        Nupcol m_nupcol{};
        OilVaporizationProperties m_oilvap{};
        Events m_events{};
        WellGroupEvents m_wellgroup_events{};
        std::vector<DeckKeyword> m_geo_keywords{};
        MessageLimits m_message_limits{};
        WellProducerCMode m_whistctl_mode = WellProducerCMode::CMODE_UNDEFINED;
        std::optional<double> m_sumthin{};
        bool m_rptonly{false};
    };

} // namespace Opm

#endif // SCHEDULE_TSTEP_HPP
