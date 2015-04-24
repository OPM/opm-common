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
#ifndef OPM_PARSER_PLYSHLOG_TABLE_HPP
#define	OPM_PARSER_PLYSHLOG_TABLE_HPP

#include "SingleRecordTable.hpp"

namespace Opm {
    // forward declaration
    class EclipseState;

    class PlyshlogTable {

        friend class EclipseState;
        PlyshlogTable() = default;

        /*!
         * \brief Read the PLYSHLOG keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckKeywordConstPtr keyword) {

            // the reference conditions
            DeckRecordConstPtr record1 = keyword->getRecord(0);

            const auto itemRefPolymerConcentration = record1->getItem("REF_POLYMER_CONCENTRATION");
            const auto itemRefSalinity = record1->getItem("REF_SALINITY");
            const auto itemRefTemperature = record1->getItem("REF_TEMPERATURE");

            assert(itemRefPolymerConcentration->hasValue(0));

            const double refPolymerConcentration = itemRefPolymerConcentration->getRawDouble(0);

            setRefPolymerConcentration(refPolymerConcentration);

            if (itemRefSalinity->hasValue(0)) {
                setHasRefSalinity(true);
                const double refSalinity = itemRefSalinity->getRawDouble(0);
                setRefSalinity(refSalinity);
            } else {
                setHasRefSalinity(false);
            }

            if (itemRefTemperature->hasValue(0)) {
                setHasRefTemperature(true);
                const double refTemperature = itemRefTemperature->getRawDouble(0);
                setRefTemperature(refTemperature);
            } else {
                setHasRefTemperature(false);
            }

            m_data = new SingleRecordTable();

            m_data->init(keyword,
                         std::vector<std::string>{
                                "WaterVelocity",
                                "ShearMultiplier"
                             },
                             /*recordIdx*/ 1,
                             /*firstEntityOffset=*/0);

            m_data->checkNonDefaultable("WaterVelocity");
            m_data->checkMonotonic("WaterVelocity", /*isAscending=*/true);
            m_data->checkNonDefaultable("ShearMultiplier");

        }


        double getRefPolymerConcentration() const {
            return m_refPolymerConcentration;
        }
        double getRefSalinity() const {
            return m_refSalinity;
        }

        double getRefTemperature() const{
            return m_refTemperature;
        }

        void setRefPolymerConcentration(const double refPlymerConcentration) {
            m_refPolymerConcentration = refPlymerConcentration;
        }

        void setRefSalinity(const double refSalinity) {
            m_refSalinity = refSalinity;
        }

        void setRefTemperature(const double refTemperature) {
            m_refTemperature = refTemperature;
        }

        bool hasRefSalinity() {
            return m_hasRefSalinity;
        }

        bool hasRefTemperature() {
            return m_hasRefTemperature;
        }

        void setHasRefSalinity(const bool has){
            m_hasRefSalinity = has;
        }

        void setHasRefTemperature(const bool has){
            m_refTemperature = has;
        }

        const std::vector<double> &getWaterVelocityColumn() const
        { return m_data->getColumn(0); }

        const std::vector<double> &getShearMultiplierColumn() const
        { return m_data->getColumn(1); }


    private:
        double m_refPolymerConcentration;
        double m_refSalinity;
        double m_refTemperature;

        bool m_hasRefSalinity;
        bool m_hasRefTemperature;

        SingleRecordTable *m_data;

    };

}

#endif
