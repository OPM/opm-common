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

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <fmt/format.h>

#include <vector>

namespace Opm {

class Deck;
class Runspec;
// class Ta

// TODO: adding serialization?

class CompostionalConfig {
public:
    enum class EOSType {
        PR,   // Peng-Robinson
        RK,   // Redlich-Kwong
        SRK,  // Soave-Redlich-Kwong
        ZJ    // Zudkevitch-Joffe-Redlich-Kwong
    };

    static EOSType eosTypeFromString(const std::string& str);

    static std::string eosTypeToString(EOSType eos);

    CompostionalConfig() = default;

    CompostionalConfig(const Deck& deck, const Runspec& runspec);

    static CompostionalConfig serializationTestObject();

    bool operator==(const CompostionalConfig& other) const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(num_comps);
        serializer(eos_types);
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
    std::vector<EOSType> eos_types;
    std::vector<std::vector<double>> acentric_factors;
    std::vector<std::vector<double>> critical_pressure;
    std::vector<std::vector<double>> critical_temperature;
    std::vector<std::vector<double>> critical_volume;
    std::vector<std::vector<double>> binary_interaction_coefficient; // TODO: might need a separate class for better efficiency

    static void warningForExistingCompKeywords(const PROPSSection& section);

    // The following function is used to parse keywords that do not have default values
    // at the moment, it is used to parse PCRIT, TCRIT and VCRIT
    template<typename Keyword>
    static void processKeyword(const PROPSSection& props_section,
                               std::vector<std::vector<double>>& target,
                               const unsigned num_eos_res,
                               const unsigned num_comps,
                               const std::string& keywordName) {
        target.resize(num_eos_res);
        for (auto& vec : target) {
            vec.resize(num_comps);
        }

        if (props_section.hasKeyword<Keyword>()) {
            const auto& keywords = props_section.get<Keyword>();
            // we do not allow multiple input of the keyword ACF unless proven otherwi
            if (keywords.size() > 1) {
                throw OpmInputError(fmt::format("there are multiple {} keyword specifications", keywordName),
                                    keywords.begin()->location());
            }

            // there is no default value, we make sure we specify the exact number of values for
            // all the EOS regions and components
            const auto& kw = keywords.back();
            if (kw.size() != num_eos_res) {
                throw OpmInputError(fmt::format("there are {} EOS regions, while only {} regions are specified in {}",
                                                num_eos_res, kw.size(), keywordName), kw.location());
            }

            for (size_t i = 0; i < kw.size(); ++i) {
                const auto& item = kw.getRecord(i).template getItem<typename Keyword::DATA>();
                const auto data = item.template getData<double>();
                if (data.size() != num_comps) {
                    const auto msg = fmt::format("in keyword {}, {} values are specified, which is different from the number of components {}",
                                                 keywordName, data.size(), num_comps);
                    throw OpmInputError(msg, kw.location());
                }
                target[i] = data;
            }
        }
    }
};

}
#define OPM_COMPOSITIONALCONFIG_HPP

#endif // OPM_COMPOSITIONALCONFIG_HPP
