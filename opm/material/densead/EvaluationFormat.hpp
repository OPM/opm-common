// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/

#ifndef OPM_EVALUATION_FORMAT_HPP
#define OPM_EVALUATION_FORMAT_HPP

#include <opm/material/densead/Evaluation.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

template<class ValueT, int numDerivs, unsigned staticSize>
struct fmt::formatter<Opm::DenseAd::Evaluation<ValueT,numDerivs,staticSize>>
{
    std::string spec;
    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        auto it = ctx.begin();
        spec = (it != ctx.end() && *it != '}') ? "{:" : "{";
        while (it != ctx.end() && *it != '}')
            spec += *it++;
        spec += '}';
        return it;
    }

    template<class FormatContext>
    auto format(const Opm::DenseAd::Evaluation<ValueT,numDerivs,staticSize>& e,
                FormatContext& ctx)
    {
        std::array<ValueT,numDerivs> tmp;
        for (int i = 0; i < numDerivs; ++i)
            tmp[i] = e.derivative(i);
        return fmt::format_to(ctx.out(), "v: "+ spec +" / d: [" + spec +"]", e.value(), fmt::join(tmp, ", "));
    }
};

#endif // OPM_EVALUATION_FORMAT_HPP
