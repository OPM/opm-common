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

Opm::EclEpsGridProperties::
EclEpsGridProperties(const EclipseState& eclState,
                     const bool          useImbibition)
{
    const auto& fp = eclState.fieldProps();

    auto try_get = [&fp, kwPrefix = useImbibition ? "I" : ""]
        (const std::string& keyword)
    {
        return fp.has_double(kwPrefix + keyword)
            ? &fp.get_double(kwPrefix + keyword)
            : nullptr;
    };

    this->satnum_ = useImbibition
        ? &fp.get_int("IMBNUM")
        : &fp.get_int("SATNUM");

    this->swl_   = try_get("SWL");
    this->sgl_   = try_get("SGL");

    this->swcr_  = try_get("SWCR");
    this->sgcr_  = try_get("SGCR");
    this->sowcr_ = try_get("SOWCR");
    this->sogcr_ = try_get("SOGCR");

    this->swu_   = try_get("SWU");
    this->sgu_   = try_get("SGU");

    this->pcw_   = try_get("PCW");
    this->pcg_   = try_get("PCG");

    this->krw_   = try_get("KRW");
    this->krwr_  = try_get("KRWR");
    this->kro_   = try_get("KRO");
    this->krorg_ = try_get("KRORG");
    this->krorw_ = try_get("KRORW");
    this->krg_   = try_get("KRG");
    this->krgr_  = try_get("KRGR");

    // _may_ be needed to calculate the Leverett capillary pressure scaling factor
    if (fp.has_double("PORO")) {
        this->poro_ = &fp.get_double("PORO");
    }

    this->permx_ = fp.has_double("PERMX")
        ? &fp.get_double("PERMX")
        : nullptr;

    this->permy_ = fp.has_double("PERMY")
        ? &fp.get_double("PERMY")
        : this->permx_;

    this->permz_ = fp.has_double("PERMZ")
        ? &fp.get_double("PERMZ")
        : this->permx_;
}
