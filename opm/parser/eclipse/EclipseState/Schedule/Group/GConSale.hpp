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

#ifndef GCONSALE_H
#define GCONSALE_H

#include <map>
#include <string>

namespace Opm {

    class GConSale {
    public:

        enum class MaxProcedure {
            NONE, CON, CON_P, WELL, PLUG, RATE, MAXR, END
        };

        struct GCONSALEGroup {
            UDAValue sales_target;
            UDAValue max_sales_rate;
            UDAValue min_sales_rate;
            MaxProcedure max_proc;
        };

        GConSale();
        
        bool has(const std::string& name) const;
        const GCONSALEGroup& get(const std::string& name) const;
        static MaxProcedure stringToProcedure(const std::string& procedure);
        void add(const std::string& name, const UDAValue& sales_target, const UDAValue& max_rate, const UDAValue& min_rate, const std::string& procedure);
        size_t size() const;

    private:
        std::map<std::string, GCONSALEGroup> groups;
    };

}


#endif
