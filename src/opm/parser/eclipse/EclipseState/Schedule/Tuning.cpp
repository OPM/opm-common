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

    Tuning::Tuning( const TimeMap& timemap )
        /* Hardcoding default values to support getting defaults before any TUNING keyword has occured */
    {
        m_has_fields.insert({"TMAXWC", {timemap, false}});
        m_has_fields.insert({"TRGSFT", {timemap, false}});
        m_has_fields.insert({"XXXDPR", {timemap, false}});

        // record 1
        m_fields.insert({"TSINIT", {timemap,   1.0 * Metric::Time}});
        m_fields.insert({"TSMAXZ", {timemap, 365.0 * Metric::Time}});
        m_fields.insert({"TSMINZ", {timemap,   0.1 * Metric::Time}});
        m_fields.insert({"TSMCHP", {timemap,  0.15 * Metric::Time}});
        m_fields.insert({"TSFMAX", {timemap,   3.0}});
        m_fields.insert({"TSFMIN", {timemap,   0.3}});
        m_fields.insert({"TSFCNV", {timemap,   0.1}});
        m_fields.insert({"TFDIFF", {timemap,  1.25}});
        m_fields.insert({"THRUPT", {timemap,  1.0E20}});
        m_fields.insert({"TMAXWC", {timemap,   0.0 * Metric::Time}});

        // record 2
        m_fields.insert({"TRGTTE", {timemap,   0.1}});
        m_fields.insert({"TRGCNV", {timemap,   0.001}});
        m_fields.insert({"TRGMBE", {timemap,   1.0E-7}});
        m_fields.insert({"TRGLCV", {timemap,   0.0001}});
        m_fields.insert({"XXXTTE", {timemap,   10.0}});
        m_fields.insert({"XXXCNV", {timemap,   0.01}});
        m_fields.insert({"XXXMBE", {timemap,   1.0E-6}});
        m_fields.insert({"XXXLCV", {timemap,   0.001}});
        m_fields.insert({"XXXWFL", {timemap,   0.001}});
        m_fields.insert({"TRGFIP", {timemap,   0.025}});
        m_fields.insert({"TRGSFT", {timemap,   0.0}});
        m_fields.insert({"THIONX", {timemap,   0.01}});
        m_int_fields.insert({"TRWGHT", {timemap, 1}});

        // record 3
        m_int_fields.insert({"NEWTMX", {timemap, 12}});
        m_int_fields.insert({"NEWTMN", {timemap, 1}});
        m_int_fields.insert({"LITMAX", {timemap, 25}});
        m_int_fields.insert({"LITMIN", {timemap, 1}});
        m_int_fields.insert({"MXWSIT", {timemap, 8}});
        m_int_fields.insert({"MXWPIT", {timemap, 8}});
        m_fields.insert({"DDPLIM", {timemap,   1.0E6 * Metric::Pressure}});
        m_fields.insert({"DDSLIM", {timemap,   1.0E6}});
        m_fields.insert({"TRGDPR", {timemap,   1.0E6 * Metric::Pressure}});
        m_fields.insert({"XXXDPR", {timemap,     0.0 * Metric::Pressure}});
    }


    template<>
    double Tuning::get(const std::string& tuningItem, size_t timestep) const {
        if (m_ResetValue.find(tuningItem) != m_ResetValue.end()) {
            timestep = 0;
        }

        const auto dblIt = m_fields.find(tuningItem);
        if (dblIt == m_fields.end()) {
            throw std::invalid_argument("Method get(): The TUNING keyword item: " + tuningItem + " was not recognized or has wrong type");
        }

        return dblIt->second.get(timestep);
    }

    template<>
    int Tuning::get(const std::string& tuningItem, size_t timestep) const {
        const auto intIt = m_int_fields.find(tuningItem);
        if (intIt == m_int_fields.end()) {
            throw std::invalid_argument("Method get(): The TUNING keyword item: " + tuningItem + " was not recognized or has wrong type");
        }

        return intIt->second.get(timestep);
    }

    template<>
    void Tuning::set(const std::string& tuningItem, size_t timestep, double value) {
        const auto dblIt = m_fields.find(tuningItem);
        if (dblIt == m_fields.end()) {
            throw std::invalid_argument("Method set(): The TUNING keyword item: " + tuningItem + " was not recognized or has wrong type");
        }
        dblIt->second.update(timestep, value);
        auto hasIt = m_has_fields.find(tuningItem);
        if (hasIt != m_has_fields.end())
            hasIt->second.update(timestep, 1);
    }

    template<>
    void Tuning::set(const std::string& tuningItem, size_t timestep, int value) {
        const auto intIt = m_int_fields.find(tuningItem);
        if (intIt == m_int_fields.end()) {
            throw std::invalid_argument("Method set(): The TUNING keyword item: " + tuningItem + " was not recognized or has wrong type");
        }
        intIt->second.update(timestep, value);
        auto hasIt = m_has_fields.find(tuningItem);
        if (hasIt != m_has_fields.end())
            hasIt->second.update(timestep, 1);
    }

    template<>
    void Tuning::setInitial(const std::string& tuningItem, double value, bool resetVector) {
        const auto dblIt = m_fields.find(tuningItem);
        if (dblIt == m_fields.end()) {
            throw std::invalid_argument("Method setInitial(): The TUNING keyword item: " + tuningItem + " was not recognized or has wrong type");
        }

        dblIt->second.updateInitial(value);
        if (resetVector) {
            m_ResetValue[tuningItem] = true;
        }
    }


    template<>
    void Tuning::setInitial(const std::string& tuningItem, int value, bool resetVector) {
        const auto intIt = m_int_fields.find(tuningItem);
        if (intIt == m_int_fields.end()) {
            throw std::invalid_argument("Method setInitial(): The TUNING keyword item: " + tuningItem + " was not recognized or has wrong type");
        }

        intIt->second.updateInitial(value);
        if (resetVector) {
            m_ResetValue[tuningItem] = true;
        }
    }


    bool Tuning::has(const std::string& tuningItem, size_t timestep) const {
        const auto hasIt = m_has_fields.find(tuningItem);
        if (hasIt == m_has_fields.end())
            return false;

        return hasIt->second.get(timestep) == 1 ? true : false;
    }
} //namespace Opm
