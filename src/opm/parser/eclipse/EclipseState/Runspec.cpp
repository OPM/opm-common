/*
  Copyright 2016  Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the terms
  of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  OPM is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <opm/parser/eclipse/EclipseState/Runspec.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckSection.hpp>

#include <opm/parser/eclipse/Parser/ParserKeywords/A.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/B.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/F.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/N.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/O.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/S.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/T.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/W.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <ostream>
#include <stdexcept>
#include <type_traits>

namespace {
    Opm::Phases inferActivePhases(const Opm::Deck& deck)
    {
        return {
            deck.hasKeyword<Opm::ParserKeywords::OIL>(),
            deck.hasKeyword<Opm::ParserKeywords::GAS>(),
            deck.hasKeyword<Opm::ParserKeywords::WATER>(),
            deck.hasKeyword<Opm::ParserKeywords::SOLVENT>(),
            deck.hasKeyword<Opm::ParserKeywords::POLYMER>(),
            deck.hasKeyword<Opm::ParserKeywords::THERMAL>(),
            deck.hasKeyword<Opm::ParserKeywords::POLYMW>(),
            deck.hasKeyword<Opm::ParserKeywords::FOAM>(),
            deck.hasKeyword<Opm::ParserKeywords::BRINE>(),
            deck.hasKeyword<Opm::ParserKeywords::PVTSOL>()
        };
    }

    Opm::SatFuncControls::ThreePhaseOilKrModel
    inferThreePhaseOilKrModel(const Opm::Deck& deck)
    {
        using KroModel = Opm::SatFuncControls::ThreePhaseOilKrModel;

        if (deck.hasKeyword<Opm::ParserKeywords::STONE1>())
            return KroModel::Stone1;

        if (deck.hasKeyword<Opm::ParserKeywords::STONE>() ||
            deck.hasKeyword<Opm::ParserKeywords::STONE2>())
            return KroModel::Stone2;

        return KroModel::Default;
    }

    Opm::SatFuncControls::KeywordFamily
    inferKeywordFamily(const Opm::Deck& deck)
    {
        const auto phases = inferActivePhases(deck);
        const auto wat    = phases.active(Opm::Phase::WATER);
        const auto oil    = phases.active(Opm::Phase::OIL);
        const auto gas    = phases.active(Opm::Phase::GAS);

        const auto threeP = gas && oil && wat;
        const auto twoP = (!gas && oil && wat) || (gas && oil && !wat);

        const auto family1 =       // SGOF/SLGOF and/or SWOF
            (gas && (deck.hasKeyword<Opm::ParserKeywords::SGOF>() ||
                     deck.hasKeyword<Opm::ParserKeywords::SLGOF>())) ||
            (wat && deck.hasKeyword<Opm::ParserKeywords::SWOF>());

        // note: we allow for SOF2 to be part of family1 for threeP +
        // solvent simulations.

        const auto family2 =      // SGFN, SOF{2,3}, SWFN
            (gas && deck.hasKeyword<Opm::ParserKeywords::SGFN>()) ||
            (oil && ((threeP && deck.hasKeyword<Opm::ParserKeywords::SOF3>()) ||
                     (twoP && deck.hasKeyword<Opm::ParserKeywords::SOF2>()))) ||
            (wat && deck.hasKeyword<Opm::ParserKeywords::SWFN>());

        if (family1)
            return Opm::SatFuncControls::KeywordFamily::Family_I;

        if (family2)
            return Opm::SatFuncControls::KeywordFamily::Family_II;

        return Opm::SatFuncControls::KeywordFamily::Undefined;
    }
}

namespace Opm {

Phase get_phase( const std::string& str ) {
    if( str == "OIL" ) return Phase::OIL;
    if( str == "GAS" ) return Phase::GAS;
    if( str == "WAT" ) return Phase::WATER;
    if( str == "WATER" )   return Phase::WATER;
    if( str == "SOLVENT" ) return Phase::SOLVENT;
    if( str == "POLYMER" ) return Phase::POLYMER;
    if( str == "ENERGY" ) return Phase::ENERGY;
    if( str == "POLYMW" ) return Phase::POLYMW;
    if( str == "FOAM" ) return Phase::FOAM;
    if( str == "BRINE" ) return Phase::BRINE;
    if( str == "ZFRACTION" ) return Phase::ZFRACTION;

    throw std::invalid_argument( "Unknown phase '" + str + "'" );
}

std::ostream& operator<<( std::ostream& stream, const Phase& p ) {
    switch( p ) {
        case Phase::OIL:     return stream << "OIL";
        case Phase::GAS:     return stream << "GAS";
        case Phase::WATER:   return stream << "WATER";
        case Phase::SOLVENT: return stream << "SOLVENT";
        case Phase::POLYMER: return stream << "POLYMER";
        case Phase::ENERGY:  return stream << "ENERGY";
        case Phase::POLYMW:  return stream << "POLYMW";
        case Phase::FOAM:    return stream << "FOAM";
        case Phase::BRINE:   return stream << "BRINE";
        case Phase::ZFRACTION:    return stream << "ZFRACTION";

    }

    return stream;
}

using un = std::underlying_type< Phase >::type;

Phases::Phases( bool oil, bool gas, bool wat, bool sol, bool pol, bool energy, bool polymw, bool foam, bool brine, bool zfraction) noexcept :
    bits( (oil ? (1 << static_cast< un >( Phase::OIL ) )     : 0) |
          (gas ? (1 << static_cast< un >( Phase::GAS ) )     : 0) |
          (wat ? (1 << static_cast< un >( Phase::WATER ) )   : 0) |
          (sol ? (1 << static_cast< un >( Phase::SOLVENT ) ) : 0) |
          (pol ? (1 << static_cast< un >( Phase::POLYMER ) ) : 0) |
          (energy ? (1 << static_cast< un >( Phase::ENERGY ) ) : 0) |
          (polymw ? (1 << static_cast< un >( Phase::POLYMW ) ) : 0) |
          (foam ? (1 << static_cast< un >( Phase::FOAM ) ) : 0) |
          (brine ? (1 << static_cast< un >( Phase::BRINE ) ) : 0) |
          (zfraction ? (1 << static_cast< un >( Phase::ZFRACTION ) ) : 0) )

{}

Phases Phases::serializeObject()
{
    return Phases(true, true, true, false, true, false, true, false);
}

bool Phases::active( Phase p ) const noexcept {
    return this->bits[ static_cast< int >( p ) ];
}

size_t Phases::size() const noexcept {
    return this->bits.count();
}

bool Phases::operator==(const Phases& data) const {
    return bits == data.bits;
}

Welldims::Welldims(const Deck& deck)
{
    using WD = ParserKeywords::WELLDIMS;
    if (deck.hasKeyword<WD>()) {
        const auto& keyword = deck.getKeyword<WD>(0);
        const auto& wd = keyword.getRecord(0);

        this->nCWMax = wd.getItem<WD::MAXCONN>().get<int>(0);
        this->nWGMax = wd.getItem<WD::MAX_GROUPSIZE>().get<int>(0);

        // This is the E100 definition.  E300 instead uses
        //
        //   Max{ "MAXGROUPS", "MAXWELLS" }
        //
        // i.e., the maximum of item 1 and item 4 here.
        this->nGMax = wd.getItem<WD::MAXGROUPS>().get<int>(0);
	      this->nWMax = wd.getItem<WD::MAXWELLS>().get<int>(0);

        this->m_location = keyword.location();
    }
}

Welldims Welldims::serializeObject()
{
    Welldims result;
    result.nWMax = 1;
    result.nCWMax = 2;
    result.nWGMax = 3;
    result.nGMax = 4;
    result.m_location = KeywordLocation::serializeObject();
    return result;
}

WellSegmentDims::WellSegmentDims() :
    nSegWellMax( ParserKeywords::WSEGDIMS::NSWLMX::defaultValue ),
    nSegmentMax( ParserKeywords::WSEGDIMS::NSEGMX::defaultValue ),
    nLatBranchMax( ParserKeywords::WSEGDIMS::NLBRMX::defaultValue )
{}

WellSegmentDims::WellSegmentDims(const Deck& deck) : WellSegmentDims()
{
    if (deck.hasKeyword("WSEGDIMS")) {
        const auto& wsd = deck.getKeyword("WSEGDIMS", 0).getRecord(0);

        this->nSegWellMax   = wsd.getItem("NSWLMX").get<int>(0);
        this->nSegmentMax   = wsd.getItem("NSEGMX").get<int>(0);
        this->nLatBranchMax = wsd.getItem("NLBRMX").get<int>(0);
    }
}

WellSegmentDims WellSegmentDims::serializeObject()
{
    WellSegmentDims result;
    result.nSegWellMax = 1;
    result.nSegmentMax = 2;
    result.nLatBranchMax = 3;

    return result;
}

bool WellSegmentDims::operator==(const WellSegmentDims& data) const
{
    return this->maxSegmentedWells() == data.maxSegmentedWells() &&
           this->maxSegmentsPerWell() == data.maxSegmentsPerWell() &&
           this->maxLateralBranchesPerWell() == data.maxLateralBranchesPerWell();
}

NetworkDims::NetworkDims() :
    nMaxNoNodes( 0 ),
    nMaxNoBranches( 0 ),
    nMaxNoBranchesConToNode( ParserKeywords::NETWORK::NBCMAX::defaultValue )
{}

NetworkDims::NetworkDims(const Deck& deck) : NetworkDims()
{
    if (deck.hasKeyword("NETWORK")) {
        const auto& wsd = deck.getKeyword("NETWORK", 0).getRecord(0);

        this->nMaxNoNodes   = wsd.getItem("NODMAX").get<int>(0);
        this->nMaxNoBranches   = wsd.getItem("NBRMAX").get<int>(0);
        this->nMaxNoBranchesConToNode = wsd.getItem("NBCMAX").get<int>(0);
    }
}

NetworkDims NetworkDims::serializeObject()
{
    NetworkDims result;
    result.nMaxNoNodes = 1;
    result.nMaxNoBranches = 2;
    result.nMaxNoBranchesConToNode = 3;

    return result;
}

bool NetworkDims::operator==(const NetworkDims& data) const
{
    return this->maxNONodes() == data.maxNONodes() &&
           this->maxNoBranches() == data.maxNoBranches() &&
           this->maxNoBranchesConToNode() == data.maxNoBranchesConToNode();
}

AquiferDimensions::AquiferDimensions()
    : maxNumAnalyticAquifers   { ParserKeywords::AQUDIMS::NANAQU::defaultValue }
    , maxNumAnalyticAquiferConn{ ParserKeywords::AQUDIMS::NCAMAX::defaultValue }
{}

AquiferDimensions::AquiferDimensions(const Deck& deck)
    : AquiferDimensions{}
{
    using AD = ParserKeywords::AQUDIMS;

    if (deck.hasKeyword<AD>()) {
        const auto& keyword = deck.getKeyword<AD>(0);
        const auto& ad = keyword.getRecord(0);

        this->maxNumAnalyticAquifers    = ad.getItem<AD::NANAQU>().get<int>(0);
        this->maxNumAnalyticAquiferConn = ad.getItem<AD::NCAMAX>().get<int>(0);
    }
}

AquiferDimensions AquiferDimensions::serializeObject()
{
    auto dim = AquiferDimensions{};

    dim.maxNumAnalyticAquifers = 3;
    dim.maxNumAnalyticAquiferConn = 10;

    return dim;
}

bool operator==(const AquiferDimensions& lhs, const AquiferDimensions& rhs)
{
    return (lhs.maxAnalyticAquifers() == rhs.maxAnalyticAquifers())
        && (lhs.maxAnalyticAquiferConnections() == rhs.maxAnalyticAquiferConnections());
}

EclHysterConfig::EclHysterConfig(const Opm::Deck& deck)
    {

        if (!deck.hasKeyword("SATOPTS"))
            return;

        const auto& satoptsItem = deck.getKeyword("SATOPTS").getRecord(0).getItem(0);
        for (unsigned i = 0; i < satoptsItem.data_size(); ++i) {
            std::string satoptsValue = satoptsItem.get< std::string >(0);
            std::transform(satoptsValue.begin(),
                           satoptsValue.end(),
                           satoptsValue.begin(),
                           ::toupper);

            if (satoptsValue == "HYSTER")
                activeHyst = true;
        }

        // check for the (deprecated) HYST keyword
        if (deck.hasKeyword("HYST"))
            activeHyst = true;

        if (!activeHyst)
	      return;

        if (!deck.hasKeyword("EHYSTR"))
            throw std::runtime_error("Enabling hysteresis via the HYST parameter for SATOPTS requires the "
                                     "presence of the EHYSTR keyword");
	    /*!
	* \brief Set the type of the hysteresis model which is used for relative permeability.
	*
	* -1: relperm hysteresis is disabled
	* 0: use the Carlson model for relative permeability hysteresis of the non-wetting
	*    phase and the drainage curve for the relperm of the wetting phase
	* 1: use the Carlson model for relative permeability hysteresis of the non-wetting
	*    phase and the imbibition curve for the relperm of the wetting phase
	*/
        const auto& ehystrKeyword = deck.getKeyword("EHYSTR");
        if (deck.hasKeyword("NOHYKR"))
            krHystMod = -1;
        else {
            krHystMod = ehystrKeyword.getRecord(0).getItem("relative_perm_hyst").get<int>(0);

            if (krHystMod != 0 && krHystMod != 1)
                throw std::runtime_error(
                    "Only the Carlson relative permeability hysteresis models (indicated by '0' or "
                    "'1' for the second item of the 'EHYSTR' keyword) are supported");
        }

        // this is slightly screwed: it is possible to specify contradicting hysteresis
        // models with HYPC/NOHYPC and the fifth item of EHYSTR. Let's ignore that for
        // now.
            /*!
	* \brief Return the type of the hysteresis model which is used for capillary pressure.
	*
	* -1: capillary pressure hysteresis is disabled
	* 0: use the Killough model for capillary pressure hysteresis
	*/
        std::string whereFlag =
            ehystrKeyword.getRecord(0).getItem("limiting_hyst_flag").getTrimmedString(0);
        if (deck.hasKeyword("NOHYPC") || whereFlag == "KR")
            pcHystMod = -1;
        else {
            // if capillary pressure hysteresis is enabled, Eclipse always uses the
            // Killough model
            pcHystMod = 0;

            throw std::runtime_error("Capillary pressure hysteresis is not supported yet");
        }
    }

EclHysterConfig EclHysterConfig::serializeObject()
{
    EclHysterConfig result;
    result.activeHyst = true;
    result.pcHystMod = 1;
    result.krHystMod = 2;

    return result;
}

bool EclHysterConfig::active() const
    { return activeHyst; }

int EclHysterConfig::pcHysteresisModel() const
    { return pcHystMod; }

int EclHysterConfig::krHysteresisModel() const
    { return krHystMod; }

bool EclHysterConfig::operator==(const EclHysterConfig& data) const {
    return this->active() == data.active() &&
           this->pcHysteresisModel() == data.pcHysteresisModel() &&
           this->krHysteresisModel() == data.krHysteresisModel();
}

SatFuncControls::SatFuncControls()
    : tolcrit(ParserKeywords::TOLCRIT::VALUE::defaultValue)
{}

SatFuncControls::SatFuncControls(const Deck& deck)
    : SatFuncControls()
{
    using TolCrit = ParserKeywords::TOLCRIT;

    if (deck.hasKeyword<TolCrit>()) {
        // SIDouble doesn't perform any unit conversions here since
        // TOLCRIT is a pure scalar (Dimension = 1).
        this->tolcrit = deck.getKeyword<TolCrit>(0).getRecord(0)
            .getItem<TolCrit::VALUE>().getSIDouble(0);
    }

    this->krmodel = inferThreePhaseOilKrModel(deck);
    this->satfunc_family = inferKeywordFamily(deck);
}

SatFuncControls::SatFuncControls(const double tolcritArg,
                                 const ThreePhaseOilKrModel model,
                                 const KeywordFamily family)
    : tolcrit(tolcritArg)
    , krmodel(model)
    , satfunc_family(family)
{}

SatFuncControls SatFuncControls::serializeObject()
{
    return SatFuncControls {
        1.0, ThreePhaseOilKrModel::Stone2, KeywordFamily::Family_I
    };
}

bool SatFuncControls::operator==(const SatFuncControls& rhs) const
{
    return (this->minimumRelpermMobilityThreshold() == rhs.minimumRelpermMobilityThreshold())
        && (this->krModel() == rhs.krModel())
        && (this->family() == rhs.family());
}

Runspec::Runspec( const Deck& deck ) :
    active_phases( inferActivePhases(deck) ),
    m_tabdims( deck ),
    endscale( deck ),
    welldims( deck ),
    wsegdims( deck ),
    netwrkdims( deck ),
    udq_params( deck ),
    hystpar( deck ),
    m_actdims( deck ),
    m_sfuncctrl( deck ),
    m_nupcol( ParserKeywords::NUPCOL::NUM_ITER::defaultValue ),
    m_co2storage (false)
{
    if (DeckSection::hasRUNSPEC(deck)) {
        const RUNSPECSection runspecSection{deck};
        if (runspecSection.hasKeyword("NUPCOL")) {
            using NC = ParserKeywords::NUPCOL;
            const auto& item = runspecSection.getKeyword<NC>().getRecord(0).getItem<NC::NUM_ITER>();
            m_nupcol = item.get<int>(0);
            if (item.defaultApplied(0)) {
                std::string msg = "OPM Flow uses 12 as default NUPCOL value";
                OpmLog::note(msg);
            }
        }
        if (runspecSection.hasKeyword<ParserKeywords::CO2STORE>() ||
                runspecSection.hasKeyword<ParserKeywords::CO2STOR>()) {
            m_co2storage = true;
            std::string msg = "The CO2 storage option is given. PVT properties from the Brine-CO2 system is used \n"
                              "See the OPM manual for details on the used models.";
            OpmLog::note(msg);
        }
    }
}

Runspec Runspec::serializeObject()
{
    Runspec result;
    result.active_phases = Phases::serializeObject();
    result.m_tabdims = Tabdims::serializeObject();
    result.endscale = EndpointScaling::serializeObject();
    result.welldims = Welldims::serializeObject();
    result.wsegdims = WellSegmentDims::serializeObject();
    result.aquiferdims = AquiferDimensions::serializeObject();
    result.udq_params = UDQParams::serializeObject();
    result.hystpar = EclHysterConfig::serializeObject();
    result.m_actdims = Actdims::serializeObject();
    result.m_sfuncctrl = SatFuncControls::serializeObject();
    result.m_nupcol = 2;
    result.m_co2storage = true;

    return result;
}

const Phases& Runspec::phases() const noexcept {
    return this->active_phases;
}

const Tabdims& Runspec::tabdims() const noexcept {
    return this->m_tabdims;
}

const Actdims& Runspec::actdims() const noexcept {
    return this->m_actdims;
}

const EndpointScaling& Runspec::endpointScaling() const noexcept {
    return this->endscale;
}

const Welldims& Runspec::wellDimensions() const noexcept
{
    return this->welldims;
}

const WellSegmentDims& Runspec::wellSegmentDimensions() const noexcept
{
    return this->wsegdims;
}

const NetworkDims& Runspec::networkDimensions() const noexcept
{
    return this->netwrkdims;
}

const AquiferDimensions& Runspec::aquiferDimensions() const noexcept
{
    return this->aquiferdims;
}

const EclHysterConfig& Runspec::hysterPar() const noexcept
{
    return this->hystpar;
}

const SatFuncControls& Runspec::saturationFunctionControls() const noexcept
{
    return this->m_sfuncctrl;
}

int Runspec::nupcol() const noexcept
{
    return this->m_nupcol;
}

bool Runspec::co2Storage() const noexcept
{
    return this->m_co2storage;
}

/*
  Returns an integer in the range 0...7 which can be used to indicate
  available phases in Eclipse restart and init files.
*/
int Runspec::eclPhaseMask( ) const noexcept {
    const int water = 1 << 2;
    const int oil   = 1 << 0;
    const int gas   = 1 << 1;

    return ( active_phases.active( Phase::WATER ) ? water : 0 )
         | ( active_phases.active( Phase::OIL ) ? oil : 0 )
         | ( active_phases.active( Phase::GAS ) ? gas : 0 );
}


const UDQParams& Runspec::udqParams() const noexcept {
    return this->udq_params;
}

bool Runspec::operator==(const Runspec& data) const {
    return this->phases() == data.phases() &&
           this->tabdims() == data.tabdims() &&
           this->endpointScaling() == data.endpointScaling() &&
           this->wellDimensions() == data.wellDimensions() &&
           this->wellSegmentDimensions() == data.wellSegmentDimensions() &&
          (this->aquiferDimensions() == data.aquiferDimensions()) &&
           this->hysterPar() == data.hysterPar() &&
           this->actdims() == data.actdims() &&
           this->saturationFunctionControls() == data.saturationFunctionControls() &&
           this->m_nupcol == data.m_nupcol &&
           this->m_co2storage == data.m_co2storage;
}

}
