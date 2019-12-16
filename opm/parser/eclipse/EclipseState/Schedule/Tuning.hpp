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

        template<class T>
        void set(const std::string& tuningItem, size_t timestep, T value);
        template<class T>
        T get(const std::string& tuningItem, size_t timestep) const;

        bool has(const std::string& tuningItem, size_t timestep) const;

        template<class T>
        void setInitial(const std::string& tuningItem, T value, bool resetVector);

    private:
        std::map<std::string, DynamicState<double>> m_fields;
        std::map<std::string, DynamicState<int>> m_int_fields;
        std::map<std::string, DynamicState<int>> m_has_fields;
        std::map<std::string, bool> m_ResetValue;
    };

    template<>
    int Tuning::get<int>(const std::string& tuningItem, size_t timestep) const;
    template<>
    double Tuning::get<double>(const std::string& tuningItem, size_t timestep) const;

    template<>
    void Tuning::set<int>(const std::string& tuningItem, size_t timestep, int value);
    template<>
    void Tuning::set<double>(const std::string& tuningItem, size_t timestep, double value);

    template<>
    void Tuning::setInitial<int>(const std::string& tuningItem, int value, bool resetVector);
    template<>
    void Tuning::setInitial<double>(const std::string& tuningItem, double value, bool resetVector);

} //namespace Opm

#endif
