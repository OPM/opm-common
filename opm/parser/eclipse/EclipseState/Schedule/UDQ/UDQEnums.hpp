/*
  Copyright 2019 Equinor ASA.

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


#ifndef UDQ_ENUMS_HPP
#define UDQ_ENUMS_HPP

namespace Opm {

enum class UDQVarType {
    NONE = 0,
    SCALAR = 1,
    WELL_VAR = 2,
    CONNECTION_VAR= 3,
    FIELD_VAR = 4,
    GROUP_VAR = 5,
    REGION_VAR = 6,
    SEGMENT_VAR = 7,
    AQUIFER_VAR = 8,
    BLOCK_VAR = 9
};



enum class UDQTokenType{
    error = 0,
    number = 1,
    open_paren = 2,
    close_paren = 3,
    ecl_expr = 7,
    //
    binary_op_add = 8,
    binary_op_sub = 9,
    binary_op_div = 10,
    binary_op_mul = 11,
    binary_op_pow = 12,
    binary_op_uadd = 13,
    binary_op_umul = 14,
    binary_op_umin = 15,
    binary_op_umax = 16,
    binary_cmp_eq  = 17,
    binary_cmp_ne  = 18,
    binary_cmp_le  = 19,
    binary_cmp_ge  = 20,
    binary_cmp_lt  = 21,
    binary_cmp_gt  = 22,
    //
    elemental_func_randn = 23,
    elemental_func_randu = 24,
    elemental_func_rrandn = 25,
    elemental_func_rrandu = 26,
    elemental_func_abs = 27,
    elemental_func_def = 28,
    elemental_func_exp = 29,
    elemental_func_idv = 30,
    elemental_func_ln = 31,
    elemental_func_log = 32,
    elemental_func_nint = 33,
    elemental_func_sorta = 34,
    elemental_func_sortd = 35,
    elemental_func_undef = 36,
    //
    scalar_func_sum = 37,
    scalar_func_avea = 38,
    scalar_func_aveg = 39,
    scalar_func_aveh = 40,
    scalar_func_max = 41,
    scalar_func_min = 42,
    scalar_func_norm1 = 43,
    scalar_func_norm2 = 44,
    scalar_func_normi = 45,
    scalar_func_prod = 46,
    //
    table_lookup = 47,
    //
    end = 100
};


enum class UDQAction {
    ASSIGN,
    DEFINE,
    UNITS,
    UPDATE
};


enum class UDAControl {
    WCONPROD_ORAT,
    WCONPROD_GRAT,
    WCONPROD_WRAT,
    WCONPROD_LRAT,
    WCONPROD_RESV,
    WCONPROD_BHP,
    WCONPROD_THP,
    //
    WCONINJE_RATE,
    WCONINJE_RESV,
    WCONINJE_BHP,
    WCONINJE_THP,
    //
    GCONPROD_OIL_TARGET,
    GCONPROD_WATER_TARGET,
    GCONPROD_GAS_TARGET,
    GCONPROD_LIQUID_TARGET
};


namespace UDQ {

    bool compatibleTypes(UDQVarType lhs, UDQVarType rhs);
    UDQVarType targetType(const std::string& keyword);
    UDQVarType varType(const std::string& keyword);
    UDQAction actionType(const std::string& action_string);
    UDQTokenType funcType(const std::string& func_name);
    bool binaryFunc(UDQTokenType token_type);
    bool elementalUnaryFunc(UDQTokenType token_type);
    bool scalarFunc(UDQTokenType token_type);
    bool cmpFunc(UDQTokenType token_type);

    std::string typeName(UDQVarType var_type);
}
}

#endif
