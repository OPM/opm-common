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

#include <ostream>
#include <type_traits>

#include <ert/ecl/ecl_util.h>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/W.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>

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

    }

    return stream;
}

using un = std::underlying_type< Phase >::type;

Phases::Phases( bool oil, bool gas, bool wat, bool sol, bool pol, bool energy, bool polymw ) noexcept :
    bits( (oil ? (1 << static_cast< un >( Phase::OIL ) )     : 0) |
          (gas ? (1 << static_cast< un >( Phase::GAS ) )     : 0) |
          (wat ? (1 << static_cast< un >( Phase::WATER ) )   : 0) |
          (sol ? (1 << static_cast< un >( Phase::SOLVENT ) ) : 0) |
          (pol ? (1 << static_cast< un >( Phase::POLYMER ) ) : 0) |
          (energy ? (1 << static_cast< un >( Phase::ENERGY ) ) : 0) |
          (polymw ? (1 << static_cast< un >( Phase::POLYMW ) ) : 0) )

{}

bool Phases::active( Phase p ) const noexcept {
    return this->bits[ static_cast< int >( p ) ];
}

size_t Phases::size() const noexcept {
    return this->bits.count();
}

Welldims::Welldims(const Deck& deck)
{
    if (deck.hasKeyword("WELLDIMS")) {
        const auto& wd = deck.getKeyword("WELLDIMS", 0).getRecord(0);

        this->nCWMax = wd.getItem("MAXCONN")      .get<int>(0);
        this->nWGMax = wd.getItem("MAX_GROUPSIZE").get<int>(0);

        // This is the E100 definition.  E300 instead uses
        //
        //   Max{ "MAXGROUPS", "MAXWELLS" }
        //
        // i.e., the maximum of item 1 and item 4 here.
        this->nGMax = wd.getItem("MAXGROUPS").get<int>(0);
	this->nWMax = wd.getItem("MAXWELLS").get<int>(0);
    }
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

EclHysteresisConfig::EclHysteresisConfig() :
    enableHysteresis_( false ),
    pcHysteresisModel_( 0 ),
    krHysteresisModel_( 0 )
    {}


EclHysteresisConfig::EclHysteresisConfig(const Opm::Deck& deck) : EclHysteresisConfig()
    {
        enableHysteresis_ = false;

        if (!deck.hasKeyword("SATOPTS"))
            return;

        const auto& satoptsItem = deck.getKeyword("SATOPTS").getRecord(0).getItem(0);
        for (unsigned i = 0; i < satoptsItem.size(); ++i) {
            std::string satoptsValue = satoptsItem.get< std::string >(0);
            std::transform(satoptsValue.begin(),
                           satoptsValue.end(),
                           satoptsValue.begin(),
                           ::toupper);

            if (satoptsValue == "HYSTER")
                enableHysteresis_ = true;
        }

        // check for the (deprecated) HYST keyword
        if (deck.hasKeyword("HYST"))
            enableHysteresis_ = true;

        if (!enableHysteresis_)
            return;

        if (!deck.hasKeyword("EHYSTR"))
            throw std::runtime_error("Enabling hysteresis via the HYST parameter for SATOPTS requires the "
                                     "presence of the EHYSTR keyword");

        const auto& ehystrKeyword = deck.getKeyword("EHYSTR");
        if (deck.hasKeyword("NOHYKR"))
            krHysteresisModel_ = -1;
        else {
            krHysteresisModel_ = ehystrKeyword.getRecord(0).getItem("relative_perm_hyst").get<int>(0);

            if (krHysteresisModel_ != 0 && krHysteresisModel_ != 1)
                throw std::runtime_error(
                    "Only the Carlson relative permeability hystersis models (indicated by '0' or "
                    "'1' for the second item of the 'EHYSTR' keyword) are supported");
        }

        // this is slightly screwed: it is possible to specify contradicting hysteresis
        // models with HYPC/NOHYPC and the fifth item of EHYSTR. Let's ignore that for
        // now.
        std::string whereFlag =
            ehystrKeyword.getRecord(0).getItem("limiting_hyst_flag").getTrimmedString(0);
        if (deck.hasKeyword("NOHYPC") || whereFlag == "KR")
            pcHysteresisModel_ = -1;
        else {
            // if capillary pressure hysteresis is enabled, Eclipse always uses the
            // Killough model
            pcHysteresisModel_ = 0;

            throw std::runtime_error("Capillary pressure hysteresis is not supported yet");
        }
    }
    
void EclHysteresisConfig::setEnableHysteresis(bool yesno)
    { enableHysteresis_ = yesno; }

const bool EclHysteresisConfig::enableHysteresis() const 
    { return enableHysteresis_; }
    
void EclHysteresisConfig::setPcHysteresisModel(int value)
    { pcHysteresisModel_ = value; }
    
const int EclHysteresisConfig::pcHysteresisModel() const
    { return pcHysteresisModel_; }
    
void EclHysteresisConfig::setKrHysteresisModel(int value)
    { krHysteresisModel_ = value; }
    
const int EclHysteresisConfig::krHysteresisModel() const
    { return krHysteresisModel_; }

    
Runspec::Runspec( const Deck& deck ) :
    active_phases( Phases( deck.hasKeyword( "OIL" ),
                           deck.hasKeyword( "GAS" ),
                           deck.hasKeyword( "WATER" ),
                           deck.hasKeyword( "SOLVENT" ),
                           deck.hasKeyword( "POLYMER" ),
                           deck.hasKeyword( "THERMAL" ),
                           deck.hasKeyword( "POLYMW"  ) ) ),
    m_tabdims( deck ),
    endscale( deck ),
    welldims( deck ),
    wsegdims( deck ),
    udq_params( deck ),
    hystpar( deck )
{}

const Phases& Runspec::phases() const noexcept {
    return this->active_phases;
}

const Tabdims& Runspec::tabdims() const noexcept {
    return this->m_tabdims;
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

const EclHysteresisConfig& Runspec::hysterPar() const noexcept
{
    return this->hystpar;
}

/*
  Returns an integer in the range 0...7 which can be used to indicate
  available phases in Eclipse restart and init files.
*/
int Runspec::eclPhaseMask( ) const noexcept {
    return ( active_phases.active( Phase::WATER ) ? ECL_WATER_PHASE : 0 )
         | ( active_phases.active( Phase::OIL ) ? ECL_OIL_PHASE : 0 )
         | ( active_phases.active( Phase::GAS ) ? ECL_GAS_PHASE : 0 );
}


const UDQParams& Runspec::udqParams() const noexcept {
    return this->udq_params;
}

}
