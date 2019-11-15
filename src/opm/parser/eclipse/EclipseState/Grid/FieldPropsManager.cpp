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


#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FieldPropsManager.hpp>

#include "FieldProps.hpp"


namespace Opm {

FieldPropsManager::FieldPropsManager(const Deck& deck, const EclipseGrid& grid_arg, const TableManager& tables) :
    fp(std::make_shared<FieldProps>(deck, grid_arg, tables))
{}

void FieldPropsManager::reset_grid(const EclipseGrid& grid) {
    this->fp->reset_grid(grid);
}

template <typename T>
const std::vector<T>& FieldPropsManager::get(const std::string& keyword) const {
    const auto& data_ptr = this->try_get<T>(keyword);
    if (data_ptr)
        return *data_ptr;

    if (!this->fp->has<T>(keyword))
        throw std::out_of_range("No such keyword in deck: " + keyword);

    throw std::logic_error("Internal error - should not be here");
}


template <typename T>
const std::vector<T>* FieldPropsManager::try_get(const std::string& keyword) const {
    const auto& data_ptr = this->fp->try_get<T>(keyword);
    if (data_ptr)
        return std::addressof(data_ptr->data);

    if (!FieldProps::supported<T>(keyword))
        throw std::invalid_argument("The keyword: " + keyword + " is not supported");

    return nullptr;
}


template <typename T>
std::vector<T> FieldPropsManager::get_global(const std::string& keyword) const {
    const auto& data = this->get<T>(keyword);
    return this->fp->global_copy(data);
}

template <typename T>
bool FieldPropsManager::supported(const std::string& keyword) {
    return FieldProps::supported<T>(keyword);
}


template <typename T>
bool FieldPropsManager::has(const std::string& keyword) const {
    if (!this->fp->has<T>(keyword))
        return false;

    const auto& data_ptr = this->fp->get<T>(keyword);
    return data_ptr.valid();
}

template <typename T>
std::vector<bool> FieldPropsManager::defaulted(const std::string& keyword) const {
    return this->fp->defaulted<T>(keyword);
}


const std::string& FieldPropsManager::default_region() const {
    return this->fp->default_region();
}

template bool FieldPropsManager::supported<int>(const std::string&);
template bool FieldPropsManager::supported<double>(const std::string&);

template bool FieldPropsManager::has<int>(const std::string&) const;
template bool FieldPropsManager::has<double>(const std::string&) const;

template std::vector<bool> FieldPropsManager::defaulted<int>(const std::string&) const;
template std::vector<bool> FieldPropsManager::defaulted<double>(const std::string&) const;

template std::vector<int> FieldPropsManager::get_global(const std::string& keyword) const;
template std::vector<double> FieldPropsManager::get_global(const std::string& keyword) const;

template const std::vector<int>& FieldPropsManager::get(const std::string& keyword) const;
template const std::vector<double>& FieldPropsManager::get(const std::string& keyword) const;

template const std::vector<int>* FieldPropsManager::try_get(const std::string& keyword) const;
template const std::vector<double>* FieldPropsManager::try_get(const std::string& keyword) const;

}
