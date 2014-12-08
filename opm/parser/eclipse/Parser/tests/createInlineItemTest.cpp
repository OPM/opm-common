#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string.h>

#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>
#include <opm/parser/eclipse/Parser/ParserFloatItem.hpp>

using namespace Opm;

static void createHeader(std::ofstream& of , const std::string& test_module) {
    of << "#define BOOST_TEST_MODULE "  << test_module << std::endl;
    of << "#include <boost/test/unit_test.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserFloatItem.hpp>" << std::endl;
    of << "using namespace Opm;"  << std::endl << std::endl;
}


static void startTest(std::ofstream& of, const std::string& test_name) {
    of << "BOOST_AUTO_TEST_CASE(" << test_name << ") {" << std::endl;
}


static void endTest(std::ofstream& of) {
    of << "}" << std::endl << std::endl;
}


static void intItem(std::ofstream& of) {
    startTest(of , "IntItem");
    of << "   ParserIntItem * item = new ParserIntItem(\"NAME\" , SINGLE);" << std::endl;
    of << "   ParserIntItem * inlineItem = ";
    {
       ParserIntItem * item = new ParserIntItem("NAME" , SINGLE);
       item->inlineNew( of );
       of << ";" << std::endl;
       delete item;
    }
    of << "   BOOST_CHECK( item->equal( *inlineItem ) );" << std::endl;
    endTest(of);
}


static void intItemWithDefault(std::ofstream& of) {
    startTest(of , "IntItemWithDefault");
    of << "   ParserIntItem * item = new ParserIntItem(\"NAME\" , SINGLE , 100);" << std::endl;
    of << "   ParserIntItem * inlineItem = ";
    {
       ParserIntItem * item = new ParserIntItem("NAME" , SINGLE , 100);
       item->inlineNew( of );
       of << ";" << std::endl;
       delete item;
    }
    of << "   BOOST_CHECK( item->equal( *inlineItem ) );" << std::endl;
    endTest(of);
}


 /*****************************************************************/


static void DoubleItem(std::ofstream& of) {
    startTest(of , "DoubleItem");
    of << "   ParserDoubleItem * item = new ParserDoubleItem(\"NAME\" , ALL);" << std::endl;
    of << "   ParserDoubleItem * inlineItem = ";
    {
       ParserDoubleItem * item = new ParserDoubleItem("NAME" , ALL);
       item->inlineNew( of );
       of << ";" << std::endl;
       delete item;
    }
    of << "   BOOST_CHECK( item->equal( *inlineItem ) );" << std::endl;
    endTest(of);
}


static void DoubleItemWithDefault(std::ofstream& of) {
    startTest(of , "DoubleItemWithDefault");
    of << "   ParserDoubleItem * item = new ParserDoubleItem(\"NAME\" , SINGLE , 100.89);" << std::endl;
    of << "   ParserDoubleItem * inlineItem = ";
    {
       ParserDoubleItem * item = new ParserDoubleItem("NAME" , SINGLE , 100.89);
       item->inlineNew( of );
       of << ";" << std::endl;
       delete item;
    }
    of << "   BOOST_CHECK( item->equal( *inlineItem ) );" << std::endl;
    endTest(of);
}

 /*****************************************************************/


static void FloatItem(std::ofstream& of) {
    startTest(of , "FloatItem");
    of << "   ParserFloatItem * item = new ParserFloatItem(\"NAME\" , ALL);" << std::endl;
    of << "   ParserFloatItem * inlineItem = ";
    {
       ParserFloatItem * item = new ParserFloatItem("NAME" , ALL);
       item->inlineNew( of );
       of << ";" << std::endl;
       delete item;
    }
    of << "   BOOST_CHECK( item->equal( *inlineItem ) );" << std::endl;
    endTest(of);
}


static void FloatItemWithDefault(std::ofstream& of) {
    startTest(of , "FloatItemWithDefault");
    of << "   ParserFloatItem * item = new ParserFloatItem(\"NAME\" , SINGLE , 100.89);" << std::endl;
    of << "   ParserFloatItem * inlineItem = ";
    {
       ParserFloatItem * item = new ParserFloatItem("NAME" , SINGLE , 100.89);
       item->inlineNew( of );
       of << ";" << std::endl;
       delete item;
    }
    of << "   BOOST_CHECK( item->equal( *inlineItem ) );" << std::endl;
    endTest(of);
}

 /*****************************************************************/

static void stringItem(std::ofstream& of) {
    startTest(of , "StringItem");
    of << "   ParserStringItem * item = new ParserStringItem(\"NAME\" , SINGLE);" << std::endl;
    of << "   ParserStringItem * inlineItem = ";
    {
       ParserStringItem * item = new ParserStringItem("NAME" , SINGLE);
       item->inlineNew( of );
       of << ";" << std::endl;
       delete item;
    }
    of << "   BOOST_CHECK( item->equal( *inlineItem ) );" << std::endl;
    endTest(of);
}


static void stringItemWithDefault(std::ofstream& of) {
    startTest(of , "StringItemWithDefault");
    of << "   ParserStringItem * item = new ParserStringItem(\"NAME\" , SINGLE , \"100\");" << std::endl;
    of << "   ParserStringItem * inlineItem = ";
    {
       ParserStringItem * item = new ParserStringItem("NAME" , SINGLE , "100");
       item->inlineNew( of );
       of << ";" << std::endl;
       delete item;
    }
    of << "   BOOST_CHECK( item->equal( *inlineItem ) );" << std::endl;
    endTest(of);
}


int main(int /* argc */ , char ** argv) {
    const char * test_src = argv[1];
    const char * test_module = argv[2];
    std::ofstream of( test_src );
    createHeader(of , test_module);

    intItem( of );
    intItemWithDefault( of );

    DoubleItem( of );
    DoubleItemWithDefault( of );

    FloatItem( of );
    FloatItemWithDefault( of );

    stringItem( of );
    stringItemWithDefault( of );

    of.close();
}
