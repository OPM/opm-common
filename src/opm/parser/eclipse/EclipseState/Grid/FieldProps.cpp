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
#include <opm/parser/eclipse/Parser/ParserKeywords/A.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/B.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/E.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/M.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/P.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>

#include "FieldProps.hpp"


namespace Opm {

namespace {

namespace keywords {


static const std::map<std::string, std::string> unit_string = {{"PORO", "1"},
                                                               {"PERMX", "Permeability"},
                                                               {"PERMY", "Permeability"},
                                                               {"PERMZ", "Permeability"},
                                                               {"NTG", "1"},
                                                               {"SWATINIT", "1"}};

static const std::set<std::string> oper_keywords = {"ADD", "EQUALS", "MAXVALUE", "MINVALUE", "MULTIPLY"};
static const std::set<std::string> region_oper_keywords = {"ADDREG", "EQUALREG"};
static const std::set<std::string> box_keywords = {"BOX", "ENDBOX"};
static const std::map<std::string, double> double_scalar_init = {{"NTG", 1}};

static const std::map<std::string, int> int_scalar_init = {{"SATNUM", 1},
                                                           {"FIPNUM", 1}};   // All FIPxxx keywords should (probably) be added with init==1 

/*
bool isFipxxx< int >(const std::string& keyword) {
    // FIPxxxx can be any keyword, e.g. FIPREG or FIPXYZ that has the pattern "FIP.+"
    // However, it can not be FIPOWG as that is an actual keyword.
    if (keyword.size() < 4 || keyword == "FIPOWG") {
        return false;
    }
    return keyword[0] == 'F' && keyword[1] == 'I' && keyword[2] == 'P';
}
*/

namespace GRID {
static const std::set<std::string> double_keywords = {"MULTPV", "NTG", "PORO", "PERMX", "PERMY", "PERMZ", "THCONR"};
static const std::set<std::string> int_keywords    = {"FLUXNUM", "MULTNUM", "OPERNUM", "ROCKNUM"};
static const std::set<std::string> top_keywords    = {"PORO", "PERMX", "PERMY", "PERMZ"};
}

namespace EDIT {
static const std::set<std::string> double_keywords = {"MULTPV"};
static const std::set<std::string> int_keywords = {};
}

namespace PROPS {
static const std::set<std::string> double_keywords = {"SWATINIT"};
static const std::set<std::string> int_keywords = {};
}

namespace REGIONS {
static const std::set<std::string> int_keywords = {"ENDNUM", "EQLNUM", "FIPNUM", "IMBNUM", "MISCNUM", "OPERNUM", "PVTNUM", "ROCKNUM", "SATNUM"};
}

namespace SOLUTION {
static const std::set<std::string> double_keywords = {"PRESSURE", "SWAT", "SGAS", "TEMPI"};
static const std::set<std::string> int_keywords = {};
}

namespace SCHEDULE {
static const std::set<std::string> int_keywords = {"ROCKNUM"};
static const std::set<std::string> double_keywords = {};
}
}

/*
 * The EQUALREG, MULTREG, COPYREG, ... keywords are used to manipulate
 * vectors based on region values; for instance the statement
 *
 *   EQUALREG
 *      PORO  0.25  3    /   -- Region array not specified
 *      PERMX 100   3  F /
 *   /
 *
 * will set the PORO field to 0.25 for all cells in region 3 and the PERMX
 * value to 100 mD for the same cells. The fourth optional argument to the
 * EQUALREG keyword is used to indicate which REGION array should be used
 * for the selection.
 *
 * If the REGION array is not indicated (as in the PORO case) above, the
 * default region to use in the xxxREG keywords depends on the GRIDOPTS
 * keyword:
 *
 *   1. If GRIDOPTS is present, and the NRMULT item is greater than zero,
 *      the xxxREG keywords will default to use the MULTNUM region.
 *
 *   2. If the GRIDOPTS keyword is not present - or the NRMULT item equals
 *      zero, the xxxREG keywords will default to use the FLUXNUM keyword.
 *
 * This quite weird behaviour comes from reading the GRIDOPTS and MULTNUM
 * documentation, and practical experience with ECLIPSE
 * simulations. Ufortunately the documentation of the xxxREG keywords does
 * not confirm this.
 */
std::string default_region_keyword(const Deck& deck) {
    if (deck.hasKeyword("GRIDOPTS")) {
        const auto& gridOpts = deck.getKeyword("GRIDOPTS");
        const auto& record = gridOpts.getRecord(0);
        const auto& nrmult_item = record.getItem("NRMULT");

        if (nrmult_item.get<int>(0) > 0)
            return "MULTNUM"; // GRIDOPTS and positive NRMULT
    }
    return "FLUXNUM";
}



template <typename T>
void assign_deck(const DeckKeyword& keyword, FieldProps::FieldData<T>& field_data, const std::vector<T>& deck_data, const Box& box) {
    if (box.size() != deck_data.size()) {
        const auto& location = keyword.location();
        std::string msg = "Fundamental error with keyword: " + keyword.name() +
                           " at: " + location.filename + ", line: " + std::to_string(location.lineno) +
                           " got " + std::to_string(deck_data.size()) + " elements - expected : " + std::to_string(box.size());
        throw std::invalid_argument(msg);
    }


    for (const auto& cell_index : box.index_list()) {
        field_data.data[cell_index.active_index] = deck_data[cell_index.data_index];
        field_data.value_status[cell_index.active_index] = value::status::deck_value;
    }
}

template <typename T>
void distribute_toplayer(const EclipseGrid& grid, FieldProps::FieldData<T>& field_data, const std::vector<T>& deck_data, const Box& box) {
    const std::size_t layer_size = grid.getNX() * grid.getNY();
    FieldProps::FieldData<double> toplayer(grid.getNX() * grid.getNY());
    for (const auto& cell_index : box.index_list()) {
        if (cell_index.global_index < layer_size) {
            toplayer.data[cell_index.global_index] = deck_data[cell_index.data_index];
            toplayer.value_status[cell_index.global_index] = value::status::deck_value;
        }
    }

    for (std::size_t active_index = 0; active_index < field_data.size(); active_index++) {
        if (field_data.value_status[active_index] == value::status::uninitialized) {
            std::size_t global_index = grid.getGlobalIndex(active_index);
            const auto ijk = grid.getIJK(global_index);
            std::size_t layer_index = ijk[0] + ijk[1] * grid.getNX();
            if (toplayer.value_status[layer_index] == value::status::deck_value) {
                field_data.data[active_index] = toplayer.data[layer_index];
                field_data.value_status[active_index] = value::status::valid_default;
            }
        }
    }
}


template <typename T>
void assign_scalar(FieldProps::FieldData<T>& field_data, T value, const std::vector<Box::cell_index>& index_list) {
    for (const auto& cell_index : index_list) {
        field_data.data[cell_index.active_index] = value;
        field_data.value_status[cell_index.active_index] = value::status::deck_value;
    }
}

template <typename T>
void multiply_scalar(FieldProps::FieldData<T>& field_data, T value, const std::vector<Box::cell_index>& index_list) {
    for (const auto& cell_index : index_list) {
        if (value::has_value(field_data.value_status[cell_index.active_index]))
            field_data.data[cell_index.active_index] *= value;
    }
}

template <typename T>
void add_scalar(FieldProps::FieldData<T>& field_data, T value, const std::vector<Box::cell_index>& index_list) {
    for (const auto& cell_index : index_list) {
        if (value::has_value(field_data.value_status[cell_index.active_index]))
            field_data.data[cell_index.active_index] += value;
    }
}

template <typename T>
void min_value(FieldProps::FieldData<T>& field_data, T min_value, const std::vector<Box::cell_index>& index_list) {
    for (const auto& cell_index : index_list) {
        if (value::has_value(field_data.value_status[cell_index.active_index])) {
            T value = field_data.data[cell_index.active_index];
            field_data.data[cell_index.active_index] = std::max(value, min_value);
        }
    }
}

template <typename T>
void max_value(FieldProps::FieldData<T>& field_data, T max_value, const std::vector<Box::cell_index>& index_list) {
    for (const auto& cell_index : index_list) {
        if (value::has_value(field_data.value_status[cell_index.active_index])) {
            T value = field_data.data[cell_index.active_index];
            field_data.data[cell_index.active_index] = std::min(value, max_value);
        }
    }
}

std::string make_region_name(const std::string& deck_value) {
    if (deck_value == "O")
        return "OPERNUM";

    if (deck_value == "F")
        return "FLUXNUM";

    if (deck_value == "M")
        return "MULTNUM";

    throw std::invalid_argument("The input string: " + deck_value + " was invalid. Expected: O/F/M");
}

FieldProps::ScalarOperation fromString(const std::string& keyword) {
    if (keyword == ParserKeywords::ADD::keywordName || keyword == ParserKeywords::ADDREG::keywordName)
        return FieldProps::ScalarOperation::ADD;

    if (keyword == ParserKeywords::EQUALS::keywordName || keyword == ParserKeywords::EQUALREG::keywordName)
        return FieldProps::ScalarOperation::EQUAL;

    if (keyword == ParserKeywords::MULTIPLY::keywordName || keyword == ParserKeywords::MULTIREG::keywordName)
        return FieldProps::ScalarOperation::MUL;

    if (keyword == ParserKeywords::MINVALUE::keywordName)
        return FieldProps::ScalarOperation::MIN;

    if (keyword == ParserKeywords::MAXVALUE::keywordName)
        return FieldProps::ScalarOperation::MAX;

    throw std::invalid_argument("Keyword operation not recognized");
}


void handle_box_keyword(const DeckKeyword& deckKeyword,  Box& box) {
    if (deckKeyword.name() == ParserKeywords::BOX::keywordName) {
        const auto& record = deckKeyword.getRecord(0);
        box.update(record);
    } else
        box.reset();
}

}



FieldProps::FieldProps(const Deck& deck, const EclipseGrid& grid_arg, const TableManager& table_arg) :
    unit_system(deck.getActiveUnitSystem()),
    grid(std::addressof(grid_arg)),
    tables(table_arg),
    active_size(grid_arg.getNumActive()),
    actnum(grid_arg.getACTNUM()),
    m_default_region(default_region_keyword(deck))
{
    if (Section::hasGRID(deck))
        this->scanGRIDSection(GRIDSection(deck));

    if (Section::hasEDIT(deck))
        this->scanEDITSection(EDITSection(deck));

    if (Section::hasPROPS(deck))
        this->scanPROPSSection(PROPSSection(deck));

    if (Section::hasREGIONS(deck))
        this->scanREGIONSSection(REGIONSSection(deck));

    if (Section::hasSOLUTION(deck))
        this->scanSOLUTIONSection(SOLUTIONSection(deck));
}



void FieldProps::reset_grid(const EclipseGrid& grid_arg) {
    const auto& new_actnum = grid_arg.getACTNUM();
    if (new_actnum == this->actnum)
        return;

    std::vector<bool> active_map(this->active_size, true);
    std::size_t active_index = 0;
    for (std::size_t g = 0; g < this->actnum.size(); g++) {
        if (this->actnum[g] != 0) {
            if (new_actnum[g] == 0)
                active_map[active_index] = false;
            active_index += 1;
        } else {
            if (new_actnum[g] != 0)
                throw std::logic_error("It is not possible to activate cells");
        }
    }

    for (auto& data : this->double_data)
        data.second.compress(active_map);

    for (auto& data : this->int_data)
        data.second.compress(active_map);

    this->grid = std::addressof(grid_arg);
    this->actnum = new_actnum;
}

template <>
bool FieldProps::supported<double>(const std::string& keyword) {
    if (keywords::GRID::double_keywords.count(keyword) != 0)
        return true;

    if (keywords::EDIT::double_keywords.count(keyword) != 0)
        return true;

    if (keywords::PROPS::double_keywords.count(keyword) != 0)
        return true;

    if (keywords::SOLUTION::double_keywords.count(keyword) != 0)
        return true;

    return false;
}

template <>
bool FieldProps::supported<int>(const std::string& keyword) {
    if (keywords::REGIONS::int_keywords.count(keyword) != 0)
        return true;

    if (keywords::GRID::int_keywords.count(keyword) != 0)
        return true;

    if (keywords::SCHEDULE::int_keywords.count(keyword) != 0)
        return true;

    return false;
}

template <>
FieldProps::FieldData<double>& FieldProps::get(const std::string& keyword) {
    auto iter = this->double_data.find(keyword);
    if (iter != this->double_data.end())
        return iter->second;

    if (FieldProps::supported<double>(keyword)) {
        this->double_data[keyword] = FieldData<double>(this->grid->getNumActive());
        auto init_iter = keywords::double_scalar_init.find(keyword);
        if (init_iter != keywords::double_scalar_init.end())
            this->double_data[keyword].default_assign(init_iter->second);

        return this->double_data[keyword];
    } else
        throw std::out_of_range("Double keyword: " + keyword + " is not supported");
}



template <>
FieldProps::FieldData<int>& FieldProps::get(const std::string& keyword) {
    auto iter = this->int_data.find(keyword);
    if (iter != this->int_data.end())
        return iter->second;

    if (FieldProps::supported<int>(keyword)) {
        this->int_data[keyword] = FieldData<int>(this->grid->getNumActive());
        auto init_iter = keywords::int_scalar_init.find(keyword);
        if (init_iter != keywords::int_scalar_init.end())
            this->int_data[keyword].default_assign(init_iter->second);

        return this->int_data[keyword];
    } else
        throw std::out_of_range("Integer keyword " + keyword + " is not supported");
}

std::vector<Box::cell_index> FieldProps::region_index( const DeckItem& region_item, int region_value ) {
    std::string region_name = region_item.defaultApplied(0) ? this->m_default_region : make_region_name(region_item.get<std::string>(0));
    const auto& region = this->get<int>(region_name);
    if (!region.valid())
        throw std::invalid_argument("Trying to work with invalid region: " + region_name);

    std::vector<Box::cell_index> index_list;
    std::size_t active_index = 0;
    const auto& region_data = region.data;
    for (std::size_t g = 0; g < this->actnum.size(); g++) {
        if (this->actnum[g] != 0) {
            if (region_data[active_index] == region_value)
                index_list.emplace_back( g, active_index, g );
            active_index += 1;
        }
    }
    return index_list;
}


template <>
bool FieldProps::has<double>(const std::string& keyword) const {
    return (this->double_data.count(keyword) != 0);
}

template <>
bool FieldProps::has<int>(const std::string& keyword) const {
    return (this->int_data.count(keyword) != 0);
}

template <>
std::vector<std::string> FieldProps::keys<double>() const {
    std::vector<std::string> klist;
    for (const auto& data_pair : this->double_data)
        klist.push_back(data_pair.first);
    return klist;
}

template <>
std::vector<std::string> FieldProps::keys<int>() const {
    std::vector<std::string> klist;
    for (const auto& data_pair : this->int_data)
        klist.push_back(data_pair.first);
    return klist;
}


template <>
void FieldProps::erase<int>(const std::string& keyword) {
    this->int_data.erase(keyword);
}

template <>
void FieldProps::erase<double>(const std::string& keyword) {
    this->double_data.erase(keyword);
}




double FieldProps::getSIValue(const std::string& keyword, double raw_value) const {
    const auto& iter = keywords::unit_string.find(keyword);
    if (iter == keywords::unit_string.end())
        throw std::logic_error("Trying to look up dimension string for keyword: " + keyword);

    const auto& dim_string = iter->second;
    const auto& dim = this->unit_system.parse( dim_string );
    return dim.convertRawToSi(raw_value);
}





void FieldProps::handle_int_keyword(const DeckKeyword& keyword, const Box& box) {
    auto& field_data = this->get<int>(keyword.name());
    const auto& deck_data = keyword.getIntData();
    assign_deck(keyword, field_data, deck_data, box);
}


void FieldProps::handle_double_keyword(const DeckKeyword& keyword, const Box& box) {
    auto& field_data = this->get<double>(keyword.name());
    const auto& deck_data = keyword.getSIDoubleData();
    assign_deck(keyword, field_data, deck_data, box);
}


void FieldProps::handle_grid_section_double_keyword(const DeckKeyword& keyword, const Box& box) {
    auto& field_data = this->get<double>(keyword.name());
    const auto& deck_data = keyword.getSIDoubleData();
    assign_deck(keyword, field_data, deck_data, box);
    if (field_data.valid())
        return;

    if (keywords::GRID::top_keywords.count(keyword.name()) == 1)
        distribute_toplayer(*this->grid, field_data, deck_data, box);
}



template <typename T>
void FieldProps::apply(ScalarOperation op, FieldData<T>& data, T scalar_value, const std::vector<Box::cell_index>& index_list) {
    if (op == ScalarOperation::EQUAL)
        assign_scalar(data, scalar_value, index_list);

    else if (op == ScalarOperation::MUL)
        multiply_scalar(data, scalar_value, index_list);

    else if (op == ScalarOperation::ADD)
        add_scalar(data, scalar_value, index_list);

    else if (op == ScalarOperation::MIN)
        min_value(data, scalar_value, index_list);

    else if (op == ScalarOperation::MAX)
        max_value(data, scalar_value, index_list);
}


void FieldProps::handle_region_operation(const DeckKeyword& keyword) {
    for (const auto& record : keyword) {
        const std::string& target_kw = record.getItem(0).get<std::string>(0);
        int region_value = record.getItem(2).get<int>(0);
        const auto& index_list = this->region_index(record.getItem(3), region_value);

        if (FieldProps::supported<double>(target_kw)) {
            double value = record.getItem(1).get<double>(0);
            if (keyword.name() != ParserKeywords::MULTIPLY::keywordName)
                value = this->getSIValue(target_kw, value);

            auto& field_data = this->get<double>(target_kw);
            FieldProps::apply(fromString(keyword.name()), field_data, value, index_list);
            continue;
        }

        if (FieldProps::supported<int>(target_kw)) {
            continue;
        }

        //throw std::out_of_range("The keyword: " + keyword + " is not supported");
    }
}

/* This can just use a local box - no need for the manager */
void FieldProps::handle_operation(const DeckKeyword& keyword, Box box) {
    for (const auto& record : keyword) {
        const std::string& target_kw = record.getItem(0).get<std::string>(0);
        box.update(record);

        if (FieldProps::supported<double>(target_kw)) {
            double scalar_value = record.getItem(1).get<double>(0);
            if (keyword.name() != ParserKeywords::MULTIPLY::keywordName)
                scalar_value = this->getSIValue(target_kw, scalar_value);

            auto& field_data = this->get<double>(target_kw);
            FieldProps::apply(fromString(keyword.name()), field_data, scalar_value, box.index_list());
            continue;
        }


        if (FieldProps::supported<int>(target_kw)) {
            int scalar_value = static_cast<int>(record.getItem(1).get<double>(0));
            auto& field_data = this->get<int>(target_kw);
            FieldProps::apply(fromString(keyword.name()), field_data, scalar_value, box.index_list());
            continue;
        }

        //throw std::out_of_range("The keyword: " + target_kw + " is not supported");
    }
}


void FieldProps::handle_COPY(const DeckKeyword& keyword, Box box, bool region) {
    for (const auto& record : keyword) {
        const std::string& src_kw = record.getItem(0).get<std::string>(0);
        const std::string& target_kw = record.getItem(1).get<std::string>(0);
        std::vector<Box::cell_index> index_list;

        if (region) {
            int region_value = record.getItem(2).get<int>(0);
            const auto& region_item = record.getItem(4);
            index_list = this->region_index(region_item, region_value);
        } else {
            box.update(record);
            index_list = box.index_list();
        }


        if (FieldProps::supported<double>(src_kw)) {
            const auto * src_data = this->try_get<double>(src_kw);
            if (!src_data)
                throw std::invalid_argument("Tried to copy from not fully initialized keyword: " + src_kw);
            auto& target_data = this->get<double>(target_kw);
            target_data.copy(*src_data, index_list);

            continue;
        }

        if (FieldProps::supported<int>(src_kw)) {
            const auto * src_data = this->try_get<int>(src_kw);
            if (!src_data)
                throw std::invalid_argument("Tried to copy from not fully initialized keyword: " + src_kw);
            auto& target_data = this->get<int>(target_kw);
            target_data.copy(*src_data, index_list);

            continue;
        }
    }
}



void FieldProps::handle_keyword(const DeckKeyword& keyword, Box& box) {
    const std::string& name = keyword.name();

    if (keywords::oper_keywords.count(name) == 1)
        this->handle_operation(keyword, box);

    else if (keywords::region_oper_keywords.count(name) == 1)
        this->handle_region_operation(keyword);

    else if (keywords::box_keywords.count(name) == 1)
        handle_box_keyword(keyword, box);

    else if (name == ParserKeywords::COPY::keywordName)
        handle_COPY(keyword, box, false);

    else if (name == ParserKeywords::COPYREG::keywordName)
        handle_COPY(keyword, box, true);
}

/**********************************************************************/


void FieldProps::scanGRIDSection(const GRIDSection& grid_section) {
    Box box(*this->grid);

    for (const auto& keyword : grid_section) {
        const std::string& name = keyword.name();

        if (keywords::GRID::double_keywords.count(name) == 1) {
            this->handle_grid_section_double_keyword(keyword, box);
            continue;
        }

        if (keywords::GRID::int_keywords.count(name) == 1) {
            this->handle_int_keyword(keyword, box);
            continue;
        }

        this->handle_keyword(keyword, box);
    }
}

void FieldProps::scanEDITSection(const EDITSection& edit_section) {
    Box box(*this->grid);
    for (const auto& keyword : edit_section) {
        const std::string& name = keyword.name();
        if (keywords::EDIT::double_keywords.count(name) == 1) {
            this->handle_double_keyword(keyword, box);
            continue;
        }

        if (keywords::EDIT::int_keywords.count(name) == 1) {
            this->handle_int_keyword(keyword, box);
            continue;
        }

        this->handle_keyword(keyword, box);
    }
}


void FieldProps::scanPROPSSection(const PROPSSection& props_section) {
    Box box(*this->grid);

    for (const auto& keyword : props_section) {
        const std::string& name = keyword.name();
        if (keywords::PROPS::double_keywords.count(name) == 1) {
            this->handle_double_keyword(keyword, box);
            continue;
        }

        if (keywords::PROPS::int_keywords.count(name) == 1) {
            this->handle_int_keyword(keyword, box);
            continue;
        }

        this->handle_keyword(keyword, box);
    }
}


void FieldProps::scanREGIONSSection(const REGIONSSection& regions_section) {
    Box box(*this->grid);

    for (const auto& keyword : regions_section) {
        const std::string& name = keyword.name();
        if (keywords::REGIONS::int_keywords.count(name) == 1) {
            this->handle_int_keyword(keyword, box);
            continue;
        }

        this->handle_keyword(keyword, box);
    }
}


void FieldProps::scanSOLUTIONSection(const SOLUTIONSection& solution_section) {
    Box box(*this->grid);
    for (const auto& keyword : solution_section) {
        const std::string& name = keyword.name();
        if (keywords::SOLUTION::double_keywords.count(name) == 1) {
            this->handle_double_keyword(keyword, box);
            continue;
        }

        this->handle_keyword(keyword, box);
    }
}

void FieldProps::scanSCHEDULESection(const SCHEDULESection& schedule_section) {
    Box box(*this->grid);
    for (const auto& keyword : schedule_section) {
        const std::string& name = keyword.name();
        if (keywords::SCHEDULE::double_keywords.count(name) == 1) {
            this->handle_double_keyword(keyword, box);
            continue;
        }

        if (keywords::SCHEDULE::int_keywords.count(name) == 1) {
            this->handle_int_keyword(keyword, box);
            continue;
        }

        this->handle_keyword(keyword, box);
    }
}

const std::string& FieldProps::default_region() const {
    return this->m_default_region;
}

template std::vector<bool> FieldProps::defaulted<int>(const std::string& keyword);
template std::vector<bool> FieldProps::defaulted<double>(const std::string& keyword);

}
