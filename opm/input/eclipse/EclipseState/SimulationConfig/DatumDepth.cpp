// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2024 Equinor ASA.

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

#include <opm/input/eclipse/EclipseState/SimulationConfig/DatumDepth.hpp>

#include <opm/common/ErrorMacros.hpp>

#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/D.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/E.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

// ===========================================================================

Opm::DatumDepth::Global::Global(const SOLUTIONSection& soln)
{
    if (soln.hasKeyword<ParserKeywords::DATUM>()) {
        // Keyword DATUM entered in SOLUTION section.  Use global datum
        // depth from there.
        this->depth_ = soln.get<ParserKeywords::DATUM>()
            .back()
            .getRecord(0)
            .getItem<ParserKeywords::DATUM::DEPTH>()
            .getSIDouble(0);
    }
    else {
        // Keyword DATUM not entered in SOLUTION section, but EQUIL exists
        // in the model.  Global datum depth is first equilibration region's
        // datum depth (item 1).

        assert (soln.hasKeyword<ParserKeywords::EQUIL>());

        this->depth_ = soln.get<ParserKeywords::EQUIL>()
            .back()
            .getRecord(0)
            .getItem<ParserKeywords::EQUIL::DATUM_DEPTH>()
            .getSIDouble(0);
    }
}

Opm::DatumDepth::Global
Opm::DatumDepth::Global::serializationTestObject()
{
    auto g = Global{};
    g.depth_ = 1234.56;

    return g;
}

// ---------------------------------------------------------------------------

namespace {
    const std::vector<double>&
    datumDepthVector(const Opm::DeckKeyword& datum)
    {
        return datum.getRecord(0)
            .getItem<Opm::ParserKeywords::DATUMR::data>()
            .getSIDoubleData();
    }

    const std::vector<double>&
    datumRDepthVector(const Opm::SOLUTIONSection& soln)
    {
        return datumDepthVector(soln.get<Opm::ParserKeywords::DATUMR>().back());
    }
}

Opm::DatumDepth::DefaultRegion::DefaultRegion(const SOLUTIONSection& soln)
    : depth_ { datumRDepthVector(soln) }
{}

Opm::DatumDepth::DefaultRegion
Opm::DatumDepth::DefaultRegion::serializationTestObject()
{
    auto d = DefaultRegion{};

    d.depth_ = { 123.45, 678.91 };

    return d;
}

// ---------------------------------------------------------------------------

namespace {

    bool allBlank(std::string_view s)
    {
        return s.empty() ||
            std::all_of(s.begin(), s.end(),
                        [](const unsigned char c)
                        { return std::isspace(c); });
    }

    // Trim initial "FIP" prefix
    std::string_view normaliseRSetName(std::string_view rset)
    {
        return (rset.find("FIP") == std::string_view::size_type{0})
            ? rset.substr(3) : rset;
    }

    /// Bring all DATUMRX record into a single map<>
    std::unordered_map<std::string, std::vector<double>>
    normaliseDatumRX(const Opm::SOLUTIONSection& soln)
    {
        using Kw = Opm::ParserKeywords::DATUMRX;

        auto depth = std::unordered_map<std::string, std::vector<double>>{};

        for (const auto& datumrx : soln.get<Kw>()) {
            for (const auto& record : datumrx) {
                const auto& rsetItem = record.getItem<Kw::REGION_FAMILY>();
                const auto& rset =
                    (rsetItem.defaultApplied(0) ||
                     allBlank(rsetItem.get<std::string>(0)))
                    ? "NUM"     // FIPNUM
                    : std::string { normaliseRSetName(rsetItem.getTrimmedString(0)) };

                depth.insert_or_assign(rset, record.getItem(1).getSIDoubleData());
            }
        }

        return depth;
    }

    /// Internalise DATUMR information as fallback for DATUMRX
    std::vector<double> datumRXDefault(const Opm::SOLUTIONSection& soln)
    {
        const auto datumR_ix = soln.index(Opm::ParserKeywords::DATUMR::keywordName);
        if (! datumR_ix.empty() &&
            (datumR_ix.back() < soln.index(Opm::ParserKeywords::DATUMRX::keywordName).front()))
        {
            return datumDepthVector(soln[datumR_ix.back()]);
        }

        // No fall-back DATUMR vector exists.
        return {};
    }

    /// Build internal representation of DATUMRX information.
    ///
    /// \param[in] soln SOLUTION section information
    ///
    /// \return Internal representation
    ///
    ///   - get<0> Normalised region set names (FIP prefix dropped)
    ///   - get<1> Start pointers for each region set
    ///   - get<2> Linearised datum depths for all regions in all region sets
    ///   - get<3> Fallback per-region datum depths
    std::tuple<std::vector<std::string>,
               std::vector<std::vector<double>::size_type>,
               std::vector<double>,
               std::vector<double>>
    internaliseDatumRX(const Opm::SOLUTIONSection& soln)
    {
        auto rsetNames = std::vector<std::string>{};
        auto rsetStart = std::vector<std::vector<double>::size_type>{};
        auto depth = std::vector<double>{};

        rsetStart.push_back(0);
        for (const auto& [rset, rset_depth] : normaliseDatumRX(soln)) {
            rsetNames.push_back(rset);
            depth.insert(depth.end(), rset_depth.begin(), rset_depth.end());
            rsetStart.push_back(depth.size());
        }

        return {
            std::move(rsetNames),
            std::move(rsetStart),
            std::move(depth),
            datumRXDefault(soln)
        };
    }
}

Opm::DatumDepth::UserDefined::UserDefined(const SOLUTIONSection& soln)
{
    std::tie(this->rsetNames_,
             this->rsetStart_,
             this->depth_,
             this->default_) = internaliseDatumRX(soln);

    /// Build name->index lookup table.  Sort indexes alphabetically by
    /// region set names.
    this->rsetIndex_.resize(this->rsetNames_.size());
    std::iota(this->rsetIndex_.begin(), this->rsetIndex_.end(),
              std::vector<double>::size_type{0});

    std::ranges::sort(this->rsetIndex_,
                      [this](const auto i1, const auto i2)
                      { return this->rsetNames_[i1] < this->rsetNames_[i2]; });
}

Opm::DatumDepth::UserDefined
Opm::DatumDepth::UserDefined::serializationTestObject()
{
    using namespace std::string_literals;

    auto u = UserDefined{};

    u.rsetNames_ = { "NUM"s, "ABC"s, "UNIT"s };
    u.rsetStart_ = { 0, 1, 2, 3, };
    u.depth_ = { 17.29, 2.718, -3.1415, };
    u.default_ = { 355.113, };
    u.rsetIndex_ = { 1, 0, 2, };

    return u;
}

double
Opm::DatumDepth::UserDefined::operator()(std::string_view rset, const int region) const
{
    auto canonicalRSet = normaliseRSetName(rset);

    auto ixPos = std::lower_bound(this->rsetIndex_.begin(), this->rsetIndex_.end(),
                                  canonicalRSet,
                                  [this](const auto i, std::string_view rsName)
                                  {
                                      return this->rsetNames_[i] < rsName;
                                  });

    if ((ixPos == this->rsetIndex_.end()) || (this->rsetNames_[*ixPos] != canonicalRSet)) {
        // Rset not among those explicitly defined in the DATUMRX input.
        // Fall back to the default per-region datum depths from DATUMR.
        return this->depthValue(rset, this->default_.begin(), this->default_.end(), region);
    }

    auto begin = this->depth_.begin() + this->rsetStart_[*ixPos + 0];
    auto end   = this->depth_.begin() + this->rsetStart_[*ixPos + 1];

    return this->depthValue(rset, begin, end, region);
}

double
Opm::DatumDepth::UserDefined::depthValue(std::string_view                           rset,
                                         const std::vector<double>::const_iterator  begin,
                                         const std::vector<double>::const_iterator  end,
                                         const std::vector<double>::difference_type ix) const
{
    if (end == begin) {
        const auto msg =
            fmt::format("Region set {} does not have a "
                        "valid entry in DATUMRX or "
                        "fallback datum depths (DATUMR) "
                        "are not available", rset);

        OPM_THROW_NOLOG(std::invalid_argument, msg);
    }

    return (ix >= std::distance(begin, end))
        ? *(end - 1)            // Regions beyond last get final value
        : *(begin + ix);
}

// ===========================================================================

Opm::DatumDepth::DatumDepth(const SOLUTIONSection& soln)
{
    // Please note that the case order is quite deliberate here; from most
    // specific to least specific.  This is *mostly* because DATUMRX falls
    // back to DATUMR for those region sets which are not explicitly defined
    // in DATUMRX.  We defer that complexity to the UserDefined constructor.

    if (soln.hasKeyword<ParserKeywords::DATUMRX>()) {
        this->datum_.emplace<UserDefined>(soln);
    }
    else if (soln.hasKeyword<ParserKeywords::DATUMR>()) {
        this->datum_.emplace<DefaultRegion>(soln);
    }
    else if (soln.hasKeyword<ParserKeywords::DATUM>() ||
             soln.hasKeyword<ParserKeywords::EQUIL>())
    {
        this->datum_.emplace<Global>(soln);
    }

    // If neither of the above conditions trigger, then the datum depth for
    // depth-corrected pressures (e.g., summary vector BPPO) is zero.  This
    // corresponds to a default constructed first member of this->datum_ so
    // there's no additional initialisation logic needed here.
}

Opm::DatumDepth
Opm::DatumDepth::serializationTestObjectZero()
{
    return {};
}

Opm::DatumDepth
Opm::DatumDepth::serializationTestObjectGlobal()
{
    auto d = DatumDepth{};
    d.datum_ = Global::serializationTestObject();
    return d;
}

Opm::DatumDepth
Opm::DatumDepth::serializationTestObjectDefaultRegion()
{
    auto d = DatumDepth{};
    d.datum_ = DefaultRegion::serializationTestObject();
    return d;
}

Opm::DatumDepth
Opm::DatumDepth::serializationTestObjectUserDefined()
{
    auto d = DatumDepth{};
    d.datum_ = UserDefined::serializationTestObject();
    return d;
}
