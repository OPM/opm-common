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


#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>
#include <string>

namespace Opm {

    Tuning::Tuning( const TimeMap& timemap ) :
        /* Hardcoding default values to support getting defaults before any TUNING keyword has occured */
        m_TSINIT( timemap, 1.0   * Metric::Time ),
        m_TSMAXZ( timemap, 365.0 * Metric::Time ),
        m_TSMINZ( timemap, 0.1   * Metric::Time ),
        m_TSMCHP( timemap, 0.15  * Metric::Time ),
        m_TSFMAX( timemap, 3.0 ),
        m_TSFMIN( timemap, 0.3 ),
        m_TSFCNV( timemap, 0.1 ),
        m_TFDIFF( timemap, 1.25 ),
        m_THRUPT( timemap, 1.0E20 ),
        m_TMAXWC( timemap, 0.0   * Metric::Time ),
        m_TMAXWC_has_value( timemap, false ),
        m_TRGTTE( timemap, 0.1 ),
        m_TRGCNV( timemap, 0.001 ),
        m_TRGMBE( timemap, 1.0E-7 ),
        m_TRGLCV( timemap, 0.0001 ),
        m_XXXTTE( timemap, 10.0 ),
        m_XXXCNV( timemap, 0.01 ),
        m_XXXMBE( timemap, 1.0E-6 ),
        m_XXXLCV( timemap, 0.001 ),
        m_XXXWFL( timemap, 0.001 ),
        m_TRGFIP( timemap, 0.025 ),
        m_TRGSFT( timemap, 0.0 ),
        m_TRGSFT_has_value( timemap, false ),
        m_THIONX( timemap, 0.01 ),
        m_TRWGHT( timemap, 1 ),
        m_NEWTMX( timemap, 12 ),
        m_NEWTMN( timemap, 1 ),
        m_LITMAX( timemap, 25 ),
        m_LITMIN( timemap, 1 ),
        m_MXWSIT( timemap, 8 ),
        m_MXWPIT( timemap, 8 ),
        m_DDPLIM( timemap, 1.0E6 * Metric::Pressure ),
        m_DDSLIM( timemap, 1.0E6 ),
        m_TRGDPR( timemap, 1.0E6 * Metric::Pressure ),
        m_XXXDPR( timemap, 0.0   * Metric::Pressure ),
        m_XXXDPR_has_value( timemap, false )
    {
    }


    void Tuning::getTuningItemValue(const std::string& tuningItem, size_t timestep, double& value) {
        if(m_ResetValue.find(tuningItem)!= m_ResetValue.end()){
            timestep = 0;
        }

        /*The following code diverges from coding standard to improve readabillity*/
        if        ("TSINIT" == tuningItem)  {  value = m_TSINIT.get(timestep); }  //RECORD 1
        else if   ("TSMAXZ" == tuningItem)  {  value = m_TSMAXZ.get(timestep); }
        else if   ("TSMINZ" == tuningItem)  {  value = m_TSMINZ.get(timestep); }
        else if   ("TSMCHP" == tuningItem)  {  value = m_TSMCHP.get(timestep); }
        else if   ("TSFMAX" == tuningItem)  {  value = m_TSFMAX.get(timestep); }
        else if   ("TSFMIN" == tuningItem)  {  value = m_TSFMIN.get(timestep); }
        else if   ("TSFCNV" == tuningItem)  {  value = m_TSFCNV.get(timestep); }
        else if   ("TFDIFF" == tuningItem)  {  value = m_TFDIFF.get(timestep); }
        else if   ("THRUPT" == tuningItem)  {  value = m_THRUPT.get(timestep); }
        else if   ("TMAXWC" == tuningItem)  {  value = m_TMAXWC.get(timestep); }

        else if   ("TRGTTE" == tuningItem)  {  value = m_TRGTTE.get(timestep); }  //RECORD 2
        else if   ("TRGCNV" == tuningItem)  {  value = m_TRGCNV.get(timestep); }
        else if   ("TRGMBE" == tuningItem)  {  value = m_TRGMBE.get(timestep); }
        else if   ("TRGLCV" == tuningItem)  {  value = m_TRGLCV.get(timestep); }
        else if   ("XXXTTE" == tuningItem)  {  value = m_XXXTTE.get(timestep); }
        else if   ("XXXCNV" == tuningItem)  {  value = m_XXXCNV.get(timestep); }
        else if   ("XXXMBE" == tuningItem)  {  value = m_XXXMBE.get(timestep); }
        else if   ("XXXLCV" == tuningItem)  {  value = m_XXXLCV.get(timestep); }
        else if   ("XXXWFL" == tuningItem)  {  value = m_XXXWFL.get(timestep); }
        else if   ("TRGFIP" == tuningItem)  {  value = m_TRGFIP.get(timestep); }
        else if   ("TRGSFT" == tuningItem)  {  value = m_TRGSFT.get(timestep); }
        else if   ("THIONX" == tuningItem)  {  value = m_THIONX.get(timestep); }

        else if   ("DDPLIM" == tuningItem)  {  value = m_DDPLIM.get(timestep); }  //RECORD 3
        else if   ("DDSLIM" == tuningItem)  {  value = m_DDSLIM.get(timestep); }
        else if   ("TRGDPR" == tuningItem)  {  value = m_TRGDPR.get(timestep); }
        else if   ("XXXDPR" == tuningItem)  {  value = m_XXXDPR.get(timestep); }

        else {
            throw std::invalid_argument("Method getTuningItemValue(): The TUNING keyword item: " + tuningItem + " was not recognized or has wrong type");
        }
    }

    void Tuning::getTuningItemValue(const std::string& tuningItem, size_t timestep, int& value) {

        /*The following code diverges from coding standard to improve readabillity*/
        if        ("TRWGHT" == tuningItem)  {  value = m_TRWGHT.get(timestep); }  //RECORD 2

        else if   ("NEWTMX" == tuningItem)  {  value = m_NEWTMX.get(timestep); }  //RECORD 3
        else if   ("NEWTMN" == tuningItem)  {  value = m_NEWTMN.get(timestep); }
        else if   ("LITMAX" == tuningItem)  {  value = m_LITMAX.get(timestep); }
        else if   ("LITMIN" == tuningItem)  {  value = m_LITMIN.get(timestep); }
        else if   ("MXWSIT" == tuningItem)  {  value = m_MXWSIT.get(timestep); }
        else if   ("MXWPIT" == tuningItem)  {  value = m_MXWPIT.get(timestep); }

        else {
            throw std::invalid_argument("Method getTuningItemValue(): The TUNING keyword item: " + tuningItem + " was not recognized or has wrong type");
        }
    }

    void Tuning::setTuningInitialValue(const std::string& tuningItem, double value, bool resetVector) {
        /*The following code diverges from coding standard to improve readabillity*/
        if        ("TSINIT" == tuningItem)  {  m_TSINIT.updateInitial(value); }  //RECORD 1
        else if   ("TSMAXZ" == tuningItem)  {  m_TSMAXZ.updateInitial(value); }
        else if   ("TSMINZ" == tuningItem)  {  m_TSMINZ.updateInitial(value); }
        else if   ("TSMCHP" == tuningItem)  {  m_TSMCHP.updateInitial(value); }
        else if   ("TSFMAX" == tuningItem)  {  m_TSFMAX.updateInitial(value); }
        else if   ("TSFMIN" == tuningItem)  {  m_TSFMIN.updateInitial(value); }
        else if   ("TSFCNV" == tuningItem)  {  m_TSFCNV.updateInitial(value); }
        else if   ("TFDIFF" == tuningItem)  {  m_TFDIFF.updateInitial(value); }
        else if   ("THRUPT" == tuningItem)  {  m_THRUPT.updateInitial(value); }
        else if   ("TMAXWC" == tuningItem)  {  m_TMAXWC.updateInitial(value); }

        else if   ("TRGTTE" == tuningItem)  {  m_TRGTTE.updateInitial(value); }  //RECORD 2
        else if   ("TRGCNV" == tuningItem)  {  m_TRGCNV.updateInitial(value); }
        else if   ("TRGMBE" == tuningItem)  {  m_TRGMBE.updateInitial(value); }
        else if   ("TRGLCV" == tuningItem)  {  m_TRGLCV.updateInitial(value); }
        else if   ("XXXTTE" == tuningItem)  {  m_XXXTTE.updateInitial(value); }
        else if   ("XXXCNV" == tuningItem)  {  m_XXXCNV.updateInitial(value); }
        else if   ("XXXMBE" == tuningItem)  {  m_XXXMBE.updateInitial(value); }
        else if   ("XXXLCV" == tuningItem)  {  m_XXXLCV.updateInitial(value); }
        else if   ("XXXWFL" == tuningItem)  {  m_XXXWFL.updateInitial(value); }
        else if   ("TRGFIP" == tuningItem)  {  m_TRGFIP.updateInitial(value); }
        else if   ("TRGSFT" == tuningItem)  {  m_TRGSFT.updateInitial(value); }
        else if   ("THIONX" == tuningItem)  {  m_THIONX.updateInitial(value); }

        else if   ("DDPLIM" == tuningItem)  {  m_DDPLIM.updateInitial(value); }  //RECORD 3
        else if   ("DDSLIM" == tuningItem)  {  m_DDSLIM.updateInitial(value); }
        else if   ("TRGDPR" == tuningItem)  {  m_TRGDPR.updateInitial(value); }
        else if   ("XXXDPR" == tuningItem)  {  m_XXXDPR.updateInitial(value); }

        else {
            throw std::invalid_argument("Method getTuningItemValue(): The TUNING keyword item: " + tuningItem + " was not recognized or has wrong type");
        }
        if(resetVector){
            m_ResetValue[tuningItem]=true;
        }
    }

    void Tuning::setTuningInitialValue(const std::string& tuningItem, int value, bool resetVector) {
        /*The following code diverges from coding standard to improve readabillity*/
        if        ("TRWGHT" == tuningItem)  { m_TRWGHT.updateInitial(value); }  //RECORD 2

        else if   ("NEWTMX" == tuningItem)  { m_NEWTMX.updateInitial(value); }  //RECORD 3
        else if   ("NEWTMN" == tuningItem)  { m_NEWTMN.updateInitial(value); }
        else if   ("LITMAX" == tuningItem)  { m_LITMAX.updateInitial(value); }
        else if   ("LITMIN" == tuningItem)  { m_LITMIN.updateInitial(value); }
        else if   ("MXWSIT" == tuningItem)  { m_MXWSIT.updateInitial(value); }
        else if   ("MXWPIT" == tuningItem)  { m_MXWPIT.updateInitial(value); }

        else {
            throw std::invalid_argument("Method getTuningItemValue(): The TUNING keyword item: " + tuningItem + " was not recognized or has wrong type");
        }
        if(resetVector){
            m_ResetValue[tuningItem]=true;
        }
    }

namespace {

    template< typename T >
    double getValue( const std::string& tuningItem,
                     const DynamicState< T >& values,
                     const std::map< std::string, bool >& rv,
                     size_t timestep ) {

        if( rv.find(tuningItem) != rv.end() )
            timestep = 0;

        return values.get(timestep);
    }

    bool getBoolValue( const std::string& tuningItem,
                       const DynamicState< int >& values,
                       const std::map< std::string, bool >& rv,
                       size_t timestep ) {

        return bool( getValue( tuningItem, values, rv, timestep ) );
    }

}

    /*The following "get" method declarations diverges from coding standard to improve readability*/
    double Tuning::getTSINIT(size_t timestep) const { return getValue("TSINIT", m_TSINIT, m_ResetValue, timestep); }
    double Tuning::getTSMAXZ(size_t timestep) const { return getValue("TSMAXZ", m_TSMAXZ, m_ResetValue, timestep); }
    double Tuning::getTSMINZ(size_t timestep) const { return getValue("TSMINZ", m_TSMINZ, m_ResetValue, timestep); }
    double Tuning::getTSMCHP(size_t timestep) const { return getValue("TSMCHP", m_TSMCHP, m_ResetValue, timestep); }
    double Tuning::getTSFMAX(size_t timestep) const { return getValue("TSFMAX", m_TSFMAX, m_ResetValue, timestep); }
    double Tuning::getTSFMIN(size_t timestep) const { return getValue("TSFMIN", m_TSFMIN, m_ResetValue, timestep); }
    double Tuning::getTSFCNV(size_t timestep) const { return getValue("TSFCNV", m_TSFCNV, m_ResetValue, timestep); }
    double Tuning::getTFDIFF(size_t timestep) const { return getValue("TFDIFF", m_TFDIFF, m_ResetValue, timestep); }
    double Tuning::getTHRUPT(size_t timestep) const { return getValue("THRUPT", m_THRUPT, m_ResetValue, timestep); }
    double Tuning::getTMAXWC(size_t timestep) const { return getValue("TMAXWC", m_TMAXWC, m_ResetValue, timestep); }
    bool   Tuning::getTMAXWChasValue(size_t timestep) const { return getBoolValue("TMAXWC",m_TMAXWC_has_value, m_ResetValue, timestep); }
    double Tuning::getTRGTTE(size_t timestep) const { return getValue("TRGTTE", m_TRGTTE, m_ResetValue, timestep); }
    double Tuning::getTRGCNV(size_t timestep) const { return getValue("TRGCNV", m_TRGCNV, m_ResetValue, timestep); }
    double Tuning::getTRGMBE(size_t timestep) const { return getValue("TRGMBE", m_TRGMBE, m_ResetValue, timestep); }
    double Tuning::getTRGLCV(size_t timestep) const { return getValue("TRGLCV", m_TRGLCV, m_ResetValue, timestep); }
    double Tuning::getXXXTTE(size_t timestep) const { return getValue("XXXTTE", m_XXXTTE, m_ResetValue, timestep); }
    double Tuning::getXXXCNV(size_t timestep) const { return getValue("XXXCNV", m_XXXCNV, m_ResetValue, timestep); }
    double Tuning::getXXXMBE(size_t timestep) const { return getValue("XXXMBE", m_XXXMBE, m_ResetValue, timestep); }
    double Tuning::getXXXLCV(size_t timestep) const { return getValue("XXXLCV", m_XXXLCV, m_ResetValue, timestep); }
    double Tuning::getXXXWFL(size_t timestep) const { return getValue("XXXWFL", m_XXXWFL, m_ResetValue, timestep); }
    double Tuning::getTRGFIP(size_t timestep) const { return getValue("TRGFIP", m_TRGFIP, m_ResetValue, timestep); }
    double Tuning::getTRGSFT(size_t timestep) const { return getValue("TRGSFT", m_TRGSFT, m_ResetValue, timestep); }
    bool   Tuning::getTRGSFThasValue(size_t timestep) const { return getBoolValue("TRGSFT",m_TRGSFT_has_value, m_ResetValue, timestep);  }
    double Tuning::getTHIONX(size_t timestep) const { return getValue("",m_THIONX, m_ResetValue, timestep); }
    int    Tuning::getTRWGHT(size_t timestep) const { return getValue("TRWGHT", m_TRWGHT, m_ResetValue, timestep); }
    int    Tuning::getNEWTMX(size_t timestep) const { return getValue("NEWTMX", m_NEWTMX, m_ResetValue, timestep); }
    int    Tuning::getNEWTMN(size_t timestep) const { return getValue("NEWTMN", m_NEWTMN, m_ResetValue, timestep); }
    int    Tuning::getLITMAX(size_t timestep) const { return getValue("LITMAX", m_LITMAX, m_ResetValue, timestep); }
    int    Tuning::getLITMIN(size_t timestep) const { return getValue("LITMIN", m_LITMIN, m_ResetValue, timestep); }
    int    Tuning::getMXWSIT(size_t timestep) const { return getValue("MXWSIT", m_MXWSIT, m_ResetValue, timestep); }
    int    Tuning::getMXWPIT(size_t timestep) const { return getValue("MXWPIT", m_MXWPIT, m_ResetValue, timestep); }
    double Tuning::getDDPLIM(size_t timestep) const { return getValue("DDPLIM", m_DDPLIM, m_ResetValue, timestep); }
    double Tuning::getDDSLIM(size_t timestep) const { return getValue("DDSLIM", m_DDSLIM, m_ResetValue, timestep); }
    double Tuning::getTRGDPR(size_t timestep) const { return getValue("TRGDPR", m_TRGDPR, m_ResetValue, timestep); }
    double Tuning::getXXXDPR(size_t timestep) const { return getValue("XXXDPR", m_XXXDPR, m_ResetValue, timestep); }
    bool   Tuning::getXXXDPRhasValue(size_t timestep) const { return getBoolValue("XXXDPR", m_XXXDPR_has_value, m_ResetValue, timestep);
    }

    /*The following "set" method declarations diverges from coding standard to improve readability*/
    void Tuning::setTSINIT(size_t timestep, double TSINIT) { m_TSINIT.update(timestep, TSINIT); }
    void Tuning::setTSMAXZ(size_t timestep, double TSMAXZ) { m_TSMAXZ.update(timestep, TSMAXZ); }
    void Tuning::setTSMINZ(size_t timestep, double TSMINZ) { m_TSMINZ.update(timestep, TSMINZ); }
    void Tuning::setTSMCHP(size_t timestep, double TSMCHP) { m_TSMCHP.update(timestep, TSMCHP); }
    void Tuning::setTSFMAX(size_t timestep, double TSFMAX) { m_TSFMAX.update(timestep, TSFMAX); }
    void Tuning::setTSFMIN(size_t timestep, double TSFMIN) { m_TSFMIN.update(timestep, TSFMIN); }
    void Tuning::setTSFCNV(size_t timestep, double TSFCNV) { m_TSFCNV.update(timestep, TSFCNV); }
    void Tuning::setTFDIFF(size_t timestep, double TFDIFF) { m_TFDIFF.update(timestep, TFDIFF); }
    void Tuning::setTHRUPT(size_t timestep, double THRUPT) { m_THRUPT.update(timestep, THRUPT); }
    void Tuning::setTMAXWC(size_t timestep, double TMAXWC) { m_TMAXWC.update(timestep, TMAXWC);
                                                             m_TMAXWC_has_value.update(timestep, true); }
    void Tuning::setTRGTTE(size_t timestep, double TRGTTE) { m_TRGTTE.update(timestep, TRGTTE); }
    void Tuning::setTRGCNV(size_t timestep, double TRGCNV) { m_TRGCNV.update(timestep, TRGCNV); }
    void Tuning::setTRGMBE(size_t timestep, double TRGMBE) { m_TRGMBE.update(timestep, TRGMBE); }
    void Tuning::setTRGLCV(size_t timestep, double TRGLCV) { m_TRGLCV.update(timestep, TRGLCV); }
    void Tuning::setXXXTTE(size_t timestep, double XXXTTE) { m_XXXTTE.update(timestep, XXXTTE); }
    void Tuning::setXXXCNV(size_t timestep, double XXXCNV) { m_XXXCNV.update(timestep, XXXCNV); }
    void Tuning::setXXXMBE(size_t timestep, double XXXMBE) { m_XXXMBE.update(timestep, XXXMBE); }
    void Tuning::setXXXLCV(size_t timestep, double XXXLCV) { m_XXXLCV.update(timestep, XXXLCV); }
    void Tuning::setXXXWFL(size_t timestep, double XXXWFL) { m_XXXWFL.update(timestep, XXXWFL); }
    void Tuning::setTRGFIP(size_t timestep, double TRGFIP) { m_TRGFIP.update(timestep, TRGFIP); }
    void Tuning::setTRGSFT(size_t timestep, double TRGSFT) { m_TRGSFT.update(timestep, TRGSFT);
                                                             m_TRGSFT_has_value.update(timestep, true); }
    void Tuning::setTHIONX(size_t timestep, double THIONX) { m_THIONX.update(timestep, THIONX); }
    void Tuning::setTRWGHT(size_t timestep, int TRWGHT) {    m_TRWGHT.update(timestep, TRWGHT); }
    void Tuning::setNEWTMX(size_t timestep, int NEWTMX) {    m_NEWTMX.update(timestep, NEWTMX); }
    void Tuning::setNEWTMN(size_t timestep, int NEWTMN) {    m_NEWTMN.update(timestep, NEWTMN); }
    void Tuning::setLITMAX(size_t timestep, int LITMAX) {    m_LITMAX.update(timestep, LITMAX); }
    void Tuning::setLITMIN(size_t timestep, int LITMIN) {    m_LITMIN.update(timestep, LITMIN); }
    void Tuning::setMXWSIT(size_t timestep, int MXWSIT) {    m_MXWSIT.update(timestep, MXWSIT); }
    void Tuning::setMXWPIT(size_t timestep, int MXWPIT) {    m_MXWPIT.update(timestep, MXWPIT); }
    void Tuning::setDDPLIM(size_t timestep, double DDPLIM) { m_DDPLIM.update(timestep, DDPLIM); }
    void Tuning::setDDSLIM(size_t timestep, double DDSLIM) { m_DDSLIM.update(timestep, DDSLIM); }
    void Tuning::setTRGDPR(size_t timestep, double TRGDPR) { m_TRGDPR.update(timestep, TRGDPR); }
    void Tuning::setXXXDPR(size_t timestep, double XXXDPR) { m_XXXDPR.update(timestep, XXXDPR);
                                                             m_XXXDPR_has_value.update(timestep, true); }

} //namespace Opm
