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

#ifndef DEBUG_CONFIG_HPP
#define DEBUG_CONFIG_HPP

#include <string>
#include <unordered_map>

namespace Opm
{

/*
  The DebugConfig class is an internalization of the OPM specific keyword
  DEBUGF. The DebugConfig class is created to be used in two different ways:

    1. As a configuration object attached to the logging instance. Used this way
       the object can be used to provide more fine grained control over what
       goes to the debug file.

    2. For ad hoc debugging the developer can sprinkle the code with statements like:

       if (debug_config(DebugConfig::Topic::WELLS)) {
           printf("We are fighting with the wells ....\n");
       }

  The DEBUGF keyword is initialized from the RUNSPEC section, and then you can
  update in the SCHEDULE section, by sprinkling DEBUGF keywords in the SCHEDULE
  section the debug configuration can be changed dynamically.

  The DEBUGF keyword is based string mnemomics. For each string mneomics there
  should be a corresponding enum value in the DebugConfig::Topic enum. In
  addition to the persistent mneomics which have a corresponding enum, you can
  create string keys to use in ad hoc debugging dynamically:


      DEBUGF
         MY_DEBUG_KEY=1 /

      ....

      if (debug_config("MY_DEBUG_KEY")) {
         ....
      }


  There are four levels of verbosity, the verbosity can be given with a
  name or a numeric value.

      DEBUGF
          WELLS=2 INIT=SILENT GROUP_CONTROL /

  If no verbosity level is specified the default level is NORMAL. In addition
  the symbols 'ON' and 'OFF' are available which map to levels NORMAL and SILENT
  respectively. If a mnemomic is not explicitly mentioned in a DEBUGF keyword it
  will retain the current value, the exception is an empty DEBUGF keywords:

      DEBUGF
      /

  which will reset all debugging to the the default levels.

  The implementation file DebugConfig.cpp contains two static vectors
  default_config and verbosity_map which defines the string constants used and
  the default verbosity levels.
*/



class DebugConfig {
public:

    enum class Verbosity {
        SILENT = 0,
        NORMAL = 1,
        VERBOSE = 2,
        DETAILED = 3
    };


    enum class Topic {
        WELLS = 0,
        INIT = 1,
    };

    DebugConfig();

    void update(Topic topic, Verbosity verbosity);
    bool update(const std::string& setting, const std::string& value);

    // When no verbosity is supplied the debug setting will be set to NORMAL.
    void update(const std::string& setting);
    void update(Topic topic);

    // This well set all verbosity settings back to default, and all string
    // settings will be cleared. This function is called when an empty DEBUGF
    // keyword is encountered in the deck.
    void reset();

    Verbosity operator[](Topic topic) const;
    Verbosity operator[](const std::string& topic) const;


    // The operator() will return true if the debug setting is NORMAL or above.
    bool operator()(Topic topic) const;
    bool operator()(const std::string& topic) const;


    bool operator==(const DebugConfig& other) const;
    static DebugConfig serializeObject();

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer.template map<std::unordered_map<Topic,Verbosity>,false>(this->settings);
        serializer.template map<std::unordered_map<std::string, Verbosity>,false>(this->string_settings);
    }

private:
    void update(std::string setting, Verbosity verbosity);
    void default_init();

    std::unordered_map<Topic, Verbosity> settings;
    std::unordered_map<std::string, Verbosity> string_settings;
};



}

#endif
