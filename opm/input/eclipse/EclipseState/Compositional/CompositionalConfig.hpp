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
    }
private:
    int num_comps = -1; // might not be needed, while might be convenient // size_t?
    std::vector<EOSType> eos_types;
    std::vector<std::vector<double>> acentric_factors;
};

}
#define OPM_COMPOSITIONALCONFIG_HPP

#endif // OPM_COMPOSITIONALCONFIG_HPP
