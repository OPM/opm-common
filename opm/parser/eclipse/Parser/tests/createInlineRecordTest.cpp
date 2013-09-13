#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string.h>

#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>

using namespace Opm;

void createHeader(std::ofstream& of , const std::string& test_module) {
    of << "#define BOOST_TEST_MODULE "  << test_module << std::endl;
    of << "#include <boost/test/unit_test.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserRecord.hpp>" << std::endl;
    of << "using namespace Opm;"  << std::endl << std::endl;
}


void startTest(std::ofstream& of, const std::string& test_name) {
    of << "BOOST_AUTO_TEST_CASE(" << test_name << ") {" << std::endl;
}


void endTest(std::ofstream& of) {
    of << "}" << std::endl << std::endl;
}


 void recordsEqual(std::ofstream& of) {
    startTest(of , "recordsEqual");

    of << "ParserIntItemPtr itemInt(new ParserIntItem(\"INTITEM1\", SINGLE , 0));" << std::endl;
    of << "ParserDoubleItemPtr itemDouble(new ParserDoubleItem(\"DOUBLEITEM1\", SINGLE , 0));" << std::endl;
    of << "ParserStringItemPtr itemString(new ParserStringItem(\"STRINGITEM1\", SINGLE));" << std::endl;
    of << "ParserRecordPtr record(new ParserRecord());" << std::endl;
    of << "record->addItem(itemInt);"    << std::endl;
    of << "record->addItem(itemDouble);" << std::endl;
    of << "record->addItem(itemString);" << std::endl;


    ParserIntItemPtr itemInt(new ParserIntItem("INTITEM1", SINGLE , 0));
    ParserDoubleItemPtr itemDouble(new ParserDoubleItem("DOUBLEITEM1", SINGLE , 0));
    ParserStringItemPtr itemString(new ParserStringItem("STRINGITEM1", SINGLE));
    ParserRecordPtr record(new ParserRecord());
    record->addItem(itemInt);
    record->addItem(itemDouble);
    record->addItem(itemString);

    record->inlineNew(of , "inlineRecord");


    of << "BOOST_CHECK( record->equal( *inlineRecord));" << std::endl;
    endTest(of);
}




int main(int argc , char ** argv) {
    const char * test_src = argv[1];
    const char * test_module = argv[2];
    std::ofstream of( test_src );
    createHeader(of , test_module);

    recordsEqual(of);

    of.close();
}
