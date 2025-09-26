/*
  Copyright 2023 Equinor

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

#define BOOST_TEST_MODULE CSR_Graph_From_Coordinates

#include <boost/test/unit_test.hpp>

#include <opm/common/utility/CSRGraphFromCoordinates.hpp>

#include <cstddef>
#include <stdexcept>

BOOST_AUTO_TEST_SUITE(No_Self_Connections)

BOOST_AUTO_TEST_SUITE(Untracked)

namespace {
    // Vertex = int, TrackCompressedIdx = false, PermitSelfConnections = false
    using CSRGraph = Opm::utility::CSRGraphFromCoordinates<>;
}

BOOST_AUTO_TEST_CASE(Clear_Empty_is_Valid)
{
    auto graph = CSRGraph{};

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{0});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{0});

    graph.clear();

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{0});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{0});
}

BOOST_AUTO_TEST_CASE(Negative_Vertex_ID)
{
    auto graph = CSRGraph{};

    BOOST_CHECK_THROW(graph.addConnection( 0, - 1), std::invalid_argument);
    BOOST_CHECK_THROW(graph.addConnection(-1,  10), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0  0 ]
    //    [ 1  0  1  0 ]
    //    [ 0  1  0  1 ]
    //    [ 0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,     5,  6 ]
    //         JA = [ 1  | 0, 2 | 1, 3 | 2 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{6});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 1, 3, 5, 6 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 1, 0, 2, 1, 3, 2 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Clear)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0  0 ]
    //    [ 1  0  1  0 ]
    //    [ 0  1  0  1 ]
    //    [ 0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,     5,  6 ]
    //         JA = [ 1  | 0, 2 | 1, 3 | 2 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{6});

    graph.clear();

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{0});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{0});

    BOOST_CHECK_MESSAGE(graph.startPointers().empty(),
                        "Start pointer array must be empty in cleared graph");

    BOOST_CHECK_MESSAGE(graph.columnIndices().empty(),
                        "Column index array must be empty in cleared graph");
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Compress_Small)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0  0 ]
    //    [ 1  0  1  0 ]
    //    [ 0  1  0  1 ]
    //    [ 0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,     5,  6 ]
    //         JA = [ 1  | 0, 2 | 1, 3 | 2 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    // Argument 3 is too small.  There are 4 vertices in the graph.
    BOOST_CHECK_THROW(graph.compress(3), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Ignore_Self)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0  0 ]
    //    [ 1  0  1  0 ]
    //    [ 0  1  0  1 ]
    //    [ 0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,     5,  6 ]
    //         JA = [ 1  | 0, 2 | 1, 3 | 2 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i);         // Self connection => dropped
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i + 1); // Self connection => dropped
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{6});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 1, 3, 5, 6 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 1, 0, 2, 1, 3, 2 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0  0 ]
    //    [ 1  0  1  0 ]
    //    [ 0  1  0  1 ]
    //    [ 0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,     5,  6 ]
    //         JA = [ 1  | 0, 2 | 1, 3 | 2 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{6});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 1, 3, 5, 6 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 1, 0, 2, 1, 3, 2 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Repeated)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0  0 ]
    //    [ 1  0  1  0 ]
    //    [ 0  1  0  1 ]
    //    [ 0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,     5,  6 ]
    //         JA = [ 1  | 0, 2 | 1, 3 | 2 ]

    for (auto n = 0; n < 20; ++n) {
        for (auto i = 0; i < 4 - 1; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
            graph.addConnection(i, i + 1);
        }
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{6});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 1, 3, 5, 6 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 1, 0, 2, 1, 3, 2 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |  4  |  5  |  6  |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0  0  0  0  0 ]
    //    [ 1  0  1  0  0  0  0 ]
    //    [ 0  1  0  1  0  0  0 ]
    //    [ 0  0  1  0  0  0  0 ]
    //    [ 0  0  0  1  0  1  0 ]
    //    [ 0  0  0  0  1  0  1 ]
    //    [ 0  0  0  0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,     5,     7,     9,     11, 12 ]
    //         JA = [ 1  | 0, 2 | 1, 3 | 2, 4 | 3, 5 | 4, 6 |  5 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 1, 3, 5, 7, 9, 11, 12, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 1, 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1_Expand)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |  4  |  5  |  6  |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0  0  0  0  0 ]
    //    [ 1  0  1  0  0  0  0 ]
    //    [ 0  1  0  1  0  0  0 ]
    //    [ 0  0  1  0  0  0  0 ]
    //    [ 0  0  0  1  0  1  0 ]
    //    [ 0  0  0  0  1  0  1 ]
    //    [ 0  0  0  0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,     5,     7,     9,     11, 12 ]
    //         JA = [ 1  | 0, 2 | 1, 3 | 2, 4 | 3, 5 | 4, 6 |  5 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4, true);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 1, 3, 5, 7, 9, 11, 12, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 1, 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1_And_Existing)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |  4  |  5  |  6  |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0  0  0  0  0 ]
    //    [ 1  0  1  0  0  0  0 ]
    //    [ 0  1  0  1  0  0  0 ]
    //    [ 0  0  1  0  0  0  0 ]
    //    [ 0  0  0  1  0  1  0 ]
    //    [ 0  0  0  0  1  0  1 ]
    //    [ 0  0  0  0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,     5,     7,     9,     11, 12 ]
    //         JA = [ 1  | 0, 2 | 1, 3 | 2, 4 | 3, 5 | 4, 6 |  5 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(1, 2);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 1, 3, 5, 7, 9, 11, 12, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 1, 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1_And_Existing_Expand)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |  4  |  5  |  6  |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0  0  0  0  0 ]
    //    [ 1  0  1  0  0  0  0 ]
    //    [ 0  1  0  1  0  0  0 ]
    //    [ 0  0  1  0  0  0  0 ]
    //    [ 0  0  0  1  0  1  0 ]
    //    [ 0  0  0  0  1  0  1 ]
    //    [ 0  0  0  0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,     5,     7,     9,     11, 12 ]
    //         JA = [ 1  | 0, 2 | 1, 3 | 2, 4 | 3, 5 | 4, 6 |  5 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4, true);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 17; ++i) {
        graph.addConnection(1, 2);
        graph.addConnection(3, 2);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 1, 3, 5, 7, 9, 11, 12, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 1, 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Untracked

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Tracked)

namespace {
    // Vertex = int, TrackCompressedIdx = true, PermitSelfConnections = false
    using CSRGraph = Opm::utility::CSRGraphFromCoordinates<int, true>;
}

BOOST_AUTO_TEST_CASE(Clear_Empty_is_Valid)
{
    auto graph = CSRGraph{};

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{0});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{0});

    graph.clear();

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{0});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{0});
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric)
{
    auto graph = CSRGraph{};

    // Regular one-pass construction gives
    //   MAP = [ 0, 1, 2, 3, 4, 5 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{6});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector { 0, 1, 2, 3, 4, 5, };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Clear)
{
    auto graph = CSRGraph{};

    // Regular one-pass construction gives
    //   MAP = [ 0, 1, 2, 3, 4, 5 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    graph.clear();

    BOOST_CHECK_MESSAGE(graph.compressedIndexMap().empty(),
                        "Compressed index map must be empty in cleared graph");
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1)
{
    auto graph = CSRGraph{};

    // Recompress without preservation/expansion gives
    //   MAP = [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1_Expand)
{
    auto graph = CSRGraph{};

    // Regular one-pass construction gives
    //   MAP = [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4, true);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1_And_Existing)
{
    auto graph = CSRGraph{};

    // Recompress without preservation/expansion gives
    //   MAP = [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 2, 2, 2, 2, 2, 5, 5, 5 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 5; ++i) {
        graph.addConnection(1, 2);
    }

    for (auto i = 0; i < 3; ++i) {
        graph.addConnection(3, 2);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
            2, 2, 2, 2, 2,      // for i=0..4, 1 -> 2
            5, 5, 5,            // for i=0..2, 3 -> 2
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1_And_Existing_Expand)
{
    auto graph = CSRGraph{};

    // Recompress with preservation/expansion gives
    //   MAP = [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 2, 2, 2, 2, 2 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4, true);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 5; ++i) {
        graph.addConnection(1, 2);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
            2, 2, 2, 2, 2,
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Reverse)
{
    auto graph = CSRGraph{};

    // Regular one-pass construction gives
    //   MAP = [ 4, 5, 2, 3, 0, 1 ]

    for (auto i = 2; i >= 0; --i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{6});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector { 4, 5, 2, 3, 0, 1, };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Reverse_Add_3x1x1)
{
    auto graph = CSRGraph{};

    // Recompress without map preservation/expansion gives
    //   MAP = [ 0, 1, 2, 3, 4, 5, 11, 10, 9, 8, 7, 6 ]

    for (auto i = 2; i >= 0; --i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector { 0, 1, 2, 3, 4, 5, 11, 10, 9, 8, 7, 6, };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Reverse_Add_3x1x1_Expand)
{
    auto graph = CSRGraph{};

    // Recompress with map preservation/expansion gives
    //   MAP = [ 4, 5, 2, 3, 0, 1, 11, 10, 9, 8, 7, 6 ]

    for (auto i = 2; i >= 0; --i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector { 4, 5, 2, 3, 0, 1, 11, 10, 9, 8, 7, 6, };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Reverse_Add_3x1x1_And_Existing)
{
    auto graph = CSRGraph{};

    // Recompress without map preservation/expansion gives
    //   MAP = [ 0, 1, 2, 3, 4, 5, 11, 10, 9, 8, 7, 6, 9, 9, 9, 9, 9 ]

    for (auto i = 2; i >= 0; --i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 0; i < 5; ++i) {
        graph.addConnection(5, 4);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0, 1, 2, 3, 4, 5, 11, 10, 9, 8, 7, 6,
            9, 9, 9, 9, 9,
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Reverse_Add_3x1x1_And_Existing_Expand)
{
    auto graph = CSRGraph{};

    // Recompress with map preservation/expansion gives
    //   MAP = [ 4, 5, 2, 3, 0, 1, 11, 10, 9, 8, 7, 6, 8, 8, 8, 8, 8, 5, 5, 5, 5, 5, 5 ]

    for (auto i = 2; i >= 0; --i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(4, 5);
    }

    graph.compress(7, true);

    for (auto i = 0; i < 6; ++i) {
        graph.addConnection(3, 2);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            4, 5, 2, 3, 0, 1, 11, 10, 9, 8, 7, 6,
            8, 8, 8, 8,         // for i=0..3: 4 -> 5
            5, 5, 5, 5, 5, 5,   // for i=0..5: 3 -> 2
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Reverse_Add_3x1x1_And_Existing_Expand_NoExpand)
{
    auto graph = CSRGraph{};

    for (auto i = 2; i >= 0; --i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(4, 5);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            4, 5, 2, 3, 0, 1, 11, 10, 9, 8, 7, 6,
            8, 8, 8, 8,         // for i=0..3: 4 -> 5
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }

    for (auto i = 0; i < 6; ++i) {
        graph.addConnection(3, 2);
    }

    graph.compress(7, false);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
            5, 5, 5, 5, 5, 5,   // for i=0..5: 3 -> 2
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple)
{
    auto graph = CSRGraph{};

    // One-pass construction, with repetition, gives
    //   MAP = [
    //     0, 1, 1, 0, 0 -- i = 0
    //     2, 3, 3, 2, 2 -- i = 1
    //     4, 5, 5, 4, 4 -- i = 2
    //   ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{6});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0, 1, 1, 0, 0,      // i = 0
            2, 3, 3, 2, 2,      // i = 1
            4, 5, 5, 4, 4,      // i = 2
        };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Add_3x1x1_Reverse)
{
    auto graph = CSRGraph{};

    // Recompress without map preservation/expansion, gives
    //   MAP = [
    //      0,  1, 2, 4, 5,   -- Original
    //     11, 10, 9, 8, 7, 6 -- 3x1x1 reverse
    //   ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
             0,  1, 2, 3, 4, 5,   // Original
            11, 10, 9, 8, 7, 6,   // 3x1x1 reverse
        };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Add_3x1x1_Reverse_Expand)
{
    auto graph = CSRGraph{};

    // One-pass construction, with repetition and recompress with
    // preservation/expansion, gives
    //
    //   MAP = [
    //      0,  1, 1, 0, 0    -- i = 0
    //      2,  3, 3, 2, 2    -- i = 1
    //      4,  5, 5, 4, 4    -- i = 2
    //     11, 10, 9, 8, 7, 6 -- 3x1x1 reverse
    //   ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
             0,  1, 1, 0, 0,      // i = 0
             2,  3, 3, 2, 2,      // i = 1
             4,  5, 5, 4, 4,      // i = 2
            11, 10, 9, 8, 7, 6,   // 3x1x1 reverse
        };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Add_3x1x1_Reverse_And_Existing)
{
    auto graph = CSRGraph{};

    // Recompress without map preservation/expansion, gives
    //   MAP = [
    //      0,  1, 2, 4, 5,   -- Original
    //     11, 10, 9, 8, 7, 6 -- 3x1x1 reverse
    //      2,  2, 2, 2, 2    -- for i=0..4: 1 -> 2
    //   ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 0; i < 5; ++i) {
        graph.addConnection(1, 2);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
             0,  1, 2, 3, 4, 5,   // Original
            11, 10, 9, 8, 7, 6,   // 3x1x1 reverse
             2,  2, 2, 2, 2,      // for i=0..4: 1 -> 2
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Add_3x1x1_Reverse_And_Existing_Expand)
{
    auto graph = CSRGraph{};

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 0; i < 6; ++i) {
        graph.addConnection(5, 6);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
             0,  1,  1,  0,  0,      // i = 0
             2,  3,  3,  2,  2,      // i = 1
             4,  5,  5,  4,  4,      // i = 2
            11, 10,  9,  8,  7,  6,  // 3x1x1 reverse
            10, 10, 10, 10, 10, 10,  // for i=0..5: 5 -> 6
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Add_3x1x1_Reverse_And_Existing_Expand_NoExpand)
{
    auto graph = CSRGraph{};

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    graph.compress(7, true);

    for (auto i = 0; i < 6; ++i) {
        graph.addConnection(5, 6);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
            10, 10, 10, 10, 10, 10,  // for i=0..5: 5 -> 6
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Repeated)
{
    auto graph = CSRGraph{};

    // Twenty pass construction, with repetition, gives
    //   MAP = repmat([
    //     0, 1, 1, 0, 0 -- i = 0
    //     2, 3, 3, 2, 2 -- i = 1
    //     4, 5, 5, 4, 4 -- i = 2
    //   ], 1, 20)

    const auto nrep = 20;

    for (auto n = 0; n < nrep; ++n) {
        for (auto i = 0; i < 4 - 1; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
            graph.addConnection(i, i + 1);
        }
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{6});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect0 = std::vector {
            0, 1, 1, 0, 0,      // i = 0
            2, 3, 3, 2, 2,      // i = 1
            4, 5, 5, 4, 4,      // i = 2
        };

        auto expect = std::vector<int>{};
        for (auto n = 0; n < nrep; ++n) {
            expect.insert(expect.end(), expect0.begin(), expect0.end());
        }

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Repeated_Add_3x1x1)
{
    auto graph = CSRGraph{};

    // Twenty pass construction, with repetition, and recompression without
    // map preservation/expansion, gives
    //
    //   MAP = [0, 1, 2, 3, 4, 5, -- Original
    //     repmat([11, 10, 9, 8, 7, 6, 6, 7, 8, 9, 10, 11], 1, 20)] -- 3x1x1

    const auto nrep = 20;

    for (auto n = 0; n < nrep; ++n) {
        for (auto i = 0; i < 4 - 1; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
            graph.addConnection(i, i + 1);
        }
    }

    graph.compress(4);

    for (auto n = 0; n < nrep; ++n) {
        for (auto i = 5; i >= 3; --i) {
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
        }
        for (auto i = 3; i < 6; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
        }
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();

        auto expectAdd = std::vector {
            11, 10, 9, 8, 7, 6,
            6, 7, 8, 9, 10, 11,
        };

        auto expect = std::vector { 0, 1, 2, 3, 4, 5, }; // Compressed original
        for (auto n = 0; n < nrep; ++n) {
            expect.insert(expect.end(), expectAdd.begin(), expectAdd.end());
        }

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Repeated_Add_3x1x1_Expand)
{
    auto graph = CSRGraph{};

    // Twenty pass construction, with repetition, and recompression with map
    // preservation/expansion, gives
    //
    //   MAP = [repmat([
    //     0, 1, 1, 0, 0 -- i = 0
    //     2, 3, 3, 2, 2 -- i = 1
    //     4, 5, 5, 4, 4 -- i = 2
    //   ], 1, 20),
    //   repmat([11, 10, 9, 8, 7, 6, 6, 7, 8, 9, 10, 11], 1, 20)]

    const auto nrep = 20;

    for (auto n = 0; n < nrep; ++n) {
        for (auto i = 0; i < 4 - 1; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
            graph.addConnection(i, i + 1);
        }
    }

    graph.compress(4);

    for (auto n = 0; n < nrep; ++n) {
        for (auto i = 5; i >= 3; --i) {
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
        }
        for (auto i = 3; i < 6; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
        }
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect0 = std::vector {
            0, 1, 1, 0, 0,      // i = 0
            2, 3, 3, 2, 2,      // i = 1
            4, 5, 5, 4, 4,      // i = 2
        };

        auto expectAdd = std::vector {
            11, 10, 9, 8, 7, 6,
            6, 7, 8, 9, 10, 11,
        };

        auto expect = std::vector<int>{};
        for (auto n = 0; n < nrep; ++n) {
            expect.insert(expect.end(), expect0.begin(), expect0.end());
        }

        for (auto n = 0; n < nrep; ++n) {
            expect.insert(expect.end(), expectAdd.begin(), expectAdd.end());
        }

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Repeated_Add_3x1x1_Expand_NoExpand)
{
    auto graph = CSRGraph{};

    // Twenty pass construction, with repetition, and recompression with map
    // preservation/expansion, gives
    //
    //   MAP = [repmat([
    //     0, 1, 1, 0, 0 -- i = 0
    //     2, 3, 3, 2, 2 -- i = 1
    //     4, 5, 5, 4, 4 -- i = 2
    //   ], 1, 20),
    //   repmat([11, 10, 9, 8, 7, 6, 6, 7, 8, 9, 10, 11], 1, 20)]

    const auto nrep = 20;

    for (auto n = 0; n < nrep; ++n) {
        for (auto i = 0; i < 4 - 1; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
            graph.addConnection(i, i + 1);
        }
    }

    graph.compress(4);

    for (auto n = 0; n < nrep; ++n) {
        for (auto i = 5; i >= 3; --i) {
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
        }
        for (auto i = 3; i < 6; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
        }
    }

    graph.compress(7, true);

    for (auto n = 0; n < nrep; ++n) {
        graph.addConnection(1, 0);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect0 = std::vector {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        };

        auto expect = expect0;
        expect.insert(expect.end(), nrep, 1);

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Tracked

BOOST_AUTO_TEST_SUITE_END()     // No_Self_Connections

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Permit_Self_Connections)

BOOST_AUTO_TEST_SUITE(Untracked)

namespace {
    // Vertex = int, TrackCompressedIdx = false, PermitSelfConnections = true
    using CSRGraph = Opm::utility::CSRGraphFromCoordinates<int, false, true>;
}

BOOST_AUTO_TEST_CASE(Clear_Empty_is_Valid)
{
    auto graph = CSRGraph{};

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{0});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{0});

    graph.clear();

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{0});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{0});
}

BOOST_AUTO_TEST_CASE(Negative_Vertex_ID)
{
    auto graph = CSRGraph{};

    BOOST_CHECK_THROW(graph.addConnection( 0, - 1), std::invalid_argument);
    BOOST_CHECK_THROW(graph.addConnection(-1,  10), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 1  1  0  0 ]
    //    [ 1  1  1  0 ]
    //    [ 0  1  1  1 ]
    //    [ 0  0  1  1 ]
    //
    // => CSR: IA = [ 0,      2,        5,        8,     10 ]
    //         JA = [ 0, 1  | 0, 1, 2 | 1, 2, 3 | 2, 3 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{10});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 2, 5, 8, 10 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 0, 1, 0, 1, 2, 1, 2, 3, 2, 3, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Clear)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 1  1  0  0 ]
    //    [ 1  1  1  0 ]
    //    [ 0  1  1  1 ]
    //    [ 0  0  1  1 ]
    //
    // => CSR: IA = [ 0,      2,        5,        8,     10 ]
    //         JA = [ 0, 1  | 0, 1, 2 | 1, 2, 3 | 2, 3 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{10});

    graph.clear();

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{0});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{0});

    BOOST_CHECK_MESSAGE(graph.startPointers().empty(),
                        "Start pointer array must be empty in cleared graph");

    BOOST_CHECK_MESSAGE(graph.columnIndices().empty(),
                        "Column index array must be empty in cleared graph");
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Compress_Small)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 1  1  0  0 ]
    //    [ 1  1  1  0 ]
    //    [ 0  1  1  1 ]
    //    [ 0  0  1  1 ]
    //
    // => CSR: IA = [ 0,      2,        5,        8,     10 ]
    //         JA = [ 0, 1  | 0, 1, 2 | 1, 2, 3 | 2, 3 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    // Argument 3 is too small.  There are 4 vertices in the graph.
    BOOST_CHECK_THROW(graph.compress(3), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 1  1  0  0 ]
    //    [ 1  1  1  0 ]
    //    [ 0  1  1  1 ]
    //    [ 0  0  1  1 ]
    //
    // => CSR: IA = [ 0,      2,        5,        8,     10 ]
    //         JA = [ 0, 1  | 0, 1, 2 | 1, 2, 3 | 2, 3 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{10});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 2, 5, 8, 10 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 0, 1, 0, 1, 2, 1, 2, 3, 2, 3, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Repeated)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 1  1  0  0 ]
    //    [ 1  1  1  0 ]
    //    [ 0  1  1  1 ]
    //    [ 0  0  1  1 ]
    //
    // => CSR: IA = [ 0,      2,        5,        8,     10 ]
    //         JA = [ 0, 1  | 0, 1, 2 | 1, 2, 3 | 2, 3 ]

    for (auto n = 0; n < 20; ++n) {
        for (auto i = 0; i < 4 - 1; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
            graph.addConnection(i, i + 1);
        }

        for (auto i = 0; i < 4; ++i) {
            graph.addConnection(i, i);
        }
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{10});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 2, 5, 8, 10 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 0, 1, 0, 1, 2, 1, 2, 3, 2, 3, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |  4  |  5  |  6  |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 1  1  0  0  0  0  0 ]
    //    [ 1  1  1  0  0  0  0 ]
    //    [ 0  1  1  1  0  0  0 ]
    //    [ 0  0  1  1  0  0  0 ]
    //    [ 0  0  0  1  1  1  0 ]
    //    [ 0  0  0  0  1  1  1 ]
    //    [ 0  0  0  0  0  1  1 ]
    //
    // => CSR: IA = [ 0,      2,        5,        8,        11,        14,         17,    19 ]
    //         JA = [ 0, 1  | 0, 1, 2 | 1, 2, 3 | 2, 3, 4 | 3 , 4, 5 | 4 , 5, 6 |  5, 6 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 2, 5, 8, 11, 14, 17, 19, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 0, 1, 0, 1, 2, 1, 2, 3, 2, 3, 4, 3, 4, 5, 4, 5, 6, 5, 6, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1_Expand)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |  4  |  5  |  6  |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 1  1  0  0  0  0  0 ]
    //    [ 1  1  1  0  0  0  0 ]
    //    [ 0  1  1  1  0  0  0 ]
    //    [ 0  0  1  1  0  0  0 ]
    //    [ 0  0  0  1  1  1  0 ]
    //    [ 0  0  0  0  1  1  1 ]
    //    [ 0  0  0  0  0  1  1 ]
    //
    // => CSR: IA = [ 0,      2,        5,        8,        11,        14,         17,    19 ]
    //         JA = [ 0, 1  | 0, 1, 2 | 1, 2, 3 | 2, 3, 4 | 3 , 4, 5 | 4 , 5, 6 |  5, 6 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(4, true);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 2, 5, 8, 11, 14, 17, 19, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 0, 1, 0, 1, 2, 1, 2, 3, 2, 3, 4, 3, 4, 5, 4, 5, 6, 5, 6, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1_And_Existing)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |  4  |  5  |  6  |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 1  1  0  0  0  0  0 ]
    //    [ 1  1  1  0  0  0  0 ]
    //    [ 0  1  1  1  0  0  0 ]
    //    [ 0  0  1  1  0  0  0 ]
    //    [ 0  0  0  1  1  1  0 ]
    //    [ 0  0  0  0  1  1  1 ]
    //    [ 0  0  0  0  0  1  1 ]
    //
    // => CSR: IA = [ 0,      2,        5,        8,        11,        14,         17,    19 ]
    //         JA = [ 0, 1  | 0, 1, 2 | 1, 2, 3 | 2, 3, 4 | 3 , 4, 5 | 4 , 5, 6 |  5, 6 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(1, 2);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 2, 5, 8, 11, 14, 17, 19, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 0, 1, 0, 1, 2, 1, 2, 3, 2, 3, 4, 3, 4, 5, 4, 5, 6, 5, 6, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1_And_Existing_Expand)
{
    auto graph = CSRGraph{};

    // +-----+-----+-----+-----+-----+-----+-----+
    // |  0  |  1  |  2  |  3  |  4  |  5  |  6  |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 1  1  0  0  0  0  0 ]
    //    [ 1  1  1  0  0  0  0 ]
    //    [ 0  1  1  1  0  0  0 ]
    //    [ 0  0  1  1  0  0  0 ]
    //    [ 0  0  0  1  1  1  0 ]
    //    [ 0  0  0  0  1  1  1 ]
    //    [ 0  0  0  0  0  1  1 ]
    //
    // => CSR: IA = [ 0,      2,        5,        8,        11,        14,         17,    19 ]
    //         JA = [ 0, 1  | 0, 1, 2 | 1, 2, 3 | 2, 3, 4 | 3 , 4, 5 | 4 , 5, 6 |  5, 6 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(4, true);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 17; ++i) {
        graph.addConnection(1, 2);
        graph.addConnection(3, 2);

        for (auto k = 4; k < 7; ++k) {
            graph.addConnection(k, k);
        }
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 2, 5, 8, 11, 14, 17, 19, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 0, 1, 0, 1, 2, 1, 2, 3, 2, 3, 4, 3, 4, 5, 4, 5, 6, 5, 6, };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Untracked

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Tracked)

namespace {
    // Vertex = int, TrackCompressedIdx = true, PermitSelfConnections = true
    using CSRGraph = Opm::utility::CSRGraphFromCoordinates<int, true, true>;
}

BOOST_AUTO_TEST_CASE(Clear_Empty_is_Valid)
{
    auto graph = CSRGraph{};

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{0});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{0});

    graph.clear();

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{0});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{0});
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric)
{
    auto graph = CSRGraph{};

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{10});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector { 1, 2, 4, 5, 7, 8, 0, 3, 6, 9, };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Clear)
{
    auto graph = CSRGraph{};

    // Regular one-pass construction gives
    //   MAP = [ 0, 1, 2, 3, 4, 5 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    graph.clear();

    BOOST_CHECK_MESSAGE(graph.compressedIndexMap().empty(),
                        "Compressed index map must be empty in cleared graph");
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1)
{
    auto graph = CSRGraph{};

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
             0,  1,  2,  3,  4,  5,  6,  7,  8, 9, // Original
            10, 11, 13, 14, 16, 17, 12, 15, 18,    // Expanded/additional
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1_Expand)
{
    auto graph = CSRGraph{};

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(4, true);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
             1,  2,  4,  5,  7,  8,  0,  3,  6, 9, // Original
            10, 11, 13, 14, 16, 17, 12, 15, 18,    // Expanded
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1_And_Existing)
{
    auto graph = CSRGraph{};

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    for (auto i = 0; i < 5; ++i) {
        graph.addConnection(1, 2);
    }

    for (auto i = 0; i < 3; ++i) {
        graph.addConnection(3, 2);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
             0,  1,  2,  3,  4,  5,  6,  7,  8, 9,
            10, 11, 13, 14, 16, 17, 12, 15, 18,
            4, 4, 4, 4, 4,      // for i=0..4, 1 -> 2
            8, 8, 8,            // for i=0..2, 3 -> 2
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Add_3x1x1_And_Existing_Expand)
{
    auto graph = CSRGraph{};

    // Recompress with preservation/expansion gives
    //   MAP = [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 2, 2, 2, 2, 2 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    graph.compress(4, true);

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 5; ++i) {
        graph.addConnection(1, 2);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{12});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
            2, 2, 2, 2, 2,
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Reverse)
{
    auto graph = CSRGraph{};

    for (auto i = 2; i >= 0; --i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 3; i >= 0; --i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{10});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            7, 8, 4, 5, 1, 2, 9, 6, 3, 0,
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Reverse_Add_3x1x1)
{
    auto graph = CSRGraph{};

    for (auto i = 2; i >= 0; --i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 3; i >= 0; --i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 6; i >= 4; --i) {
        graph.addConnection(i, i);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
            17, 16, 14, 13, 11, 10, 18, 15, 12,
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Reverse_Add_3x1x1_Expand)
{
    auto graph = CSRGraph{};

    for (auto i = 2; i >= 0; --i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 3; i >= 0; --i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 6; i >= 4; --i) {
        graph.addConnection(i, i);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            7, 8, 4, 5, 1, 2, 9, 6, 3, 0,
            17, 16, 14, 13, 11, 10, 18, 15, 12,
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Reverse_Add_3x1x1_And_Existing)
{
    auto graph = CSRGraph{};

    for (auto i = 2; i >= 0; --i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 3; i >= 0; --i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 6; i >= 4; --i) {
        graph.addConnection(i, i);
    }

    for (auto i = 0; i < 5; ++i) {
        graph.addConnection(5, 4);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
            17, 16, 14, 13, 11, 10, 18, 15, 12,
            14, 14, 14, 14, 14,
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Reverse_Add_3x1x1_And_Existing_Expand)
{
    auto graph = CSRGraph{};

    for (auto i = 2; i >= 0; --i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 3; i >= 0; --i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 6; i >= 4; --i) {
        graph.addConnection(i, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(4, 5);
    }

    graph.compress(7, true);

    for (auto i = 0; i < 6; ++i) {
        graph.addConnection(3, 2);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            7, 8, 4, 5, 1, 2, 9, 6, 3, 0, 17, 16, 14, 13, 11, 10, 18, 15, 12,
            13, 13, 13, 13,         // for i=0..3: 4 -> 5
            8, 8, 8, 8, 8, 8,       // for i=0..5: 3 -> 2
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Reverse_Add_3x1x1_And_Existing_Expand_NoExpand)
{
    auto graph = CSRGraph{};

    for (auto i = 2; i >= 0; --i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 3; i >= 0; --i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(4, 5);
    }

    for (auto i = 6; i >= 4; --i) {
        graph.addConnection(i, i);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            7, 8, 4, 5, 1, 2, 9, 6, 3, 0, 17, 16, 14, 13, 11, 10,
            13, 13, 13, 13,     // for i=0..3: 4 -> 5
            18, 15, 12,
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }

    for (auto i = 0; i < 6; ++i) {
        graph.addConnection(3, 2);
    }

    graph.compress(7, false);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
            8, 8, 8, 8, 8, 8,   // for i=0..5: 3 -> 2
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple)
{
    auto graph = CSRGraph{};

    graph.addConnection(1, 1);
    graph.addConnection(0, 0);

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    graph.addConnection(2, 2);
    graph.addConnection(3, 3);

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{10});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            3, 0,
            1, 2, 2, 1, 1,      // i = 0
            4, 5, 5, 4, 4,      // i = 1
            7, 8, 8, 7, 7,      // i = 2
            6, 9,
        };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Add_3x1x1_Reverse)
{
    auto graph = CSRGraph{};

    graph.addConnection(1, 1);
    graph.addConnection(0, 0);
    graph.addConnection(0, 0);
    graph.addConnection(1, 1);

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    graph.addConnection(2, 2);
    graph.addConnection(3, 3);
    graph.addConnection(3, 3);
    graph.addConnection(2, 2);

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 6; i >= 4; --i) {
        graph.addConnection(i, i);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0,  1, 2, 3, 4, 5, 6, 7, 8, 9,      // Original
            17, 16, 14, 13, 11, 10, 18, 15, 12, // 3x1x1 reverse
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Add_3x1x1_Reverse_Expand)
{
    auto graph = CSRGraph{};

    graph.addConnection(1, 1);
    graph.addConnection(0, 0);
    graph.addConnection(0, 0);
    graph.addConnection(1, 1);

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    graph.addConnection(2, 2);
    graph.addConnection(3, 3);
    graph.addConnection(3, 3);
    graph.addConnection(2, 2);

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 6; i >= 4; --i) {
        graph.addConnection(i, i);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
             3,  0,  0,  3,
             1,  2,  2,  1,  1,      // i = 0
             4,  5,  5,  4,  4,      // i = 1
             7,  8,  8,  7,  7,      // i = 2
             6,  9,  9,  6,
            17, 16, 14, 13, 11, 10, 18, 15, 12,   // 3x1x1 reverse
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Add_3x1x1_Reverse_And_Existing)
{
    auto graph = CSRGraph{};

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    for (auto i = 0; i < 5; ++i) {
        graph.addConnection(1, 2);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0,  1,  2,  3,  4,  5,  6,  7,  8, 9, // Original
           17, 16, 14, 13, 11, 10, 12, 15, 18,    // 3x1x1 reverse
            4,  4,  4,  4,  4,                    // for i=0..4: 1 -> 2
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Add_3x1x1_Reverse_And_Existing_Expand)
{
    auto graph = CSRGraph{};

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 0; i < 6; ++i) {
        graph.addConnection(5, 6);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
             1,  2,  2,  1,  1,      // i = 0
             4,  5,  5,  4,  4,      // i = 1
             7,  8,  8,  7,  7,      // i = 2
             0,  3,  6,  9,
            17, 16, 14, 13, 11, 10,  // 3x1x1 reverse
            16, 16, 16, 16, 16, 16,  // for i=0..5: 5 -> 6
            12, 15, 18,
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Add_3x1x1_Reverse_And_Existing_Expand_NoExpand)
{
    auto graph = CSRGraph{};

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    for (auto i = 3; i >= 0; --i) {
        graph.addConnection(i, i);
    }

    graph.compress(4);

    for (auto i = 5; i >= 3; --i) {
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
    }

    graph.compress(7, true);

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    for (auto i = 0; i < 6; ++i) {
        graph.addConnection(5, 6);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 16, 17,
            12, 15, 18,
            16, 16, 16, 16, 16, 16,  // for i=0..5: 5 -> 6
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Repeated)
{
    auto graph = CSRGraph{};

    const auto nrep = 20;

    for (auto n = 0; n < nrep; ++n) {
        for (auto i = 0; i < 4 - 1; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
            graph.addConnection(i, i + 1);
        }

        for (auto i = 0; i < 4; ++i) {
            graph.addConnection(i, i);
        }
    }

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{10});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect0 = std::vector {
            1, 2, 2, 1, 1,      // i = 0
            4, 5, 5, 4, 4,      // i = 1
            7, 8, 8, 7, 7,      // i = 2
            0, 3, 6, 9,
        };

        auto expect = std::vector<int>{};
        for (auto n = 0; n < nrep; ++n) {
            expect.insert(expect.end(), expect0.begin(), expect0.end());
        }

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Repeated_Add_3x1x1)
{
    auto graph = CSRGraph{};

    const auto nrep = 20;

    for (auto n = 0; n < nrep; ++n) {
        for (auto i = 0; i < 4 - 1; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
            graph.addConnection(i, i + 1);
        }

        for (auto i = 0; i < 4; ++i) {
            graph.addConnection(i, i);
        }
    }

    graph.compress(4);

    for (auto n = 0; n < nrep; ++n) {
        for (auto i = 5; i >= 3; --i) {
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
        }

        graph.addConnection(6, 6);
        graph.addConnection(5, 5);

        for (auto i = 3; i < 6; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
        }

        graph.addConnection(4, 4);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();

        auto expectAdd = std::vector {
            17, 16, 14, 13, 11, 10,
            18, 15,
            10, 11, 13, 14, 16, 17,
            12,
        };

        auto expect = std::vector { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, }; // Compressed original
        for (auto n = 0; n < nrep; ++n) {
            expect.insert(expect.end(), expectAdd.begin(), expectAdd.end());
        }

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Repeated_Add_3x1x1_Expand)
{
    auto graph = CSRGraph{};

    const auto nrep = 20;

    for (auto n = 0; n < nrep; ++n) {
        for (auto i = 0; i < 4 - 1; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
            graph.addConnection(i, i + 1);
        }

        for (auto i = 0; i < 4; ++i) {
            graph.addConnection(i, i);
            graph.addConnection(3 - i, 3 - i);
        }
    }

    graph.compress(4);

    for (auto n = 0; n < nrep; ++n) {
        for (auto i = 5; i >= 3; --i) {
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
        }

        for (auto i = 6; i >= 4; --i) {
            graph.addConnection(i, i);
        }

        for (auto i = 3; i < 6; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
        }
    }

    graph.compress(7, true);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect0 = std::vector {
            1, 2, 2, 1, 1,      // i = 0
            4, 5, 5, 4, 4,      // i = 1
            7, 8, 8, 7, 7,      // i = 2
            0, 9,
            3, 6,
            6, 3,
            9, 0,
        };

        auto expectAdd = std::vector {
            17, 16, 14, 13, 11, 10,
            18, 15, 12,
            10, 11, 13, 14, 16, 17,
        };

        auto expect = std::vector<int>{};
        for (auto n = 0; n < nrep; ++n) {
            expect.insert(expect.end(), expect0.begin(), expect0.end());
        }

        for (auto n = 0; n < nrep; ++n) {
            expect.insert(expect.end(), expectAdd.begin(), expectAdd.end());
        }

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Repeated_Add_3x1x1_Expand_NoExpand)
{
    auto graph = CSRGraph{};

    const auto nrep = 20;

    for (auto n = 0; n < nrep; ++n) {
        for (auto i = 0; i < 4 - 1; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
            graph.addConnection(i, i + 1);
        }

        for (auto i = 0; i < 4; ++i) {
            graph.addConnection(i, i);
            graph.addConnection(3 - i, 3 - i);
        }
    }

    graph.compress(4);

    for (auto n = 0; n < nrep; ++n) {
        for (auto i = 5; i >= 3; --i) {
            graph.addConnection(i + 1, i);
            graph.addConnection(i, i + 1);
        }

        for (auto i = 6; i >= 4; --i) {
            graph.addConnection(i, i);
        }

        for (auto i = 3; i < 6; ++i) {
            graph.addConnection(i, i + 1);
            graph.addConnection(i + 1, i);
        }
    }

    graph.compress(7, true);

    for (auto n = 0; n < nrep; ++n) {
        graph.addConnection(1, 0);
    }

    graph.compress(7);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{7});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{19});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect0 = std::vector {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        };

        auto expect = expect0;
        expect.insert(expect.end(), nrep, 2);

        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Tracked

BOOST_AUTO_TEST_SUITE_END()     // Permit_Self_Connections
