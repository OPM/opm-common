/*
  Copyright 2020 Equinor ASA.

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

#ifndef ORIGINAL_OIP
#define ORIGINAL_OIP

#include <string>
#include <unordered_map>
#include <vector>

namespace Opm {


class Inplace {
public:

    enum class Phase {
        WATER = 0,
        OIL = 1,
        GAS = 2,
        OilInLiquidPhase = 3,
        OilInGasPhase = 4,
        GasInLiquidPhase = 5,
        GasInGasPhase = 6,
        PoreVolume = 7
    };

    /*
      The purpose of this class is to transport inplace values from the
      simulator code to the summary output code. The code is written very much
      to fit in with the current implementation in the simulator, in particular
      that the add/get functions exist in two varieties is a result of that.

      The functions which don't accept region_name & region_number arguments
      should be called for totals, i.e. field properties.
    */


    void add(const std::string& region, const std::string& tag, std::size_t region_number, double value);
    void add(const std::string& region, Phase phase, std::size_t region_number, double value);
    void add(Phase phase, double value);
    void add(const std::string& tag, double value);

    double get(const std::string& region, const std::string& tag, std::size_t region_number) const;
    double get(const std::string& region, Phase phase, std::size_t region_number) const;
    double get(Phase phase) const;
    double get(const std::string& tag) const;

    std::size_t max_region() const;
    std::size_t max_region(const std::string& region_name) const;

    /*
      The get_vector functions return a vector length max_region() which
      contains the values added with the add() function and indexed with
      (region_number - 1). This is an incarnation of id <-> index confusion and
      should be replaced with a std::map instead.
    */
    std::vector<double> get_vector(const std::string& region, const std::string& tag) const;
    std::vector<double> get_vector(const std::string& region, Phase phase) const;

    static const std::vector<Phase>& phases();
private:
    std::unordered_map<std::string, std::unordered_map<Phase, std::unordered_map<std::size_t, double>>> phase_values;
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::size_t, double>>> tag_values;
};


}

#endif
