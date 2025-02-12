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

#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldProps.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace {
    void apply_action(const Opm::Fieldprops::ScalarOperation op,
                      const std::vector<double>& action_data,
                      std::vector<double>& data,
                      const std::size_t action_index,
                      const std::size_t data_index)
    {
        switch (op) {
        case Opm::Fieldprops::ScalarOperation::EQUAL:
            // EQUAL is assignment.
            data[data_index] = action_data[action_index];
            break;

        case Opm::Fieldprops::ScalarOperation::MUL:
            // MUL is scalar multiplication.
            data[data_index] *= action_data[action_index];
            break;

        case Opm::Fieldprops::ScalarOperation::ADD:
            // MUL is scalar addition.
            data[data_index] += action_data[action_index];
            break;

        case Opm::Fieldprops::ScalarOperation::MAX:
            // Recall: MAX is "MAXVALUE", which imposes an upper bound on the
            // data value.  Thus, std::min() is the correct filter operation
            // here despite the name.
            data[data_index] = std::min(action_data[action_index], data[data_index]);
            break;

        case Opm::Fieldprops::ScalarOperation::MIN:
            // Recall: MIN is "MINVALUE", which imposes a lower bound on the
            // data value.  Thus, std::max() is the correct filter operation
            // here despite the name.
            data[data_index] = std::max(action_data[action_index], data[data_index]);
            break;

        default:
            throw std::logic_error {
                fmt::format("Unhandled operation '{}' in apply_action()",
                            static_cast<std::underlying_type_t<Opm::Fieldprops::ScalarOperation>>(op))
            };
        }
    }

} // Anonymous namespace

namespace Opm {

bool FieldPropsManager::operator==(const FieldPropsManager& other) const {
    return *this->fp == *other.fp;
}

bool FieldPropsManager::rst_cmp(const FieldPropsManager& full_arg, const FieldPropsManager& rst_arg)
{
    return FieldProps::rst_cmp(*full_arg.fp, *rst_arg.fp);
}

FieldPropsManager::FieldPropsManager(const Deck& deck, const Phases& phases, EclipseGrid& grid_arg,
                                     const TableManager& tables, const std::size_t ncomps)
    : fp { std::make_shared<FieldProps>(deck, phases, grid_arg, tables, ncomps) }
{}

void FieldPropsManager::deleteMINPVV() {
    this->fp->deleteMINPVV();
}

void FieldPropsManager::reset_actnum(const std::vector<int>& actnum) {
    this->fp->reset_actnum(actnum);
}

bool FieldPropsManager::is_usable() const
{
    return static_cast<bool>(this->fp);
}

void FieldPropsManager::apply_schedule_keywords(const std::vector<DeckKeyword>& keywords) {
    this->fp->handle_schedule_keywords(keywords);
}


template <typename T>
const std::vector<T>& FieldPropsManager::get(const std::string& keyword) const {
    return this->fp->get<T>(keyword);
}


template <typename T>
const std::vector<T>* FieldPropsManager::try_get(const std::string& keyword) const {
    const auto& data = this->fp->try_get<T>(keyword);
    if (data.valid())
        return data.ptr();

    return nullptr;
}

const Fieldprops::FieldData<int>&
FieldPropsManager::get_int_field_data(const std::string& keyword) const
{
    const auto& data = this->fp->try_get<int>(keyword);
    if (!data.valid())
        throw std::out_of_range("Invalid field data requested.");
    return data.field_data();
}

const Fieldprops::FieldData<double>&
FieldPropsManager::get_double_field_data(const std::string& keyword,
                                         bool allow_unsupported) const
{
    const auto flags = allow_unsupported
        ? FieldProps::TryGetFlags::AllowUnsupported
        : 0u;

    const auto& data = this->fp->try_get<double>(keyword, flags);
    if (allow_unsupported || data.valid())
        return data.field_data();

    throw std::out_of_range("Invalid field data requested.");
}

template <typename T>
std::vector<T> FieldPropsManager::get_global(const std::string& keyword) const {
    return this->fp->get_global<T>(keyword);
}

template <typename T>
std::vector<T> FieldPropsManager::get_copy(const std::string& keyword, bool global) const {
    return this->fp->get_copy<T>(keyword, global);
}

template <typename T>
bool FieldPropsManager::supported(const std::string& keyword) {
    return FieldProps::supported<T>(keyword);
}

template <typename T>
bool FieldPropsManager::has(const std::string& keyword) const {
    if (!this->fp->has<T>(keyword))
        return false;
    const auto& data = this->fp->try_get<T>(keyword);
    return data.valid();
}

template <typename T>
std::vector<bool> FieldPropsManager::defaulted(const std::string& keyword) const {
    return this->fp->defaulted<T>(keyword);
}


const std::string& FieldPropsManager::default_region() const {
    return this->fp->default_region();
}

template <typename T>
std::vector<std::string> FieldPropsManager::keys() const {
    return this->fp->keys<T>();
}

std::vector<std::string> FieldPropsManager::fip_regions() const
{
    return this->fp->fip_regions();
}

std::vector<int> FieldPropsManager::actnum() const {
    return this->fp->actnum();
}

std::vector<double> FieldPropsManager::porv(bool global) const {
    const auto& field_data = this->fp->try_get<double>("PORV").field_data();
    if (global)
        return this->fp->global_copy(field_data.data, field_data.kw_info.scalar_init);
    else
        return field_data.data;
}

std::size_t FieldPropsManager::active_size() const {
    return this->fp->active_size;
}

void FieldPropsManager::apply_tran(const std::string& keyword, std::vector<double>& data) const {
    this->fp->apply_tran(keyword, data);
}

void FieldPropsManager::apply_tranz_global(const std::vector<std::size_t>& indices,
                                           std::vector<double>& data) const {
    this->fp->apply_tranz_global(indices, data);
}

bool FieldPropsManager::tran_active(const std::string& keyword) const {
    return this->fp->tran_active(keyword);
}

void FieldPropsManager::apply_numerical_aquifers(const NumericalAquifers& aquifers) {
    return this->fp->apply_numerical_aquifers(aquifers);
}

const std::unordered_map<std::string,Fieldprops::TranCalculator>&
FieldPropsManager::getTran() const
{
    return this->fp->getTran();
}

void FieldPropsManager::prune_global_for_schedule_run()
{
    this->fp->prune_global_for_schedule_run();
}


void FieldPropsManager::set_active_indices(const std::vector<int>& indices)
{
    fp->set_active_indices(indices);
}

template<class MapType>
void apply_tran(const std::unordered_map<std::string, Fieldprops::TranCalculator>& tran,
                const MapType& double_data,
                std::size_t active_size,
                const std::string& keyword, std::vector<double>& data)
{
    const auto& calculator = tran.at(keyword);
    for (const auto& action : calculator) {
        const auto& action_data = double_data.at(action.field);

        for (std::size_t index = 0; index < active_size; index++) {

            if (!value::has_value(action_data.value_status[index]))
                continue;

            apply_action(action.op, action_data.data, data, index, index);
        }
    }
}


template<class MapType>
void apply_tran(const Fieldprops::TranCalculator& calculator,
                const MapType& double_data,
                const std::vector<std::size_t>& indices,
                std::vector<double>& data)
{
    for (const auto& action : calculator) {
        const auto& action_data = double_data.at(action.field);

        for (auto action_index = indices.begin(); action_index != indices.end();
             ++action_index)
        {
            if (!value::has_value((*action_data.global_value_status)[*action_index]))
            {
                continue;
            }

            apply_action(action.op, *action_data.global_data, data, *action_index,
                         action_index-indices.begin());
        }
    }
}
template
void apply_tran(const std::unordered_map<std::string, Fieldprops::TranCalculator>&,
                const std::unordered_map<std::string, Fieldprops::FieldData<double>>&,
                std::size_t, const std::string&, std::vector<double>&);

template
void apply_tran(const std::unordered_map<std::string, Fieldprops::TranCalculator>&,
                const std::map<std::string, Fieldprops::FieldData<double>>&,
                std::size_t, const std::string&, std::vector<double>&);

template
void apply_tran(const Fieldprops::TranCalculator&,
                const std::map<std::string, Fieldprops::FieldData<double>>&,
                const std::vector<std::size_t>&, std::vector<double>&);

template
void apply_tran(const Fieldprops::TranCalculator&,
                const std::unordered_map<std::string, Fieldprops::FieldData<double>>&,
                const std::vector<std::size_t>&, std::vector<double>&);

template bool FieldPropsManager::supported<int>(const std::string&);
template bool FieldPropsManager::supported<double>(const std::string&);

template bool FieldPropsManager::has<int>(const std::string&) const;
template bool FieldPropsManager::has<double>(const std::string&) const;

template std::vector<bool> FieldPropsManager::defaulted<int>(const std::string&) const;
template std::vector<bool> FieldPropsManager::defaulted<double>(const std::string&) const;

template std::vector<std::string> FieldPropsManager::keys<int>() const;
template std::vector<std::string> FieldPropsManager::keys<double>() const;

template std::vector<int> FieldPropsManager::get_global(const std::string& keyword) const;
template std::vector<double> FieldPropsManager::get_global(const std::string& keyword) const;

template const std::vector<int>& FieldPropsManager::get(const std::string& keyword) const;
template const std::vector<double>& FieldPropsManager::get(const std::string& keyword) const;

template std::vector<int> FieldPropsManager::get_copy(const std::string& keyword, bool global) const;
template std::vector<double> FieldPropsManager::get_copy(const std::string& keyword, bool global) const;

template const std::vector<int>* FieldPropsManager::try_get(const std::string& keyword) const;
template const std::vector<double>* FieldPropsManager::try_get(const std::string& keyword) const;

} // namespace Opm
