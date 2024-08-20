/*
  Copyright 2024 Equinor AS.

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

#include <opm/input/eclipse/EclipseState/Grid/FieldData.hpp>

#include <opm/common/OpmLog/KeywordLocation.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/EclipseState/Grid/Box.hpp>

#include <opm/input/eclipse/Deck/value_status.hpp>

#include <string>
#include <vector>

#include <fmt/format.h>

template <typename T>
void
Opm::Fieldprops::FieldData<T>::
checkInitialisedCopy(const FieldData&                    src,
                     const std::vector<Box::cell_index>& index_list,
                     const std::string&                  from,
                     const std::string&                  to,
                     const KeywordLocation&              loc)
{
    auto unInit = 0;

    for (const auto& ci : index_list) {
        const auto ix = ci.active_index;
        const auto st = src.value_status[ix];

        if (st != value::status::deck_value) {
            ++unInit;
            continue;
        }

        this->data[ix] = src.data[ix];
        this->value_status[ix] = st;
    }

    if (unInit > 0) {
        const auto* plural = (unInit > 1) ? "s" : "";

        throw OpmInputError {
            fmt::format
            (R"(Copying from source array {0}
would generate an undefined result in {2} block{3} of target array {1}.)",
             from, to, unInit, plural),
            loc
        };
    }
}

template
void Opm::Fieldprops::FieldData<double>::
checkInitialisedCopy(const FieldData&,
                     const std::vector<Box::cell_index>&,
                     const std::string&,
                     const std::string&,
                     const KeywordLocation&);

template
void Opm::Fieldprops::FieldData<int>::
checkInitialisedCopy(const FieldData&,
                     const std::vector<Box::cell_index>&,
                     const std::string&,
                     const std::string&,
                     const KeywordLocation&);
