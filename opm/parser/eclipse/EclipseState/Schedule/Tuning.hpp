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

#ifndef OPM_TUNING_HPP
#define OPM_TUNING_HPP

namespace Opm {

    template< typename > class DynamicState;

    class TimeMap;

    class Tuning {

    /*
    When the TUNING keyword has occured in the Schedule section and
    has been handled by the Schedule::handleTUNING() method,
    the value for each TUNING keyword item is either
    set from the keyword occurence or a default is set if specified in
    the keyword description. Items that do not have a specified default
    has got a separate <itemname>hasValue() method.

    Before any TUNING keyword has occured in the Schedule section,
    the different TUNING keyword items has got hardcoded default values
    (See Tuning constructor)
    Hardcoded values are set as the same as specified in the keyword description,
    or 0 if no default is specified in the description.
    */

    public:
        explicit Tuning(const TimeMap& timemap);

        void setTuningInitialValue(const std::string tuningItem, double value,bool resetVector);
        void setTuningInitialValue(const std::string tuningItem, int value, bool resetVector);
        
        void getTuningItemValue(const std::string& tuningItem, size_t timestep, double& value);
        void getTuningItemValue(const std::string& tuningItem, size_t timestep, int& value);


        /* Record 1 */
        double getTSINIT(size_t timestep) const;
        double getTSMAXZ(size_t timestep) const;
        double getTSMINZ(size_t timestep) const;
        double getTSMCHP(size_t timestep) const;
        double getTSFMAX(size_t timestep) const;
        double getTSFMIN(size_t timestep) const;
        double getTSFCNV(size_t timestep) const;
        double getTFDIFF(size_t timestep) const;
        double getTHRUPT(size_t timestep) const;
        double getTMAXWC(size_t timestep) const;
        bool   getTMAXWChasValue(size_t timestep) const;
        void   setTSINIT(size_t timestep, double TSINIT);
        void   setTSMAXZ(size_t timestep, double TSMAXZ);
        void   setTSMINZ(size_t timestep, double TSMINZ);
        void   setTSMCHP(size_t timestep, double TSMCHP);
        void   setTSFMAX(size_t timestep, double TSFMAX);
        void   setTSFMIN(size_t timestep, double TSFMIN);
        void   setTSFCNV(size_t timestep, double TSFCNV);
        void   setTFDIFF(size_t timestep, double TFDIFF);
        void   setTHRUPT(size_t timestep, double THRUPT);
        void   setTMAXWC(size_t timestep, double TMAXWC);
        /* Record 2 */
        double getTRGTTE(size_t timestep) const;
        double getTRGCNV(size_t timestep) const;
        double getTRGMBE(size_t timestep) const;
        double getTRGLCV(size_t timestep) const;
        double getXXXTTE(size_t timestep) const;
        double getXXXCNV(size_t timestep) const;
        double getXXXMBE(size_t timestep) const;
        double getXXXLCV(size_t timestep) const;
        double getXXXWFL(size_t timestep) const;
        double getTRGFIP(size_t timestep) const;
        double getTRGSFT(size_t timestep) const;
        bool   getTRGSFThasValue(size_t timestep) const;
        double getTHIONX(size_t timestep) const;
        int    getTRWGHT(size_t timestep) const;
        void   setTRGTTE(size_t timestep, double TRGTTE);
        void   setTRGCNV(size_t timestep, double TRGCNV);
        void   setTRGMBE(size_t timestep, double TRGMBE);
        void   setTRGLCV(size_t timestep, double TRGLCV);
        void   setXXXTTE(size_t timestep, double XXXTTE);
        void   setXXXCNV(size_t timestep, double XXXCNV);
        void   setXXXMBE(size_t timestep, double XXXMBE);
        void   setXXXLCV(size_t timestep, double XXXLCV);
        void   setXXXWFL(size_t timestep, double XXXWFL);
        void   setTRGFIP(size_t timestep, double TRGFIP);
        void   setTRGSFT(size_t timestep, double TRGFIP);
        void   setTHIONX(size_t timestep, double THIONX);
        void   setTRWGHT(size_t timestep, int TRWGHT);
        /* Record 3 */
        int    getNEWTMX(size_t timestep) const;
        int    getNEWTMN(size_t timestep) const;
        int    getLITMAX(size_t timestep) const;
        int    getLITMIN(size_t timestep) const;
        int    getMXWSIT(size_t timestep) const;
        int    getMXWPIT(size_t timestep) const;
        double getDDPLIM(size_t timestep) const;
        double getDDSLIM(size_t timestep) const;
        double getTRGDPR(size_t timestep) const;
        double getXXXDPR(size_t timestep) const;
        bool   getXXXDPRhasValue(size_t timestep) const;
        void   setNEWTMX(size_t timestep, int NEWTMX);
        void   setNEWTMN(size_t timestep, int NEWTMN);
        void   setLITMAX(size_t timestep, int LITMAX);
        void   setLITMIN(size_t timestep, int LITMIN);
        void   setMXWSIT(size_t timestep, int MXWSIT);
        void   setMXWPIT(size_t timestep, int MXWPIT);
        void   setDDPLIM(size_t timestep, double DDPLIM);
        void   setDDSLIM(size_t timestep, double DDSLIM);
        void   setTRGDPR(size_t timestep, double TRGDPR);
        void   setXXXDPR(size_t timestep, double XXXDPR);


    private:
        /* Record1 */
        DynamicState<double> m_TSINIT;
        DynamicState<double> m_TSMAXZ;
        DynamicState<double> m_TSMINZ;
        DynamicState<double> m_TSMCHP;
        DynamicState<double> m_TSFMAX;
        DynamicState<double> m_TSFMIN;
        DynamicState<double> m_TSFCNV;
        DynamicState<double> m_TFDIFF;
        DynamicState<double> m_THRUPT;
        DynamicState<double> m_TMAXWC;
        DynamicState<int>    m_TMAXWC_has_value;
        /* Record 2 */
        DynamicState<double> m_TRGTTE;
        DynamicState<double> m_TRGCNV;
        DynamicState<double> m_TRGMBE;
        DynamicState<double> m_TRGLCV;
        DynamicState<double> m_XXXTTE;
        DynamicState<double> m_XXXCNV;
        DynamicState<double> m_XXXMBE;
        DynamicState<double> m_XXXLCV;
        DynamicState<double> m_XXXWFL;
        DynamicState<double> m_TRGFIP;
        DynamicState<double> m_TRGSFT;
        DynamicState<int>    m_TRGSFT_has_value;
        DynamicState<double> m_THIONX;
        DynamicState<int>    m_TRWGHT;
        /* Record 3 */
        DynamicState<int>    m_NEWTMX;
        DynamicState<int>    m_NEWTMN;
        DynamicState<int>    m_LITMAX;
        DynamicState<int>    m_LITMIN;
        DynamicState<int>    m_MXWSIT;
        DynamicState<int>    m_MXWPIT;
        DynamicState<double> m_DDPLIM;
        DynamicState<double> m_DDSLIM;
        DynamicState<double> m_TRGDPR;
        DynamicState<double> m_XXXDPR;
        DynamicState<int>    m_XXXDPR_has_value;
        std::map<std::string, bool> m_ResetValue;
    };

} //namespace Opm

#endif
