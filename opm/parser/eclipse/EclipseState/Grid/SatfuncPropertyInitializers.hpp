/*
  Copyright 2014 Andreas Lauser

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
#ifndef ECLIPSE_SATFUNCPROPERTY_INITIALIZERS_HPP
#define ECLIPSE_SATFUNCPROPERTY_INITIALIZERS_HPP

#include <vector>
#include <string>

#include <opm/parser/eclipse/EclipseState/Grid/GridPropertyInitializers.hpp>


namespace Opm {

class Deck;
class EclipseState;
class EnptvdTable;
class ImptvdTable;
class TableContainer;


class EndpointInitializer : public GridPropertyBaseInitializer<double> {
public:
    enum SaturationFunctionFamily { noFamily = 0, FamilyI = 1, FamilyII = 2};

    /*
      See the "Saturation Functions" chapter in the Eclipse Technical
      Description; there are several alternative families of keywords
      which can be used to enter relperm and capillary pressure
      tables.
    */


protected:

    /*
      The method here goes through the saturation function tables
      Either family I (SWOF,SGOF) or family II (SWFN, SGFN and SOF3)
      must be specified. Other keyword alternatives like SOF2
      and SGWFN and the two dimensional saturation tables
      are currently not supported.

      ** Must be fixed. **
      */


    void findSaturationEndpoints( const EclipseState& ) const;
    void findCriticalPoints( const EclipseState& ) const;
    void findVerticalPoints( const EclipseState& ) const;

    // The saturation function family.
    // If SWOF and SGOF are specified in the deck it return FamilyI
    // If SWFN, SGFN and SOF3 are specified in the deck it return FamilyII
    // If keywords are missing or mixed, an error is given.
    SaturationFunctionFamily
    getSaturationFunctionFamily( const EclipseState& ) const;

    double selectValue(const TableContainer& depthTables,
                       int tableIdx,
                       const std::string& columnName,
                       double cellDepth,
                       double fallbackValue,
                       bool useOneMinusTableValue) const;

    mutable std::vector<double> m_criticalGasSat;
    mutable std::vector<double> m_criticalWaterSat;
    mutable std::vector<double> m_criticalOilOWSat;
    mutable std::vector<double> m_criticalOilOGSat;

    mutable std::vector<double> m_minGasSat;
    mutable std::vector<double> m_maxGasSat;
    mutable std::vector<double> m_minWaterSat;
    mutable std::vector<double> m_maxWaterSat;

    mutable std::vector<double> m_maxPcow;
    mutable std::vector<double> m_maxPcog;
    mutable std::vector<double> m_maxKrw;
    mutable std::vector<double> m_krwr;
    mutable std::vector<double> m_maxKro;
    mutable std::vector<double> m_krorw;
    mutable std::vector<double> m_krorg;
    mutable std::vector<double> m_maxKrg;
    mutable std::vector<double> m_krgr;
};


class SatnumEndpointInitializer : public EndpointInitializer {
public:
    void apply(std::vector<double>&, const Deck&, const EclipseState& ) const = 0;


    void satnumApply( std::vector<double>& values,
                      const std::string& columnName,
                      const std::vector<double>& fallbackValues,
                      const Deck& m_deck,
                      const EclipseState& m_eclipseState,
                      bool useOneMinusTableValue
                      ) const;
};



class ImbnumEndpointInitializer : public EndpointInitializer {
public:

    void apply(std::vector<double>&, const Deck&, const EclipseState& ) const = 0;

    void imbnumApply( std::vector<double>& values,
                      const std::string& columnName,
                      const std::vector<double>& fallBackValues ,
                      const Deck& m_deck,
                      const EclipseState& m_eclipseState,
                      bool useOneMinusTableValue
                      ) const;
};


/*****************************************************************/

class SGLEndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "SGCO" , this->m_minGasSat , deck, es, false);
    }
};


class ISGLEndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "SGCO" , this->m_minGasSat , deck, es, false);
    }
};

/*****************************************************************/

class SGUEndpointInitializer : public SatnumEndpointInitializer {
public:
    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "SGMAX" , this->m_maxGasSat, deck, es, false);
    }
};


class ISGUEndpointInitializer : public ImbnumEndpointInitializer {
public:
    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "SGMAX" , this->m_maxGasSat , deck, es, false);
    }
};

/*****************************************************************/

class SWLEndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "SWCO" , this->m_minWaterSat , deck, es, false);
    }
};

class ISWLEndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "SWCO" , this->m_minWaterSat , deck, es, false);
    }
};

/*****************************************************************/

class SWUEndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "SWMAX" , this->m_maxWaterSat , deck, es, true);
    }
};

class ISWUEndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "SWMAX" , this->m_maxWaterSat , deck, es, true);
    }
};

/*****************************************************************/

class SGCREndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "SGCRIT" , this->m_criticalGasSat , deck, es, false);
    }
};

class ISGCREndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "SGCRIT" , this->m_criticalGasSat , deck, es, false);
    }
};

/*****************************************************************/

class SOWCREndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "SOWCRIT", this->m_criticalOilOWSat , deck, es, false);
    }
};

class ISOWCREndpointInitializer : public ImbnumEndpointInitializer
{
public:
    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "SOWCRIT" , this->m_criticalOilOWSat , deck, es, false);
    }
};

/*****************************************************************/

class SOGCREndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "SOGCRIT" , this->m_criticalOilOGSat , deck, es, false);
    }
};

class ISOGCREndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "SOGCRIT" , this->m_criticalOilOGSat , deck, es, false);
    }
};

/*****************************************************************/

class SWCREndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "SWCRIT" , this->m_criticalWaterSat , deck, es, false);
    }
};

class ISWCREndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "SWCRIT" , this->m_criticalWaterSat , deck, es, false);
    }
};

/*****************************************************************/

class PCWEndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "PCW" , this->m_maxPcow , deck, es, false);
    }
};

class IPCWEndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "IPCW" , this->m_maxPcow , deck, es, false);
    }
};


/*****************************************************************/

class PCGEndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "PCG" , this->m_maxPcog , deck, es, false);
    }
};

class IPCGEndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "IPCG" , this->m_maxPcog , deck, es, false);
    }
};

/*****************************************************************/

class KRWEndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "KRW" , this->m_maxKrw , deck, es, false);
    }
};

class IKRWEndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "IKRW" , this->m_maxKrw , deck, es, false);
    }
};


/*****************************************************************/

class KRWREndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "KRWR" , this->m_krwr , deck, es, false);
    }
};

class IKRWREndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "IKRWR" , this->m_krwr , deck, es, false);
    }
};

/*****************************************************************/

class KROEndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "KRO" , this->m_maxKro , deck, es, false);
    }
};

class IKROEndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "IKRO" , this->m_maxKro , deck, es, false);
    }
};


/*****************************************************************/

class KRORWEndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "KRORW" , this->m_krorw , deck, es, false);
    }
};

class IKRORWEndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "IKRORW" , this->m_krorw , deck, es, false);
    }
};

/*****************************************************************/

class KRORGEndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "KRORG" , this->m_krorg , deck, es, false);
    }
};

class IKRORGEndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "IKRORG" , this->m_krorg , deck, es, false);
    }
};

/*****************************************************************/

class KRGEndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "KRG" , this->m_maxKrg , deck, es, false);
    }
};

class IKRGEndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "IKRG" , this->m_maxKrg , deck, es, false);
    }
};

/*****************************************************************/

class KRGREndpointInitializer : public SatnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->satnumApply(values , "KRGR" , this->m_krgr , deck, es, false);
    }
};



class IKRGREndpointInitializer : public ImbnumEndpointInitializer {
public:

    void apply(std::vector<double>& values, const Deck& deck, const EclipseState& es ) const
    {
        this->imbnumApply(values , "IKRGR" , this->m_krgr , deck, es, false);
    }
};

}

#endif
