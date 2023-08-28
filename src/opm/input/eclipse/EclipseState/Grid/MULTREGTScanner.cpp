/*
  Copyright 2014 Statoil ASA.

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

#include <opm/input/eclipse/EclipseState/Grid/MULTREGTScanner.hpp>

#include <opm/input/eclipse/EclipseState/Grid/FaceDir.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/M.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

std::vector<int> unique(std::vector<int> data)
{
    std::sort(data.begin(), data.end());
    data.erase(std::unique(data.begin(), data.end()), data.end());

    return data;
}

bool is_adjacent(const int x, const int y)
{
    assert ((x >= 0) && (y >= 0));

    return std::abs(x - y) == 1;
}

bool is_adjacent(const std::array<int, 3>& ijk1,
                 const std::array<int, 3>& ijk2,
                 const std::array<int, 3>& compIx)
{
    return is_adjacent(ijk1[compIx[0]], ijk2[compIx[0]])
        && (ijk1[compIx[1]] == ijk2[compIx[1]])
        && (ijk1[compIx[2]] == ijk2[compIx[2]]);
}

bool is_adjacent(const Opm::GridDims& gridDims,
                 const std::size_t    gi1,
                 const std::size_t    gi2)
{
    const auto ijk1 = gridDims.getIJK(gi1);
    const auto ijk2 = gridDims.getIJK(gi2);

    return is_adjacent(ijk1, ijk2, {0, 1, 2})  // (I,J,K) <-> (I+1,J,K)
        || is_adjacent(ijk1, ijk2, {1, 2, 0})  // (I,J,K) <-> (I,J+1,K)
        || is_adjacent(ijk1, ijk2, {2, 0, 1}); // (I,J,K) <-> (I,J,K+1)
}

} // Anonymous namespace

namespace Opm {

    namespace MULTREGT {

        std::string RegionNameFromDeckValue(const std::string& stringValue)
        {
            if (stringValue == "O") { return "OPERNUM"; }
            if (stringValue == "F") { return "FLUXNUM"; }
            if (stringValue == "M") { return "MULTNUM"; }

            throw std::invalid_argument {
                "The input string: " + stringValue + " was invalid. Expected: O/F/M"
            };
        }

        NNCBehaviourEnum NNCBehaviourFromString(const std::string& stringValue)
        {
            if (stringValue == "ALL")      { return NNCBehaviourEnum::ALL; }
            if (stringValue == "NNC")      { return NNCBehaviourEnum::NNC; }
            if (stringValue == "NONNC")    { return NNCBehaviourEnum::NONNC; }
            if (stringValue == "NOAQUNNC") { return NNCBehaviourEnum::NOAQUNNC; }

            throw std::invalid_argument {
                "The input string: " + stringValue + " was invalid. Expected: ALL/NNC/NONNC/NOAQUNNC"
            };
        }

    } // namespace MULTREGT

    // -----------------------------------------------------------------------

    // Later records with the same region IDs overwrite earlier.  As an
    // example, in the MULTREGT keyword
    //
    //   MULTREGT
    //     2  4   0.75    Z   ALL    M /
    //     2  4   2.50   XY   ALL    F /
    //   /
    //
    // the second record will overwrite the first.  We enforce this
    // behaviour through maps keyed on 'pair<region1,region2>'.
    //
    // This function starts with some initial preprocessing to create a map
    // which looks like this:
    //
    //    searchMap = {
    //       "MULTNUM" : {
    //           std::pair(1,2) : recordIx,
    //           std::pair(4,7) : recordIx,
    //           ...
    //       },
    //       "FLUXNUM" : {
    //           std::pair(4,8) : recordIx,
    //           std::pair(1,4) : recordIx,
    //           ...
    //       },
    //       ...
    //    }
    //
    // Then it will go through the different regions and look for interfaces
    // with the wanted region values.

    MULTREGTScanner::MULTREGTScanner(const GridDims&                        gridDims_arg,
                                     const FieldPropsManager*               fp_arg,
                                     const std::vector<const DeckKeyword*>& keywords)
        : gridDims       { gridDims_arg }
        , fp             { fp_arg }
        , default_region { this->fp->default_region() }
    {
        for (const auto* keywordRecordPtr : keywords) {
            this->addKeyword(*keywordRecordPtr);
        }

        MULTREGTSearchMap searchPairs;
        for (auto recordIx = 0*this->m_records.size(); recordIx < this->m_records.size(); ++recordIx) {
            const auto& record = this->m_records[recordIx];
            const std::string& region_name = record.region_name;
            if (this->fp->has_int(region_name)) {
                const auto srcRegion    = record.src_value;
                const auto targetRegion = record.target_value;

                // The MULTREGT keyword is directionally independent meaning
                // we add both directions, symmetrically, to the lookup
                // table.
                if (srcRegion != targetRegion) {
                    std::pair<int,int> pair1{ srcRegion, targetRegion };
                    std::pair<int,int> pair2{ targetRegion, srcRegion };
                    searchPairs[pair1] = recordIx;
                    searchPairs[pair2] = recordIx;
                }
            }
            else {
                throw std::logic_error {
                    "MULTREGT record is based on region: "
                    +  region_name
                    + " which is not in the deck"
                };
            }

            if (this->regions.count(region_name) == 0) {
                this->regions[region_name] = this->fp->get_global_int(region_name);
            }
        }

        for (const auto& [regPair, recordIx] : searchPairs) {
            const std::string& keyword = this->m_records[recordIx].region_name;

            this->m_searchMap[keyword][regPair] = recordIx;
        }
    }

    MULTREGTScanner::MULTREGTScanner(const MULTREGTScanner& rhs)
    {
        *this = rhs;
    }

    MULTREGTScanner MULTREGTScanner::serializationTestObject()
    {
        MULTREGTScanner result;

        result.gridDims = GridDims::serializationTestObject();
        result.m_records = {{4, 5, 6.0, 7, MULTREGT::NNCBehaviourEnum::ALL, "test1"}};
        result.m_searchMap["MULTNUM"].emplace(std::piecewise_construct,
                                              std::forward_as_tuple(std::make_pair(1, 2)),
                                              std::forward_as_tuple(0));
        result.regions = {{"test3", {11}}};
        result.default_region = "test4";

        return result;
    }

    bool MULTREGTScanner::operator==(const MULTREGTScanner& data) const
    {
        return (this->gridDims == data.gridDims)
            && (this->default_region == data.default_region)
            && (this->m_records == data.m_records)
            && (this->m_searchMap == data.m_searchMap)
            && (this->regions == data.regions)
            ;
    }

    MULTREGTScanner& MULTREGTScanner::operator=(const MULTREGTScanner& data)
    {
        this->gridDims = data.gridDims;
        this->fp = data.fp;
        this->default_region = data.default_region;

        this->m_records = data.m_records;
        this->m_searchMap = data.m_searchMap;
        this->regions = data.regions;

        return *this;
    }

    // This function will check the region values in globalIndex1 and
    // globalIndex and see if they match the regionvalues specified in the
    // deck.  The function checks both directions:
    //
    // Assume the relevant MULTREGT record looks like:
    //
    //    1  2   0.10  XYZ  ALL M /
    //
    // I.e., we are checking for the boundary between regions 1 and 2.  We
    // assign the transmissibility multiplier to the correct face of the
    // cell with value 1:
    //
    //    -----------
    //    | 1  | 2  |   =>  MultTrans( i,j,k,FaceDir::XPlus ) *= 0.50
    //    -----------
    //
    //    -----------
    //    | 2  | 1  |   =>  MultTrans( i+1,j,k,FaceDir::XMinus ) *= 0.50
    //    -----------
    double MULTREGTScanner::getRegionMultiplier(const std::size_t globalIndex1,
                                                const std::size_t globalIndex2,
                                                const FaceDir::DirEnum faceDir) const
    {
        if (this->m_searchMap.empty()) {
            return 1.0;
        }

        auto regPairFound = [faceDir, this](const auto& regMap, auto regPairPos)
        {
            return (regPairPos != regMap.end())
                && ((this->m_records[regPairPos->second].directions & faceDir) != 0);
        };

        auto ignoreMultiplierRecord =
            [is_adj = is_adjacent(this->gridDims, globalIndex1, globalIndex2)]
            (const MULTREGT::NNCBehaviourEnum nnc_behaviour)
        {
            // We ignore the record if either of the following conditions hold
            //
            //   1. Cells are adjacent, but record stipulates NNCs only
            //   2. Connection is an NNC, but record stipulates no NNCs
            return ( is_adj && (nnc_behaviour == MULTREGT::NNCBehaviourEnum::NNC))
                || (!is_adj && (nnc_behaviour == MULTREGT::NNCBehaviourEnum::NONNC));
        };

        for (const auto& [regName, regMap] : this->m_searchMap) {
            const auto& region_data = this->regions.at(regName);

            const int regionId1 = region_data[globalIndex1];
            const int regionId2 = region_data[globalIndex2];

            auto regPairPos = regMap.find({ regionId1, regionId2 });
            if ((regionId1 != regionId2) && !regPairFound(regMap, regPairPos)) {
                // 1 -> 2 not found.  Try reverse direction.
                regPairPos = regMap.find({ regionId2, regionId1 });
            }

            if (! regPairFound(regMap, regPairPos)) {
                // Neither 1->2 nor 2->1 found.  Move on to next region set.
                continue;
            }

            const auto& record = this->m_records[regPairPos->second];
            const auto applyMultiplier =
                (record.nnc_behaviour == MULTREGT::NNCBehaviourEnum::ALL) ||
                ! ignoreMultiplierRecord(record.nnc_behaviour);

            if (applyMultiplier) {
                return record.trans_mult;
            }
        }

        return 1.0;
    }

    void MULTREGTScanner::assertKeywordSupported(const DeckKeyword& deckKeyword)
    {
        using Kw = ParserKeywords::MULTREGT;

        for (const auto& deckRecord : deckKeyword) {
            const auto& srcItem = deckRecord.getItem<Kw::SRC_REGION>();
            const auto& targetItem = deckRecord.getItem<Kw::TARGET_REGION>();

            if (!srcItem.defaultApplied(0) && !targetItem.defaultApplied(0)) {
                if (srcItem.get<int>(0) == targetItem.get<int>(0)) {
                    throw std::invalid_argument {
                        "Sorry - MULTREGT applied internally to a region is not yet supported"
                    };
                }
            }

            const auto nnc_behaviour = MULTREGT::
                NNCBehaviourFromString(deckRecord.getItem("NNC_MULT").get<std::string>(0));

            if (nnc_behaviour == MULTREGT::NNCBehaviourEnum::NOAQUNNC) {
                throw std::invalid_argument {
                    "Sorry - currently we do not support 'NOAQUNNC' for MULTREGT."
                };
            }
        }
    }

    void MULTREGTScanner::addKeyword(const DeckKeyword& deckKeyword)
    {
        using Kw = ParserKeywords::MULTREGT;

        this->assertKeywordSupported(deckKeyword);

        for (const auto& deckRecord : deckKeyword) {
            std::vector<int> src_regions;
            std::vector<int> target_regions;
            std::string region_name = this->default_region;

            const auto& srcItem = deckRecord.getItem<Kw::SRC_REGION>();
            const auto& targetItem = deckRecord.getItem<Kw::TARGET_REGION>();
            const auto& regionItem = deckRecord.getItem<Kw::REGION_DEF>();

            const auto trans_mult = deckRecord.getItem<Kw::TRAN_MULT>().get<double>(0);
            const auto directions = FaceDir::FromMULTREGTString(deckRecord.getItem<Kw::DIRECTIONS>().get<std::string>(0));
            const auto nnc_behaviour = MULTREGT::NNCBehaviourFromString(deckRecord.getItem<Kw::NNC_MULT>().get<std::string>(0));

            if (regionItem.defaultApplied(0)) {
                if (!m_records.empty())
                    region_name = m_records.back().region_name;
            } else
                region_name = MULTREGT::RegionNameFromDeckValue( regionItem.get<std::string>(0) );

            if (srcItem.defaultApplied(0) || srcItem.get<int>(0) < 0) {
                src_regions = unique(this->fp->get_int(region_name));
            }
            else {
                src_regions.push_back(srcItem.get<int>(0));
            }

            if (targetItem.defaultApplied(0) || targetItem.get<int>(0) < 0) {
                target_regions = unique(fp->get_int(region_name));
            }
            else {
                target_regions.push_back(targetItem.get<int>(0));
            }

            for (int src_region : src_regions) {
                for (int target_region : target_regions) {
                    m_records.push_back({src_region, target_region, trans_mult, directions, nnc_behaviour, region_name});
                }
            }
        }
    }
} // namespace Opm
