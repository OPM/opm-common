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

#ifndef OPM_PARSER_MULTREGTSCANNER_HPP
#define OPM_PARSER_MULTREGTSCANNER_HPP

#include <opm/input/eclipse/EclipseState/Grid/FaceDir.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

#include <cstddef>
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

        double getRegionMultiplier(std::size_t globalCellIdx1,
                                   std::size_t globalCellIdx2,
                                   FaceDir::DirEnum faceDir) const;


        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(gridDims);

            serializer(m_records);
            serializer(m_searchMap);

            serializer(regions);
        }

    private:
        using MULTREGTSearchMap = std::map<
            std::pair<int, int>,
            std::vector<MULTREGTRecord>::size_type
        >;

        GridDims gridDims{};
        const FieldPropsManager* fp{nullptr};

        std::vector<MULTREGTRecord> m_records{};
        std::map<std::string, MULTREGTSearchMap> m_searchMap{};
        std::map<std::string, std::vector<int>> regions{};

        void addKeyword(const DeckKeyword& deckKeyword);
        void assertKeywordSupported(const DeckKeyword& deckKeyword);
    };

} // namespace Opm

#endif // OPM_PARSER_MULTREGTSCANNER_HPP
