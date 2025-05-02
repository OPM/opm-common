/*
  Copyright 2025 Equinor ASA

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

#define BOOST_TEST_MODULE Well_Fracture_Seeds

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Schedule/Well/WellFractureSeeds.hpp>

#include <array>
#include <cstddef>
#include <utility>

using NormalVector = Opm::WellFractureSeeds::NormalVector;

BOOST_AUTO_TEST_SUITE(Insert_Unique)

BOOST_AUTO_TEST_CASE(Single_Seed)
{
    auto seeds = Opm::WellFractureSeeds { "W1" };

    BOOST_CHECK_EQUAL(seeds.name(), "W1");
    BOOST_CHECK_MESSAGE(seeds.empty(), "Default constructed WellFractureSeeds object must be empty");

    BOOST_CHECK_MESSAGE(seeds.updateSeed(1729, NormalVector { 1.0, 0.0, 0.0 }),
                        "Inserting into empty collection must succeed");

    BOOST_CHECK_EQUAL(seeds.numSeeds(), std::size_t{1});

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 1729 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 0.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 0.0, 1.0e-8);
    }

    {
        const auto& c = seeds.seedCells();
        const auto expect = std::vector { std::size_t{1729} };

        BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), expect.begin(), expect.end());
    }

    seeds.finalizeSeeds();

    BOOST_CHECK_EQUAL(seeds.name(), "W1");

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 1729 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 0.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 0.0, 1.0e-8);
    }

    {
        const auto& c = seeds.seedCells();
        const auto expect = std::vector { std::size_t{1729} };

        BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Copy_Constructor)
{
    auto seeds = Opm::WellFractureSeeds { "W1" };
    seeds.updateSeed(1729, NormalVector { 1.0, 0.0, 0.0 });

    const auto s2 = seeds;

    BOOST_CHECK_MESSAGE(seeds == s2, "Copy constructed seeds object must be equal");

    BOOST_CHECK_EQUAL(s2.numSeeds(), std::size_t{1});

    {
        const auto* n = s2.getNormal(Opm::WellFractureSeeds::SeedCell { 1729 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 0.0, 1.0e-8);
    }

    {
        const auto& n = s2.getNormal(Opm::WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 0.0, 1.0e-8);
    }

    {
        const auto& c = s2.seedCells();
        const auto expect = std::vector { std::size_t{1729} };

        BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Move_Constructor)
{
    auto seeds = Opm::WellFractureSeeds { "W1" };
    seeds.updateSeed(1729, NormalVector { 1.0, 0.0, 0.0 });

    const auto s2 = std::move(seeds);

    BOOST_CHECK_EQUAL(s2.numSeeds(), std::size_t{1});

    {
        const auto* n = s2.getNormal(Opm::WellFractureSeeds::SeedCell { 1729 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 0.0, 1.0e-8);
    }

    {
        const auto& n = s2.getNormal(Opm::WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 0.0, 1.0e-8);
    }

    {
        const auto& c = s2.seedCells();
        const auto expect = std::vector { std::size_t{1729} };

        BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Assignment_Operator)
{
    auto seeds = Opm::WellFractureSeeds { "W1" };
    seeds.updateSeed(1729, NormalVector { 1.0, 0.0, 0.0 });

    auto s2 = Opm::WellFractureSeeds { "W2" };
    s2.updateSeed(2718, NormalVector { 0.0, -1.0, 0.0 });

    s2 = seeds;

    BOOST_CHECK_MESSAGE(seeds == s2, "Directly assigned seeds object must be equal");

    BOOST_CHECK_EQUAL(s2.numSeeds(), std::size_t{1});

    {
        const auto* n = s2.getNormal(Opm::WellFractureSeeds::SeedCell { 1729 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 0.0, 1.0e-8);
    }

    {
        const auto& n = s2.getNormal(Opm::WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 0.0, 1.0e-8);
    }

    {
        const auto& c = s2.seedCells();
        const auto expect = std::vector { std::size_t{1729} };

        BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Move_Assignment_Operator)
{
    auto seeds = Opm::WellFractureSeeds { "W1" };
    seeds.updateSeed(1729, NormalVector { 1.0, 0.0, 0.0 });

    auto s2 = Opm::WellFractureSeeds { "W2" };
    s2.updateSeed(2718, NormalVector { 0.0, -1.0, 0.0 });

    s2 = std::move(seeds);

    BOOST_CHECK_EQUAL(s2.numSeeds(), std::size_t{1});

    {
        const auto* n = s2.getNormal(Opm::WellFractureSeeds::SeedCell { 1729 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 0.0, 1.0e-8);
    }

    {
        const auto& n = s2.getNormal(Opm::WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 0.0, 1.0e-8);
    }

    {
        const auto& c = s2.seedCells();
        const auto expect = std::vector { std::size_t{1729} };

        BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Request_Missing_Seed)
{
    auto seeds = Opm::WellFractureSeeds { "W1" };
    seeds.updateSeed(1729, NormalVector { 1.0, 0.0, 0.0 });

    BOOST_CHECK_EQUAL(seeds.numSeeds(), std::size_t{1});

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 271828 });
        BOOST_CHECK_MESSAGE(n == nullptr, "Normal vector in non-existing seed cell must be null");
    }

    seeds.finalizeSeeds();

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 314159 });
        BOOST_CHECK_MESSAGE(n == nullptr, "Normal vector in non-existing seed cell must be null");
    }
}

BOOST_AUTO_TEST_CASE(Multiple_Seeds)
{
    auto seeds = Opm::WellFractureSeeds { "W1" };
    seeds.updateSeed(1729, NormalVector { 1.0, 0.0, 0.0 });
    seeds.updateSeed(1122, NormalVector { 0.0, 1.0, 0.0 });
    seeds.updateSeed(3344, NormalVector { 0.0, 0.0, 1.0 });

    BOOST_CHECK_EQUAL(seeds.numSeeds(), std::size_t{3});

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 1729 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 0.0, 1.0e-8);
    }

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 1122 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 0.0, 1.0e-8);
    }

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 3344 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 1.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 0.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 1 });

        BOOST_CHECK_CLOSE(n[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 0.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 2 });

        BOOST_CHECK_CLOSE(n[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 1.0, 1.0e-8);
    }

    {
        const auto& c = seeds.seedCells();
        const auto expect = std::vector {
            std::size_t{1729},
            std::size_t{1122},
            std::size_t{3344},
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), expect.begin(), expect.end());
    }

    seeds.finalizeSeeds();

    BOOST_CHECK_EQUAL(seeds.name(), "W1");

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 1729 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 0.0, 1.0e-8);
    }

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 1122 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 0.0, 1.0e-8);
    }

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 3344 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 1.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 0.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 1 });

        BOOST_CHECK_CLOSE(n[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 0.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 2 });

        BOOST_CHECK_CLOSE(n[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 1.0, 1.0e-8);
    }

    {
        const auto& c = seeds.seedCells();
        const auto expect = std::vector {
            std::size_t{1729},
            std::size_t{1122},
            std::size_t{3344},
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_SUITE_END() // Insert_Unique

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Insert_Duplicate)

BOOST_AUTO_TEST_CASE(Different_Normal_Vectors)
{
    auto seeds = Opm::WellFractureSeeds { "W1" };
    seeds.updateSeed(1729, NormalVector { 1.0, 0.0, 0.0 });

    BOOST_CHECK_MESSAGE(seeds.updateSeed(1729, NormalVector { 0.0, -1.0, 0.0 }),
                        "Updating seed with different "
                        "normal vector must succeed");

    BOOST_CHECK_EQUAL(seeds.numSeeds(), std::size_t{1});

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 1729 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0],  0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], -1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2],  0.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n[0],  0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], -1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2],  0.0, 1.0e-8);
    }

    {
        const auto& c = seeds.seedCells();
        const auto expect = std::vector { std::size_t{1729} };

        BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), expect.begin(), expect.end());
    }

    seeds.finalizeSeeds();

    BOOST_CHECK_EQUAL(seeds.name(), "W1");

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 1729 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0],  0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], -1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2],  0.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n[0],  0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], -1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2],  0.0, 1.0e-8);
    }

    {
        const auto& c = seeds.seedCells();
        const auto expect = std::vector { std::size_t{1729} };

        BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Same_Normal_Vector)
{
    auto seeds = Opm::WellFractureSeeds { "W1" };
    seeds.updateSeed(1729, NormalVector { 1.0, 0.0, 0.0 });

    BOOST_CHECK_MESSAGE(! seeds.updateSeed(1729, NormalVector { 1.0, 0.0, 0.0 }),
                        "Updating seed with same "
                        "normal vector must NOT succeed");

    BOOST_CHECK_EQUAL(seeds.numSeeds(), std::size_t{1});

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 1729 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 0.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 0.0, 1.0e-8);
    }

    {
        const auto& c = seeds.seedCells();
        const auto expect = std::vector { std::size_t{1729} };

        BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), expect.begin(), expect.end());
    }

    seeds.finalizeSeeds();

    BOOST_CHECK_EQUAL(seeds.name(), "W1");

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 1729 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 0.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n[0], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 0.0, 1.0e-8);
    }

    {
        const auto& c = seeds.seedCells();
        const auto expect = std::vector { std::size_t{1729} };

        BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(Multiple_Seeds_Different_Normals)
{
    auto seeds = Opm::WellFractureSeeds { "W1" };
    seeds.updateSeed(1729, NormalVector { 1.0, 0.0, 0.0 });
    seeds.updateSeed(1122, NormalVector { 0.0, 1.0, 0.0 });
    seeds.updateSeed(3344, NormalVector { 0.0, 0.0, 1.0 });
    seeds.updateSeed(1729, NormalVector { 0.0, 0.0, 1.1 });

    BOOST_CHECK_EQUAL(seeds.numSeeds(), std::size_t{3});

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 1729 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 1.1, 1.0e-8);
    }

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 1122 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 0.0, 1.0e-8);
    }

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 3344 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 1.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 1.1, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 1 });

        BOOST_CHECK_CLOSE(n[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 0.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 2 });

        BOOST_CHECK_CLOSE(n[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 1.0, 1.0e-8);
    }

    {
        const auto& c = seeds.seedCells();
        const auto expect = std::vector {
            std::size_t{1729},
            std::size_t{1122},
            std::size_t{3344},
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), expect.begin(), expect.end());
    }

    seeds.finalizeSeeds();

    BOOST_CHECK_EQUAL(seeds.name(), "W1");

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 1729 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 1.1, 1.0e-8);
    }

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 1122 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 0.0, 1.0e-8);
    }

    {
        const auto* n = seeds.getNormal(Opm::WellFractureSeeds::SeedCell { 3344 });
        BOOST_CHECK_MESSAGE(n != nullptr, "Normal vector in existing seed cell must not be null");

        BOOST_CHECK_CLOSE((*n)[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE((*n)[2], 1.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 1.1, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 1 });

        BOOST_CHECK_CLOSE(n[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 0.0, 1.0e-8);
    }

    {
        const auto& n = seeds.getNormal(Opm::WellFractureSeeds::SeedIndex { 2 });

        BOOST_CHECK_CLOSE(n[0], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[1], 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n[2], 1.0, 1.0e-8);
    }

    {
        const auto& c = seeds.seedCells();
        const auto expect = std::vector {
            std::size_t{1729},
            std::size_t{1122},
            std::size_t{3344},
        };

        BOOST_CHECK_EQUAL_COLLECTIONS(c.begin(), c.end(), expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Insert_Duplicate
