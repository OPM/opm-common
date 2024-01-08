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

#ifndef OPM_PARSER_MULTREGTSCANNER_HPP
#define OPM_PARSER_MULTREGTSCANNER_HPP

#include <opm/input/eclipse/EclipseState/Grid/FaceDir.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Opm {

    class DeckRecord;
    class DeckKeyword;
    class FieldPropsManager;

} // namespace Opm

namespace Opm {

    namespace MULTREGT {
        enum class NNCBehaviourEnum
        {
            NNC = 1,
            NONNC = 2,
            ALL = 3,
            NOAQUNNC = 4
        };

        std::string RegionNameFromDeckValue(const std::string& stringValue);
        NNCBehaviourEnum NNCBehaviourFromString(const std::string& stringValue);
    } // namespace MULTREGT

    struct MULTREGTRecord
    {
        int src_value;
        int target_value;
        double trans_mult;
        int directions;
        MULTREGT::NNCBehaviourEnum nnc_behaviour;
        std::string region_name;

        bool operator==(const MULTREGTRecord& data) const
        {
            return (src_value == data.src_value)
                && (target_value == data.target_value)
                && (trans_mult == data.trans_mult)
                && (directions == data.directions)
                && (nnc_behaviour == data.nnc_behaviour)
                && (region_name == data.region_name)
                ;
        }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(src_value);
            serializer(target_value);
            serializer(trans_mult);
            serializer(directions);
            serializer(nnc_behaviour);
            serializer(region_name);
        }
    };

    class MULTREGTScanner
    {
    public:
        MULTREGTScanner() = default;
        MULTREGTScanner(const MULTREGTScanner& data);
        MULTREGTScanner(const GridDims& grid,
                        const FieldPropsManager* fp_arg,
                        const std::vector<const DeckKeyword*>& keywords);

        static MULTREGTScanner serializationTestObject();

        bool operator==(const MULTREGTScanner& data) const;
        MULTREGTScanner& operator=(const MULTREGTScanner& data);

        void applyNumericalAquifer(const std::vector<std::size_t>& aquifer_cells);

        double getRegionMultiplier(std::size_t globalCellIdx1,
                                   std::size_t globalCellIdx2,
                                   FaceDir::DirEnum faceDir) const;

        double getRegionMultiplierNNC(std::size_t globalCellIdx1,
                                      std::size_t globalCellIdx2) const;

        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(gridDims);

            serializer(m_records);
            serializer(m_records_same);
            serializer(m_searchMap);

            serializer(regions);
            serializer(aquifer_cells);
        }

    private:

        // For any key k in the map k.first <= k.second holds.
        using MULTREGTSearchMap = std::map<
            std::pair<int, int>,
            std::vector<MULTREGTRecord>::size_type
        >;

        /// \brief Apply regionMultiplier from entries where source and target region differ
        ///
        /// \param regMaps the maps for the region name (FLUXNUM or else)
        ///        first entry is between regions with different indices
        ///        second entry is between regions withe the same index
        /// \param regionId1 Id of egion for first cell
        /// \param regionId Id of regions for the second cell (not less than regionId1!)
        /// \param applyMultiplier Functor returning true if multiplier should be applied
        /// \param regPairFound Functor to check whether there is a entry for region pair.
        template<typename ApplyDecision, typename RegPairFound>
        double applyMultiplierDifferentRegion(const std::array<MULTREGTSearchMap,2>& regMaps,
                                              double multiplier,
                                              std::size_t regionId1,
                                              std::size_t regionId2,
                                              const ApplyDecision& applyMultiplier,
                                              const RegPairFound& regPairFound) const;

        /// \brief Apply region multipliers from entries with same source and target region
        ///
        /// Note that a pair where both region indices are the same is special.
        /// For connections between it and all other regions the multipliers
        /// will not override otherwise explicitly specified (as pairs with
        /// different ids) multipliers, but accumulated to these.
        /// \param regMaps the maps for the region name (FLUXNUM or else)
        ///        first entry is between regions with different indices
        ///        second entry is between regions withe the same index
        /// \param regionId1 Id of egion for first cell
        /// \param regionId Id of regions for the second cell (not less than regionId1!)
        /// \param applyMultiplier Functor returning true if multiplier should be applied
        /// \param regPairFound Functor to check whether there is a entry for region pair.
        template<typename ApplyDecision, typename RegPairFound>
        double applyMultiplierSameRegion(const std::array<MULTREGTSearchMap,2>& regMaps,
                                         double multiplier,
                                         std::size_t regionId1,
                                         std::size_t regionId2,
                                         const ApplyDecision& applyMultiplier,
                                         const RegPairFound& regPairFound) const;
        template<int index>
        void fillSearchMap(const std::vector<MULTREGTRecord>& records);

        GridDims gridDims{};
        const FieldPropsManager* fp{nullptr};

        // For any record stored index of source region is less than
        // target region.
        std::vector<MULTREGTRecord> m_records{};
        /// \brief Records for special case where source and target region id is the same
        ///
        /// The multiplier is applied to conections in the same regio as well as to connection
        /// between the regions if this id and any other region.
        /// Note that this will not override any entries from m_records, but multiplier will be
        /// applied cumulatively.
        std::vector<MULTREGTRecord> m_records_same{};
        std::map<std::string, std::array<MULTREGTSearchMap,2>> m_searchMap{};
        std::map<std::string, std::vector<int>> regions{};
        std::vector<std::size_t> aquifer_cells{};

        void addKeyword(const DeckKeyword& deckKeyword);

        bool isAquNNC(std::size_t globalCellIdx1, std::size_t globalCellIdx2) const;
        bool isAquCell(std::size_t globalCellIdx) const;
    };

} // namespace Opm

#endif // OPM_PARSER_MULTREGTSCANNER_HPP
