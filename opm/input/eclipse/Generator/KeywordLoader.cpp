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
#include <opm/input/eclipse/Generator/KeywordLoader.hpp>

#include <opm/input/eclipse/Parser/ParserKeyword.hpp>
#include <opm/json/JsonObject.hpp>

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace Opm {

    KeywordLoader::KeywordLoader(const std::vector<std::string>& keyword_files, bool verbose)
    {
        std::map<std::string, ParserKeyword> keyword_map;
        for (const auto& keyword_file : keyword_files) {
            if (verbose) {
                std::cout << "Loading keyword from file: " << keyword_file << std::endl;
            }

            const std::filesystem::path path(keyword_file);
            std::unique_ptr<ParserKeyword> parserKeyword;

            try {
                Json::JsonObject jsonConfig = Json::JsonObject(path);
                parserKeyword = std::make_unique<ParserKeyword>(jsonConfig);
                auto abs_path = std::filesystem::absolute(path);
            }
            catch (const std::exception& exc) {
                std::cerr << std::endl;
                std::cerr << "Failed to create parserkeyword from: " << path.string() << std::endl;
                std::cerr << exc.what() << std::endl;
                std::cerr << std::endl;
                throw;
            }

            const std::string& name = parserKeyword->getName();
            if (keyword_map.find(name) != keyword_map.end()) {
                this->m_jsonFile.erase(name);
                keyword_map.erase(name);
            }

            this->m_jsonFile.emplace(name, keyword_file);
            keyword_map.emplace(name, std::move(*parserKeyword));
        }

        this->keywords['Y'] = {};
        this->keywords['X'] = {};
        for (auto& [name, parserKeyword] : keyword_map) {
            this->keywords[name[0]].push_back(std::move(parserKeyword));
        }
    }

    std::string KeywordLoader::getJsonFile(const std::string& keyword) const
    {
        const auto iter = m_jsonFile.find(keyword);
        if (iter == m_jsonFile.end()) {
            throw std::invalid_argument("Keyword " + keyword + " not loaded");
        }
        else {
            return (*iter).second;
        }
    }

}
