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

template<class T> void fillVectorFromStringToken(std::string token, std::deque<T>& dataVector, T defaultValue, bool& defaultActive) const {
    std::istringstream inputStream(token);
    size_t starPos = token.find('*');
    T value;
    bool hasStar = (starPos != std::string::npos);

    defaultActive = false;

    if (hasStar) {
        bool singleDefault = (starPos == 0);

        if (singleDefault) {
            defaultActive = true;
            inputStream.get();
            if (token.size() > 1)
                throw std::invalid_argument("Token : " + token + " is invalid.");
            dataVector.push_back(defaultValue);
        } else {
            size_t multiplier;
            int starChar;

            inputStream >> multiplier;
            starChar = inputStream.get();
            if (starChar != '*')
                throw std::invalid_argument("Error ...");

            defaultActive = (inputStream.peek() == std::char_traits<char>::eof());

            if (defaultActive)
                value = defaultValue;
            else
                inputStream >> value;

            for (size_t i = 0; i < multiplier; i++)
                dataVector.push_back(value);
        }
    } else {
        inputStream >> value;
        dataVector.push_back(value);
    }

    inputStream.get();
    if (!inputStream.eof())
        throw std::invalid_argument("Spurious data at the end of: <" + token + ">");
}

template<class T> std::deque<T> readFromRawRecord(RawRecordPtr rawRecord, bool scanAll, T defaultValue, bool& defaultActive) const {
    bool cont = true;
    std::deque<T> data;
    if (rawRecord->size() == 0) {
        data.push_back(defaultValue);
    } else {
        do {
            std::string token = rawRecord->pop_front();
            fillVectorFromStringToken(token, data, defaultValue, defaultActive);

            if (rawRecord->size() == 0 || !scanAll)
                cont = false;

        } while (cont);
    }
    return data;
}

template <class T> void pushBackToRecord(RawRecordPtr rawRecord, std::deque<T>& data, bool defaultActive) const {

    for (size_t i = 0; i < data.size(); i++) {
        if (defaultActive)
            rawRecord->push_front("*");
        else {
            T value = data[i];
            std::string stringValue = boost::lexical_cast<std::string>(value);

            rawRecord->push_front(stringValue);
        }
    }
}
