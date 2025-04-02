/*
  Copyright 2025 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/RptschedKeywordNormalisation.hpp>

#include <opm/input/eclipse/Schedule/SimpleRPTIntegerControlHandler.hpp>

#include <opm/input/eclipse/Schedule/RPTKeywordNormalisation.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace {
    Opm::SimpleRPTIntegerControlHandler makeIntegerControlHandler()
    {
        return Opm::SimpleRPTIntegerControlHandler {
            "PRES",     // 1
            "SOIL",     // 2
            "SWAT",     // 3
            "SGAS",     // 4
            "RS",       // 5
            "RV",       // 6
            "RESTART",  // 7
            "FIP",      // 8
            "WELLS",    // 9
            "VFPPROD",  // 10
            "SUMMARY",  // 11
            "CPU",      // 12
            "AQUCT",    // 13
            "WELSPECS", // 14
            "NEWTON",   // 15
            "POILD",    // 16
            "PWAT",     // 17
            "PWATD",    // 18
            "PGAS",     // 19
            "PGASD",    // 20
            "FIPVE",    // 21
            "WOC",      // 22
            "GOC",      // 23
            "WOCDIFF",  // 24
            "GOCDIFF",  // 25
            "WOCGOC",   // 26
            "ODGAS",    // 27
            "ODWAT",    // 28
            "GDOWAT",   // 29
            "WDOGAS",   // 30
            "OILAPI",   // 31
            "FIPITR",   // 32
            "TBLK",     // 33
            "PBLK",     // 34
            "SALT",     // 35
            "PLYADS",   // 36
            "RK",       // 37
            "FIPSALT",  // 38
            "TUNING",   // 39
            "GI",       // 40
            "ROCKC",    // 41
            "SPENWAT",  // 42
            "FIPSOL",   // 43
            "SURFBLK",  // 44
            "SURFADS",  // 45
            "FIPSURF",  // 46
            "TRADS",    // 47
            "VOIL",     // 48
            "VWAT",     // 49
            "VGAS",     // 50
            "DENO",     // 51
            "DENW",     // 52
            "DENG",     // 53
            "GASCONC",  // 54
            "PB",       // 55
            "PD",       // 56
            "KRW",      // 57
            "KRO",      // 58
            "KRG",      // 59
            "MULT",     // 60
            "UNKNOWN",  // 61
            "UNKNOWN",  // 62
            "FOAM",     // 63
            "FIPFOAM",  // 64
            "TEMP",     // 65
            "FIPTEMP",  // 66
            "POTC",     // 67
            "FOAMADS",  // 68
            "FOAMDCY",  // 69
            "FOAMMOB",  // 70
            "RECOV",    // 71
            "FLOOIL",   // 72
            "FLOWAT",   // 73
            "FLOGAS",   // 74
            "SGTRAP",   // 75
            "FIPRESV",  // 76
            "FLOSOL",   // 77
            "KRN",      // 78
            "GRAD",     // 79
        };
    }

    class IsRptSchedMnemonic
    {
    public:
        IsRptSchedMnemonic();

        bool operator()(const std::string& mnemonic) const;

    private:
        std::vector<std::string> mnemonics_{};
    };

    IsRptSchedMnemonic::IsRptSchedMnemonic()
        : mnemonics_ {
                // Note: This list of mnemonics *must* be in alphabetically
                // sorted order.
                "ALKALINE", "ANIONS",  "AQUCT",    "AQUFET",   "AQUFETP",  "BFORG",
                "CATIONS",  "CPU",     "DENG",     "DENO",     "DENW",     "ESALPLY",
                "ESALSUR",  "FFORG",   "FIP",      "FIPFOAM",  "FIPHEAT",  "FIPRESV",
                "FIPSALT",  "FIPSOL",  "FIPSURF",  "FIPTEMP",  "FIPTR",    "FIPVE",
                "FLOGAS",   "FLOOIL",  "FLOSOL",   "FLOWAT",   "FMISC",    "FOAM",
                "FOAMADS",  "FOAMCNM", "FOAMDCY",  "FOAMMOB",  "GASCONC",  "GASSATC",
                "GDOWAT",   "GI",      "GOC",      "GOCDIFF",  "GRAD",     "KRG",
                "KRN",      "KRO",     "KRW",      "MULT",     "NEWTON",   "NOTHING",
                "NPMREB",   "ODGAS",   "ODWAT",    "OILAPI",   "PB",       "PBLK",
                "PBU",      "PD",      "PDEW",     "PGAS",     "PGASD",    "PLYADS",
                "POIL",     "POILD",   "POLYMER",  "POTC",     "POTG",     "POTO",
                "POTW",     "PRES",    "PRESSURE", "PWAT",     "PWATD",    "RECOV",
                "RESTART",  "ROCKC",   "RS",       "RSSAT",    "RV",       "RVSAT",
                "SALT",     "SGAS",    "SGTRAP",   "SIGM_MOD", "SOIL",     "SSOL",
                "SUMMARY",  "SURFADS", "SURFBLK",  "SWAT",     "TBLK",     "TEMP",
                "TRACER",   "TRADS",   "TRDCY",    "TUNING",   "VFPPROD",  "VGAS",
                "VOIL",     "VWAT",    "WDOGAS",   "WELLS",    "WELSPECL", "WELSPECS",
                "WOC",      "WOCDIFF", "WOCGOC",
            }
    {}

    bool IsRptSchedMnemonic::operator()(const std::string& mnemonic) const
    {
        return std::binary_search(this->mnemonics_.begin(),
                                  this->mnemonics_.end(), mnemonic);
    }
} // Anonymous namespace

// ===========================================================================
// Public interface below
// ===========================================================================

Opm::RPTKeywordNormalisation::MnemonicMap
Opm::normaliseRptSchedKeyword(const DeckKeyword&  kw,
                              const ParseContext& parseContext,
                              ErrorGuard&         errors)
{
    return RPTKeywordNormalisation {
        makeIntegerControlHandler(), IsRptSchedMnemonic{}
    }.normaliseKeyword(kw, parseContext, errors);
}
