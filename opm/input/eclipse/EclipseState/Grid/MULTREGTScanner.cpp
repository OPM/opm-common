/*
  Copyright 2014--2023 Equinor ASA.

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
        : gridDims { gridDims_arg }
        , fp       { fp_arg }
    {
        for (const auto* keywordRecordPtr : keywords) {
            this->addKeyword(*keywordRecordPtr);
        }

        this->template fillSearchMap<0>(m_records);
        this->template fillSearchMap<1>(m_records_same);
    }

    template<int index>
    void MULTREGTScanner::fillSearchMap(const std::vector<MULTREGTRecord>& records) {
        MULTREGTSearchMap searchPairs;
        for (auto recordIx = 0*records.size(); recordIx < records.size(); ++recordIx) {
            const auto& record = records[recordIx];
            const std::string& region_name = record.region_name;
            if (this->fp->has_int(region_name)) {
                const auto srcRegion    = record.src_value;
                const auto targetRegion = record.target_value;

                // The MULTREGT keyword is directionally independent meaning
                // we add both directions, symmetrically, to the lookup
                // table.
                std::pair<int,int> pair = (srcRegion <= targetRegion) ?
                    std::make_pair(srcRegion, targetRegion) : std::make_pair(targetRegion, srcRegion);
                searchPairs[pair] = recordIx;
            }
            else {
                throw std::logic_error {
                    "MULTREGT record is based on region: "
                    +  region_name
                    + " which is not in the deck"
                };
            }

            this->regions.try_emplace(region_name, this->fp->get_global_int(region_name));
        }

        for (const auto& [regPair, recordIx] : searchPairs) {
            const std::string& keyword = records[recordIx].region_name;

            std::get<index>(this->m_searchMap[keyword])[regPair] = recordIx;
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
        result.m_records_same = {{4, 4, 4.0, 3, MULTREGT::NNCBehaviourEnum::NONNC, "test2"}};
        result.m_searchMap["MULTNUM"][0].emplace(std::piecewise_construct,
                                                 std::forward_as_tuple(std::make_pair(1, 2)),
                                                 std::forward_as_tuple(0));
        result.m_searchMap["MULTNUM"][1].emplace(std::piecewise_construct,
                                                 std::forward_as_tuple(std::make_pair(2, 2)),
                                                 std::forward_as_tuple(0));
        result.regions = {{"test3", {11}}};
        result.aquifer_cells = { std::size_t{17}, std::size_t{29} };

        return result;
    }

    bool MULTREGTScanner::operator==(const MULTREGTScanner& data) const
    {
        return (this->gridDims == data.gridDims)
            && (this->m_records == data.m_records)
            && (this->m_records_same == data.m_records_same)
            && (this->m_searchMap == data.m_searchMap)
            && (this->regions == data.regions)
            && (this->aquifer_cells == data.aquifer_cells)
            ;
    }

    MULTREGTScanner& MULTREGTScanner::operator=(const MULTREGTScanner& data)
    {
        this->gridDims = data.gridDims;
        this->fp = data.fp;

        this->m_records = data.m_records;
        this->m_records_same = data.m_records_same;
        this->m_searchMap = data.m_searchMap;
        this->regions = data.regions;
        this->aquifer_cells = data.aquifer_cells;

        return *this;
    }

    void MULTREGTScanner::applyNumericalAquifer(const std::vector<std::size_t>& aquifer_cells_arg)
    {
        this->aquifer_cells.insert(this->aquifer_cells.end(),
                                   aquifer_cells_arg.begin(),
                                   aquifer_cells_arg.end());

        std::sort(this->aquifer_cells.begin(), this->aquifer_cells.end());
        this->aquifer_cells.erase(std::unique(this->aquifer_cells.begin(),
                                              this->aquifer_cells.end()),
                                  this->aquifer_cells.end());
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
        // If multiple records, from different region sets and region
        // IDs--e.g., both regions 1/2 in 'M' (MULTNUM) and regions 2/3 in
        // 'F' (FLUXNUM) apply to the same connection--then the total
        // multiplier value is the product of the values from each record.
        auto multiplier = 1.0;

        if (this->m_searchMap.empty()) {
            return multiplier;
        }

        auto regPairFoundDifferent = [faceDir, this](const auto& regMap, const auto& regPairPos)
        {
            return (regPairPos != regMap.end())
                && ((this->m_records[regPairPos->second].directions & faceDir) != 0);
        };

        auto regPairFoundSame = [faceDir, this](const auto& regMap, const auto& regPairPos)
        {
            return (regPairPos != regMap.end())
                && ((this->m_records_same[regPairPos->second].directions & faceDir) != 0);
        };

        auto ignoreMultiplierRecord =
            [is_adj = is_adjacent(this->gridDims, globalIndex1, globalIndex2),
             is_aqu = this->isAquNNC(globalIndex1, globalIndex2)]
            (const MULTREGT::NNCBehaviourEnum nnc_behaviour)
        {
            // We ignore the record if either of the following conditions hold
            //
            //   1. Cells are adjacent, but record stipulates NNCs only
            //   2. Connection is an NNC, but record stipulates no NNCs
            //   3. Connection is associated to a numerical aquifer, but
            //      record stipulates that no such connections apply.
            return ((is_adj && !is_aqu) && (nnc_behaviour == MULTREGT::NNCBehaviourEnum::NNC))
                || ((!is_adj || is_aqu) && (nnc_behaviour == MULTREGT::NNCBehaviourEnum::NONNC))
                || (is_aqu              && (nnc_behaviour == MULTREGT::NNCBehaviourEnum::NOAQUNNC));
        };


        for (const auto& [regName, regMaps] : this->m_searchMap) {
            const auto& region_data = this->regions.at(regName);

            auto regionId1 = region_data[globalIndex1];
            auto regionId2 = region_data[globalIndex2];

            if (regionId1 > regionId2) {
                std::swap(regionId1, regionId2);
            }

            const auto applyMultiplier = [ignoreMultiplierRecord](const MULTREGTRecord& record)
            {
                return (record.nnc_behaviour == MULTREGT::NNCBehaviourEnum::ALL) ||
                ! ignoreMultiplierRecord(record.nnc_behaviour);
            };

            multiplier = this->applyMultiplierDifferentRegion(regMaps,
                                                                       multiplier,
                                                                       regionId1,
                                                                       regionId2,
                                                                       applyMultiplier,
                                                                       regPairFoundDifferent);
            // same region. Note that a pair where both region indices are the same is special.
            // For connections between it and all other regions the multipliers
            // will not override otherwise explicitly specified (as pairs with
            // different ids) multipliers, but accumulated to these.
            multiplier = this->applyMultiplierSameRegion(regMaps,
                                                                  multiplier,
                                                                  regionId1,
                                                                  regionId2,
                                                                  applyMultiplier,
                                                                  regPairFoundSame);
        }

        return multiplier;
    }

    double MULTREGTScanner::getRegionMultiplierNNC(const std::size_t globalCellIdx1,
                                                   const std::size_t globalCellIdx2) const
    {
        // If multiple records, from different region sets and region
        // IDs--e.g., both regions 1/2 in 'M' (MULTNUM) and regions 2/3 in
        // 'F' (FLUXNUM) apply to the same connection--then the total
        // multiplier value is the product of the values from each record.
        auto multiplier = 1.0;

        if (this->m_searchMap.empty()) {
            return multiplier;
        }

        auto ignoreMultiplierRecord =
            [is_aqu = this->isAquNNC(globalCellIdx1, globalCellIdx2)]
            (const MULTREGT::NNCBehaviourEnum nnc_behaviour)
        {
            return (nnc_behaviour == MULTREGT::NNCBehaviourEnum::NONNC)
                || (is_aqu && (nnc_behaviour == MULTREGT::NNCBehaviourEnum::NOAQUNNC));
        };

        for (const auto& [regName, regMaps] : this->m_searchMap) {
            const auto& region_data = this->regions.at(regName);

            auto regionId1 = region_data[globalCellIdx1];
            auto regionId2 = region_data[globalCellIdx2];

            if (regionId1> regionId2) {
                std::swap(regionId1, regionId2);
            }

            const auto applyMultiplier = [ignoreMultiplierRecord](const auto& record){
                return ! ignoreMultiplierRecord(record.nnc_behaviour);
            };

            const auto regPairFound = [](const auto& regMap, const auto& regPairPos)
            {
                // all entries match no matter what FaceDir says.
                return (regPairPos != regMap.end());
            };

            multiplier = this->applyMultiplierSameRegion(regMaps,
                                                                  multiplier,
                                                                  regionId1,
                                                                  regionId2,
                                                                  applyMultiplier,
                                                                  regPairFound);
            // same region. Note that a pair where both region indices are the same is special.
            // For connections between it and all other regions the multipliers
            // will not override otherwise explicitly specified (as pairs with
            // different ids) multipliers, but accumulated to these.
            multiplier = this->applyMultiplierDifferentRegion(regMaps,
                                                                       multiplier,
                                                                       regionId1,
                                                                       regionId2,
                                                                       applyMultiplier,
                                                                       regPairFound);
        }

        return multiplier;
    }

    template<typename ApplyDecision, typename RegPairFound>
    double MULTREGTScanner::applyMultiplierDifferentRegion(const std::array<MULTREGTSearchMap,2>& regMaps,
                                                           double multiplier,
                                                           std::size_t regionId1,
                                                           std::size_t regionId2,
                                                           const ApplyDecision& applyMultiplier,
                                                           const RegPairFound& regPairFound) const
    {
        const auto& myMap = std::get<0>(regMaps);
        decltype(myMap.begin()) regPairPos;

        regPairPos = myMap.find({ regionId1, regionId2 });

        if (!regPairFound(myMap, regPairPos)) {
            // Pair not found.
            return multiplier;
        }
        const auto& record = this->m_records[regPairPos->second];

        if (applyMultiplier(record)) {
            multiplier *= record.trans_mult;
        }

        return multiplier;
    }


    template<typename ApplyDecision, typename RegPairFound>
    double MULTREGTScanner::applyMultiplierSameRegion(const std::array<MULTREGTSearchMap,2>& regMaps,
                                                      double multiplier,
                                                      std::size_t regionId1,
                                                      std::size_t regionId2,
                                                      const ApplyDecision& applyMultiplier,
                                                      const RegPairFound& regPairFound) const
    {
        const auto& myMap = std::get<1>(regMaps);
        decltype(myMap.begin()) regPairPos;

        // search for entry where the two region ids are the same
        // where one of those is a region of ours.
        regPairPos = myMap.find({ regionId1, regionId1 });

        if (regPairFound(myMap, regPairPos)) {
            const auto& record = this->m_records_same[regPairPos->second];

            if (applyMultiplier(record)) {
                multiplier *= record.trans_mult;
            }
        }
        if (regionId1 != regionId2)
        {
            // also try to apply other region multiplier.
            regPairPos = myMap.find({ regionId2, regionId2 });

            if (regPairFound(myMap, regPairPos)) {
                const auto& record = this->m_records_same[regPairPos->second];

                if (applyMultiplier(record)) {
                    multiplier *= record.trans_mult;
                }
            }
        }

        return multiplier;
    }

    void MULTREGTScanner::addKeyword(const DeckKeyword& deckKeyword)
    {
        using Kw = ParserKeywords::MULTREGT;

        for (const auto& deckRecord : deckKeyword) {
            std::vector<int> src_regions;
            std::vector<int> target_regions;

            const auto& srcItem = deckRecord.getItem<Kw::SRC_REGION>();
            const auto& targetItem = deckRecord.getItem<Kw::TARGET_REGION>();
            const auto& regionItem = deckRecord.getItem<Kw::REGION_DEF>();

            const auto trans_mult = deckRecord.getItem<Kw::TRAN_MULT>().get<double>(0);
            const auto directions = FaceDir::FromMULTREGTString(deckRecord.getItem<Kw::DIRECTIONS>().get<std::string>(0));
            const auto nnc_behaviour = MULTREGT::NNCBehaviourFromString(deckRecord.getItem<Kw::NNC_MULT>().get<std::string>(0));

            const auto region_name = (!this->m_records.empty() && regionItem.defaultApplied(0))
                ? this->m_records.back().region_name
                : MULTREGT::RegionNameFromDeckValue(regionItem.get<std::string>(0));

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

            if (target_regions.size() == 1 && src_regions.size() == 1 &&
                src_regions[0] == target_regions[0]) {
                // MULTREGT with same source and target region.
                // Use connections to self and all other regions
                m_records_same.push_back({src_regions[0], src_regions[0], trans_mult, directions, nnc_behaviour, region_name});
            }
            else {
                for (int src_region : src_regions) {
                    for (int target_region : target_regions) {
                        if (src_region <= target_region) {
                            // same region should not happen for defaulted regions
                            if (src_region != target_region) {
                                m_records.push_back({src_region, target_region, trans_mult, directions, nnc_behaviour, region_name});
                            }
                        }
                        else {
                            m_records.push_back({target_region, src_region, trans_mult, directions, nnc_behaviour, region_name});
                        }
                    }
                }
            }
        }
    }

    bool MULTREGTScanner::isAquNNC(const std::size_t globalCellIdx1,
                                   const std::size_t globalCellIdx2) const
    {
        return this->isAquCell(globalCellIdx1)
            || this->isAquCell(globalCellIdx2);
    }

    bool MULTREGTScanner::isAquCell(const std::size_t globalCellIdx) const
    {
        return std::binary_search(this->aquifer_cells.begin(),
                                  this->aquifer_cells.end(),
                                  globalCellIdx);
    }
} // namespace Opm
