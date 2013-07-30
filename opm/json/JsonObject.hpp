/*
  Copyright 2013 Statoil ASA.

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

#ifndef JSON_OBJECT_HPP
#define JSON_OBJECT_HPP

#include <string>

#include <boost/filesystem/path.hpp>

#include <opm/json/cjson/cJSON.h>


namespace Json {

    class JsonObject {
    public:
        JsonObject(const boost::filesystem::path& jsonFile );
        JsonObject(const std::string& inline_json);
        JsonObject(const char * inline_json);
        JsonObject(cJSON * root);
        ~JsonObject();
        
        bool has_item(const std::string& key) const;
        JsonObject get_object(const std::string& key);
        std::string get_string(const std::string& key);
        size_t size();
        
    private:
        void initialize(const std::string& inline_json);
        cJSON * root;
        bool    owner;
    };
}



#endif


