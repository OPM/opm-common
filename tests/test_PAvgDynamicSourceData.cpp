/*
  Copyright (c) 2023 Equinor ASA

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

#define BOOST_TEST_MODULE test_PAvgDynamicSourceData

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Schedule/Well/PAvgDynamicSourceData.hpp>

#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Dynamic_Source_Data)

namespace {
    std::vector<std::size_t> small()
    {
        return { 1, 2, 3, 5, };
    }

    std::vector<std::size_t> repeated()
    {
        return { 1, 2, 3, 5, 3, };
    }

    Opm::PAvgDynamicSourceData<double> smallResult()
    {
        using S = decltype(std::declval<Opm::PAvgDynamicSourceData<double>>()[0]);
        using I = typename S::Item;

        auto src = Opm::PAvgDynamicSourceData<double> { small() };

        src[1].set(I::Pressure, 123.4).set(I::MixtureDensity, 121.2).set(I::PoreVol, 543.21);
        src[2].set(I::Pressure, 12.34).set(I::MixtureDensity, 12.12).set(I::PoreVol, 54.321);
        src[3].set(I::Pressure, 1.234).set(I::MixtureDensity, 1.212).set(I::PoreVol, 5.4321);
        src[5].set(I::Pressure, 1234) .set(I::MixtureDensity, 1212.).set(I::PoreVol, 5432.1);

        return src;
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(Mutable)
{
    using S = decltype(std::declval<Opm::PAvgDynamicSourceData<double>>()[0]);
    using I = typename S::Item;

    auto src = Opm::PAvgDynamicSourceData<double>{ small() };

    src[1].set(I::Pressure, 123.4).set(I::MixtureDensity, 121.2).set(I::PoreVol, 543.21);

    {
        const auto s1 = src[1];
        BOOST_CHECK_CLOSE(s1[I::Pressure], 123.4, 1.0e-10);
        BOOST_CHECK_CLOSE(s1[I::MixtureDensity], 121.2, 1.0e-10);
        BOOST_CHECK_CLOSE(s1[I::PoreVol], 543.21, 1.0e-10);
    }

    {
        const auto s2 = src[2];
        BOOST_CHECK_CLOSE(s2[I::Pressure], 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(s2[I::MixtureDensity], 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(s2[I::PoreVol], 0.0, 1.0e-10);
    }

    src[2].set(I::Pressure, 123.4).set(I::MixtureDensity, 121.2).set(I::PoreVol, 543.21);

    {
        const auto s2 = src[2];
        BOOST_CHECK_CLOSE(s2[I::Pressure], 123.4, 1.0e-10);
        BOOST_CHECK_CLOSE(s2[I::MixtureDensity], 121.2, 1.0e-10);
        BOOST_CHECK_CLOSE(s2[I::PoreVol], 543.21, 1.0e-10);
    }

    BOOST_CHECK_THROW(src[4].set(I::Pressure, 0.5), std::invalid_argument);

    src[5].set(I::Pressure, 123.4).set(I::MixtureDensity, 121.2).set(I::PoreVol, 543.21);

    {
        const auto s5 = src[5];
        BOOST_CHECK_CLOSE(s5[I::Pressure], 123.4, 1.0e-10);
        BOOST_CHECK_CLOSE(s5[I::MixtureDensity], 121.2, 1.0e-10);
        BOOST_CHECK_CLOSE(s5[I::PoreVol], 543.21, 1.0e-10);
    }
}

BOOST_AUTO_TEST_CASE(Immutable)
{
    const auto src = smallResult();

    using S = decltype(src[0]);
    using I = typename S::Item;

    {
        const auto s1 = src[1];
        BOOST_CHECK_CLOSE(s1[I::Pressure], 123.4, 1.0e-10);
        BOOST_CHECK_CLOSE(s1[I::MixtureDensity], 121.2, 1.0e-10);
        BOOST_CHECK_CLOSE(s1[I::PoreVol], 543.21, 1.0e-10);
    }

    {
        const auto s2 = src[2];
        BOOST_CHECK_CLOSE(s2[I::Pressure], 12.34, 1.0e-10);
        BOOST_CHECK_CLOSE(s2[I::MixtureDensity], 12.12, 1.0e-10);
        BOOST_CHECK_CLOSE(s2[I::PoreVol], 54.321, 1.0e-10);
    }

    {
        const auto s3 = src[3];
        BOOST_CHECK_CLOSE(s3[I::Pressure], 1.234, 1.0e-10);
        BOOST_CHECK_CLOSE(s3[I::MixtureDensity], 1.212, 1.0e-10);
        BOOST_CHECK_CLOSE(s3[I::PoreVol], 5.4321, 1.0e-10);
    }

    {
        const auto s5 = src[5];
        BOOST_CHECK_CLOSE(s5[I::Pressure], 1234., 1.0e-10);
        BOOST_CHECK_CLOSE(s5[I::MixtureDensity], 1212., 1.0e-10);
        BOOST_CHECK_CLOSE(s5[I::PoreVol], 5432.1, 1.0e-10);
    }

    BOOST_CHECK_THROW(std::ignore = src[1729], std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Repeated)
{
    BOOST_CHECK_THROW(std::ignore = Opm::PAvgDynamicSourceData<double>{ repeated() },
                      std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()     // Dynamic_Source_Data
