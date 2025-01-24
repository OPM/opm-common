/*
  Copyright 2025 SINTEF
  Copyright 2025 Equinor

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

#define BOOST_TEST_MODULE CSR_Graph_From_Coordinates_Merge

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

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Add_Single_Merge)
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

    // Group vertices 1,2 together - they will be merged into a single vertex
    graph.addVertexGroup({1, 2});

    // +-----+-----+-----+-----+
    // |  0  |     1     |  2  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0 ]
    //    [ 1  0  1 ]
    //    [ 0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,  4 ]
    //         JA = [ 1  | 0, 2 | 1 ]

    graph.compress(3);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{3});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{4});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 1, 3, 4 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 1, 0, 2, 1 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_7x1x1_Two_Disjoint_Merges)
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

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    // Group vertices 1,2 together - they will be merged into a single vertex
    graph.addVertexGroup({0, 1, 2});
    // Group vertices 5,6 together - they will be merged into a single vertex
    graph.addVertexGroup({5, 6});

    // +-----+-----+-----+-----+-----+-----+-----+
    // |        0        |  1  |  2  |     3     |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0  0 ]
    //    [ 1  0  1  0 ]
    //    [ 0  1  0  1 ]
    //    [ 0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,     5,  6 ]
    //         JA = [ 1  | 0, 2 | 1, 3 | 2 ]

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

BOOST_AUTO_TEST_CASE(Linear_7x1x1_Two_Intersecting_Merges)
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

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    // Group vertices 1,2,3 together - they will be merged into a single vertex
    graph.addVertexGroup({1, 2, 3});
    
    // Group vertices 3,4 together - they will be merged into a single vertex
    // Note: Since vertex 3 appears in both groups, these groups will be merged
    graph.addVertexGroup({3, 4});

    // +-----+-----+-----+-----+-----+-----+-----+
    // |  0  |        1              |  2  |  3  |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0  0 ]
    //    [ 1  0  1  0 ]
    //    [ 0  1  0  1 ]
    //    [ 0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,     5,  6 ]
    //         JA = [ 1  | 0, 2 | 1, 3 | 2 ]

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

BOOST_AUTO_TEST_CASE(Directed_Graph_With_Merges)
{
    auto graph = CSRGraph{};
    // Initial directed graph structure:
    //
    // 0 -----> 1 -----> 2
    //          |        ^
    //          |        |
    //          v        |
    //          3 -----> 4
    //
    // => Laplacian (adjacency matrix for directed graph)
    //    [ 0  1  0  0  0 ]
    //    [ 0  0  1  1  0 ]
    //    [ 0  0  0  0  0 ]
    //    [ 0  0  0  0  1 ]
    //    [ 0  0  1  0  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,  3,  4,  5 ]
    //         JA = [ 1  | 2, 3 |   | 4 | 2 ]
    // Add directed edges
    graph.addConnection(0, 1);  // 0 -> 1
    graph.addConnection(1, 2);  // 1 -> 2
    graph.addConnection(1, 3);  // 1 -> 3
    graph.addConnection(3, 4);  // 3 -> 4
    graph.addConnection(4, 2);  // 4 -> 2
    // Merge vertices 3 and 4 into vertex 3
    // After merge:
    // 0 -----> 1 -----> 2
    //          |        ^
    //          |        |
    //          v        |
    //          3 -------'
    //
    // => Laplacian (adjacency matrix for directed graph)
    //    [ 0  1  0  0 ]
    //    [ 0  0  1  1 ]
    //    [ 0  0  0  0 ]
    //    [ 0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,  3,  4 ]
    //         JA = [ 1  | 2, 3 |   | 2 ]
    graph.addVertexGroup({3, 4});
    graph.compress(4);
    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{4});
    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 1, 3, 3, 4 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }
    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 1, 2, 3, 2 };
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

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Add_Single_Merge)
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
    //
    // One-pass construction, with repetition, gives
    //   MAP = [
    //     0, 1, 1, 0, 0   -- i = 0: (0->1, 1->0, 1->0, 0->1, 0->1)
    //     2, 3, 3, 2, 2   -- i = 1: (1->2, 2->1, 2->1, 1->2, 1->2)
    //     4, 5, 5, 4, 4   -- i = 2: (2->3, 3->2, 3->2, 2->3, 2->3)
    //   ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    // Group vertices 1,2 together - they will be merged into a single vertex
    graph.addVertexGroup({1, 2});

    // +-----+-----+-----+-----+
    // |  0  |     1     |  2  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0 ]
    //    [ 1  0  1 ]
    //    [ 0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,  4 ]
    //         JA = [ 1  | 0, 2 | 1 ]
    //
    // One-pass construction, with repetition, gives
    //   MAP = [
    //     0, 1, 1, 0, 0   -- i = 0: (0->1, 1->0, 1->0, 0->1, 0->1)
    //     x, x, x, x, x   -- i = 1: (1->1, 1->1, 1->1, 1->1, 1->1)
    //     2, 3, 3, 2, 2   -- i = 2: (1->2, 2->1, 2->1, 1->2, 1->2)
    //   ]
    //
    // Note that in the current implementation, when tracking compressed
    // indices, and not permitting self connections, the newly introduced
    // self connections are removed from the graph.

    graph.compress(3);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{3});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{4});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0, 1, 1, 0, 0,      // i = 0
        //  x, x, x, x, x,      // i = 1: (1->1, 1->1, 1->1, 1->1, 1->1)
            2, 3, 3, 2, 2,      // i = 2
        };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_7x1x1_Two_Disjoint_Merges)
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
    //
    // One-pass construction edges:
    // 0->1
    // 1->0
    // 1->2
    // 2->1
    // 2->3
    // 3->2
    // x - (0->0)
    // x - (1->1)
    // x - (2->2)
    // x - (3->3)
    // 3->4
    // 4->3
    // 4->5
    // 5->4
    // 5->6
    // 6->5
    // x - (4->4)
    // x - (5->5)
    // x - (6->6)

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    // Group vertices 0, 1,2 together - they will be merged into a single vertex
    graph.addVertexGroup({0,1, 2});
    // Group vertices 5,6 together - they will be merged into a single vertex
    graph.addVertexGroup({5, 6});

    // +-----+-----+-----+-----+-----+-----+-----+
    // |        0        |  1  |  2  |     3     |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0  0 ]
    //    [ 1  0  1  0 ]
    //    [ 0  1  0  1 ]
    //    [ 0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,     5,  6 ]
    //         JA = [ 1  | 0, 2 | 1, 3 | 2 ]

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{6});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
                // One-pass construction edges, after merging vertices 1,2 into 0 and 5,6 into 6:
                // x - (0->0)
                // x - (0->0)
                // x - (0->0)
                // x - (0->0)
            0,  // 0->1
            1,  // 1->0
                // x - (0->0)
                // x - (0->0)
                // x - (0->0)
                // x - (1->1)
            2,  // 1->2
            3,  // 2->1
            4,  // 2->3
            5,  // 3->2
                // x - (3->3)
                // x - (3->3)
                // x - (2->2)
                // x - (3->3)
                // x - (3->3)
        };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}


BOOST_AUTO_TEST_CASE(Linear_7x1x1_Two_Intersecting_Merges)
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
    // One-pass construction edges:
    // 0->1
    // 1->0
    // 1->2
    // 2->1
    // 2->3
    // 3->2
    // x - (0->0)
    // x - (1->1)
    // x - (2->2)
    // x - (3->3)
    // 3->4
    // 4->3
    // 4->5
    // 5->4
    // 5->6
    // 6->5
    // x - (4->4)
    // x - (5->5)
    // x - (6->6)

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    // Group vertices 1,2,3 together - they will be merged into a single vertex
    graph.addVertexGroup({1, 2, 3});
    
    // Group vertices 3,4 together - they will be merged into a single vertex
    // Note: Since vertex 3 appears in both groups, these groups will be merged
    graph.addVertexGroup({3, 4});

    // +-----+-----+-----+-----+-----+-----+-----+
    // |  0  |        1              |  2  |  3  |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0  0 ]
    //    [ 1  0  1  0 ]
    //    [ 0  1  0  1 ]
    //    [ 0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,     5,  6 ]
    //         JA = [ 1  | 0, 2 | 1, 3 | 2 ]

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{6});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
                // One-pass construction edges, after merging vertices 1,2,3 into 1 and 3,4 into 3:
            0,  // 0->1
            1,  // 1->0
                // x - (1->1)
                // x - (1->1)
                // x - (1->1)
                // x - (1->1)
                // x - (0->0)
                // x - (1->1)
                // x - (1->1)
                // x - (1->1)
                // x - (1->1)
                // x - (1->1)
            2,  // 1->2
            3,  // 2->1
            4,  // 2->3
            5   // 3->2
                // x - (1->1)
                // x - (2->2)
                // x - (3->3)
        };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Directed_Graph_With_Merge)
{
    auto graph = CSRGraph{};

    // Initial directed graph structure:
    //
    // 0 -----> 1 -----> 2
    //          |        ^
    //          |        |
    //          v        |
    //          3 -----> 4
    //
    // => Laplacian (adjacency matrix for directed graph)
    //    [ 0  1  0  0  0 ]
    //    [ 0  0  1  1  0 ]
    //    [ 0  0  0  0  0 ]
    //    [ 0  0  0  0  1 ]
    //    [ 0  0  1  0  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,  3,  4,  5 ]
    //         JA = [ 1  | 2, 3 |   | 4 | 2 ]

    // Add directed edges
    graph.addConnection(0, 1);  // 0 -> 1
    graph.addConnection(1, 2);  // 1 -> 2
    graph.addConnection(1, 3);  // 1 -> 3
    graph.addConnection(3, 4);  // 3 -> 4
    graph.addConnection(4, 2);  // 4 -> 2

    // Group vertices 3,4 together - they will be merged into a single vertex
    graph.addVertexGroup({3, 4});

    // After merge:
    // 0 -----> 1 -----> 2
    //          |        ^
    //          |        |
    //          v        |
    //          3 -------'
    //
    // => Laplacian (adjacency matrix for directed graph)
    //    [ 0  1  0  0 ]
    //    [ 0  0  1  1 ]
    //    [ 0  0  0  0 ]
    //    [ 0  0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,  3,  4 ]
    //         JA = [ 1  | 2, 3 |   | 2 ]
    //
    // One-pass construction, with tracking, gives
    //   MAP = [
    //     0,  // 0->1
    //     1,  // 1->2
    //     2,  // 1->3
    //     x,  // (3->4 becomes 3->3, removed as self-connection)
    //     3   // (4->2 becomes 3->2)
    //   ]

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{4});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0,  // 0->1
            1,  // 1->2
            2,  // 1->3
                // x - (3->3)
            3   // (4->2 becomes 3->2)
        };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap.begin(), nzMap.end(),
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

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Add_Single_Merge)
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

    // Group vertices 1,2 together - they will be merged into a single vertex
    graph.addVertexGroup({1, 2});

    // +-----+-----+-----+-----+
    // |  0  |     1     |  2  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0 ]
    //    [ 1  1  1 ]
    //    [ 0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,        4,  5 ]
    //         JA = [ 1  | 0, 1, 2 | 1 ]

    graph.compress(3);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{3});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{5});

    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 1, 4, 5 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }

    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 1, 0, 1, 2, 1 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_7x1x1_Two_Disjoint_Merges)
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
    // => CSR: IA = [ 0,      2,        5,        8,        11,       14,        17, 19 ]
    //         JA = [ 0, 1  | 0, 1, 2 | 1, 2, 3 | 2, 3, 4 | 3, 4, 5 | 4, 5, 6 |  5, 6 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    // Group vertices 0,1,2 together - they will be merged into a single vertex
    graph.addVertexGroup({0, 1, 2});
    // Group vertices 5,6 together - they will be merged into a single vertex
    graph.addVertexGroup({5, 6});

    // +-----+-----+-----+-----+-----+-----+-----+
    // |        0        |  1  |  2  |     3     |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 1  1  0  0 ]
    //    [ 1  1  1  0 ]
    //    [ 0  1  1  1 ]
    //    [ 0  0  1  1 ]
    //
    // => CSR: IA = [ 0,     2,        5,        8,  10 ]
    //         JA = [ 0, 1 | 0, 1, 2 | 1, 2, 3 | 2, 3 ]

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
        const auto expect = std::vector { 0, 1, 0, 1, 2, 1, 2, 3, 2, 3 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_7x1x1_Two_Intersecting_Merges)
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
    // => CSR: IA = [ 0,      2,        5,        8,        11,       14,        17, 19 ]
    //         JA = [ 0, 1  | 0, 1, 2 | 1, 2, 3 | 2, 3, 4 | 3, 4, 5 | 4, 5, 6 |  5, 6 ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    // Group vertices 1,2,3 together - they will be merged into a single vertex
    graph.addVertexGroup({1, 2, 3});
    
    // Group vertices 3,4 together - they will be merged into a single vertex
    // Note: Since vertex 3 appears in both groups, these groups will be merged
    graph.addVertexGroup({3, 4});

    // +-----+-----+-----+-----+-----+-----+-----+
    // |  0  |        1              |  2  |  3  |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 1  1  0  0 ]
    //    [ 1  1  1  0 ]
    //    [ 0  1  1  1 ]
    //    [ 0  0  1  1 ]
    //
    // => CSR: IA = [ 0,     2,        5,        8,  10 ]
    //         JA = [ 0, 1 | 0, 1, 2 | 1, 2, 3 | 2, 3 ]

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
        const auto expect = std::vector { 0, 1, 0, 1, 2, 1, 2, 3, 2, 3 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ja.begin(), ja.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Directed_Graph_With_Merges)
{
    auto graph = CSRGraph{};
    // Initial directed graph structure:
    //
    // 0 -----> 1 -----> 2
    //          |        ^
    //          |        |
    //          v        |
    //          3 -----> 4
    //
    // => Laplacian (adjacency matrix for directed graph)
    //    [ 0  1  0  0  0 ]
    //    [ 0  0  1  1  0 ]
    //    [ 0  0  0  0  0 ]
    //    [ 0  0  0  0  1 ]
    //    [ 0  0  1  0  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,  3,  4, 5 ]
    //         JA = [ 1  | 2, 3 |   | 4 | 2 ]
    // Add directed edges
    graph.addConnection(0, 1);  // 0 -> 1
    graph.addConnection(1, 2);  // 1 -> 2
    graph.addConnection(1, 3);  // 1 -> 3
    graph.addConnection(3, 4);  // 3 -> 4
    graph.addConnection(4, 2);  // 4 -> 2

    // Group vertices 3,4 together - they will be merged into a single vertex
    graph.addVertexGroup({3, 4});

    // After merge:
    // 0 -----> 1 -----> 2
    //          |        ^
    //          |        |
    //          v        |
    //          3 -------'
    //
    // => Laplacian (adjacency matrix for directed graph)
    //    [ 0  1  0  0 ]
    //    [ 0  0  1  1 ]
    //    [ 0  0  0  0 ]
    //    [ 0  0  1  1 ]
    //
    // => CSR: IA = [ 0,  1,     3,  3,   5 ]
    //         JA = [ 1 | 2, 3 |   | 2, 3 ]

    graph.compress(4);
    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{5});
    {
        const auto& ia = graph.startPointers();
        const auto expect = std::vector { 0, 1, 3, 3, 5 };
        BOOST_CHECK_EQUAL_COLLECTIONS(ia.begin(), ia.end(), expect.begin(), expect.end());
    }
    {
        const auto& ja = graph.columnIndices();
        const auto expect = std::vector { 1, 2, 3, 2, 3 };
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

BOOST_AUTO_TEST_CASE(Linear_4x1x1_Symmetric_Multiple_Add_Single_Merge)
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
    //
    // One-pass construction, with repetition, gives
    //   MAP = [
    //     0, 1, 1, 0, 0   -- i = 0: (0->1, 1->0, 1->0, 0->1, 0->1)
    //     2, 3, 3, 2, 2   -- i = 1: (1->2, 2->1, 2->1, 1->2, 1->2)
    //     4, 5, 5, 4, 4   -- i = 2: (2->3, 3->2, 3->2, 2->3, 2->3)
    //   ]

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
        graph.addConnection(i + 1, i);
        graph.addConnection(i, i + 1);
        graph.addConnection(i, i + 1);
    }

    // Group vertices 1,2 together - they will be merged into a single vertex
    graph.addVertexGroup({1, 2});

    // +-----+-----+-----+-----+
    // |  0  |     1     |  2  |
    // +-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 0  1  0 ]
    //    [ 1  1  1 ]
    //    [ 0  1  0 ]
    //
    // => CSR: IA = [ 0,   1,        4,  5 ]
    //         JA = [ 1  | 0, 1, 2 | 1 ]
    //
    // One-pass construction, with repetition, gives
    //   MAP = [
    //     0, 1, 1, 0, 0   -- i = 0: (0->1, 1->0, 1->0, 0->1, 0->1)
    //     2, 2, 2, 2, 2   -- i = 1: (1->1, 1->1, 1->1, 1->1, 1->1)
    //     3, 4, 4, 3, 3   -- i = 2: (1->2, 2->1, 2->1, 1->2, 1->2)
    //   ]
    //

    graph.compress(3);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{3});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{5});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0, 1, 1, 0, 0,      // i = 0
            2, 2, 2, 2, 2,      // i = 1
            3, 4, 4, 3, 3,      // i = 2
        };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_7x1x1_Two_Disjoint_Merges)
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
    // => CSR: IA = [ 0,      2,        5,        8,        11,       14,        17, 19 ]
    //         JA = [ 0, 1  | 0, 1, 2 | 1, 2, 3 | 2, 3, 4 | 3, 4, 5 | 4, 5, 6 |  5, 6 ]
    //
    // One-pass construction edges:
    // 0->1
    // 1->0
    // 1->2
    // 2->1
    // 2->3
    // 3->2
    // 0->0
    // 1->1
    // 2->2
    // 3->3
    // 3->4
    // 4->3
    // 4->5
    // 5->4
    // 5->6
    // 6->5
    // 4->4
    // 5->5
    // 6->6

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    // Group vertices 1,2 together - they will be merged into a single vertex
    graph.addVertexGroup({0,1, 2});
    // Group vertices 5,6 together - they will be merged into a single vertex
    graph.addVertexGroup({5, 6});

    // +-----+-----+-----+-----+-----+-----+-----+
    // |        0        |  1  |  2  |     3     |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 1  1  0  0 ]
    //    [ 1  1  1  0 ]
    //    [ 0  1  1  1 ]
    //    [ 0  0  1  1 ]
    //
    // => CSR: IA = [ 0,     2,        5,        8,  10 ]
    //         JA = [ 0, 1 | 0, 1, 2 | 1, 2, 3 | 2, 3 ]

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{10});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
                // One-pass construction edges, after merging vertices 1,2 into 0 and 5,6 into 6:
            0,  // 0->0
            0,  // 0->0
            0,  // 0->0
            0,  // 0->0
            1,  // 0->1
            2,  // 1->0
            0,  // 0->0
            0,  // 0->0
            0,  // 0->0
            3,  // 1->1
            4,  // 1->2
            5,  // 2->1
            7,  // 2->3
            8,  // 3->2
            9,  // 3->3
            9,  // 3->3
            6,  // 2->2
            9,  // 3->3
            9,  // 3->3
        };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Linear_7x1x1_Two_Intersecting_Merges)
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
    // => CSR: IA = [ 0,      2,        5,        8,        11,       14,        17, 19 ]
    //         JA = [ 0, 1  | 0, 1, 2 | 1, 2, 3 | 2, 3, 4 | 3, 4, 5 | 4, 5, 6 |  5, 6 ]
    // One-pass construction edges:
    // 0->1
    // 1->0
    // 1->2
    // 2->1
    // 2->3
    // 3->2
    // 0->0
    // 1->1
    // 2->2
    // 3->3
    // 3->4
    // 4->3
    // 4->5
    // 5->4
    // 5->6
    // 6->5
    // 4->4
    // 5->5
    // 6->6

    for (auto i = 0; i < 4 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 0; i < 4; ++i) {
        graph.addConnection(i, i);
    }

    for (auto i = 4 - 1; i < 7 - 1; ++i) {
        graph.addConnection(i, i + 1);
        graph.addConnection(i + 1, i);
    }

    for (auto i = 4; i < 7; ++i) {
        graph.addConnection(i, i);
    }

    // Group vertices 1,2,3 together - they will be merged into a single vertex
    graph.addVertexGroup({1, 2, 3});
    
    // Group vertices 3,4 together - they will be merged into a single vertex
    // Note: Since vertex 3 appears in both groups, these groups will be merged
    graph.addVertexGroup({3, 4});


    // +-----+-----+-----+-----+-----+-----+-----+
    // |  0  |        1              |  2  |  3  |
    // +-----+-----+-----+-----+-----+-----+-----+
    //
    // => Laplacian
    //    [ 1  1  0  0 ]
    //    [ 1  1  1  0 ]
    //    [ 0  1  1  1 ]
    //    [ 0  0  1  1 ]
    //
    // => CSR: IA = [ 0,     2,        5,        8,  10 ]
    //         JA = [ 0, 1 | 0, 1, 2 | 1, 2, 3 | 2, 3 ]

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{10});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
                // One-pass construction edges, after merging vertices 1,2,3 into 1 and 3,4 into 3:
            1,  // 0->1
            2,  // 1->0
            3,  // 1->1
            3,  // 1->1
            3,  // 1->1
            3,  // 1->1
            0,  // 0->0
            3,  // 1->1
            3,  // 1->1
            3,  // 1->1
            3,  // 1->1
            3,  // 1->1
            4,  // 1->2
            5,  // 2->1
            7,  // 2->3
            8,  // 3->2
            3,  // 1->1
            6,  // 2->2
            9,  // 3->3
        };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap .begin(), nzMap .end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Directed_Graph_With_Merge)
{
    auto graph = CSRGraph{};

    // Initial directed graph structure:
    //
    // 0 -----> 1 -----> 2
    //          |        ^
    //          |        |
    //          v        |
    //          3 -----> 4
    //
    // => Laplacian (adjacency matrix for directed graph)
    //    [ 0  1  0  0  0 ]
    //    [ 0  0  1  1  0 ]
    //    [ 0  0  0  0  0 ]
    //    [ 0  0  0  0  1 ]
    //    [ 0  0  1  0  0 ]
    //
    // => CSR: IA = [ 0,   1,     3,  3,  4,  5 ]
    //         JA = [ 1  | 2, 3 |   | 4 | 2 ]

    // Add directed edges
    graph.addConnection(0, 1);  // 0 -> 1
    graph.addConnection(1, 2);  // 1 -> 2
    graph.addConnection(1, 3);  // 1 -> 3
    graph.addConnection(3, 4);  // 3 -> 4
    graph.addConnection(4, 2);  // 4 -> 2

    // Group vertices 3,4 together - they will be merged into a single vertex
    graph.addVertexGroup({3, 4});

    // Merge vertices 3 and 4
    // After merge:
    // 0 -----> 1 -----> 2
    //          |        ^
    //          |        |
    //          v        |
    //          3 -------'
    //
    // => Laplacian (adjacency matrix for directed graph)
    //    [ 0  1  0  0 ]
    //    [ 0  0  1  1 ]
    //    [ 0  0  0  0 ]
    //    [ 0  0  1  1 ]
    //
    // => CSR: IA = [ 0,  1,     3,  3,   5 ]
    //         JA = [ 1 | 2, 3 |   | 2, 3 ]
    //
    // One-pass construction, with tracking, gives
    //   MAP = [
    //     0,  // 0->1
    //     1,  // 1->2
    //     2,  // 1->3
    //     3,  // (3->4 becomes 3->3)
    //     4   // (4->2 becomes 3->2)
    //   ]

    graph.compress(4);

    BOOST_CHECK_EQUAL(graph.numVertices(), std::size_t{4});
    BOOST_CHECK_EQUAL(graph.numEdges(), std::size_t{5});

    {
        const auto& nzMap = graph.compressedIndexMap();
        const auto expect = std::vector {
            0,  // 0->1
            1,  // 1->2
            2,  // 1->3
            4,  // 3->3
            3,  // 3->2
        };
        BOOST_CHECK_EQUAL_COLLECTIONS(nzMap.begin(), nzMap.end(),
                                    expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Tracked

BOOST_AUTO_TEST_SUITE_END()     // Permit_Self_Connections
