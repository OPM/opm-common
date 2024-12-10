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

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>

#include <iostream>
#include <iomanip>
#include <numeric>
#include <algorithm>

#include <fmt/format.h>

namespace Opm {

    void ErrorGuard::addError(const std::string& errorKey, const std::string& msg) {
        this->error_list.emplace_back(errorKey, msg);
    }


    void ErrorGuard::addWarning(const std::string& errorKey, const std::string& msg) {
        this->warning_list.emplace_back(errorKey, msg);
    }


    void ErrorGuard::dump() const {
        const auto width = maxMessageWidth();

        if (!this->warning_list.empty()) {
            std::cerr << "Warnings:" << std::endl;
            for (const auto& pair : this->warning_list)
                std::cerr << "  " << std::setw(width) << pair.first << ": " << pair.second << std::endl;
            std::cerr << std::endl;
        }

        if (!this->error_list.empty()) {
            std::cerr << std::endl << std::endl << "Errors:" << std::endl;

            for (const auto& [error_key, error_msg] : this->error_list) {
                std::cerr << std::left << "  " << std::setw(width) << error_key << ": " << error_msg << std::endl;
            }
            std::cerr << std::endl;
        }
    }


    std::string ErrorGuard::formattedErrors() const {
        // format error messages to be logged before exiting
        std::string error_msgs;
        if (!this->error_list.empty()) {
            for (const auto& [error_key, error_msg] : this->error_list) {
                error_msgs += fmt::format("\n{}", error_msg);
            }
        }
        return error_msgs;
    }


    std::size_t ErrorGuard::maxMessageWidth() const {
        auto maxit = [](const std::size_t acc, const auto& pair)
                     {
                         return std::max(acc, pair.first.size());
                     };
        std::size_t width = std::accumulate(this->warning_list.begin(),
                                            this->warning_list.end(), 0UL, maxit);
        width = std::accumulate(this->error_list.begin(),
                                this->error_list.end(), width, maxit);
        return width;
    }

    void ErrorGuard::clear() {
        this->warning_list.clear();
        this->error_list.clear();
    }

    void ErrorGuard::terminate() const {
        this->dump();
        std::exit(1);
    }


    ErrorGuard::~ErrorGuard() {
        if (*this)
            this->terminate();
    }

}
