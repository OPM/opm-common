/*
  Copyright 2018 NORCE.

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

#ifndef WELLTRACERPROPERTIES_HPP_HEADER_INCLUDED
#define WELLTRACERPROPERTIES_HPP_HEADER_INCLUDED

#include <opm/input/eclipse/Deck/UDAValue.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <vector>
#include <string>
#include <map>

namespace Opm {

    class SummaryState;

    class WellTracerProperties
    {
    public:
        struct Well { const std::string& name; };
        struct Tracer { const std::string& name; };

        void setConcentration(const Tracer& tracer, const UDAValue& conc);
        double getConcentration(const Well& well, const Tracer& tracer, const SummaryState& st) const;

        bool operator==(const WellTracerProperties& other) const;
        bool operator!=(const WellTracerProperties& other) const;

        static WellTracerProperties serializationTestObject();

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_tracerConcentrations);
        }

    private:
        std::map<std::string, UDAValue> m_tracerConcentrations;
    };

}

#endif
