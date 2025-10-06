#include <opm/input/eclipse/EclipseState/InitConfig/Equil.hpp>

#include <opm/common/OpmLog/KeywordLocation.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/E.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp>

#include <stdexcept>
#include <string>
#include <vector>

#include <stddef.h>

#include <fmt/core.h>

namespace Opm {
    EquilRecord::EquilRecord(const double datum_depth_arg, const double datum_depth_pc_arg,
                             const double woc_depth      , const double woc_pc,
                             const double goc_depth      , const double goc_pc,
                             const bool   live_oil_init  ,
                             const bool   wet_gas_init   ,
                             const int    target_accuracy,
                             const bool   humid_gas_init )
        : datum_depth(datum_depth_arg)
        , datum_depth_ps(datum_depth_pc_arg)
        , water_oil_contact_depth(woc_depth)
        , water_oil_contact_capillary_pressure(woc_pc)
        , gas_oil_contact_depth(goc_depth)
        , gas_oil_contact_capillary_pressure(goc_pc)
        , live_oil_init_proc(live_oil_init)
        , wet_gas_init_proc(wet_gas_init)
        , init_target_accuracy(target_accuracy)
        , humid_gas_init_proc(humid_gas_init)
    {
        // \Note: this is only used in the serializationTestObject(), we throw anyway
        if (gas_oil_contact_depth > water_oil_contact_depth) {
            const std::string msg = fmt::format("Equilibration data specification error:\n"
                                                "Depth of gas-oil contact ({}) is below depth of water-oil contact ({})",
                                                gas_oil_contact_depth, water_oil_contact_depth);
            throw std::logic_error(msg);
        }
    }

    EquilRecord::EquilRecord(const DeckRecord& record, const Phases& phases, int region, const KeywordLocation& location)
        : datum_depth(record.getItem<ParserKeywords::EQUIL::DATUM_DEPTH>().getSIDouble(0))
        , datum_depth_ps(record.getItem<ParserKeywords::EQUIL::DATUM_PRESSURE>().getSIDouble(0))
        , water_oil_contact_depth(record.getItem<ParserKeywords::EQUIL::OWC>().getSIDouble(0))
        , water_oil_contact_capillary_pressure(record.getItem<ParserKeywords::EQUIL::PC_OWC>().getSIDouble(0))
        , gas_oil_contact_depth(record.getItem<ParserKeywords::EQUIL::GOC>().getSIDouble(0))
        , gas_oil_contact_capillary_pressure(record.getItem<ParserKeywords::EQUIL::PC_GOC>().getSIDouble(0))
        , live_oil_init_proc(record.getItem<ParserKeywords::EQUIL::BLACK_OIL_INIT>().get<int>(0) <= 0)
        , wet_gas_init_proc(record.getItem<ParserKeywords::EQUIL::BLACK_OIL_INIT_WG>().get<int>(0) <= 0)
        , init_target_accuracy(record.getItem<ParserKeywords::EQUIL::OIP_INIT>().get<int>(0))
        , humid_gas_init_proc(record.getItem<ParserKeywords::EQUIL::BLACK_OIL_INIT_HG>().get<int>(0) <= 0)
    {
        const bool three_phases = phases.active(Phase::WATER) && phases.active(Phase::OIL) && phases.active(Phase::GAS);
        if (three_phases && (gas_oil_contact_depth > water_oil_contact_depth)) {
            const auto goc_depth_input = record.getItem<ParserKeywords::EQUIL::GOC>().get<double>(0);
            const auto woc_depth_input = record.getItem<ParserKeywords::EQUIL::OWC>().get<double>(0);
            const std::string msg = fmt::format("Equilibration input error for region {}:\n"
                                                "Depth of gas-oil contact ({}) is below depth of water-oil contact ({}).",
                                                region, goc_depth_input, woc_depth_input);
            throw OpmInputError(msg, location);
        }
    }

    EquilRecord EquilRecord::serializationTestObject()
    {
        return EquilRecord{1.0, 2.0, 5.0, 4.0, 3.0, 6.0, true, false, 1, false};
    }

    double EquilRecord::datumDepth() const {
        return this->datum_depth;
    }

    double EquilRecord::datumDepthPressure() const {
        return this->datum_depth_ps;
    }

    double EquilRecord::waterOilContactDepth() const {
        return this->water_oil_contact_depth;
    }

    double EquilRecord::waterOilContactCapillaryPressure() const {
        return this->water_oil_contact_capillary_pressure;
    }

    double EquilRecord::gasOilContactDepth() const {
        return this->gas_oil_contact_depth;
    }

    double EquilRecord::gasOilContactCapillaryPressure() const {
        return this->gas_oil_contact_capillary_pressure;
    }

    bool EquilRecord::liveOilInitConstantRs() const {
        return this->live_oil_init_proc;
    }

    bool EquilRecord::wetGasInitConstantRv() const {
        return this->wet_gas_init_proc;
    }

    int EquilRecord::initializationTargetAccuracy() const {
        return this->init_target_accuracy;
    }

    bool EquilRecord::humidGasInitConstantRvw() const {
        return this->humid_gas_init_proc;
    }

    bool EquilRecord::operator==(const EquilRecord& data) const {
        return datum_depth == data.datum_depth &&
               datum_depth_ps == data.datum_depth_ps &&
               water_oil_contact_depth == data.water_oil_contact_depth &&
               water_oil_contact_capillary_pressure ==
               data.water_oil_contact_capillary_pressure &&
               data.gas_oil_contact_depth == data.gas_oil_contact_depth &&
               gas_oil_contact_capillary_pressure ==
               data.gas_oil_contact_capillary_pressure &&
               live_oil_init_proc == data.live_oil_init_proc &&
               wet_gas_init_proc == data.wet_gas_init_proc &&
               init_target_accuracy == data.init_target_accuracy &&
               humid_gas_init_proc == data.humid_gas_init_proc;
    }

    StressEquilRecord::StressEquilRecord(const DeckRecord& record, const Phases& /* phases */,
                                         int /* region */, const KeywordLocation& /* location */)
        : datum_depth(record.getItem<ParserKeywords::STREQUIL::DATUM_DEPTH>().getSIDouble(0))
        , datum_posx(record.getItem<ParserKeywords::STREQUIL::DATUM_POSX>().getSIDouble(0))
        , datum_posy(record.getItem<ParserKeywords::STREQUIL::DATUM_POSY>().getSIDouble(0))
        , stress_xx(record.getItem<ParserKeywords::STREQUIL::STRESSXX>().getSIDouble(0))
        , stress_xx_grad(record.getItem<ParserKeywords::STREQUIL::STRESSXXGRAD>().getSIDouble(0))
        , stress_yy(record.getItem<ParserKeywords::STREQUIL::STRESSYY>().getSIDouble(0))
        , stress_yy_grad(record.getItem<ParserKeywords::STREQUIL::STRESSYYGRAD>().getSIDouble(0))
        , stress_zz(record.getItem<ParserKeywords::STREQUIL::STRESSZZ>().getSIDouble(0))
        , stress_zz_grad(record.getItem<ParserKeywords::STREQUIL::STRESSZZGRAD>().getSIDouble(0))
        , stress_xy(record.getItem<ParserKeywords::STREQUIL::STRESSXY>().getSIDouble(0))
        , stress_xy_grad(record.getItem<ParserKeywords::STREQUIL::STRESSXYGRAD>().getSIDouble(0))
        , stress_xz(record.getItem<ParserKeywords::STREQUIL::STRESSXZ>().getSIDouble(0))
        , stress_xz_grad(record.getItem<ParserKeywords::STREQUIL::STRESSXZGRAD>().getSIDouble(0))
        , stress_yz(record.getItem<ParserKeywords::STREQUIL::STRESSYZ>().getSIDouble(0))
        , stress_yz_grad(record.getItem<ParserKeywords::STREQUIL::STRESSYZGRAD>().getSIDouble(0))
    {}

    StressEquilRecord StressEquilRecord::serializationTestObject()
    {
        StressEquilRecord result;
        result.datum_depth = 1.0;
        result.datum_posx = 2.0;
        result.datum_posy = 3.0;
        result.stress_xx = 4.0;
        result.stress_xx_grad = 5.0;
        result.stress_yy = 6.0;
        result.stress_yy_grad = 7.0;
        result.stress_zz = 8.0;
        result.stress_zz_grad = 9.0;

        result.stress_xy = 4.0;
        result.stress_xy_grad = 5.0;
        result.stress_xz = 6.0;
        result.stress_xz_grad = 7.0;
        result.stress_yz = 8.0;
        result.stress_yz_grad = 9.0;

        return result;
    }

    double StressEquilRecord::datumDepth() const {
        return this->datum_depth;
    }

    double StressEquilRecord::datumPosX() const {
        return this->datum_posx;
    }

    double StressEquilRecord::datumPosY() const {
        return this->datum_posy;
    }

    double StressEquilRecord::stressXX() const {
        return this->stress_xx;
    }

    double StressEquilRecord::stressXX_grad() const {
        return this->stress_xx_grad;
    }

    double StressEquilRecord::stressYY() const {
        return this->stress_yy;
    }

    double StressEquilRecord::stressYY_grad() const {
        return this->stress_yy_grad;
    }

    double StressEquilRecord::stressZZ() const {
        return this->stress_zz;
    }

    double StressEquilRecord::stressZZ_grad() const {
        return this->stress_zz_grad;
    }

    double StressEquilRecord::stressXY() const
    {
        return this->stress_xy;
    }

    double StressEquilRecord::stressXY_grad() const
    {
        return this->stress_xy_grad;
    }

    double StressEquilRecord::stressXZ() const
    {
        return this->stress_xz;
    }

    double StressEquilRecord::stressXZ_grad() const
    {
        return this->stress_xz_grad;
    }

    double StressEquilRecord::stressYZ() const
    {
        return this->stress_yz;
    }

    double StressEquilRecord::stressYZ_grad() const
    {
        return this->stress_yz_grad;
    }

    bool StressEquilRecord::operator==(const StressEquilRecord& data) const
    {
        return (datum_depth == data.datum_depth)
            && (datum_posx == data.datum_posx)
            && (datum_posy == data.datum_posy)

            // Diagonal terms
            && (stress_xx == data.stress_xx) && (stress_xx_grad == data.stress_xx_grad)
            && (stress_yy == data.stress_yy) && (stress_yy_grad == data.stress_yy_grad)
            && (stress_zz == data.stress_zz) && (stress_zz_grad == data.stress_zz_grad)

            // Cross terms
            && (stress_xy == data.stress_xy) && (stress_xy_grad == data.stress_xy_grad)
            && (stress_xz == data.stress_xz) && (stress_xz_grad == data.stress_xz_grad)
            && (stress_yz == data.stress_yz) && (stress_yz_grad == data.stress_yz_grad)
            ;
    }

    /* ----------------------------------------------------------------- */

    template<class RecordType>
    EquilContainer<RecordType>::EquilContainer(const DeckKeyword& keyword, const Phases& phases)
    {
        int region = 0;
        for (const auto& record : keyword) {
            this->m_records.emplace_back(record, phases, ++region, keyword.location());
        }
    }

    template<class RecordType>
    EquilContainer<RecordType> EquilContainer<RecordType>::serializationTestObject()
    {
        EquilContainer<RecordType> result;
        result.m_records = {RecordType::serializationTestObject()};

        return result;
    }

    template<class RecordType>
    const RecordType& EquilContainer<RecordType>::getRecord(std::size_t id) const {
        return this->m_records.at( id );
    }

    template<class RecordType>
    std::size_t EquilContainer<RecordType>::size() const {
        return this->m_records.size();
    }

    template<class RecordType>
    bool EquilContainer<RecordType>::empty() const {
        return this->m_records.empty();
    }

    template<class RecordType>
    typename EquilContainer<RecordType>::const_iterator
    EquilContainer<RecordType>::begin() const {
        return this->m_records.begin();
    }

    template<class RecordType>
    typename EquilContainer<RecordType>::const_iterator
    EquilContainer<RecordType>::end() const {
        return this->m_records.end();
    }

    template<class RecordType>
    bool EquilContainer<RecordType>::
    operator==(const EquilContainer<RecordType>& data) const {
        return this->m_records == data.m_records;
    }

    template class EquilContainer<EquilRecord>;
    template class EquilContainer<StressEquilRecord>;
}
