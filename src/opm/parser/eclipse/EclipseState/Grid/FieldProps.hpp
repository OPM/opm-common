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

#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/BoxManager.hpp>

namespace Opm {

class Deck;
class EclipseGrid;

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
        std::vector<bool> assigned;


        FieldData() = default;

        FieldData(std::size_t active_size) :
            data(std::vector<T>(active_size)),
            assigned(std::vector<bool>(active_size, false))
        {
        }


        std::size_t size() const {
            return this->data.size();
        }

        bool valid() const {
            const auto& it = std::find(this->assigned.begin(), this->assigned.end(), false);
            if (it == this->assigned.end())
                return true;

            return false;
        }


        void compress(const std::vector<bool>& active_map) {
            std::size_t shift = 0;
            for (std::size_t g = 0; g < active_map.size(); g++) {
                if (active_map[g] && shift > 0) {
                    this->data[g - shift] = this->data[g];
                    this->assigned[g - shift] = this->assigned[g];
                    continue;
                }

                if (!active_map[g])
                    shift += 1;
            }

            this->data.resize(this->data.size() - shift);
            this->assigned.resize(this->assigned.size() - shift);
        }

        void assign(T value) {
            std::fill(this->data.begin(), this->data.end(), value);
            std::fill(this->assigned.begin(), this->assigned.end(), true);
        }

        void assign(const FieldData<T>& src, const Box& box) {
            for (const auto& ci : box.index_list()) {
                if (src.assigned[ci.active_index]) {
                    this->data[ci.active_index] = src.data[ci.active_index];
                    this->assigned[ci.active_index] = true;
                }
            }
        }

    };


    FieldProps(const Deck& deck, const EclipseGrid& grid);
    void reset_grid(const EclipseGrid& grid);

    template <typename T>
    FieldData<T>& get(const std::string& keyword);

    template <typename T>
    static bool supported(const std::string& keyword);

    template <typename T>
    bool has(const std::string& keyword) const;

    template <typename T>
    const FieldData<T>* try_get(const std::string& keyword) {
        const FieldData<T> * field_data;

        try {
            field_data = std::addressof(this->get<T>(keyword));
        } catch (const std::out_of_range&) {
            return nullptr;
        }

        if (field_data->valid())
            return field_data;

        this->erase<T>(keyword);
        return nullptr;
    }

    template <typename T>
    std::vector<T> global_copy(const std::vector<T>& data) const {
        std::vector<T> global_data(this->grid->getCartesianSize());
        std::size_t i = 0;
        for (std::size_t g = 0; g < this->grid->getCartesianSize(); g++) {
            if (this->grid->cellActive(g)) {
                global_data[g] = data[i];
                i++;
            }
        }
        return global_data;
    }


private:
    void scanGRIDSection(const GRIDSection& grid_section);
    void scanPROPSSection(const PROPSSection& props_section);
    void scanREGIONSSection(const REGIONSSection& regions_section);
    double getSIValue(const std::string& keyword, double raw_value) const;
    template <typename T>
    void erase(const std::string& keyword);

    template <typename T>
    static void apply(ScalarOperation op, FieldData<T>& data, T scalar_value, const std::vector<Box::cell_index>& index_list);
    std::vector<Box::cell_index> region_index( const DeckItem& regionItem, int region_value );
    void handle_operation(const DeckKeyword& keyword, BoxManager& box_manager);
    void handle_region_operation(const DeckKeyword& keyword);
    void handle_props_section_double_keyword(const DeckKeyword& keyword, const BoxManager& box_manager);
    void handle_grid_section_double_keyword(const DeckKeyword& keyword, const BoxManager& box_manager);
    void handle_int_keyword(const DeckKeyword& keyword, const BoxManager& box_manager);
    void handle_COPY(const DeckKeyword& keyword, BoxManager& box_manager);

    const UnitSystem unit_system;
    const EclipseGrid* grid;   // A reseatable pointer to const.
    std::size_t active_size;
    std::vector<int> actnum;
    const std::string default_region;
    std::unordered_map<std::string, FieldData<int>> int_data;
    std::unordered_map<std::string, FieldData<double>> double_data;
};

}
#endif
