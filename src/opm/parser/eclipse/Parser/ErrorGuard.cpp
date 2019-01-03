/*
  Copyright 2019 Joakim Hove/datagr

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

#include <iostream>
#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>


namespace Opm {

    void ErrorGuard::addError(const std::string& errorKey, const std::string& msg) {
        this->error_list.emplace_back(errorKey, msg);
    }


    void ErrorGuard::addWarning(const std::string& errorKey, const std::string& msg) {
        this->warning_list.emplace_back(errorKey, msg);
    }


    void ErrorGuard::dump() const {
        if (!this->warning_list.empty()) {
            std::cerr << "Warnings:" << std::endl;
            for (const auto& pair : this->warning_list)
                std::cerr << pair.first << ": " << pair.second << std::endl;
        }

        if (!this->error_list.empty()) {
            std::cerr << std::endl << std::endl << "Errors:" << std::endl;
            for (const auto& pair : this->error_list)
                std::cerr << pair.first << ": " << pair.second << std::endl;
        }
    }


    void ErrorGuard::clear() {
        this->warning_list.clear();
        this->error_list.clear();
    }

    void ErrorGuard::terminate() const {
        this->dump();
        std::terminate();
    }


    ErrorGuard::~ErrorGuard() {
        if (*this)
            this->terminate();
    }

}
