#ifndef OPM_EQUIL_HPP
#define OPM_EQUIL_HPP

#include <cstddef>
#include <vector>

namespace Opm {
    class DeckKeyword;
    class DeckRecord;

    class EquilRecord {
        public:
            EquilRecord() = default;
            EquilRecord(double datum_depth_arg, double datum_depth_pc_arg,
                        double woc_depth, double woc_pc,
                        double goc_depth, double goc_pc,
                        bool live_oil_init,
                        bool wet_gas_init,
                        int target_accuracy,
                        bool humid_gas_init);
            explicit EquilRecord(const DeckRecord& record);

            static EquilRecord serializationTestObject();
            double datumDepth() const;
            double datumDepthPressure() const;
            double waterOilContactDepth() const;
            double waterOilContactCapillaryPressure() const;
            double gasOilContactDepth() const;
            double gasOilContactCapillaryPressure() const;

            bool liveOilInitConstantRs() const;
            bool wetGasInitConstantRv() const;
            int initializationTargetAccuracy() const;
            bool humidGasInitConstantRvw() const;

            bool operator==(const EquilRecord& data) const;

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(datum_depth);
                serializer(datum_depth_ps);
                serializer(water_oil_contact_depth);
                serializer(water_oil_contact_capillary_pressure);
                serializer(gas_oil_contact_depth);
                serializer(gas_oil_contact_capillary_pressure);
                serializer(live_oil_init_proc);
                serializer(wet_gas_init_proc);
                serializer(init_target_accuracy);
                serializer(humid_gas_init_proc);
            }

        private:
            double datum_depth = 0.0;
            double datum_depth_ps = 0.0;
            double water_oil_contact_depth = 0.0;
            double water_oil_contact_capillary_pressure = 0.0;
            double gas_oil_contact_depth = 0.0;
            double gas_oil_contact_capillary_pressure = 0.0;

            bool live_oil_init_proc = false;
            bool wet_gas_init_proc = false;
            int init_target_accuracy = 0;
            bool humid_gas_init_proc = false;
    };

    class StressEquilRecord {
        public:
            StressEquilRecord() = default;
            explicit StressEquilRecord(const DeckRecord& record);

            static StressEquilRecord serializationTestObject();

            bool operator==(const StressEquilRecord& data) const;

            double datumDepth() const;
            double datumPosX() const;
            double datumPosY() const;
            double stressXX() const;
            double stressXX_grad() const;
            double stressYY() const;
            double stressYY_grad() const;
            double stressZZ() const;
            double stressZZ_grad() const;

            double stressXY() const;
            double stressXY_grad() const;
            double stressXZ() const;
            double stressXZ_grad() const;
            double stressYZ() const;
            double stressYZ_grad() const;

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(datum_depth);
                serializer(datum_posx);
                serializer(datum_posy);
                serializer(stress_xx);
                serializer(stress_xx_grad);
                serializer(stress_yy);
                serializer(stress_yy_grad);
                serializer(stress_zz);
                serializer(stress_zz_grad);

                serializer(stress_xy);
                serializer(stress_xy_grad);
                serializer(stress_xz);
                serializer(stress_xz_grad);
                serializer(stress_yz);
                serializer(stress_yz_grad);


            }

        private:
            double datum_depth = 0.0;
            double datum_posx = 0.0;
            double datum_posy = 0.0;
            double stress_xx = 0.0;
            double stress_xx_grad = 0.0;
            double stress_yy = 0.0;
            double stress_yy_grad = 0.0;
            double stress_zz = 0.0;
            double stress_zz_grad = 0.0;

            double stress_xy = 0.0;
            double stress_xy_grad = 0.0;
            double stress_xz = 0.0;
            double stress_xz_grad = 0.0;
            double stress_yz = 0.0;
            double stress_yz_grad = 0.0;

            
    };

    template<class RecordType>
    class EquilContainer {
        public:
            using const_iterator = typename std::vector<RecordType>::const_iterator;

            EquilContainer() = default;
            explicit EquilContainer( const DeckKeyword& );

            static EquilContainer serializationTestObject();

            const RecordType& getRecord(std::size_t id) const;

            size_t size() const;
            bool empty() const;

            const_iterator begin() const;
            const_iterator end() const;

            bool operator==(const EquilContainer& data) const;

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(m_records);
            }

        private:
            std::vector<RecordType> m_records;
    };

    using Equil = EquilContainer<EquilRecord>;
    using StressEquil = EquilContainer<StressEquilRecord>;
}

#endif //OPM_EQUIL_HPP
