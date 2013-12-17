#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string.h>
#include <boost/filesystem.hpp>

#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>

using namespace Opm;

void createHeader(std::ofstream& of , const std::string& test_module) {
    of << "#define BOOST_TEST_MODULE "  << test_module << std::endl;
    of << "#include <boost/test/unit_test.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserRecord.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Units/UnitSystem.hpp>" << std::endl;
    of << "using namespace Opm;"  << std::endl << std::endl;
    of << "UnitSystem * unitSystem = UnitSystem::newMETRIC();" << std::endl;
}


void startTest(std::ofstream& of, const std::string& test_name) {
    of << "BOOST_AUTO_TEST_CASE(" << test_name << ") {" << std::endl;
}


void endTest(std::ofstream& of) {
    of << "}" << std::endl << std::endl;
}

void testKeyword(const boost::filesystem::path& file , std::ofstream& of) {
    std::string keyword(file.filename().string());
    if (ParserKeyword::validName(keyword)) {
        startTest(of, keyword);
        of << "Json::JsonObject jsonKeyword(boost::filesystem::path(" << file << "));" << std::endl;
        of << "ParserKeywordConstPtr parserKeyword(new ParserKeyword(jsonKeyword));" << std::endl;

        Json::JsonObject jsonKeyword(file);
        ParserKeywordConstPtr parserKeyword(new ParserKeyword(jsonKeyword));
        of << "ParserKeyword * ";
        parserKeyword->inlineNew(of , "inlineKeyword" , "   ");

        of << "BOOST_CHECK( parserKeyword->equal( *inlineKeyword));" << std::endl;
        if (parserKeyword->hasDimension()) {
            of << "{" << std::endl;
            of << "    ParserRecordConstPtr parserRecord = parserKeyword->getRecord();" << std::endl;
            of << "    for (size_t i=0; i < parserRecord->size(); i++) { " << std::endl;
            of << "        ParserItemConstPtr item = parserRecord->get( i );" << std::endl;
            of << "        for (size_t j=0; j < item->numDimensions(); j++) {" << std::endl;
            of << "            std::string dimString = item->getDimension(j);" << std::endl;
            of << "            BOOST_CHECK_NO_THROW( unitSystem->getNewDimension( dimString ));" << std::endl;
            of << "         }" << std::endl; 
            of << "    }" << std::endl; 
            of << "}" << std::endl;
        }
        endTest(of);
    }
}

void testAllKeywords(const boost::filesystem::path& directory , std::ofstream& of) {
    boost::filesystem::directory_iterator end;
    for (boost::filesystem::directory_iterator iter(directory); iter != end; iter++) {
        if (boost::filesystem::is_directory(*iter))
            testAllKeywords(*iter , of);
        else
            testKeyword(*iter , of);
    }
}




int main(int argc , char ** argv) {
    const char * test_src = argv[1];
    const char * test_module = argv[2];
    const char * config_root = argv[3];
    boost::filesystem::path directory(config_root);
    std::ofstream of( test_src );
    createHeader(of , test_module);

    testAllKeywords( directory , of);

    of.close();
}
