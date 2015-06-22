/*
  Copyright 2015 Statoil ASA.

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

#include <stdexcept>
#include <iostream>
#include <fstream>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <opm/json/JsonObject.hpp>
#include <opm/parser/eclipse/Generator/KeywordGenerator.hpp>

namespace Opm {

    KeywordGenerator::KeywordGenerator(bool verbose)
        : m_verbose( verbose )
    {
    }




    std::string testHeader() {
        std::string header = "#define BOOST_TEST_MODULE\n"
            "#include <boost/test/unit_test.hpp>\n"
            "#include <memory>\n"
            "#include <opm/json/JsonObject.hpp>\n"
            "#include <opm/parser/eclipse/Parser/ParserKeywords.hpp>\n"
            "#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>\n"
            "#include <opm/parser/eclipse/Parser/ParserItem.hpp>\n"
            "#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>\n"
            "#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>\n"
            "#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>\n"
            "#include <opm/parser/eclipse/Parser/ParserFloatItem.hpp>\n"
            "#include <opm/parser/eclipse/Parser/ParserRecord.hpp>\n"
            "#include <opm/parser/eclipse/Units/UnitSystem.hpp>\n"
            "using namespace Opm;\n"
            "std::shared_ptr<UnitSystem> unitSystem( UnitSystem::newMETRIC() );\n";

        return header;
    }


    std::string KeywordGenerator::sourceHeader() {
        std::string header = "#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>\n"
            "#include <opm/parser/eclipse/Parser/ParserItem.hpp>\n"
            "#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>\n"
            "#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>\n"
            "#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>\n"
            "#include <opm/parser/eclipse/Parser/ParserRecord.hpp>\n"
            "#include <opm/parser/eclipse/Parser/Parser.hpp>\n"
            "#include <opm/parser/eclipse/Parser/ParserKeywords.hpp>\n\n\n"
            "namespace Opm {\n"
            "namespace ParserKeywords {\n\n";

        return header;
    }

    std::string KeywordGenerator::headerHeader() {
        std::string header = "#ifndef PARSER_KEYWORDS_HPP\n"
            "#define PARSER_KEYWORDS_HPP\n"
            "#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>\n"
            "namespace Opm {\n"
            "namespace ParserKeywords {\n\n";

        return header;
    }

    void KeywordGenerator::ensurePath( const std::string& file_name) {
        boost::filesystem::path file(file_name);
        if (!boost::filesystem::is_directory( file.parent_path()))
            boost::filesystem::create_directories( file.parent_path());
    }

    bool KeywordGenerator::updateFile(const std::stringstream& newContent , const std::string& filename) {
        bool update = true;
        {
            // Check if file already contains the newContent.
            std::ifstream inputStream(filename);
            if (inputStream) {
                std::stringstream oldContent;
                oldContent << inputStream.rdbuf();
                if (oldContent.str() == newContent.str()) {
                    update = false;
                }
            }
        }

        if (update) {
            ensurePath(filename);
            std::ofstream outputStream(filename);
            outputStream << newContent.str();
        }

        return update;
    }


    bool KeywordGenerator::updateSource(const KeywordLoader& loader , const std::string& sourceFile) const {
        std::stringstream newSource;

        newSource << sourceHeader();
        for (auto iter = loader.keyword_begin(); iter != loader.keyword_end(); ++iter) {
            std::shared_ptr<ParserKeyword> keyword = (*iter).second;
            newSource << keyword->createCode() << std::endl;
        }
        newSource << "}" << std::endl;
        {
            newSource << "void Parser::addDefaultKeywords() {" << std::endl;
            for (auto iter = loader.keyword_begin(); iter != loader.keyword_end(); ++iter) {
                std::shared_ptr<ParserKeyword> keyword = (*iter).second;
                newSource << "   addKeyword<ParserKeywords::" << keyword->className() << ">();" << std::endl;
            }
            newSource << "}" << std::endl;
        }
        newSource << "}" << std::endl;

        {
            bool update = updateFile( newSource , sourceFile );
            if (m_verbose) {
                if (update)
                    std::cout << "Updated source file written to: " << sourceFile << std::endl;
                else
                    std::cout << "No changes to source file: " << sourceFile << std::endl;
            }
            return update;
        }
    }


    bool KeywordGenerator::updateHeader(const KeywordLoader& loader , const std::string& headerFile) const {
        std::stringstream stream;

        stream << headerHeader();
        for (auto iter = loader.keyword_begin(); iter != loader.keyword_end(); ++iter) {
            std::shared_ptr<ParserKeyword> keyword = (*iter).second;
            stream << keyword->createDeclaration("   ") << std::endl;
        }
        stream << "}" << std::endl << "}" << std::endl;
        stream << "#endif" << std::endl;

        {
            bool update = updateFile( stream , headerFile );
            if (m_verbose) {
                if (update)
                    std::cout << "Updated header file written to: " << headerFile << std::endl;
                else
                    std::cout << "No changes to header file: " << headerFile << std::endl;
            }
            return update;
        }
    }


    std::string KeywordGenerator::startTest(const std::string& keyword_name) {
        return std::string("BOOST_AUTO_TEST_CASE(TEST") + keyword_name + std::string("Keyword) {\n");
    }


    std::string KeywordGenerator::endTest() {
        return "}\n\n";
    }



    bool KeywordGenerator::updateTest(const KeywordLoader& loader , const std::string& testFile) const {
        std::stringstream stream;

        stream << testHeader();
        for (auto iter = loader.keyword_begin(); iter != loader.keyword_end(); ++iter) {
            const std::string& keywordName = (*iter).first;
            std::shared_ptr<ParserKeyword> keyword = (*iter).second;
            stream << startTest(keywordName);
            stream << "    std::string jsonFile = \"" << loader.getJsonFile( keywordName) << "\";" << std::endl;
            stream << "    boost::filesystem::path jsonPath( jsonFile );" << std::endl;
            stream << "    Json::JsonObject jsonConfig( jsonPath );" << std::endl;
            stream << "    ParserKeyword jsonKeyword(jsonConfig);" << std::endl;
            stream << "    ParserKeywords::" << keywordName << " inlineKeyword;" << std::endl;
            stream << "    BOOST_CHECK( jsonKeyword.equal( inlineKeyword ));" << std::endl;
            stream << "    if (jsonKeyword.hasDimension()) {" <<std::endl;
            stream << "        ParserRecordConstPtr parserRecord = jsonKeyword.getRecord(0);" << std::endl;
            stream << "        for (size_t i=0; i < parserRecord->size(); i++){ " << std::endl;
            stream << "            ParserItemConstPtr item = parserRecord->get( i );" << std::endl;
            stream << "            for (size_t j=0; j < item->numDimensions(); j++) {" << std::endl;
            stream << "                std::string dimString = item->getDimension(j);" << std::endl;
            stream << "                BOOST_CHECK_NO_THROW( unitSystem->getNewDimension( dimString ));" << std::endl;
            stream << "             }" << std::endl;
            stream << "        }" << std::endl;
            stream << "    }" << std::endl;
            stream << endTest(  );
        }

        return updateFile( stream , testFile );
    }
}


