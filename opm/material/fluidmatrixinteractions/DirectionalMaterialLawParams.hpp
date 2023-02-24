/*
  Copyright 2022 Equinor ASA.

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
/*!
 * \file
 *
 * \brief This file contains definitions related to directional material law parameters
 */
#ifndef OPM_DIRECTIONAL_MATERIAL_LAW_PARAMS_HH
#define OPM_DIRECTIONAL_MATERIAL_LAW_PARAMS_HH

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace Opm {

template <class MaterialLawParams>
struct DirectionalMaterialLawParams {
    using vector_type = std::vector<MaterialLawParams>;
    DirectionalMaterialLawParams()
        : materialLawParamsX_{}
        , materialLawParamsY_{}
        , materialLawParamsZ_{}
    {}

    DirectionalMaterialLawParams(std::size_t size)
        : materialLawParamsX_(size)
        , materialLawParamsY_(size)
        , materialLawParamsZ_(size)
    {}

    vector_type& getArray(int index)
    {
        switch(index) {
            case 0:
                return materialLawParamsX_;
            case 1:
                return materialLawParamsY_;
            case 2:
                return materialLawParamsZ_;
            default:
                throw std::runtime_error("Unexpected mobility array index");
        }
    }

    vector_type materialLawParamsX_;
    vector_type materialLawParamsY_;
    vector_type materialLawParamsZ_;
};

} // namespace Opm

#endif
