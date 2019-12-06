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

#ifndef FIELDPROPS_HPP
#define FIELDPROPS_HPP

#include <string>
#include <unordered_set>

#include <opm/parser/eclipse/Deck/value_status.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>

namespace Opm {

class Deck;
class EclipseGrid;
class TableManager;

class FieldProps {
public:

    enum class ScalarOperation {
         ADD = 1,
         EQUAL = 2,
         MUL = 3,
         MIN = 4,
         MAX = 5
    };


    template<typename T>
    struct FieldData {
        std::vector<T> data;
        std::vector<value::status> value_status;


        FieldData() = default;

        FieldData(std::size_t active_size) :
            data(std::vector<T>(active_size)),
            value_status(active_size, value::status::uninitialized)
        {
        }


        std::size_t size() const {
            return this->data.size();
        }

        bool valid() const {
            static const std::array<value::status,2> invalid_value = {value::status::uninitialized, value::status::empty_default};
            const auto& it = std::find_first_of(this->value_status.begin(), this->value_status.end(), invalid_value.begin(), invalid_value.end());
            if (it == this->value_status.end())
                return true;

            return false;
        }


        void compress(const std::vector<bool>& active_map) {
            std::size_t shift = 0;
            for (std::size_t g = 0; g < active_map.size(); g++) {
                if (active_map[g] && shift > 0) {
                    this->data[g - shift] = this->data[g];
                    this->value_status[g - shift] = this->value_status[g];
                    continue;
                }

                if (!active_map[g])
                    shift += 1;
            }

            this->data.resize(this->data.size() - shift);
            this->value_status.resize(this->value_status.size() - shift);
        }

        void copy(const FieldData<T>& src, const std::vector<Box::cell_index>& index_list) {
            for (const auto& ci : index_list) {
                this->data[ci.active_index] = src.data[ci.active_index];
                this->value_status[ci.active_index] = src.value_status[ci.active_index];
            }
        }

        void default_assign(T value) {
            std::fill(this->data.begin(), this->data.end(), value);
            std::fill(this->value_status.begin(), this->value_status.end(), value::status::valid_default);
        }

        void default_assign(const std::vector<T>& src) {
            if (src.size() != this->size())
                throw std::invalid_argument("Size mismatch got: " + std::to_string(src.size()) + " expected: " + std::to_string(this->size()));

            std::copy(src.begin(), src.end(), this->data.begin());
            std::fill(this->value_status.begin(), this->value_status.end(), value::status::valid_default);
        }

        void default_update(const std::vector<T>& src) {
            if (src.size() != this->size())
                throw std::invalid_argument("Size mismatch got: " + std::to_string(src.size()) + " expected: " + std::to_string(this->size()));

            for (std::size_t i = 0; i < src.size(); i++) {
                if (!value::has_value(this->value_status[i])) {
                    this->value_status[i] = value::status::valid_default;
                    this->data[i] = src[i];
                }
            }
        }

        void update(std::size_t index, T value, value::status status) {
            this->data[index] = value;
            this->value_status[index] = status;
        }

    };


    FieldProps(const Deck& deck, const EclipseGrid& grid, const TableManager& table_arg);
    void reset_grid(const EclipseGrid& grid);

    const std::string& default_region() const;

    std::vector<int> actnum();
    std::vector<double> porv(bool global);

    template <typename T>
    FieldData<T>& get(const std::string& keyword);

    template <typename T>
    static bool supported(const std::string& keyword);

    template <typename T>
    bool has(const std::string& keyword) const;

    template <typename T>
    std::vector<std::string> keys() const;

    template <typename T>
    const std::vector<T>& get_valid_data(const std::string& keyword) {
        const auto& field_ptr = this->try_get<T>(keyword);
        if (field_ptr)
            return field_ptr->data;
        else
            throw std::invalid_argument("No such valid keyword: " + keyword);
    }

    template <typename T>
    const FieldData<T>* try_get(const std::string& keyword) {
        const FieldData<T> * field_data;
        bool has0 = this->has<T>(keyword);

        try {
            field_data = std::addressof(this->get<T>(keyword));
        } catch (const std::out_of_range&) {
            return nullptr;
        }

        if (field_data->valid())
            return field_data;

        if (!has0)
            this->erase<T>(keyword);

        return nullptr;
    }

    template <typename T>
    std::vector<T> global_copy(const std::vector<T>& data) const {
        std::vector<T> global_data(this->global_size);
        std::size_t i = 0;
        for (std::size_t g = 0; g < this->global_size; g++) {
            if (this->m_actnum[g]) {
                global_data[g] = data[i];
                i++;
            }
        }
        return global_data;
    }


    template <typename T>
    std::vector<bool> defaulted(const std::string& keyword) {
        const auto& field = this->get<T>(keyword);
        std::vector<bool> def(field.size());

        for (std::size_t i=0; i < def.size(); i++)
            def[i] = value::defaulted( field.value_status[i]);

        return def;
    }


private:
    void scanGRIDSection(const GRIDSection& grid_section, const EclipseGrid& grid);
    void scanEDITSection(const EDITSection& edit_section, const EclipseGrid& grid);
    void scanPROPSSection(const PROPSSection& props_section, const EclipseGrid& grid, const TableManager& tables);
    void scanREGIONSSection(const REGIONSSection& regions_section, const EclipseGrid& grid);
    void scanSOLUTIONSection(const SOLUTIONSection& solution_section, const EclipseGrid& grid);
    void scanSCHEDULESection(const SCHEDULESection& schedule_section, const EclipseGrid& grid);
    double getSIValue(const std::string& keyword, double raw_value) const;
    template <typename T>
    void erase(const std::string& keyword);


    template <typename T>
    static void apply(ScalarOperation op, FieldData<T>& data, T scalar_value, const std::vector<Box::cell_index>& index_list);
    std::vector<Box::cell_index> region_index( const DeckItem& regionItem, int region_value );
    void handle_operation(const DeckKeyword& keyword, Box box);
    void handle_region_operation(const DeckKeyword& keyword);
    void handle_COPY(const DeckKeyword& keyword, Box box, bool region);
    void distribute_toplayer(FieldProps::FieldData<double>& field_data, const std::vector<double>& deck_data, const Box& box);

    void handle_keyword(const DeckKeyword& keyword, Box& box);
    void handle_grid_section_double_keyword(const DeckKeyword& keyword, const Box& box);
    void handle_double_keyword(const DeckKeyword& keyword, const Box& box);
    void handle_int_keyword(const DeckKeyword& keyword, const Box& box);

    const UnitSystem unit_system;
    std::size_t active_size;
    std::size_t global_size;
    std::size_t nx,ny,nz;
    std::vector<int> m_actnum;
    std::vector<double> cell_volume;
    const std::string m_default_region;
    std::unordered_map<std::string, FieldData<int>> int_data;
    std::unordered_map<std::string, FieldData<double>> double_data;

    // PORV calculation is quite expensive so this is cached
    std::unique_ptr<std::vector<double>> porv_ptr;
};

}
#endif

