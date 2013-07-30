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


#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include <boost/filesystem/path.hpp>

#include <opm/json/JsonObject.hpp>
#include <opm/json/cjson/cJSON.h>

namespace Json {

    void JsonObject::initialize(const std::string& inline_json) {
        root = cJSON_Parse( inline_json.c_str() );
        if (!root)
            throw std::invalid_argument("Parsing json input failed");
        owner = true;
    }


    JsonObject::JsonObject(const std::string& inline_json) {
        initialize( inline_json );
    }


    JsonObject::JsonObject(const char * inline_json) {
        initialize( inline_json );
    }


    JsonObject::JsonObject(const boost::filesystem::path& jsonFile ) {
        std::ifstream stream(jsonFile.string().c_str());

        if (stream) { 
            std::string content( (std::istreambuf_iterator<char>(stream)), 
                                  (std::istreambuf_iterator<char>()));
            std::cout << content;
            initialize( content );
        } else
            throw std::invalid_argument("Loading json from file: " + jsonFile.string() + " failed.");
    }




    JsonObject::JsonObject( cJSON * object ) {
        root = object;
        owner = false;
    }
    
    
    JsonObject::~JsonObject() {
        if (owner && root)
            cJSON_Delete(root);
    }
    


    bool JsonObject::has_item( const std::string& key) const {
        cJSON * object = cJSON_GetObjectItem( root , key.c_str() );
        if (object) 
            return true;
        else
            return false;
    }

    

    std::string JsonObject::get_string(const std::string& key) {
        cJSON * object = cJSON_GetObjectItem( root , key.c_str() );
        if (object) {
            if (cJSON_GetArraySize( object ))
                throw std::invalid_argument("Key: " + key + " is not a scalar object");
            else  
                return object->valuestring;
        } else
            throw std::invalid_argument("Key: " + key + " does not exist in json object");
    }
    
    
    size_t JsonObject::size() {
        int int_size = cJSON_GetArraySize( root );
        return (size_t) int_size;
    }



    JsonObject JsonObject::get_object(const std::string& key) {
        cJSON * object = cJSON_GetObjectItem( root , key.c_str() );
        if (object) 
            return JsonObject( object );
        else
            throw std::invalid_argument("Key: " + key + " does not exist in json object");
    }

}

