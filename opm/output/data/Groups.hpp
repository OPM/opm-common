/*
  Copyright 2016 Statoil ASA.

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

#ifndef OPM_OUTPUT_GROUPS_HPP
#define OPM_OUTPUT_GROUPS_HPP

#include <cstddef>
#include <map>
#include <string>
#include <utility>

#include <opm/input/eclipse/EclipseState/Phase.hpp>
#include <opm/output/data/GuideRateValue.hpp>
#include <opm/input/eclipse/Schedule/Group/Group.hpp>

namespace Opm { namespace data {

    struct GroupConstraints {
        Opm::Group::ProductionCMode currentProdConstraint;
        Opm::Group::InjectionCMode  currentGasInjectionConstraint;
        Opm::Group::InjectionCMode  currentWaterInjectionConstraint;

        template <class MessageBufferType>
        void write(MessageBufferType& buffer) const;

        template <class MessageBufferType>
        void read(MessageBufferType& buffer);

        bool operator==(const GroupConstraints& other) const
        {
            return this->currentProdConstraint == other.currentProdConstraint &&
                   this->currentGasInjectionConstraint == other.currentGasInjectionConstraint &&
                   this->currentWaterInjectionConstraint == other.currentWaterInjectionConstraint;
        }

        inline GroupConstraints& set(Opm::Group::ProductionCMode cpc,
                                     Opm::Group::InjectionCMode  cgic,
                                     Opm::Group::InjectionCMode  cwic);

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(currentProdConstraint);
            serializer(currentGasInjectionConstraint);
            serializer(currentWaterInjectionConstraint);
        }

        static GroupConstraints serializationTestObject()
        {
            return GroupConstraints{Group::ProductionCMode::GRAT,
                                    Group::InjectionCMode::RATE,
                                    Group::InjectionCMode::RESV};
        }
    };

    struct GroupGuideRates {
        GuideRateValue production{};
        GuideRateValue injection{};

        template <class MessageBufferType>
        void write(MessageBufferType& buffer) const
        {
            this->production.write(buffer);
            this->injection .write(buffer);
        }

        template <class MessageBufferType>
        void read(MessageBufferType& buffer)
        {
            this->production.read(buffer);
            this->injection .read(buffer);
        }

        bool operator==(const GroupGuideRates& other) const
        {
            return this->production == other.production
                && this->injection  == other.injection;
        }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(production);
            serializer(injection);
        }

        static GroupGuideRates serializationTestObject()
        {
            return GroupGuideRates{GuideRateValue::serializationTestObject(),
                                   GuideRateValue::serializationTestObject()};
        }
    };

    struct GroupData {
        GroupConstraints currentControl;
        GroupGuideRates  guideRates{};

        template <class MessageBufferType>
        void write(MessageBufferType& buffer) const
        {
            this->currentControl.write(buffer);
            this->guideRates    .write(buffer);
        }

        template <class MessageBufferType>
        void read(MessageBufferType& buffer)
        {
            this->currentControl.read(buffer);
            this->guideRates    .read(buffer);
        }

        bool operator==(const GroupData& other) const
        {
            return this->currentControl == other.currentControl
                && this->guideRates     == other.guideRates;
        }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(currentControl);
            serializer(guideRates);
        }

        static GroupData serializationTestObject()
        {
          return GroupData{GroupConstraints::serializationTestObject(),
                           GroupGuideRates::serializationTestObject()};
        }
    };

    struct NodeData {
        double pressure { 0.0 };
        double converged_pressure { 0.0 };

        template <class MessageBufferType>
        void write(MessageBufferType& buffer) const
        {
            buffer.write(this->pressure);
            buffer.write(this->converged_pressure);
        }

        template <class MessageBufferType>
        void read(MessageBufferType& buffer)
        {
            buffer.read(this->pressure);
            buffer.read(this->converged_pressure);
        }

        bool operator==(const NodeData& other) const
        {
            return this->pressure == other.pressure && this->converged_pressure == other.converged_pressure;
        }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(pressure);
            serializer(converged_pressure);
        }

        static NodeData serializationTestObject()
        {
            return NodeData{10.0, 10.0};
        }
    };

    struct BranchData {
        double pressure_drop { 0.0 };
        double oil_rate { 0.0 };
        double water_rate { 0.0 };
        double gas_rate { 0.0 };

        template <class MessageBufferType>
        void write(MessageBufferType& buffer) const
        {
            buffer.write(this->pressure_drop);
            buffer.write(this->oil_rate);
            buffer.write(this->water_rate);
            buffer.write(this->gas_rate);
        }

        template <class MessageBufferType>
        void read(MessageBufferType& buffer)
        {
            buffer.read(this->pressure_drop);
            buffer.read(this->oil_rate);
            buffer.read(this->water_rate);
            buffer.read(this->gas_rate);
        }

        bool operator==(const BranchData& other) const
        {
            return this->pressure_drop == other.pressure_drop
                && this->oil_rate == other.oil_rate
                && this->water_rate == other.water_rate
                && this->gas_rate == other.gas_rate;
        }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(pressure_drop);
            serializer(oil_rate);
            serializer(water_rate);
            serializer(gas_rate);
        }

        static BranchData serializationTestObject()
        {
            return BranchData{10.0, 100.0, 200.0, 20000.0};
        }
    };


    class GroupAndNetworkValues {
    public:
        std::map<std::string, GroupData> groupData {};
        std::map<std::string, NodeData>  nodeData {};
        std::map<std::string, BranchData> branchData {};
        std::map<std::string, BranchData> convergedBranchData {};

        template <class MessageBufferType>
        void write(MessageBufferType& buffer) const
        {
            this->writeMap(this->groupData, buffer);
            this->writeMap(this->nodeData, buffer);
            this->writeMap(this->branchData, buffer);
            this->writeMap(this->convergedBranchData, buffer);
        }

        template <class MessageBufferType>
        void read(MessageBufferType& buffer)
        {
            this->readMap(buffer, this->groupData);
            this->readMap(buffer, this->nodeData);
            this->readMap(buffer, this->branchData);
            this->readMap(buffer, this->convergedBranchData);
        }

        bool operator==(const GroupAndNetworkValues& other) const
        {
            return (this->groupData == other.groupData)
                && (this->nodeData  == other.nodeData)
                && (this->branchData == other.branchData)
                && (this->convergedBranchData == other.convergedBranchData);
        }

        void clear()
        {
            this->groupData.clear();
            this->nodeData.clear();
            this->branchData.clear();
            this->convergedBranchData.clear();
        }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(groupData);
            serializer(nodeData);
            serializer(branchData);
            serializer(convergedBranchData);
        }

        static GroupAndNetworkValues serializationTestObject()
        {
            return GroupAndNetworkValues{
                        {{"test_data", GroupData::serializationTestObject()}},
                        {{"test_node", NodeData::serializationTestObject()}},
                        {{"test_branch", BranchData::serializationTestObject()}},
                        {{"test_converged_branch", BranchData::serializationTestObject()}}
                   };
        }

    private:
        template <class MessageBufferType, class ValueType>
        void writeMap(const std::map<std::string, ValueType>& map,
                      MessageBufferType&                      buffer) const
        {
            const unsigned int size = map.size();
            buffer.write(size);

            for (const auto& [name, elm] : map) {
                buffer.write(name);
                elm   .write(buffer);
            }
        }

        template <class MessageBufferType, class ValueType>
        void readMap(MessageBufferType&                buffer,
                     std::map<std::string, ValueType>& map)
        {
            unsigned int size;
            buffer.read(size);

            for (std::size_t i = 0; i < size; ++i) {
                std::string name;
                buffer.read(name);

                auto elm = ValueType{};
                elm.read(buffer);

                map.emplace(name, std::move(elm));
            }
        }
    };

    /* IMPLEMENTATIONS */

    template <class MessageBufferType>
    void GroupConstraints::write(MessageBufferType& buffer) const {
        buffer.write(this->currentProdConstraint);
        buffer.write(this->currentGasInjectionConstraint);
        buffer.write(this->currentWaterInjectionConstraint);
    }

    template <class MessageBufferType>
    void GroupConstraints::read(MessageBufferType& buffer) {
        buffer.read(this->currentProdConstraint);
        buffer.read(this->currentGasInjectionConstraint);
        buffer.read(this->currentWaterInjectionConstraint);
    }

    inline GroupConstraints&
    GroupConstraints::set(Opm::Group::ProductionCMode cpc,
                          Opm::Group::InjectionCMode  cgic,
                          Opm::Group::InjectionCMode  cwic)
    {
        this->currentGasInjectionConstraint = cgic;
        this->currentWaterInjectionConstraint = cwic;
        this->currentProdConstraint = cpc;

        return *this;
    }

    /// Production and injection rates for reservoir coupling master groups.
    ///
    /// Passed through DynamicSimulatorState to Summary::eval() so that
    /// rate-based summary vectors (FOPR, GOPR, etc.) include slave production.
    struct ReservoirCouplingGroupRates {
        struct ProductionRates {
            double oil{0}, gas{0}, water{0}, resv{0};
        };
        struct InjectionRates {
            double surface{0}, reservoir{0};
        };
        /// Per master-group production rates (positive values, SI units).
        std::map<std::string, ProductionRates> production;
        /// Per master-group, per-phase injection rates (SI units).
        std::map<std::string, std::map<Opm::Phase, InjectionRates>> injection;
    };

}} // Opm::data

#endif //OPM_OUTPUT_GROUPS_HPP
