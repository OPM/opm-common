/*
  Copyright 2019  Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the terms
  of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  OPM is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FIELDPROPS_MANAGER_HPP
#define FIELDPROPS_MANAGER_HPP

#include <memory>

namespace Opm {

class EclipseGrid;
class Deck;
class FieldProps;

class FieldPropsManager {
public:
    FieldPropsManager(const Deck& deck, const EclipseGrid& grid);
    void reset_grid(const EclipseGrid& grid);

    template <typename T>
    const std::vector<T>& get(const std::string& keyword) const;

    template <typename T>
    const std::vector<T>* try_get(const std::string& keyword) const;

    template <typename T>
    std::vector<T> get_global(const std::string& keyword) const;

    template <typename T>
    static bool supported(const std::string& keyword);


private:
    std::shared_ptr<FieldProps> fp;
};

}

#endif
