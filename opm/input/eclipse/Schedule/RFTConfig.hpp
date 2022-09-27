/*
  Copyright 2019 Equinor ASA.

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
#ifndef RFT_CONFIG2_HPP
#define RFT_CONFIG2_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace Opm {

class RFTConfig
{
public:
    enum class RFT {
        YES      = 1,
        REPT     = 2,
        TIMESTEP = 3,
        FOPN     = 4,
        NO       = 5,
    };
    static std::string RFT2String(const RFT enumValue);
    static RFT RFTFromString(const std::string &stringValue);

    enum class PLT {
        YES      = 1,
        REPT     = 2,
        TIMESTEP = 3,
        NO       = 5,
    };
    static std::string PLT2String(const PLT enumValue);
    static PLT PLTFromString( const std::string& stringValue);

    void first_open(bool on);
    void update(const std::string& wname, const PLT mode);
    void update(const std::string& wname, const RFT mode);
    void update_segment(const std::string& wname, const PLT mode);

    bool active() const;

    bool rft() const;
    bool rft(const std::string& wname) const;

    bool plt() const;
    bool plt(const std::string& wname) const;

    bool segment() const;
    bool segment(const std::string& wname) const;

    std::optional<RFTConfig> next() const;
    std::optional<RFTConfig> well_open(const std::string& wname) const;

    static RFTConfig serializeObject();
    bool operator==(const RFTConfig& data) const;

    template <class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->first_open_rft);
        serializer(this->rft_state);
        serializer(this->plt_state);
        serializer(this->seg_state);
        serializer(this->open_wells);
    }

private:
    template <typename Kind>
    using StateMap = std::unordered_map<std::string, Kind>;

    // Please make sure that member functions serializeOp(), operator==(),
    // and serializeObject() are also up to date when changing this list of
    // data members.
    bool first_open_rft = false;
    StateMap<RFT> rft_state{};
    StateMap<PLT> plt_state{};
    StateMap<PLT> seg_state{};
    std::unordered_map<std::string, bool> open_wells{};

    void update_state(const std::string& wname,
                      const PLT          mode,
                      StateMap<PLT>&     state);
};

} // namespace Opm

#endif // RFT_CONFIG2_HPP
