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

#include <config.h>
#include <opm/material/fluidmatrixinteractions/EclEpsGridProperties.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>

#include <string>

namespace Opm {

EclEpsGridProperties::EclEpsGridProperties(const EclipseState& eclState,
                                           bool useImbibition)
{
    const std::string kwPrefix = useImbibition ? "I" : "";

    const auto& fp = eclState.fieldProps();
    std::vector<double> empty;
    auto try_get = [&fp,&empty](const std::string& keyword) -> const std::vector<double>&
    {
        if (fp.has_double(keyword))
            return fp.get_double(keyword);

        return empty;
    };

    compressed_satnum = useImbibition
        ? fp.get_int("IMBNUM") : fp.get_int("SATNUM");

    this->compressed_swl = try_get(kwPrefix + "SWL");
    this->compressed_sgl = try_get(kwPrefix + "SGL");
    this->compressed_swcr = try_get(kwPrefix + "SWCR");
    this->compressed_sgcr = try_get(kwPrefix + "SGCR");
    this->compressed_sowcr = try_get(kwPrefix + "SOWCR");
    this->compressed_sogcr = try_get(kwPrefix + "SOGCR");
    this->compressed_swu = try_get(kwPrefix + "SWU");
    this->compressed_sgu = try_get(kwPrefix + "SGU");
    this->compressed_pcw = try_get(kwPrefix + "PCW");
    this->compressed_pcg = try_get(kwPrefix + "PCG");
    this->compressed_krw = try_get(kwPrefix + "KRW");
    this->compressed_krwr = try_get(kwPrefix + "KRWR");
    this->compressed_kro = try_get(kwPrefix + "KRO");
    this->compressed_krorg = try_get(kwPrefix + "KRORG");
    this->compressed_krorw = try_get(kwPrefix + "KRORW");
    this->compressed_krg = try_get(kwPrefix + "KRG");
    this->compressed_krgr = try_get(kwPrefix + "KRGR");

    // _may_ be needed to calculate the Leverett capillary pressure scaling factor
    if (fp.has_double("PORO"))
        this->compressed_poro = fp.get_double("PORO");

    this->compressed_permx = fp.has_double("PERMX")
        ? fp.get_double("PERMX")
        : std::vector<double>(this->compressed_satnum.size());

    this->compressed_permy = fp.has_double("PERMY")
        ? fp.get_double("PERMY") : this->compressed_permx;

    this->compressed_permz = fp.has_double("PERMZ")
        ? fp.get_double("PERMZ") : this->compressed_permx;
}

}

