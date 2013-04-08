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

#ifndef RAWDECK_HPP
#define RAWDECK_HPP
#include <list>
#include <ostream>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include "opm/parser/eclipse/Logger.hpp"
#include "RawKeyword.hpp"
#include "RawParserKWs.hpp"

namespace Opm {

    class RawDeck {

    public:
        RawDeck(RawParserKWsConstPtr rawParserKWs);
        void readDataIntoDeck(const std::string& path);
        RawKeywordPtr getKeyword(const std::string& keyword);
        unsigned int getNumberOfKeywords();
        friend std::ostream& operator<<(std::ostream& os, const RawDeck& deck);
        virtual ~RawDeck();

    private:
        std::list<RawKeywordPtr> m_keywords;
        RawParserKWsConstPtr m_rawParserKWs;
        void readDataIntoDeck(const std::string& path, std::list<RawKeywordPtr>& keywordList);
        void popAndProcessInclude(std::list<RawKeywordPtr>& keywordList, boost::filesystem::path baseDataFolder);
        bool isKeywordFinished(RawKeywordPtr rawKeyword);
        static boost::filesystem::path verifyValidInputPath(const std::string& inputPath);
    };

    typedef boost::shared_ptr<RawDeck> RawDeckPtr;
}

#endif  /* RAWDECK_HPP */

