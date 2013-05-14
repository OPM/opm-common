template<class T> void fillVectorFromStringToken(std::string token , std::vector<T>& dataVector, T defaultValue , bool& defaultActive) {
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
      dataVector.push_back( defaultValue );
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
        dataVector.push_back( value );
    } 
  } else {
    inputStream >> value;
    dataVector.push_back( value );
  } 

  inputStream.get();
  if (!inputStream.eof())
    throw std::invalid_argument("Spurious data at the end of: <" + token + ">");
}
        


template<class T> std::vector<T> readFromRawRecord(RawRecordPtr rawRecord , bool scanAll , size_t expectedItems , T defaultValue , bool& defaultActive) {
  bool cont = true;
  std::vector<T> data;
  do {
    std::string token = rawRecord->pop_front();
    fillVectorFromStringToken(token, data , defaultValue , defaultActive);
                
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
        

template <class T> void pushBackToRecord( RawRecordPtr rawRecord , std::vector<T>& data , size_t expectedItems , bool defaultActive) {
  size_t extraItems = data.size() - expectedItems;
            
  for (size_t i=1; i <= extraItems; i++) {
    if (defaultActive) 
      rawRecord->push_front("*");
    else {
      size_t offset = data.size();
      int intValue = data[ offset - i ];
      std::string stringValue = boost::lexical_cast<std::string>( intValue );
                    
      rawRecord->push_front( stringValue );
    }
  }
}
