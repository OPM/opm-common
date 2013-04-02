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

#ifndef PARSER_RECORD_SIZE_H
#define PARSER_RECORD_SIZE_H

#include <string>
#include <stdexcept>
#include <boost/shared_ptr.hpp>

#include <opm/parser/eclipse/Parser/ParserConst.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

namespace Opm {
  
  class ParserRecordSize {
    
  public:
    ParserRecordSize();
    ParserRecordSize(size_t fixedSize);
    ~ParserRecordSize();
    
    size_t recordSize();
    
    
  private:
    enum   ParserRecordSizeEnum recordSizeType;
    size_t fixedSize;
  };

  typedef boost::shared_ptr<ParserRecordSize> ParserRecordSizePtr;
  typedef boost::shared_ptr<const ParserRecordSize> ParserRecordSizeConstPtr;
}

#endif
