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

#define BOOST_TEST_MODULE ParserIntegrationTests
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/filesystem.hpp>
#include <ostream>

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>

#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

using namespace Opm;
using namespace boost::filesystem;

void
createDeckWithInclude(path& datafile)
{
    path root = unique_path("/tmp/%%%%-%%%%");
    path absoluteInclude = root / "absolute.include";
    path includePath = root / "include";

    create_directories(root);
    create_directories(includePath);

        {
            datafile = root / "TEST.DATA";

            std::ofstream of(datafile.string().c_str());

            of << "INCLUDE" << std::endl;
            of << "   \'relative.include\' /" << std::endl;

            of << std::endl;

            of << "INCLUDE" << std::endl;
            of << "   \'" << absoluteInclude.string() << "\' /" << std::endl;

            of << std::endl;

            of << "INCLUDE" << std::endl;
            of << "  \'include/nested.include\'   /" << std::endl;

            of.close();
        }
        {
            std::ofstream of(absoluteInclude.string().c_str());

            of << "DIMENS" << std::endl;
            of << "   10 20 30 /" << std::endl;
            of.close();
        }

        {
            path relativeInclude = root / "relative.include";
            std::ofstream of(relativeInclude.string().c_str());

            of << "START" << std::endl;
            of << "   10 'FEB' 2012 /" << std::endl;
            of.close();
        }

        {
            path nestedInclude = includePath / "nested.include";
            path gridInclude = includePath / "grid.include";
            std::ofstream of(nestedInclude.string().c_str());

            of << "INCLUDE" << std::endl;
            of << "   \'include/grid.include\'  /" << std::endl;
            of.close();

            std::ofstream of2(gridInclude.string().c_str());
            of2 << "GRIDUNIT" << std::endl;
            of2 << "/" << std::endl;
            of2.close();
        }


}




BOOST_AUTO_TEST_CASE(parse_fileWithWWCTKeyword_deckReturned) {
    path datafile;
    ParserPtr parser(new Parser());
    createDeckWithInclude (datafile);
    DeckConstPtr deck = parser->parse(datafile.string());

    BOOST_CHECK( deck->hasKeyword("DIMENS"));
    BOOST_CHECK( deck->hasKeyword("START"));
    BOOST_CHECK( deck->hasKeyword("GRIDUNIT"));
}

