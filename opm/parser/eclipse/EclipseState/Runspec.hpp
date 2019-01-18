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

#ifndef OPM_RUNSPEC_HPP
#define OPM_RUNSPEC_HPP

#include <iosfwd>
#include <string>

#include <opm/parser/eclipse/EclipseState/Tables/Tabdims.hpp>
#include <opm/parser/eclipse/EclipseState/EndpointScaling.hpp>
#include <opm/parser/eclipse/EclipseState/UDQParams.hpp>

namespace Opm {
class Deck;


enum class Phase {
    OIL     = 0,
    GAS     = 1,
    WATER   = 2,
    SOLVENT = 3,
    POLYMER = 4,
    ENERGY  = 5,
    POLYMW  = 6
};

Phase get_phase( const std::string& );
std::ostream& operator<<( std::ostream&, const Phase& );

class Phases {
    public:
        Phases() noexcept = default;
        Phases( bool oil, bool gas, bool wat, bool solvent = false, bool polymer = false, bool energy = false,
                bool polymw = false ) noexcept;

        bool active( Phase ) const noexcept;
        size_t size() const noexcept;
    private:
        std::bitset< 7 > bits;
};


class Welldims {
public:
    explicit Welldims(const Deck& deck);

    int maxConnPerWell() const
    {
        return this->nCWMax;
    }

    int maxWellsPerGroup() const
    {
        return this->nWGMax;
    }

    int maxGroupsInField() const
    {
        return this->nGMax;
    }

        int maxWellsInField() const
    {
        return this->nWMax;
    }
private:
    int nWMax  { 0 };
    int nCWMax { 0 };
    int nWGMax { 0 };
    int nGMax  { 0 };
};

class WellSegmentDims {
public:
    WellSegmentDims();
    explicit WellSegmentDims(const Deck& deck);

    int maxSegmentedWells() const
    {
        return this->nSegWellMax;
    }

    int maxSegmentsPerWell() const
    {
        return this->nSegmentMax;
    }

    int maxLateralBranchesPerWell() const
    {
        return this->nLatBranchMax;
    }

private:
    int nSegWellMax;
    int nSegmentMax;
    int nLatBranchMax;
};

class EclHysterConfig
{
public:
      EclHysterConfig();
      explicit EclHysterConfig(const Deck& deck);


    /*!
     * \brief Specify whether hysteresis is enabled or not.
     */
    void setEnableHysteresis(bool yesno);

    /*!
     * \brief Returns whether hysteresis is enabled.
     */
    const bool enableHysteresis() const;

    /*!
     * \brief Set the type of the hysteresis model which is used for capillary pressure.
     *
     * -1: capillary pressure hysteresis is disabled
     * 0: use the Killough model for capillary pressure hysteresis
     */
    void setPcHysteresisModel(int value);

    /*!
     * \brief Return the type of the hysteresis model which is used for capillary pressure.
     *
     * -1: capillary pressure hysteresis is disabled
     * 0: use the Killough model for capillary pressure hysteresis
     */
    const int pcHysteresisModel() const;

    /*!
     * \brief Set the type of the hysteresis model which is used for relative permeability.
     *
     * -1: relperm hysteresis is disabled
     * 0: use the Carlson model for relative permeability hysteresis of the non-wetting
     *    phase and the drainage curve for the relperm of the wetting phase
     * 1: use the Carlson model for relative permeability hysteresis of the non-wetting
     *    phase and the imbibition curve for the relperm of the wetting phase
     */
    void setKrHysteresisModel(int value);

    /*!
     * \brief Return the type of the hysteresis model which is used for relative permeability.
     *
     * -1: relperm hysteresis is disabled
     * 0: use the Carlson model for relative permeability hysteresis
     */
    const int krHysteresisModel() const;

private:
    // enable hysteresis at all
    bool enableHysteresis_;

    // the capillary pressure and the relperm hysteresis models to be used
    int pcHysteresisModel_;
    int krHysteresisModel_;
};



class Runspec {
   public:
        explicit Runspec( const Deck& );

        const UDQParams& udqParams() const noexcept;
        const Phases& phases() const noexcept;
        const Tabdims&  tabdims() const noexcept;
        const EndpointScaling& endpointScaling() const noexcept;
        const Welldims& wellDimensions() const noexcept;
        const WellSegmentDims& wellSegmentDimensions() const noexcept;
        int eclPhaseMask( ) const noexcept;
	const EclHysterConfig& hysterPar() const noexcept;

   private:
        Phases active_phases;
        Tabdims m_tabdims;
        EndpointScaling endscale;
        Welldims welldims;
        WellSegmentDims wsegdims;
        UDQParams udq_params;
	EclHysterConfig hystpar;
};


}

#endif // OPM_RUNSPEC_HPP

