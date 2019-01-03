/*
  Copyright 2015 Statoil ASA.

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

#ifndef OPM_SUMMARY_CONFIG_HPP
#define OPM_SUMMARY_CONFIG_HPP

#include <array>
#include <vector>
#include <set>

#include <ert/ecl/smspec_node.hpp>

namespace Opm {

    /*
      Very small utility class to get value semantics on the smspec_node
      pointers. This should die as soon as the smspec_node class proper gets
      value semantics.
    */

    class SummaryNode {
    public:
        SummaryNode(const std::string& keyword) :
            ecl_node(0, keyword.c_str(), "UNIT", 0.0)
        {}


        SummaryNode(const std::string& keyword, const std::string& wgname) :
            ecl_node(0, keyword.c_str(), wgname.c_str(), "UNIT", 0.0, ":")
        {}

        SummaryNode(const std::string& keyword, const std::string& wgname, int num) :
            ecl_node(0, keyword.c_str(), wgname.c_str(), num, "UNIT", 0.0, ":")
        {}

        SummaryNode(const std::string& keyword, int num) :
            ecl_node(0, keyword.c_str(), num, "UNIT", 0, ":")
        {}

        SummaryNode(const std::string& keyword, int num, const int grid_dims[3]) :
            ecl_node(0, keyword.c_str(), num, "UNIT", grid_dims, 0, ":")
        {}

        SummaryNode(const std::string& keyword, const std::string& wgname, int num, const int grid_dims[3]) :
            ecl_node(0, keyword.c_str(), wgname.c_str(), num, "UNIT", grid_dims, 0, ":")
        {}

        std::string wgname() const {
            const char * c_ptr = this->ecl_node.get_wgname();
            if (c_ptr)
                return std::string(c_ptr);
            else
                return "";
        }

        std::string keyword() const {
            return this->ecl_node.get_keyword();
        }

        std::string gen_key() const {
            return this->ecl_node.get_gen_key1();
        }

        int num() const {
            return this->ecl_node.get_num();
        }

        ecl_smspec_var_type type() const {
            return this->ecl_node.get_var_type();
        }

        int cmp(const SummaryNode& other) const {
            return this->ecl_node.cmp( other.ecl_node );
        }

    private:
        ecl::smspec_node ecl_node;
    };


    class Deck;
    class TableManager;
    class EclipseState;
    class ParserKeyword;
    class Schedule;
    class ErrorGuard;
    class ParseContext;
    class GridDims;

    class SummaryConfig {
        public:
            typedef SummaryNode keyword_type;
            typedef std::vector< keyword_type > keyword_list;
            typedef keyword_list::const_iterator const_iterator;

            SummaryConfig( const Deck&, const Schedule&,
                           const TableManager&, const ParseContext&, ErrorGuard&);

            const_iterator begin() const;
            const_iterator end() const;

            SummaryConfig& merge( const SummaryConfig& );
            SummaryConfig& merge( SummaryConfig&& );

            /*
              The hasKeyword() method will consult the internal set
              'short_keywords', i.e. the query should be based on pure
              keywords like 'WWCT' and 'BPR' - and *not* fully
              identifiers like 'WWCT:OPX' and 'BPR:10,12,3'.
            */
            bool hasKeyword( const std::string& keyword ) const;

            /*
               The hasSummaryKey() method will look for fully
               qualified keys like 'RPR:3' and 'BPR:10,15,20.
            */
            bool hasSummaryKey(const std::string& keyword ) const;
            /*
              Can be used to query if a certain 3D field, e.g. PRESSURE,
              is required to calculate the summary variables.
            */
            bool require3DField( const std::string& keyword) const;
            bool requireFIPNUM( ) const;
        private:
            SummaryConfig( const Deck& deck,
                           const Schedule& schedule,
                           const TableManager& tables,
                           const ParseContext& parseContext,
                           ErrorGuard& errors,
                           const GridDims& dims);

            /*
              The short_keywords set contains only the pure keyword
              part, e.g. "WWCT", and not the qualification with
              well/group name or a numerical value.
            */
            keyword_list keywords;
            std::set<std::string> short_keywords;
            std::set<std::string> summary_keywords;
    };

} //namespace Opm

#endif
