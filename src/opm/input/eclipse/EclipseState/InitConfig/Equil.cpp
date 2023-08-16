#include <opm/input/eclipse/EclipseState/InitConfig/Equil.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/E.hpp>

#include <vector>

#include <stddef.h>

namespace Opm {

    EquilRecord::EquilRecord()
        : EquilRecord(0.0, 0.0,
                      0.0, 0.0,
                      0.0, 0.0,
                      false,
                      false,
                      0,
                      false)
    {}

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
    {}

    EquilRecord::EquilRecord(const DeckRecord& record)
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
    {}

    EquilRecord EquilRecord::serializationTestObject()
    {
        return EquilRecord{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, true, false, 1, false};
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

    /* ----------------------------------------------------------------- */

    Equil::Equil( const DeckKeyword& keyword )
    {
        using ParserKeywords::EQUIL;

        for (const auto& record : keyword) {
            this->m_records.emplace_back(record);
        }
    }

    Equil Equil::serializationTestObject()
    {
        Equil result;
        result.m_records = {EquilRecord::serializationTestObject()};

        return result;
    }

    const EquilRecord& Equil::getRecord( size_t id ) const {
        return this->m_records.at( id );
    }

    size_t Equil::size() const {
        return this->m_records.size();
    }

    bool Equil::empty() const {
        return this->m_records.empty();
    }

    Equil::const_iterator Equil::begin() const {
        return this->m_records.begin();
    }

    Equil::const_iterator Equil::end() const {
        return this->m_records.end();
    }

    bool Equil::operator==(const Equil& data) const {
        return this->m_records == data.m_records;
    }
}
