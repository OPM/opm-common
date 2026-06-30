/*
  Copyright 2015 Statoil ASA.

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

#include <opm/input/eclipse/Schedule/Tuning.hpp>

#include <opm/input/eclipse/Units/Units.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/T.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

namespace Opm {


NextStep::NextStep(double value, bool every_report)
    : next_tstep(value)
    , persist(every_report)
{}

bool NextStep::operator==(const NextStep& other) const {
    return this->next_tstep == other.next_tstep &&
           this->persist == other.persist;
}

double NextStep::value() const {
    return this->next_tstep;
}

bool NextStep::every_report() const {
    return this->persist;
}

using TuningKw = ParserKeywords::TUNING;
using WsegIterKW = ParserKeywords::WSEGITER;
Tuning::Tuning()
    // Record1
    : TSINIT(std::nullopt)// Let simulator choose initial step if not specified
    , TSMAXZ(TuningKw::TSMAXZ::defaultValue * Metric::Time)
    , TSMINZ(TuningKw::TSMINZ::defaultValue * Metric::Time)
    , TSMCHP(TuningKw::TSMCHP::defaultValue * Metric::Time)
    , TSFMAX(TuningKw::TSFMAX::defaultValue)
    , TSFMIN(TuningKw::TSFMIN::defaultValue)
    , TFDIFF(TuningKw::TFDIFF::defaultValue)
    , TSFCNV(TuningKw::TSFCNV::defaultValue)
    , THRUPT(TuningKw::THRUPT::defaultValue)
    // Record 2
    , TRGTTE(TuningKw::TRGTTE::defaultValue)
    , TRGCNV(TuningKw::TRGCNV::defaultValue)
    , TRGMBE(TuningKw::TRGMBE::defaultValue)
    , TRGLCV(TuningKw::TRGLCV::defaultValue)
    , XXXTTE(TuningKw::XXXTTE::defaultValue)
    , XXXCNV(TuningKw::XXXCNV::defaultValue)
    , XXXMBE(TuningKw::XXXMBE::defaultValue)
    , XXXLCV(TuningKw::XXXLCV::defaultValue)
    , XXXWFL(TuningKw::XXXWFL::defaultValue)
    , TRGFIP(TuningKw::TRGFIP::defaultValue)
    , THIONX(TuningKw::THIONX::defaultValue)
    , TRWGHT(TuningKw::TRWGHT::defaultValue)

    // Record 3
    , NEWTMX(TuningKw::NEWTMX::defaultValue)
    , NEWTMN(TuningKw::NEWTMN::defaultValue)
    , LITMAX(TuningKw::LITMAX::defaultValue)
    , LITMIN(TuningKw::LITMIN::defaultValue)
    , MXWSIT(TuningKw::MXWSIT::defaultValue)
    , MXWPIT(TuningKw::MXWPIT::defaultValue)
    , DDPLIM(TuningKw::DDPLIM::defaultValue * Metric::Pressure)
    , DDSLIM(TuningKw::DDSLIM::defaultValue)
    , TRGDPR(TuningKw::TRGDPR::defaultValue * Metric::Pressure)
    , XXXDPR(0.0 * Metric::Pressure)
    , MNWRFP(TuningKw::MNWRFP::defaultValue)

    , WSEG_MAX_RESTART(WsegIterKW::MAX_TIMES_REDUCED::defaultValue)
    , WSEG_REDUCTION_FACTOR(WsegIterKW::REDUCTION_FACTOR::defaultValue)
    , WSEG_INCREASE_FACTOR(WsegIterKW::INCREASING_FACTOR::defaultValue)
{
}


Tuning Tuning::serializationTestObject() {
    Tuning result;
    result.TSINIT = std::optional<double>{1.0};
    result.TSMAXZ = 2.0;
    result.TSMINZ = 3.0;
    result.TSMCHP = 4.0;
    result.TSFMAX = 5.0;
    result.TSFMIN = 6.0;
    result.TFDIFF = 7.0;
    result.TSFCNV = 8.0;
    result.THRUPT = 9.0;
    result.TMAXWC = 10.0;
    result.TMAXWC_has_value = true;

    result.TRGTTE = 11.0;
    result.TRGTTE_has_value = true;
    result.TRGCNV = 12.0;
    result.TRGMBE = 13.0;
    result.TRGLCV = 14.0;
    result.TRGLCV_has_value = true;
    result.XXXTTE = 15.0;
    result.XXXTTE_has_value = true;
    result.XXXCNV = 16.0;
    result.XXXMBE = 17.0;
    result.XXXLCV = 18.0;
    result.XXXLCV_has_value = true;
    result.XXXWFL = 19.0;
    result.XXXWFL_has_value = true;
    result.TRGFIP = 20.0;
    result.TRGFIP_has_value = true;
    result.TRGSFT = 21.0;
    result.TRGSFT_has_value = true;
    result.THIONX = 22.0;
    result.THIONX_has_value = true;
    result.TRWGHT = 23.0;
    result.TRWGHT_has_value = true;

    result.NEWTMX = 24;
    result.NEWTMN = 25;
    result.LITMAX = 26;
    result.LITMAX_has_value = true;
    result.LITMIN = 27;
    result.LITMIN_has_value = true;
    result.MXWSIT = 28;
    result.MXWSIT_has_value = true;
    result.MXWPIT = 29;
    result.MXWPIT_has_value = true;
    result.DDPLIM = 30.0;
    result.DDPLIM_has_value = true;
    result.DDSLIM = 31.0;
    result.DDSLIM_has_value = true;
    result.TRGDPR = 32.0;
    result.TRGDPR_has_value = true;
    result.XXXDPR = 33.0;
    result.XXXDPR_has_value = true;
    result.MNWRFP = 34;
    result.MNWRFP_has_value = true;

    return result;
}

bool Tuning::operator==(const Tuning& data) const {
    return TSINIT == data.TSINIT &&
           TSMAXZ == data.TSMAXZ &&
           TSMINZ == data.TSMINZ &&
           TSMCHP == data.TSMCHP &&
           TSFMAX == data.TSFMAX &&
           TSFMIN == data.TSFMIN &&
           TSFCNV == data.TSFCNV &&
           TFDIFF == data.TFDIFF &&
           THRUPT == data.THRUPT &&
           TMAXWC == data.TMAXWC &&
           TMAXWC_has_value == data.TMAXWC_has_value &&
           TRGTTE == data.TRGTTE &&
           TRGTTE_has_value == data.TRGTTE_has_value &&
           TRGCNV == data.TRGCNV &&
           TRGMBE == data.TRGMBE &&
           TRGLCV == data.TRGLCV &&
           TRGLCV_has_value == data.TRGLCV_has_value &&
           XXXTTE == data.XXXTTE &&
           XXXTTE_has_value == data.XXXTTE_has_value &&
           XXXCNV == data.XXXCNV &&
           XXXMBE == data.XXXMBE &&
           XXXLCV == data.XXXLCV &&
           XXXLCV_has_value == data.XXXLCV_has_value &&
           XXXWFL == data.XXXWFL &&
           XXXWFL_has_value == data.XXXWFL_has_value &&
           TRGFIP == data.TRGFIP &&
           TRGFIP_has_value == data.TRGFIP_has_value &&
           TRGSFT == data.TRGSFT &&
           TRGSFT_has_value == data.TRGSFT_has_value &&
           THIONX == data.THIONX &&
           THIONX_has_value == data.THIONX_has_value &&
           TRWGHT == data.TRWGHT &&
           TRWGHT_has_value == data.TRWGHT_has_value &&
           NEWTMX == data.NEWTMX &&
           NEWTMN == data.NEWTMN &&
           LITMAX == data.LITMAX &&
           LITMAX_has_value == data.LITMAX_has_value &&
           LITMIN == data.LITMIN &&
           LITMIN_has_value == data.LITMIN_has_value &&
           MXWSIT == data.MXWSIT &&
           MXWSIT_has_value == data.MXWSIT_has_value &&
           MXWPIT == data.MXWPIT &&
           MXWPIT_has_value == data.MXWPIT_has_value &&
           DDPLIM == data.DDPLIM &&
           DDPLIM_has_value == data.DDPLIM_has_value &&
           DDSLIM == data.DDSLIM &&
           DDSLIM_has_value == data.DDSLIM_has_value &&
           TRGDPR == data.TRGDPR &&
           TRGDPR_has_value == data.TRGDPR_has_value &&
           XXXDPR == data.XXXDPR &&
           XXXDPR_has_value == data.XXXDPR_has_value &&
           MNWRFP == data.MNWRFP &&
           MNWRFP_has_value == data.MNWRFP_has_value &&
           WSEG_MAX_RESTART == data.WSEG_MAX_RESTART &&
           WSEG_REDUCTION_FACTOR == data.WSEG_REDUCTION_FACTOR &&
           WSEG_INCREASE_FACTOR == data.WSEG_INCREASE_FACTOR;
}

// ////
// TUNINGDP
// ////
using TUNINGDP = ParserKeywords::TUNINGDP;
// OBS: When TUNINGDP is _not_ set, we use default from TUNING for TRGLCV and XXXLCV and 0.0 for TRGDDP and TRGDDS
TuningDp::TuningDp()
    : TRGLCV(TuningKw::TRGLCV::defaultValue)
    , XXXLCV(TuningKw::XXXLCV::defaultValue)
    , TRGDDP(0.0 * Metric::Pressure)
    , TRGDDS(0.0)
    , TRGDDRS(0.0 * Metric::GasDissolutionFactor)
    , TRGDDRV(0.0 * Metric::OilDissolutionFactor)
    , TRGDDT(0.0 * Metric::Temperature)
{
}

// OBS: When TUNINGDP _is_ set, we must change the defaults!
void TuningDp::set_defaults()
{
    this->TRGLCV = TUNINGDP::TRGLCV::defaultValue;
    this->XXXLCV = TUNINGDP::XXXLCV::defaultValue;
    this->TRGDDP = TUNINGDP::TRGDDP::defaultValue * Metric::Pressure;
    this->TRGDDS = TUNINGDP::TRGDDS::defaultValue;
    this->TRGDDRS = TUNINGDP::TRGDDRS::defaultValue * Metric::GasDissolutionFactor;
    this->TRGDDRV = TUNINGDP::TRGDDRV::defaultValue * Metric::OilDissolutionFactor;
    this->TRGDDT = TUNINGDP::TRGDDT::defaultValue * Metric::Temperature;

    this->defaults_updated = true;
}

TuningDp TuningDp::serializationTestObject()
{
    TuningDp result;
    result.TRGLCV = 1.0;
    result.TRGLCV_has_value = true;
    result.XXXLCV = 2.0;
    result.XXXLCV_has_value = true;
    result.TRGDDP = 3.0;
    result.TRGDDS = 4.0;
    result.TRGDDRS = 5.0;
    result.TRGDDRV = 6.0;
    result.TRGDDT = 7.0;

    return result;
}

bool TuningDp::operator==(const TuningDp& other) const
{
    return this->defaults_updated == other.defaults_updated &&
           this->TRGLCV == other.TRGLCV &&
           this->TRGLCV_has_value == other.TRGLCV_has_value &&
           this->XXXLCV == other.XXXLCV &&
           this->XXXLCV_has_value == other.XXXLCV_has_value &&
           this->TRGDDP == other.TRGDDP &&
           this->TRGDDS == other.TRGDDS &&
           this->TRGDDRS == other.TRGDDRS &&
           this->TRGDDRV == other.TRGDDRV &&
           this->TRGDDT == other.TRGDDT;
}
}
