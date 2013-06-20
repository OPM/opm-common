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

template<class T> void fillVectorFromStringToken(std::string token, std::vector<T>& dataVector, T defaultValue, bool& defaultActive) const {
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

template<class T> std::vector<T> readFromRawRecord(RawRecordPtr rawRecord, bool scanAll, size_t expectedItems, T defaultValue, bool& defaultActive) const {
  bool cont = true;
  std::vector<T> data;
  do {
    std::string token = rawRecord->pop_front();
    fillVectorFromStringToken(token, data, defaultValue, defaultActive);

    if (rawRecord->size() == 0)
      cont = false;
    else {
      if (!scanAll)
        if (data.size() >= expectedItems)
          cont = false;
    }
  } while (cont);
  return data;
}

template <class T> void pushBackToRecord(RawRecordPtr rawRecord, std::vector<T>& data, size_t expectedItems, bool defaultActive) const {
  size_t extraItems = data.size() - expectedItems;

  for (size_t i = 1; i <= extraItems; i++) {
    if (defaultActive)
      rawRecord->push_front("*");
    else {
      size_t offset = data.size();
      T intValue = data[ offset - i ];
      std::string stringValue = boost::lexical_cast<std::string>(intValue);

      rawRecord->push_front(stringValue);
    }
  }
}
