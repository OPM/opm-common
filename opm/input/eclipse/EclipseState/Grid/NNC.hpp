/*
  Copyright 2015 IRIS

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

#ifndef OPM_PARSER_NNC_HPP
#define OPM_PARSER_NNC_HPP

#include <cstddef>
#include <optional>
#include <tuple>
#include <map>
#include <vector>

#include <opm/common/OpmLog/KeywordLocation.hpp>

namespace Opm
{

class GridDims;

struct NNCdata {
    NNCdata(size_t c1, size_t c2, double t)
        : cell1(c1), cell2(c2), trans(t)
    {}
    NNCdata() = default;

    bool operator==(const NNCdata& data) const
    {
        return cell1 == data.cell1 &&
               cell2 == data.cell2 &&
               trans == data.trans;
    }

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(cell1);
        serializer(cell2);
        serializer(trans);
    }

    // Observe that the operator< is only for cell ordering and does not consider the
    // trans member
    bool operator<(const NNCdata& other) const
    {
        return std::tie(this->cell1, this->cell2) < std::tie(other.cell1, other.cell2);
    }

    size_t cell1{};
    size_t cell2{};
    double trans{};
};



class Deck;
class EclipseGrid;

/*
  This class is an internalization of the NNC and EDITNNC keywords. Because the
  opm-common codebase does not itself manage the simulation grid the purpose of
  the NNC class is mainly to hold on to the NNC/EDITNNC input and pass it on to
  the grid construction proper.

  The EDITNNC keywords can operate on two different types of NNCs.

    1. NNCs which have been explicitly entered using the NNC keyword.
    2. NNCs which are inderectly inferred from the grid - e.g. due to faults.

  When processing the EDITNNC keyword the class will search through the NNCs
  configured explicitly with the NNC keyword and apply the edit transformation
  on those NNCs, EDITNNCs which affect NNCs which are not configured explicitly
  are stored for later use by the simulator.

  The class guarantees the following ordering:

    1. For all NNC / EDITNNC records we will have cell1 <= cell2
    2. The vectors NNC::input() and NNC::edit() will be ordered in ascending
       order.

  While constructing from a deck NNCs connected to inactive cells will be
  silently ignored. Do observe though that the addNNC() function does not check
  the arguments and alas there is no guarantee that only active cells are
  involved.
*/

class NNC
{
public:
    NNC() = default;
    /// Construct from input deck.
    NNC(const EclipseGrid& grid, const Deck& deck);

    static NNC serializationTestObject();

    bool addNNC(const size_t cell1, const size_t cell2, const double trans);

    /// \brief Merge additional NNCs into sorted NNCs
    void merge(const std::vector<NNCdata>& nncs);
    /// \brief Get the combined information from NNC
    const std::vector<NNCdata>& input() const { return m_input; }
    /// \brief Get the information from EDITNNC keyword
    const std::vector<NNCdata>& edit() const { return m_edit; }
    /// \brief Get the information from EDITNNCR keyword
    const std::vector<NNCdata>& editr() const { return m_editr; }
    KeywordLocation input_location(const NNCdata& nnc) const;
    KeywordLocation edit_location(const NNCdata& nnc) const;
    KeywordLocation editr_location(const NNCdata& nnc) const;


    bool operator==(const NNC& data) const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_input);
        serializer(m_edit);
        serializer(m_editr);
        serializer(m_nnc_location);
        serializer(m_edit_location);
        serializer(m_editr_location);
    }

private:

    void load_input(const EclipseGrid& grid, const Deck& deck);
    void load_edit(const EclipseGrid& grid, const Deck& deck);
    void load_editr(const EclipseGrid& grid, const Deck& deck);
    void add_edit(const NNCdata& edit_node);

    /// \brief Stores NNC not coinciding with entries in EDITNNCR
    std::vector<NNCdata> m_input;
    /// \brief EDITNNC data not coinciding with entries in EDITNNCR.
    std::vector<NNCdata> m_edit;
    /// \brief EDITNNCR data.
    std::vector<NNCdata> m_editr;
    std::optional<KeywordLocation> m_nnc_location;
    std::optional<KeywordLocation> m_edit_location;
    std::optional<KeywordLocation> m_editr_location;
};


class NNCCollection
{
public:
    NNCCollection() = default;
    explicit NNCCollection(NNC nnc_global);

    // ---- insertion --------------------------------------------------------

    /// Add a cross-grid NNC between grid1 and grid2.
    void addNNC(std::size_t grid1, std::size_t grid2, NNC nnc);

    /// Add a same-grid NNC for the given grid index.
    void addNNC(std::size_t grid, NNC nnc);

    /// Add the global (main-grid) NNC.  Equivalent to addNNC(0, nnc).
    void addNNC(NNC nnc);

    // ---- cross-grid access ------------------------------------------------

    const NNC& getNNC(std::size_t grid1, std::size_t grid2) const;
    NNC&       getNNC(std::size_t grid1, std::size_t grid2);

    bool hasCrossGridNNC(std::size_t grid1, std::size_t grid2) const;

    // ---- same-grid access -------------------------------------------------

    const NNC& getNNC(std::size_t grid) const;
    NNC&       getNNC(std::size_t grid);

    bool hasSameGridNNC(std::size_t grid) const;

    // ---- global (grid 0) access -------------------------------------------

    const NNC& getGlobalNNC() const;
    NNC&       getGlobalNNC();

    bool hasGlobalNNC() const { return hasSameGridNNC(0); };


    /// Returns a view of all same-grid NNCs as (grid_index, NNC) pairs.
    const std::map<std::size_t, NNC>& same_grid_nnc() const
    {
        return m_sameGridNNCs;
    }

    /// Returns a view of all cross-grid NNCs keyed by normalised (g1,g2) pairs.
    const std::map<std::pair<std::size_t,std::size_t>, NNC>& diff_grid_nnc() const
    {
        return m_diffGridNNCs;
    }

    bool operator==(const NNCCollection& other) const
    {
        return m_sameGridNNCs == other.m_sameGridNNCs &&
               m_diffGridNNCs == other.m_diffGridNNCs;
    }

private:

    /// Same-grid NNCs, keyed by grid index (0 == global/main grid).
    std::map<std::size_t, NNC> m_sameGridNNCs;

    /// Cross-grid NNCs, keyed by normalised (min_grid, max_grid) pair.
    std::map<std::pair<std::size_t,std::size_t>, NNC> m_diffGridNNCs;
};

} // namespace Opm


#endif // OPM_PARSER_NNC_HPP
