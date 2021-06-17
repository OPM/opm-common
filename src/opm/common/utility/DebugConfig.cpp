/*
  Copyright 2021 Equinor ASA

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

#include <opm/common/utility/DebugConfig.hpp>
#include <opm/common/utility/String.hpp>

namespace Opm
{

namespace {

static std::unordered_map<std::string, DebugConfig::Verbosity> verbosity_map = {
    {"OFF", DebugConfig::Verbosity::SILENT},
    {"ON", DebugConfig::Verbosity::NORMAL},
    {"SILENT", DebugConfig::Verbosity::SILENT},
    {"NORMAL", DebugConfig::Verbosity::NORMAL},
    {"VERBOSE", DebugConfig::Verbosity::VERBOSE},
    {"DETAILED", DebugConfig::Verbosity::DETAILED},
    {"0", DebugConfig::Verbosity::SILENT},
    {"1", DebugConfig::Verbosity::NORMAL},
    {"2", DebugConfig::Verbosity::VERBOSE},
    {"3", DebugConfig::Verbosity::DETAILED}
};

static std::unordered_map<std::string, std::pair<DebugConfig::Topic, DebugConfig::Verbosity>> default_config = {
    {"WELLS", {DebugConfig::Topic::WELLS, DebugConfig::Verbosity::SILENT}},
    {"INIT", {DebugConfig::Topic::INIT, DebugConfig::Verbosity::SILENT}},
};


}


void DebugConfig::default_init() {
    for (const auto& [_, value_pair] : default_config) {
        (void)_;
        const auto& [topic, verbosity] = value_pair;
        this->settings.insert( {topic, verbosity} );
    }
}

void DebugConfig::reset() {
    this->string_settings.clear();
    this->settings.clear();
    this->default_init();
}

DebugConfig::DebugConfig() {
    this->default_init();
}

void DebugConfig::update(Topic topic, Verbosity verbosity) {
    this->settings[topic] = verbosity;
}

void DebugConfig::update(std::string string_topic, Verbosity verbosity) {
    string_topic = uppercase(string_topic);
    auto find_iter = default_config.find(string_topic);
    if (find_iter == default_config.end())
        this->string_settings[string_topic] = verbosity;
    else {
        auto topic = find_iter->second.first;
        this->update(topic, verbosity);
    }
}

bool DebugConfig::update(const std::string& string_topic, const std::string& string_verbosity) {
    auto verb_iter = verbosity_map.find(string_verbosity);
    if (verb_iter == verbosity_map.end())
        return false;

    auto verbosity = verb_iter->second;
    this->update(string_topic, verbosity);
    return true;
}


void DebugConfig::update(const std::string& string_topic) {
    return this->update(string_topic, DebugConfig::Verbosity::NORMAL);
}

void DebugConfig::update(Topic topic) {
    this->update(topic, DebugConfig::Verbosity::NORMAL);
}

DebugConfig::Verbosity DebugConfig::operator[](const std::string& topic) const {
    auto iter = this->string_settings.find(topic);
    if (iter == this->string_settings.end())
        return DebugConfig::Verbosity::SILENT;

    return iter->second;
}

bool DebugConfig::operator()(const std::string& topic) const {
    auto iter = this->string_settings.find(topic);
    if (iter == this->string_settings.end())
        return false;

    return iter->second != DebugConfig::Verbosity::SILENT;
}

DebugConfig::Verbosity DebugConfig::operator[](Topic topic) const {
    return this->settings.at(topic);
}

bool DebugConfig::operator()(Topic topic) const {
    auto verbosity = this->operator[](topic);
    return verbosity != DebugConfig::Verbosity::SILENT;
}

bool DebugConfig::operator==(const DebugConfig& other) const {
    return this->settings == other.settings &&
           this->string_settings == other.string_settings;
}

DebugConfig DebugConfig::serializeObject() {
    DebugConfig dbg_config;
    dbg_config.update(DebugConfig::Topic::WELLS, DebugConfig::Verbosity::DETAILED);
    dbg_config.update("RESTART", DebugConfig::Verbosity::VERBOSE);
    return dbg_config;
}
}
