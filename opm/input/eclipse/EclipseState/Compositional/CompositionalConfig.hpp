/*
  Copyright (C) 2024 SINTEF Digital, Mathematics and Cybernetics.

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

#ifndef OPM_COMPOSITIONALCONFIG_HPP
#define OPM_COMPOSITIONALCONFIG_HPP

#include <opm/input/eclipse/Units/Units.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace Opm {

class Deck;
class Runspec;

class CompositionalConfig {
public:
    enum class EOSType {
        PR,   // Peng-Robinson
        RK,   // Redlich-Kwong
        SRK,  // Soave-Redlich-Kwong
        ZJ    // Zudkevitch-Joffe-Redlich-Kwong
    };

    static EOSType eosTypeFromString(const std::string& str);

    static std::string eosTypeToString(EOSType eos);

    CompositionalConfig() = default;

    CompositionalConfig(const Deck& deck, const Runspec& runspec);

    static CompositionalConfig serializationTestObject();

    bool operator==(const CompositionalConfig& other) const;

    // accessing functions
    double standardTemperature() const;
    double standardPressure() const;
    const std::vector<std::string>& compName() const;
    EOSType eosType(size_t eos_region) const;
    const std::vector<double>& molecularWeights(std::size_t eos_region) const;
    const std::vector<double>& acentricFactors(std::size_t eos_region) const;
    const std::vector<double>& criticalPressure(std::size_t eos_region) const;
    const std::vector<double>& criticalTemperature(std::size_t eos_region) const;
    const std::vector<double>& criticalVolume(std::size_t eos_region) const;
    const std::vector<double>& binaryInteractionCoefficient(size_t eos_region) const;
    std::size_t numComps() const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(num_comps);
        serializer(standard_temperature);
        serializer(standard_pressure);
        serializer(comp_names);
        serializer(eos_types);
        serializer(molecular_weights);
        serializer(acentric_factors);
        serializer(critical_pressure);
        serializer(critical_temperature);
        serializer(critical_volume);
        serializer(binary_interaction_coefficient);
    }

private:
    // TODO: num_comps might not be totally necessary, while might be convenient.
    //  We can check the number of components without accessing Runspec
    std::size_t num_comps = 0;
    double standard_temperature = 288.71; // Kelvin
    double standard_pressure = 1.0 * unit::atm; // 1 atm
    std::vector<std::string> comp_names;
    std::vector<EOSType> eos_types;
    std::vector<std::vector<double>> molecular_weights;
    std::vector<std::vector<double>> acentric_factors;
    std::vector<std::vector<double>> critical_pressure;
    std::vector<std::vector<double>> critical_temperature;
    std::vector<std::vector<double>> critical_volume;
    std::vector<std::vector<double>> binary_interaction_coefficient;
};

}
#endif // OPM_COMPOSITIONALCONFIG_HPP
