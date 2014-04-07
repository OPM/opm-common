#include <ios>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <map>
#include <string>

// http://www.ridgesolutions.ie/index.php/2013/05/30/boost-link-error-undefined-reference-to-boostfilesystemdetailcopy_file/
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>

#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>



using namespace Opm;
using namespace boost::filesystem;

typedef std::pair<std::string , std::pair<std::string , ParserKeywordConstPtr> > KeywordElementType;
typedef std::map<std::string , std::pair<std::string , ParserKeywordConstPtr> >  KeywordMapType;



void createHeader(std::iostream& of ) {
    of << "#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserRecord.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/Parser.hpp>" << std::endl;
    of << "namespace Opm {"  << std::endl << std::endl;
}



void createTestHeader(std::iostream& of , const std::string& test_module) {
    of << "#define BOOST_TEST_MODULE "  << test_module << std::endl;
    of << "#include <boost/test/unit_test.hpp>" << std::endl;
    of << "#include <memory>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserFloatItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserRecord.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Units/UnitSystem.hpp>" << std::endl;
    of << "using namespace Opm;"  << std::endl << std::endl;
    of << "std::shared_ptr<UnitSystem> unitSystem( UnitSystem::newMETRIC() );" << std::endl;
}



void startFunction(std::iostream& of) {
    of << "void Parser::addDefaultKeywords() { " << std::endl;
}


void endFunction(std::iostream& of) {
    of << "}" << std::endl;
}



bool areStreamsEqual( std::istream& lhs, std::istream& rhs )
{
    for (;;)
    {
        char l = lhs.get();
        char r = rhs.get();
        if (!lhs || !rhs)    // if either at end of file
            break;
        if (l != r)        // false if chars differ
            return false;
    }
    return !lhs && !rhs;    // true if both end of file
}


void generateKeywordSignature(std::iostream& of , KeywordMapType& keywordMap)
{    
    for (auto iter=keywordMap.begin(); iter != keywordMap.end(); ++iter) {
        KeywordElementType  keywordElement = *iter;
        const std::string& fileName = keywordElement.first;
        const std::string& keywordName = keywordElement.second.first;
        Json::JsonObject * jsonKeyword = new Json::JsonObject(boost::filesystem::path(fileName));

        of << keywordName << std::endl << jsonKeyword->get_content() << std::endl;
        
        delete jsonKeyword;
    }
}

//-----------------------------------------------------------------

void startTest(std::iostream& of, const std::string& test_name) {
    of << "BOOST_AUTO_TEST_CASE(" << test_name << ") {" << std::endl;
}


void endTest(std::iostream& of) {
    of << "}" << std::endl << std::endl;
}

void testKeyword(ParserKeywordConstPtr parserKeyword , const std::string& keywordName , const boost::filesystem::path& jsonFile , std::iostream& of) {
    startTest(of , keywordName);
    of << "Json::JsonObject jsonKeyword(boost::filesystem::path(" << jsonFile << "));" << std::endl;
    of << "ParserKeywordConstPtr parserKeyword(new ParserKeyword(jsonKeyword));" << std::endl;

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
    of << "  delete inlineKeyword;" << std::endl;
    endTest( of );
}


void generateTestForKeyword(std::iostream& of, KeywordElementType keywordElement) {
    const std::string& fileName = keywordElement.first;
    const std::string& keywordName = keywordElement.second.first;
    ParserKeywordConstPtr parserKeyword = keywordElement.second.second;

    testKeyword( parserKeyword , keywordName , fileName , of );
}


void generateKeywordTest(const char * test_file_name , KeywordMapType& keywordMap) {
    std::fstream test_file_stream( test_file_name , std::fstream::out );
    createTestHeader( test_file_stream , "TEST_KEYWORDS");
    for (auto iter=keywordMap.begin(); iter != keywordMap.end(); ++iter) 
        generateTestForKeyword(test_file_stream , *iter);
    
    test_file_stream.close( );
}

//-----------------------------------------------------------------

void generateSourceForKeyword(std::iostream& of, KeywordElementType keywordElement)
{
    const std::string& keywordName = keywordElement.second.first;
    ParserKeywordConstPtr parserKeyword = keywordElement.second.second;

    std::string indent("   ");
    of << "{" << std::endl;
    of << indent << "ParserKeyword *";
    parserKeyword->inlineNew(of , keywordName , indent);
    of << indent << "addKeyword( ParserKeywordConstPtr(" << keywordName << "));" << std::endl;
    of << "}" << std::endl << std::endl;
    
    std::cout << "Creating keyword: " << keywordName << std::endl;
}

void generateKeywordSource(const char * source_file_name , KeywordMapType& keywordMap) {
    std::fstream source_file_stream( source_file_name, std::fstream::out );
    
    createHeader(source_file_stream);
    startFunction(source_file_stream);
    for (auto iter=keywordMap.begin(); iter != keywordMap.end(); ++iter) 
        generateSourceForKeyword(source_file_stream , *iter);

    endFunction(source_file_stream);
    source_file_stream << "}" << std::endl;
    
    source_file_stream.close( );
}

//-----------------------------------------------------------------

void scanKeyword(const boost::filesystem::path& file , KeywordMapType& keywordMap) {
    if (ParserKeyword::validName(file.filename().string())) {

        Json::JsonObject * jsonKeyword;
        try {
            jsonKeyword = new Json::JsonObject(file);
        } catch(...) {
            std::cerr << "Parsing json config file: " << file.string() << " failed - keyword skipped." << std::endl;
            return;
        }
        
        
        {
            std::pair<std::string , ParserKeywordConstPtr> elm(file.filename().string() , std::shared_ptr<const ParserKeyword>(new ParserKeyword( *jsonKeyword )));
            std::pair<std::string , std::pair<std::string , ParserKeywordConstPtr> > pair(file.string() , elm);
            
            keywordMap.insert(pair);
        }

        delete jsonKeyword;
    }
}

void scanAllKeywords(const boost::filesystem::path& directory , KeywordMapType& keywordMap) {
    boost::filesystem::directory_iterator end;
    for (boost::filesystem::directory_iterator iter(directory); iter != end; iter++) {
        if (boost::filesystem::is_directory(*iter))
            scanAllKeywords(*iter , keywordMap);
        else
            scanKeyword(*iter , keywordMap);
    }
}


//-----------------------------------------------------------------

void printUsage() {
    std::cout << "Generates source code for populating the parser's list of known keywords." << std::endl;
    std::cout << "Usage: createDefaultKeywordList <configroot> <sourcefilename> [<dumpfilename>]" << std::endl;
    std::cout << " <configroot>:     Path to keyword (JSON) files" << std::endl;
    std::cout << " <sourcefilename>: Path to source file to generate" << std::endl;
    std::cout << " <testfilename>  : Path to source file with keyword testing" << std::endl;
    std::cout << " <dumpfilename>:   Path to dump file containing state of keyword list at" << std::endl;
    std::cout << "                   last build (used for build triggering)." << std::endl;
}


void ensurePath( const char * file_name ) {
    boost::filesystem::path file(file_name);
    if (!boost::filesystem::is_directory( file.parent_path()))
        boost::filesystem::create_directory( file.parent_path());
}


int main(int argc , char ** argv) {
    const char * config_root = argv[1];
    const char * source_file_name = argv[2];
    const char * test_file_name = argv[3];
    const char * signature_file_name = argv[4];

    if (!argv[1]) {
        printUsage();
        return 0;
    }
    KeywordMapType keywordMap;
    boost::filesystem::path keywordPath(config_root);
    bool needToGenerate = false;

    std::stringstream signature_stream;

    ensurePath( source_file_name );
    ensurePath( test_file_name );
    ensurePath( signature_file_name );

    scanAllKeywords( config_root , keywordMap );

    generateKeywordSignature(signature_stream , keywordMap);

    if (!boost::filesystem::exists(path(source_file_name)))
        needToGenerate = true;

    if (!boost::filesystem::exists(path(test_file_name)))
        needToGenerate = true;

    if (!needToGenerate) {
        if (boost::filesystem::exists(path(signature_file_name))) {
            std::fstream signature_stream_on_disk(signature_file_name , std::fstream::in);
            needToGenerate = !areStreamsEqual(signature_stream, signature_stream_on_disk);
            signature_stream_on_disk.close();
        } else
            needToGenerate = true;
    }


    if (needToGenerate) {
        std::cout << "Generating keywords:" << std::endl;
        generateKeywordSource(source_file_name, keywordMap );
        generateKeywordTest(test_file_name, keywordMap );
        {
            std::fstream signature_stream_on_disk(signature_file_name , std::fstream::out);
            signature_stream.seekg(std::ios_base::beg);
            signature_stream_on_disk << signature_stream.rdbuf();
            signature_stream_on_disk.close();
        }
    }
    else {
        std::cout << "No keyword changes detected." << std::endl;
    }
    return 0;
}
