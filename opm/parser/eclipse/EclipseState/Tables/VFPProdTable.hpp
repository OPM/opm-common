/*
  Copyright 2015 SINTEF ICT, Applied Mathematics.

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

#ifndef OPM_PARSER_ECLIPSE_ECLIPSESTATE_TABLES_VFPPRODTABLE_HPP_
#define OPM_PARSER_ECLIPSE_ECLIPSESTATE_TABLES_VFPPRODTABLE_HPP_

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <boost/multi_array.hpp>



namespace Opm {

/**
 * Class for reading data from a VFPPROD (vertical flow performance production) table
 */
class VFPProdTable {
public:
    typedef boost::multi_array<double, 5> array_type;
    typedef boost::array<array_type::index, 5> extents;

    ///Rate type
    enum FLO_TYPE {
        FLO_OIL=1, //< Oil rate
        FLO_LIQ, //< Liquid rate
        FLO_GAS, //< Gas rate
        FLO_INVALID
    };

    ///Water fraction variable
    enum WFR_TYPE {
        WFR_WOR=11, //< Water-oil ratio
        WFR_WCT, //< Water cut
        WFR_WGR, //< Water-gas ratio
        WFR_INVALID
    };

    ///Gas fraction variable
    enum GFR_TYPE {
        GFR_GOR=21, //< Gas-oil ratio
        GFR_GLR, //< Gas-liquid ratio
        GFR_OGR, //< Oil-gas ratio
        GFR_INVALID
    };

    ///Artificial lift quantity
    enum ALQ_TYPE {
        ALQ_GRAT=31, //< Lift as injection rate
        ALQ_IGLR, //< Injection gas-liquid ratio
        ALQ_TGLR, //< Total gas-liquid ratio
        ALQ_PUMP, //< Pump rating
        ALQ_COMP, //< Compressor power
        ALQ_BEAN, //< Choke diameter
        ALQ_UNDEF, //< Undefined
        ALQ_INVALID
    };

    /**
     * Constructor
     */
    VFPProdTable() : m_table_num(-1),
            m_datum_depth(-1),
            m_flo_type(FLO_INVALID),
            m_wfr_type(WFR_INVALID),
            m_gfr_type(GFR_INVALID),
            m_alq_type(ALQ_INVALID) {

    }

    /**
     * Initializes objects from raw data. NOTE: All raw data assumed to be in SI units
     * @param table_num VFP table number
     * @param datum_depth Reference depth for BHP
     * @param flo_type Specifies what flo_data represents
     * @param wfr_type Specifies what wfr_data represents
     * @param gfr_type Specifies what gfr_data represents
     * @param alq_type Specifies what alq_data represents
     * @param flo_data Axis for flo_type
     * @param thp_data Axis for thp_type
     * @param wfr_data Axis for wfr_type
     * @param gfr_data Axis for gfr_type
     * @param alq_data Axis for alq_type
     * @param data BHP to be interpolated. Given as a 5D array so that
     *        BHP = data[thp][wfr][gfr][alq][flo] for the indices thp, wfr, etc.
     */
    void init(int table_num,
            double datum_depth,
            FLO_TYPE flo_type,
            WFR_TYPE wfr_type,
            GFR_TYPE gfr_type,
            ALQ_TYPE alq_type,
            const std::vector<double>& flo_data,
            const std::vector<double>& thp_data,
            const std::vector<double>& wfr_data,
            const std::vector<double>& gfr_data,
            const std::vector<double>& alq_data,
            array_type data) {
        m_table_num = table_num;
        m_datum_depth = datum_depth;
        m_flo_type = flo_type;
        m_wfr_type = wfr_type;
        m_gfr_type = gfr_type;
        m_alq_type = alq_type;
        m_flo_data = flo_data;
        m_thp_data = thp_data;
        m_wfr_data = wfr_data;
        m_gfr_data = gfr_data;
        m_alq_data = alq_data;
        m_data = data;
        check();
    }

    /**
     * Constructor which parses a deck keyword and retrieves the relevant parts for a
     * VFP table.
     */
    void init(DeckKeywordConstPtr table) {
        auto iter = table->begin();

        auto header = (*iter++);

        assert(itemValid(header, "TABLE"));
        m_table_num = header->getItem("TABLE")->getInt(0);

        assert(itemValid(header, "DATUM_DEPTH"));
        m_datum_depth = header->getItem("DATUM_DEPTH")->getRawDouble(0);

        //Rate type
        assert(itemValid(header, "RATE_TYPE"));
        std::string flo_string = header->getItem("RATE_TYPE")->getString(0);
        if (flo_string == "OIL") {
            m_flo_type = FLO_OIL;
        }
        else if (flo_string == "LIQ") {
            m_flo_type = FLO_LIQ;
        }
        else if (flo_string == "GAS") {
            m_flo_type = FLO_GAS;
        }
        else {
            m_flo_type = FLO_INVALID;
            throw std::invalid_argument("Invalid RATE_TYPE string");
        }

        //Water fraction
        assert(itemValid(header, "WFR"));
        std::string wfr_string = header->getItem("WFR")->getString(0);
        if (wfr_string == "WOR") {
            m_wfr_type = WFR_WOR;
        }
        else if (wfr_string == "WCT") {
            m_wfr_type = WFR_WCT;
        }
        else if (wfr_string == "WGR") {
            m_wfr_type = WFR_WGR;
        }
        else {
            m_wfr_type = WFR_INVALID;
            throw std::invalid_argument("Invalid WFR string");
        }

        //Gas fraction
        assert(itemValid(header, "GFR"));
        std::string gfr_string = header->getItem("GFR")->getString(0);
        if (gfr_string == "GOR") {
            m_gfr_type = GFR_GOR;
        }
        else if (gfr_string == "GLR") {
            m_gfr_type = GFR_GLR;
        }
        else if (gfr_string == "OGR") {
            m_gfr_type = GFR_OGR;
        }
        else {
            m_gfr_type = GFR_INVALID;
            throw std::invalid_argument("Invalid GFR string");
        }

        //Definition of THP values, must be THP
        if (itemValid(header, "PRESSURE_DEF")) {
            std::string quantity_string = header->getItem("PRESSURE_DEF")->getString(0);
            assert(quantity_string == "THP");
        }

        //Artificial lift
        if (itemValid(header, "ALQ_DEF")) {
            std::string alq_string = header->getItem("ALQ_DEF")->getString(0);
            if (alq_string == "GRAT") {
                m_alq_type = ALQ_GRAT;
            }
            else if (alq_string == "IGLR") {
                m_alq_type = ALQ_IGLR;
            }
            else if (alq_string == "TGLR") {
                m_alq_type = ALQ_TGLR;
            }
            else if (alq_string == "PUMP") {
                m_alq_type = ALQ_PUMP;
            }
            else if (alq_string == "COMP") {
                m_alq_type = ALQ_COMP;
            }
            else if (alq_string == "BEAN") {
                m_alq_type = ALQ_BEAN;
            }
            else if (alq_string == " ") {
                m_alq_type = ALQ_UNDEF;
            }
            else {
                m_alq_type = ALQ_INVALID;
                throw std::invalid_argument("Invalid ALQ_DEF string");
            }
        }
        else {
            m_alq_type = ALQ_UNDEF;
        }

        //Units used for this table
        if (itemValid(header, "UNITS")) {
            //TODO: Should check that table unit matches rest of deck.
            std::string unit_string = header->getItem("UNITS")->getString(0);
            if (unit_string == "METRIC") {
            }
            else if (unit_string == "FIELD") {
            }
            else if (unit_string == "LAB") {
            }
            else if (unit_string == "PVT-M") {
            }
            else {
                throw std::invalid_argument("Invalid UNITS string");
            }
        }
        else {
            //Do nothing, table implicitly same unit as rest of deck
        }

        //Quantity in the body of the table
        if (itemValid(header, "BODY_DEF")) {
            std::string body_string = header->getItem("BODY_DEF")->getString(0);
            if (body_string == "TEMP") {
                throw std::invalid_argument("Invalid BODY_DEF string: TEMP not supported");
            }
            else if (body_string == "BHP") {

            }
            else {
                throw std::invalid_argument("Invalid BODY_DEF string");
            }
        }
        else {
            //Default to BHP
        }


        //Get actual rate / flow values
        m_flo_data = (*iter++)->getItem("FLOW_VALUES")->getSIDoubleData();

        //Get actual tubing head pressure values
        m_thp_data = (*iter++)->getItem("THP_VALUES")->getSIDoubleData();

        //Get actual water fraction values
        m_wfr_data = (*iter++)->getItem("WFR_VALUES")->getRawDoubleData(); //FIXME: unit

        //Get actual gas fraction values
        m_gfr_data = (*iter++)->getItem("GFR_VALUES")->getRawDoubleData(); //FIXME: unit

        //Get actual gas fraction values
        m_alq_data = (*iter++)->getItem("ALQ_VALUES")->getRawDoubleData(); //FIXME: unit

        //Finally, read the actual table itself.
        size_t nt = m_thp_data.size();
        size_t nw = m_wfr_data.size();
        size_t ng = m_gfr_data.size();
        size_t na = m_alq_data.size();
        size_t nf = m_flo_data.size();
        extents shape;
        shape[0] = nt;
        shape[1] = nw;
        shape[2] = ng;
        shape[3] = na;
        shape[4] = nf;
        m_data.resize(shape);

        for (; iter!=table->end(); ++iter) {
            //Get indices (subtract 1 to get 0-based index)
            int t = (*iter)->getItem("THP_INDEX")->getInt(0) - 1;
            int w = (*iter)->getItem("WFR_INDEX")->getInt(0) - 1;
            int g = (*iter)->getItem("GFR_INDEX")->getInt(0) - 1;
            int a = (*iter)->getItem("ALQ_INDEX")->getInt(0) - 1;

            //Rest of values (bottom hole pressure or tubing head temperature) have index of flo value
            const std::vector<double>& bhp_tht = (*iter)->getItem("VALUES")->getRawDoubleData(); //FIXME: unit
            for (int f=0; f<bhp_tht.size(); ++f) {
                m_data[t][w][g][a][f] = bhp_tht[f];
            }
            //FIXME: Alternative if guaranteed to be linear in f in memory
            //std::copy(bhp_tht.begin(), bhp_tht.end(), &m_data[t][w][g][a][0]);
        }

        check();
    }

    /**
     * Returns the table number
     * @return table number
     */
    inline int getTableNum() {
        return m_table_num;
    }

    /**
     * Returns the datum depth for the table data
     * @return datum depth
     */
    inline double getDatumDepth() {
        return m_datum_depth;
    }

    /**
     * Returns the rate/flo type for the flo axis
     * @return flo type
     */
    inline FLO_TYPE getFloType() {
        return m_flo_type;
    }

    /**
     * Returns the water fraction type for the WFR axis
     * @return water fraction type
     */
    inline WFR_TYPE getWFRType() {
        return m_wfr_type;
    }

    /**
     * Returns the gas fraction type for the GFR axis
     * @return gas fraction type
     */
    inline GFR_TYPE getGFRType() {
        return m_gfr_type;
    }

    /**
     * Returns the artificial lift quantity type for the ALQ axis
     * @return artificial lift quantity type
     */
    inline ALQ_TYPE getALQType() {
        return m_alq_type;
    }

    /**
     * Returns the coordinates of the FLO sample points in the table
     * @return Flo sample coordinates
     */
    inline const std::vector<double>& getFloAxis() {
        return m_flo_data;
    }

    /**
     * Returns the coordinates for the tubing head pressure sample points in the table
     * @return Tubing head pressure coordinates
     */
    inline const std::vector<double>& getTHPAxis() {
        return m_thp_data;
    }

    /**
     * Returns the coordinates for the water fraction sample points in the table
     * @return Water fraction coordinates
     */
    inline const std::vector<double>& getWFRAxis() {
        return m_wfr_data;
    }

    /**
     * Returns the coordinates for the gas fraction sample points in the table
     * @return Gas fraction coordinates
     */
    inline const std::vector<double>& getGFRAxis() {
        return m_gfr_data;
    }

    /**
     * Returns the coordinates for the artificial lift quantity points in the table
     * @return Artificial lift quantity coordinates
     */
    inline const std::vector<double>& getALQAxis() {
        return m_alq_data;
    }

    /**
     * Returns the data of the table itself. The data is ordered so that
     *
     * table = getTable();
     * bhp = table[thp_idx][wfr_idx][gfr_idx][alq_idx][flo_idx];
     *
     * gives the bottom hole pressure value in the table for the coordinate
     * given by
     * flo_axis = getFloAxis();
     * thp_axis = getTHPAxis();
     * ...
     *
     * flo_coord = flo_axis(flo_idx);
     * thp_coord = thp_axis(thp_idx);
     * ...
     */
    inline const array_type& getTable() {
        return m_data;
    }

private:

    //"Header" variables
    int m_table_num;
    double m_datum_depth;
    FLO_TYPE m_flo_type;
    WFR_TYPE m_wfr_type;
    GFR_TYPE m_gfr_type;
    ALQ_TYPE m_alq_type;

    //The actual table axes
    std::vector<double> m_flo_data;
    std::vector<double> m_thp_data;
    std::vector<double> m_wfr_data;
    std::vector<double> m_gfr_data;
    std::vector<double> m_alq_data;

    //The data itself, using the data ordering m_data[thp][wfr][gfr][alq][flo]
    array_type m_data;


    /**
     * Helper function that checks if an item exists in a record, and has a
     * non-zero size
     */
    bool itemValid(DeckRecordConstPtr& record, const char* name) {
        if (record->size() == 0) {
            return false;
        }
        else {
            DeckItemPtr item;
            //TODO: Should we instead here allow the exception to propagate?
            try {
                item = record->getItem(name);
            }
            catch (...) {
                return false;
            }

            if (item->size() > 0) {
                return true;
            }
            else {
                return false;
            }
        }
    }

    /**
     * Debug function that runs a series of asserts to check for sanity of inputs.
     * Called after init to check that everything looks ok.
     */
    void check() {
        //Table number
        assert(m_table_num > 0);

        //Misc types
        assert(m_flo_type >= FLO_OIL && m_flo_type < FLO_INVALID);
        assert(m_wfr_type >= WFR_WOR && m_wfr_type < WFR_INVALID);
        assert(m_gfr_type >= GFR_GOR && m_gfr_type < GFR_INVALID);
        assert(m_alq_type >= ALQ_GRAT && m_alq_type < ALQ_INVALID);

        //Data axis size
        assert(m_flo_data.size() > 0);
        assert(m_thp_data.size() > 0);
        assert(m_wfr_data.size() > 0);
        assert(m_gfr_data.size() > 0);
        assert(m_alq_data.size() > 0);

        //Data axis sorted?
        assert(is_sorted(m_flo_data.begin(), m_flo_data.end()));
        assert(is_sorted(m_thp_data.begin(), m_thp_data.end()));
        assert(is_sorted(m_wfr_data.begin(), m_wfr_data.end()));
        assert(is_sorted(m_gfr_data.begin(), m_gfr_data.end()));
        assert(is_sorted(m_alq_data.begin(), m_alq_data.end()));

        //Check data size matches axes
        assert(m_data.num_dimensions() == 5);
        assert(m_data.shape()[0] == m_thp_data.size());
        assert(m_data.shape()[1] == m_wfr_data.size());
        assert(m_data.shape()[2] == m_gfr_data.size());
        assert(m_data.shape()[3] == m_alq_data.size());
        assert(m_data.shape()[4] == m_flo_data.size());

        //Finally, check that all data is within reasonable ranges, defined to be up-to 1.0e10...
        typedef array_type::size_type size_type;
        for (size_type t=0; t<m_data.shape()[0]; ++t) {
            for (size_type w=0; w<m_data.shape()[1]; ++w) {
                for (size_type g=0; g<m_data.shape()[2]; ++g) {
                    for (size_type a=0; a<m_data.shape()[3]; ++a) {
                        for (size_type f=0; f<m_data.shape()[4]; ++f) {
                            if (m_data[t][w][g][a][f] > 1.0e10) {
                                //TODO: Replace with proper log message
                                std::cerr << "Too large value encountered in VFPPROD in ["
                                        << t << "," << w << "," << g << "," << a << "," << f << "]="
                                        << m_data[t][w][g][a][f] << std::endl;
                            }
                        }
                    }
                }
            }
        }
    }
};

}


#endif /* OPM_PARSER_ECLIPSE_ECLIPSESTATE_TABLES_VFPPRODTABLE_HPP_ */
