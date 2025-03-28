/*
  Copyright 2021 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/RSTConfig.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Utility/Functional.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/R.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <iterator>
#include <map>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace {

static constexpr auto SCHEDIntegerKeywords = std::array {
    "PRES",    // 1
    "SOIL",    // 2
    "SWAT",    // 3
    "SGAS",    // 4
    "RS",      // 5
    "RV",      // 6
    "RESTART", // 7
    "FIP",     // 8
    "WELLS",   // 9
    "VFPPROD", // 10
    "SUMMARY", // 11
    "CPU",     // 12
    "AQUCT",   // 13
    "WELSPECS",// 14
    "NEWTON",  // 15
    "POILD",   // 16
    "PWAT",    // 17
    "PWATD",   // 18
    "PGAS",    // 19
    "PGASD",   // 20
    "FIPVE",   // 21
    "WOC",     // 22
    "GOC",     // 23
    "WOCDIFF", // 24
    "GOCDIFF", // 25
    "WOCGOC",  // 26
    "ODGAS",   // 27
    "ODWAT",   // 28
    "GDOWAT",  // 29
    "WDOGAS",  // 30
    "OILAPI",  // 31
    "FIPITR",  // 32
    "TBLK",    // 33
    "PBLK",    // 34
    "SALT",    // 35
    "PLYADS",  // 36
    "RK",      // 37
    "FIPSALT", // 38
    "TUNING",  // 39
    "GI",      // 40
    "ROCKC",   // 41
    "SPENWAT", // 42
    "FIPSOL",  // 43
    "SURFBLK", // 44
    "SURFADS", // 45
    "FIPSURF", // 46
    "TRADS",   // 47
    "VOIL",    // 48
    "VWAT",    // 49
    "VGAS",    // 50
    "DENO",    // 51
    "DENW",    // 52
    "DENG",    // 53
    "GASCONC", // 54
    "PB",      // 55
    "PD",      // 56
    "KRW",     // 57
    "KRO",     // 58
    "KRG",     // 59
    "MULT",    // 60
    "UNKNOWN", // 61 (not listed in the manual)
    "UNKNOWN", // 62 (not listed in the manual)
    "FOAM",    // 63
    "FIPFOAM", // 64
    "TEMP",    // 65
    "FIPTEMP", // 66
    "POTC",    // 67
    "FOAMADS", // 68
    "FOAMDCY", // 69
    "FOAMMOB", // 70
    "RECOV",   // 71
    "FLOOIL",  // 72
    "FLOWAT",  // 73
    "FLOGAS",  // 74
    "SGTRAP",  // 75
    "FIPRESV", // 76
    "FLOSOL",  // 77
    "KRN",     // 78
    "GRAD",    // 79
};

static constexpr auto RSTIntegerKeywords = std::array {
    "BASIC",      //  1
    "FLOWS",      //  2
    "FIP",        //  3
    "POT",        //  4
    "PBPD",       //  5
    "FREQ",       //  6
    "PRES",       //  7
    "VISC",       //  8
    "DEN",        //  9
    "DRAIN",      // 10
    "KRO",        // 11
    "KRW",        // 12
    "KRG",        // 13
    "PORO",       // 14
    "NOGRAD",     // 15
    "NORST",      // 16 NORST - not supported
    "SAVE",       // 17
    "SFREQ",      // 18 SFREQ=?? - not supported
    "ALLPROPS",   // 19
    "ROCKC",      // 20
    "SGTRAP",     // 21
    "",           // 22 - Blank - ignored.
    "RSSAT",      // 23
    "RVSAT",      // 24
    "GIMULT",     // 25
    "SURFBLK",    // 26
    "",           // 27 - PCOW, PCOG, special cased
    "STREAM",     // 28 STREAM=?? - not supported
    "RK",         // 29
    "VELOCITY",   // 30
    "COMPRESS",   // 31
};

bool is_int(const std::string& x)
{
    auto is_digit = [](char c) { return std::isdigit(c); };

    return !x.empty()
        && ((x.front() == '-') || is_digit(x.front()))
        && std::all_of(x.begin() + 1, x.end(), is_digit);
}

bool is_RPTRST_mnemonic(const std::string& kw)
{
    // Every ECLIPSE 100 keyword we want to not simply ignore when handling
    // RPTRST.  The list is sorted, so we can use binary_search for log(n)
    // lookup.  It is important that the list is sorted, but these are all
    // the keywords listed in the manual and unlikely to change at all
    static constexpr const char* valid[] = {
        "ACIP",     "ACIS",     "ALLPROPS", "BASIC",    "BG",      "BO",
        "BW",       "CELLINDX", "COMPRESS", "CONV",     "DEN",     "DENG",
        "DENO",     "DENW",     "DRAIN",    "DRAINAGE", "DYNREG",  "FIP",
        "FLORES",   "FLORES-",  "FLOWS",    "FLOWS-",   "FREQ",    "GIMULT",
        "HYDH",     "HYDHFW",   "KRG",      "KRO",      "KRW",     "NOGRAD",
        "NORST",    "NPMREB",   "PBPD",     "PCGW",     "PCOG",    "PCOW",
        "PERMREDN", "POIS",     "PORO",     "PORV",     "POT",     "PRES",
        "RESIDUAL", "RFIP",     "RK",       "ROCKC",    "RPORV",   "RSSAT",
        "RSWSAT",   "RVSAT",    "RVWSAT",   "SAVE",     "SDENO",   "SFIP",
        "SFREQ",    "SGTRAP",   "SIGM_MOD", "STREAM",   "SURFBLK", "TEMP",
        "TRAS",     "VELGAS",   "VELOCITY", "VELOIL",   "VELWAT",  "VGAS",
        "VISC",     "VOIL",     "VWAT",
    };

    return std::binary_search(std::begin(valid), std::end(valid), kw);
}

bool is_RPTRST_mnemonic_compositional(const std::string& kw)
{
    // Every compositional keyword we want to not simply ignore when handling
    // RPTRST.  The list is sorted, so we can use binary_search for log(n)
    // lookup.  It is important that the list is sorted, but these are all
    // the keywords listed in the manual and unlikely to change at all
    // TODO: FLOCn, LGLCn and TRMFxxxx are not in the list and needs to be handled separately
    static constexpr const char* valid[] = {
        "AIM",      "ALSURF",   "ALSTML",   "AMF",      "AQSP",    "AQPH",
        "AREAC",    "ASPADS",   "ASPDOT",   "ASPENT",   "ASPFLO",  "ASPFLT",
        "ASPFRD",   "ASPKDM",   "ASPLIM",   "ASPLUG",   "ASPRET",  "ASPREW",
        "ASPVEL",   "ASPVOM",   "BASIC",    "BFORO",    "BG",      "BGAS",
        "BO",       "BOIL",     "BSOL",     "BTFORG",   "BTFORO",  "BW",
        "BWAT",     "CELLINDX", "CFL",      "CGAS",     "COLR",    "COILR",
        "CONV",     "DENG",     "DENO",     "DENS",     "DENW",    "DYNREG",
        "ENERGY",   "ESALTS",   "ESALTP",   "FFACTG",   "FFACTO",  "FFORO",
        "FIP",      "FLOE",     "FLOGAS",   "FLOOIL",   "FLOWAT",  "FLORES",
        "FLORES-",  "FMISC",    "FOAM",     "FOAMST",   "FOAMCNM", "FOAMMOB",
        "FPC",      "FREQ",     "FUGG",     "FUGO",     "GASPOT",  "HGAS",
        "HOIL",     "HSOL",     "HWAT",     "JV",       "KRG",     "KRO",
        "KRW",      "KRGDM",    "KRODM",    "KRWDM",    "LGLCWAT", "LGLCHC",
        "MLSC",     "MWAT",     "NCNG",     "NCNO",     "NPMREB",  "OILPOT",
        "PART",     "PCGW",     "PCOG",     "PCOW",     "PERM_MDX","PERM_MDY",
        "PERM_MDZ", "PERM_MOD", "PGAS",     "PKRG",     "PKRGR",   "PKRO",
        "PKRORG",   "PKRORW",   "PKRW",     "PKRWR",    "POIL",    "POLY",
        "POLYVM",   "PORV",     "PORV_MOD", "PPCG",     "PPCW",    "PRES_EFF",
        "PRES",     "PRESMIN",  "PRESSURE", "PSAT",     "PSGCR",   "PSGL",
        "PSGU",     "PSOGCR",   "PSOWCR",   "PSWCR",    "PSWL",    "PSWU",
        "PVDPH",    "PWAT",     "RATP",     "RATS",     "RATT",    "REAC",
        "RESTART",  "RFIP",     "ROCKC",    "ROMLS",    "RPORV",   "RS",
        "RSSAT",    "RSW",      "RV",       "RVSAT",    "SFIP",    "SFIPGAS",
        "SFIPOIL",  "SFIPWAT",  "SFOIL",    "SFSOL",    "SGAS",    "SGASMAX",
        "SGCRH",    "SGTRH",    "SGTRAP",   "SIGM_MOD", "SMF",     "SMMULT",
        "SOIL",     "SOILM",    "SOILMAX",  "SOILR",    "SOLADS",  "SOLADW",
        "SOLWET",   "SSFRAC",   "SSOLID",   "STATE",    "STEN",    "SUBG",
        "SURF",     "SURFCNM",  "SURFKR",   "SURFCP",   "SURFST",  "SWAT",
        "SWATMIN",  "TCBULK",   "TCMULT",   "TEMP",     "TOTCOMP", "TREACM",
        "TSUB",     "VGAS",     "VOIL",     "VMF",      "VWAT",    "WATPOT",
        "XFW",      "XGAS",     "XMF",      "XWAT",     "YFW",     "YMF",
        "ZMF"
    };

    return std::binary_search(std::begin(valid), std::end(valid), kw);
}

bool is_RPTSCHED_mnemonic(const std::string& kw)
{
    static constexpr const char* valid[] = {
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
    };

    return std::binary_search(std::begin(valid), std::end(valid), kw);
}

std::map<std::string, int>
RPTSCHED_integer(const std::vector<int>& ints)
{
    const std::size_t size = std::min(ints.size(), SCHEDIntegerKeywords.size());

    std::map<std::string, int> mnemonics;
    for (std::size_t i = 0; i < size; ++i) {
        mnemonics[SCHEDIntegerKeywords[i]] = ints[i];
    }

    return mnemonics;
}

std::map<std::string, int>
RPTRST_integer(const std::vector<int>& ints)
{
    std::map<std::string, int> mnemonics;

    const std::size_t PCO_index = 26;
    const std::size_t BASIC_index = 0;

    const std::size_t size = std::min(ints.size(), RSTIntegerKeywords.size());

    // Fun with special cases.  ECLIPSE seems to ignore the BASIC=0,
    // interpreting it as sort-of "don't modify".  Handle this by *not*
    // adding/updating the integer list sourced BASIC mnemonic, should it be
    // zero.  I'm not sure if this applies to other mnemonics, but the
    // ECLIPSE manual indicates that any zero here should disable the
    // output.
    //
    // See https://github.com/OPM/opm-parser/issues/886 for reference
    //
    // The current treatment of a mix on RPTRST and RPTSCHED integer
    // keywords is probably not correct, but it is extremely difficult to
    // comprehend exactly how it should be. Current code is a rather
    // arbitrary hack to get through the tests.

    if (size >= PCO_index) {
        for (std::size_t i = 0; i < std::min(size, PCO_index); ++i) {
            mnemonics[RSTIntegerKeywords[i]] = ints[i];
        }
    }
    else {
        if ((size > 0) && (ints[BASIC_index] != 0)) {
            mnemonics[RSTIntegerKeywords[BASIC_index]] = ints[BASIC_index];
        }

        for (std::size_t i = 1; i < std::min(size, PCO_index); ++i) {
            mnemonics[RSTIntegerKeywords[i]] = ints[i];
        }
    }

    for (std::size_t i = PCO_index + 1; i < size; ++i) {
        mnemonics[RSTIntegerKeywords[i]] = ints[i];
    }

    // Item 27 (index 26) sets both PCOW and PCOG, so we special case it here.
    if (ints.size() > PCO_index) {
        mnemonics["PCOW"] = ints[PCO_index];
        mnemonics["PCOG"] = ints[PCO_index];
    }

    return mnemonics;
}

template <typename F, typename G>
std::map<std::string, int>
RPT(const Opm::DeckKeyword&  keyword,
    const Opm::ParseContext& parseContext,
    Opm::ErrorGuard&         errors,
    F                        is_mnemonic,
    G                        integer_mnemonic)
{
    std::vector<std::string> items;

    const auto& deck_items = keyword.getStringData();
    const auto ints =  std::any_of(deck_items.begin(), deck_items.end(), is_int);
    const auto strs = !std::all_of(deck_items.begin(), deck_items.end(), is_int);

    // If any of the values are pure integers we assume this is meant to be
    // the slash-terminated list of integers way of configuring.  If
    // integers and non-integers are mixed, this is an error; however if the
    // error mode RPT_MIXED_STYLE is permissive we try some desperate
    // heuristics to interpret this as list of mnemonics.  See the
    // documentation of the RPT_MIXED_STYLE error handler for more details.
    if (! strs) {
        auto stoi = [](const std::string& str) { return std::stoi(str); };
        return integer_mnemonic(Opm::fun::map(stoi, deck_items));
    }

    if (ints) {
        const auto msg = std::string {
            "Error in keyword {keyword}--mixing "
            "mnemonics and integers is not permitted\n"
            "In {file} line {line}."
        };

        const auto& location = keyword.location();
        parseContext.handleError(Opm::ParseContext::RPT_MIXED_STYLE,
                                 msg, location, errors);

        std::vector<std::string> stack;
        for (std::size_t index = 0; index < deck_items.size(); ++index) {
            if (is_int(deck_items[index])) {
                if (stack.size() < 2) {
                    throw Opm::OpmInputError {
                        "Problem processing {keyword}\nIn {file} line {line}.", location
                    };
                }

                if (stack.back() == "=") {
                    stack.pop_back();
                    const std::string mnemonic = stack.back();
                    stack.pop_back();

                    items.insert(items.begin(), stack.begin(), stack.end());

                    stack.clear();
                    items.push_back(mnemonic + "=" + deck_items[index]);
                }
                else {
                    throw Opm::OpmInputError {
                        "Problem processing {keyword}\nIn {file} line {line}.", location
                    };
                }
            }
            else {
                stack.push_back(deck_items[index]);
            }
        }

        items.insert(items.begin(), stack.begin(), stack.end());
    }
    else {
        items = deck_items;
    }

    std::map<std::string, int> mnemonics;
    for (const auto& mnemonic : items) {
        const auto sep_pos = mnemonic.find_first_of( "= " );

        const auto base = mnemonic.substr(0, sep_pos);
        if (! is_mnemonic(base)) {
            const auto msg_fmt =
                fmt::format("Error in keyword {{keyword}}, "
                            "unrecognized mnemonic {}\n"
                            "In {{file}} line {{line}}.", base);

            parseContext.handleError(Opm::ParseContext::RPT_UNKNOWN_MNEMONIC,
                                     msg_fmt, keyword.location(), errors);
            continue;
        }

        int val = 1;
        if (sep_pos != std::string::npos) {
            const auto value_pos = mnemonic.find_first_not_of("= ", sep_pos);
            if (value_pos != std::string::npos) {
                val = std::stoi(mnemonic.substr(value_pos));
            }
        }

        mnemonics.emplace(base, val);
    }

    return mnemonics;
}

void expand_RPTRST_mnemonics(std::map<std::string, int>& mnemonics)
{
    auto allprops_iter = mnemonics.find("ALLPROPS");
    if (allprops_iter == mnemonics.end()) {
        return;
    }

    const auto value = allprops_iter->second;
    mnemonics.erase(allprops_iter);

    for (const auto& kw : {"BG", "BO", "BW", "KRG", "KRO", "KRW", "VOIL", "VGAS", "VWAT", "DEN"}) {
        mnemonics[kw] = value;
    }
}

std::optional<int> extract(std::map<std::string, int>& mnemonics, const std::string& key)
{
    auto iter = mnemonics.find(key);
    if (iter == mnemonics.end()) {
        return {};
    }

    int value = iter->second;
    mnemonics.erase(iter);
    return value;
}

std::pair<
    std::map<std::string, int>,
    std::pair<std::optional<int>, std::optional<int>>
    >
RPTRST(const Opm::DeckKeyword&  keyword,
       const Opm::ParseContext& parseContext,
       Opm::ErrorGuard&         errors,
       const bool               compositional = false)
{
    const auto is_mnemonic = compositional
                             ? is_RPTRST_mnemonic_compositional
                             : is_RPTRST_mnemonic;
    auto mnemonics = RPT(keyword, parseContext, errors,
                         is_mnemonic, RPTRST_integer);

    const auto basic = extract(mnemonics, "BASIC");
    const auto freq  = extract(mnemonics, "FREQ");

    expand_RPTRST_mnemonics(mnemonics);

    return { mnemonics, { basic, freq }};
}

template <typename T>
void update_optional(std::optional<T>&       target,
                     const std::optional<T>& src)
{
    if (src.has_value()) {
        target = src;
    }
}

} // Anonymous namespace

// ---------------------------------------------------------------------------

namespace Opm {

RSTConfig::RSTConfig(const SOLUTIONSection& solution_section,
                     const ParseContext&    parseContext,
                     const bool compositional_arg,
                     ErrorGuard&            errors)
    : write_rst_file(true)
    , compositional(compositional_arg)
{
    for (const auto& keyword : solution_section) {
        if (keyword.name() == ParserKeywords::RPTRST::keywordName) {
            const auto in_solution = true;
            this->handleRPTRST(keyword, parseContext, errors, in_solution);

            // Generating restart file output at time zero is normally
            // governed by setting the 'RESTART' mnemonic to a value greater
            // than one (1) in the RPTSOL keyword.  Here, when handling the
            // RPTRST keyword in the SOLUTION section, we unconditionally
            // request that restart file output be generated at time zero.
            this->write_rst_file = true;
        }
        else if (keyword.name() == ParserKeywords::RPTSOL::keywordName) {
            this->handleRPTSOL(keyword, parseContext, errors);
        }
    }
}

void RSTConfig::update(const DeckKeyword&  keyword,
                       const ParseContext& parseContext,
                       ErrorGuard&         errors)
{
    if (keyword.name() == ParserKeywords::RPTRST::keywordName) {
        this->handleRPTRST(keyword, parseContext, errors);
    }
    else if (keyword.name() == ParserKeywords::RPTSCHED::keywordName) {
        this->handleRPTSCHED(keyword, parseContext, errors);
    }
    else {
        throw std::logic_error("The RSTConfig object can only use RPTRST and RPTSCHED keywords");
    }
}

// The RPTRST keyword semantics differs between the SOLUTION and SCHEDULE
// sections.  This function takes an RSTConfig object constructed from
// SOLUTION section information and creates a transformed copy suitable as
// the first RSTConfig object in the SCHEDULE section.
RSTConfig RSTConfig::first(const RSTConfig& solution_config)
{
    auto rst_config = solution_config;

    rst_config.solution_only_keywords.clear();
    for (const auto& kw : solution_config.solution_only_keywords) {
        rst_config.keywords.erase(kw);
    }

    const auto basic = rst_config.basic;
    if (!basic.has_value()) {
        rst_config.write_rst_file = false;
        return rst_config;
    }

    const auto basic_value = basic.value();
    if (basic_value == 0) {
        rst_config.write_rst_file = false;
    }
    else if ((basic_value == 1) || (basic_value == 2)) {
        rst_config.write_rst_file = true;
    }
    else if (basic_value >= 3) {
        rst_config.write_rst_file = {};
    }

    return rst_config;
}

RSTConfig RSTConfig::serializationTestObject()
{
    RSTConfig rst_config;

    rst_config.basic = 10;
    rst_config.freq = {};
    rst_config.write_rst_file = true;
    rst_config.save = true;
    rst_config.compositional = false;
    rst_config.keywords = {{"S1", 1}, {"S2", 2}};
    rst_config.solution_only_keywords = { "FIP" };

    return rst_config;
}

bool RSTConfig::operator==(const RSTConfig& other) const
{
    return (this->write_rst_file == other.write_rst_file)
        && (this->keywords == other.keywords)
        && (this->basic == other.basic)
        && (this->freq == other.freq)
        && (this->save == other.save)
        && (this->compositional == other.compositional)
        && (this->solution_only_keywords == other.solution_only_keywords)
        ;
}

// Recall that handleRPTSOL() is private and invoked only from the
// RSTConfig constructor processing SOLUTION section information.

void RSTConfig::handleRPTSOL(const DeckKeyword&  keyword,
                             const ParseContext& parseContext,
                             ErrorGuard&         errors)
{
    // Note: We intentionally use RPTSCHED mnemonic handling here.  While
    // potentially misleading, this process does do what we want for the
    // typical cases.  Older style integer controls are however only
    // partially handled and we may choose to refine this logic by
    // introducing predicates specific to the RPTSOL keyword later.
    auto mnemonics = RPT(keyword, parseContext, errors,
                         is_RPTSCHED_mnemonic,
                         RPTSCHED_integer);

    const auto restart = extract(mnemonics, "RESTART");
    const auto request_restart =
        (restart.has_value() && (*restart > 1));

    this->write_rst_file =
        (this->write_rst_file.has_value() && *this->write_rst_file)
        || request_restart;

    if (request_restart) {
        // RPTSOL's RESTART flag is set.  Internalise new flags, as
        // "SOLUTION" only properties, from 'mnemonics'.
        this->keywords.swap(mnemonics);
        for (const auto& kw : this->keywords) {
            this->solution_only_keywords.insert(kw.first);
        }

        for (const auto& [key, value] : mnemonics) {
            // Note: Using emplace() after swap() means that the mnemonics
            // from RPTSOL overwrite those that might already have been
            // present in 'keywords'.  Recall that emplace() will not insert
            // a new element with the same key as an existing element.
            this->keywords.emplace(key, value);
        }
    }
}

void RSTConfig::handleRPTRST(const DeckKeyword&  keyword,
                             const ParseContext& parseContext,
                             ErrorGuard&         errors,
                             const bool          in_solution)
{
    const auto& [mnemonics, basic_freq] = RPTRST(keyword, parseContext, errors, compositional);

    this->update_schedule(basic_freq);

    for (const auto& [kw, num] : mnemonics) {
        // Insert_or_assign() to overwrite existing 'kw' elements.
        this->keywords.insert_or_assign(kw, num);
    }

    if (in_solution) {
        // We're processing RPTRST in the SOLUTION section.  Mnemonics from
        // RPTRST should persist beyond the SOLUTION section in this case so
        // prune these from the list of solution-only keywords.
        for (const auto& kw : mnemonics) {
            this->solution_only_keywords.erase(kw.first);
        }
    }
}

void RSTConfig::handleRPTSCHED(const DeckKeyword&  keyword,
                               const ParseContext& parseContext,
                               ErrorGuard&         errors)
{
    auto mnemonics = RPT(keyword, parseContext, errors,
                         is_RPTSCHED_mnemonic,
                         RPTSCHED_integer);

    if (const auto nothing = extract(mnemonics, "NOTHING"); nothing.has_value()) {
        this->basic = {};
        this->keywords.clear();
    }

    if (this->basic.value_or(2) <= 2) {
        const auto restart = extract(mnemonics, "RESTART");

        if (restart.has_value()) {
            const auto basic_value = std::min(2, restart.value());
            this->update_schedule({basic_value, 1});
        }
    }

    for (const auto& [kw, num] : mnemonics) {
        this->keywords.insert_or_assign(kw, num);
    }
}

void RSTConfig::update_schedule(const std::pair<std::optional<int>, std::optional<int>>& basic_freq)
{
    update_optional(this->basic, basic_freq.first);
    update_optional(this->freq, basic_freq.second);

    if (this->basic.has_value()) {
        const auto basic_value = this->basic.value();
        if (basic_value == 0) {
            this->write_rst_file = false;
        }
        else if ((basic_value == 1) || (basic_value == 2)) {
            this->write_rst_file = true;
        }
        else {
            this->write_rst_file = {};
        }
    }
}

} // namespace Opm
