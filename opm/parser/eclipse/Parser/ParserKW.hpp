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
#ifndef PARSER_KW_H
#define PARSER_KW_H

#include <string>
#include <boost/shared_ptr.hpp>

#include <opm/parser/eclipse/Parser/ParserRecordSize.hpp>

namespace Opm {
  
  class ParserKW {
    
  public:
    ParserKW();     
    ParserKW(const std::string& name , ParserRecordSizeConstPtr recordSize);
    ~ParserKW();
    
    const std::string& getName() const;
  private:
    std::string m_name;  
    ParserRecordSizeConstPtr recordSize;
  };
 
  typedef boost::shared_ptr<ParserKW> ParserKWPtr;
  typedef boost::shared_ptr<const ParserKW> ParserKWConstPtr;
}

#endif
