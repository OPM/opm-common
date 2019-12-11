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
class TableManager;

class FieldPropsManager {

struct MemInfo {
    std::size_t global_size;
    std::size_t active_size;
    std::size_t int_fields;
    std::size_t double_fields;
    std::size_t total;

    MemInfo(std::size_t gsize, std::size_t asize, std::size_t num_int, std::size_t num_double) :
        global_size(gsize),
        active_size(asize),
        int_fields(num_int),
        double_fields(num_double),
        total(asize * sizeof(int) * num_int +                          // The integer fields like SATNUM and PVTNUM
              asize * sizeof(double) * num_double +                    // The double fields like PORO and SWATINIT
              asize * sizeof(double) * 2 +                             // Depth and volume of all active cells
              asize * sizeof(unsigned char) * (num_int + num_double) + // The per cell value status flag
              gsize * sizeof(int))                                     // The global ACTNUM mapping
    {
    };


};



public:
    // The default constructor should be removed when the FieldPropsManager is mandatory
    // The default constructed fieldProps object is **NOT** usable
    FieldPropsManager() = default;
    FieldPropsManager(const Deck& deck, const EclipseGrid& grid, const TableManager& tables);
    void reset_actnum(const std::vector<int>& actnum);
    const std::string& default_region() const;
    std::vector<int> actnum() const;
    std::vector<double> porv(bool global = false) const;
    MemInfo meminfo( ) const;

    /*
      Because the FieldProps class can autocreate properties the semantics of
      get() and has() is slightly non intuitve:

      - The has<T>("KW") method will check if the current FieldProps container
        has an installed "KW" keyword; if the container has the keyword in
        question it will check if all elements have been assigned a value - only
        in that case will it return true. The has<T>("KW") method will *not* try
        to create a new keyword.

      - The has<T>("KW") method will *not* consult the supported<T>("KW")
        method; i.e. if you ask has<T>("UNKNOWN_KEYWORD") you will just get a
        false.

      - The get<T>("KW") method will try to create a new keyword if it does not
        already have the keyword you are asking for. This implies that you can
        get the following non intuitive sequence of events:

            FieldPropsManager fpm(deck, grid);

            fpm.has<int>("SATNUM");                => false
            auto satnum = fpm.get<int>("SATNUM");  => SATNUM is autocreated
            fpm.has<int>("SATNUM");                => true

      - When checking whether the container has the keyword you should rephrase
        the question slightly:

        * Does the container have the keyword *right now* => has<T>("KW")
        * Can the container provide the keyword => ptr = try_get<T>("KW")

      - It is quite simple to create a deck where the keywords are only partly
        initialized, all the methods in the FieldPropsManager only consider
        fully initialized keywords.
     */


    template <typename T>
    const std::vector<T>& get(const std::string& keyword) const;

    template <typename T>
    const std::vector<T>* try_get(const std::string& keyword) const;

    template <typename T>
    bool has(const std::string& keyword) const;

    template <typename T>
    std::vector<std::string> keys() const;

    template <typename T>
    std::vector<T> get_global(const std::string& keyword) const;

    template <typename T>
    std::vector<bool> defaulted(const std::string& keyword) const;

    template <typename T>
    static bool supported(const std::string& keyword);

private:
    std::shared_ptr<FieldProps> fp;
};

}

#endif
