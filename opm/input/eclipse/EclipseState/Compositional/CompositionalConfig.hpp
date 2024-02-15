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

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>
#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <fmt/format.h>

#include <optional>
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
    // binary_interaction_coefficient will likely need some design when we know how we use it
    const std::vector<double>& binaryInteractionCoefficient(size_t eos_region) const;



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
    size_t num_comps = 0;
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

    static void warningForExistingCompKeywords(const PROPSSection& section);

    // The following function is used to parse the following keywords:
    // MW, ACF, BIC, PCRIT, TCRIT and VCRIT
    template<typename Keyword>
    static void processKeyword(const PROPSSection& props_section,
                               std::vector<std::vector<double>>& target,
                               const std::size_t num_eos_res,
                               const std::size_t num_values,
                               const std::string& kw_name,
                               const std::optional<double> default_value = std::nullopt) {
        if ( !props_section.hasKeyword<Keyword>() ) {
            throw std::logic_error(kw_name + " is not specified for compositional simulation");
        }
        target.resize(num_eos_res);
        for (auto& vec : target) {
            if (default_value.has_value()) {
                vec.resize(num_values, default_value.value());
            } else {
                vec.resize(num_values);
            }
        }

        const auto& keywords = props_section.get<Keyword>();
        // we do not allow multiple input of the keyword unless proven otherwise
        if (keywords.size() > 1) {
            throw OpmInputError(fmt::format("there are multiple {} keyword specifications", kw_name),
                                keywords.begin()->location());
        }

        // there is no default value, we make sure we specify the exact number of values for
        // all the EOS regions and components
        const auto& kw = keywords.back();
        if (kw.size() != num_eos_res) {
            throw OpmInputError(fmt::format("there are {} EOS regions, while only {} regions are specified in {}",
                                           num_eos_res, kw.size(),
                                            kw_name), kw.location());
        }

        for (size_t i = 0; i < kw.size(); ++i) {
            const auto& item = kw.getRecord(i).template getItem<typename Keyword::DATA>();
            const auto data = item.getSIDoubleData();
            if ( !default_value.has_value() ) { // when there is no default values, we should specify all the values
                if (data.size() != num_values) {
                    const auto msg = fmt::format("in keyword {}, {} values are specified, which is different from the number of components {}",
                        kw_name, data.size(), num_values);
                    throw OpmInputError(msg, kw.location());
                }
            } else {
                if (data.size() > num_values) { // when there is default values, we should not specify more values than needed
                    const auto msg = fmt::format("in keyword {}, {} values are specified, which is bigger than the number"
                                                 " {} should be specified ",
                                      kw_name, data.size(), num_values);
                    throw OpmInputError(msg, kw.location());
                }
            }
            // using copy here to consider the situation that there is default values, we might not specify all the values
            // and keep the rest to be the default values
            std::copy(data.begin(), data.end(), target[i].begin());
        }
    }
};

}
#endif // OPM_COMPOSITIONALCONFIG_HPP
