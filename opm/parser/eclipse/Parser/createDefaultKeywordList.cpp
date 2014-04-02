#include <ios>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string.h>
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

typedef void (*keywordGenerator)(std::iostream& of, const std::string keywordName, const Json::JsonObject* jsonKeyword);

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

void startFunction(std::iostream& of) {
    of << "void Parser::addDefaultKeywords() { " << std::endl;
}


void endFunction(std::iostream& of) {
    of << "}" << std::endl;
}


void generateSourceForKeyword(std::iostream& of, std::string keywordName, const Json::JsonObject* jsonKeyword)
{
    if (ParserKeyword::validName(keywordName)) {
        ParserKeywordConstPtr parserKeyword(new ParserKeyword(*jsonKeyword));
        std::string indent("   ");
        of << "{" << std::endl;
        of << indent << "ParserKeyword *";
        parserKeyword->inlineNew(of , keywordName , indent);
        of << indent << "addKeyword( ParserKeywordConstPtr(" << keywordName << "));" << std::endl;
        of << "}" << std::endl << std::endl;

//        std::cout << "Creating keyword: " << keywordName << std::endl;
    }
}

void generateDumpForKeyword(std::iostream& of, std::string keywordName, const Json::JsonObject* jsonKeyword)
{
    of << keywordName << std::endl;
    of << jsonKeyword->get_content() << std::endl;
}

void scanKeyword(const boost::filesystem::path& file , std::iostream& of, keywordGenerator generate) {
    Json::JsonObject * jsonKeyword;
    try {
        jsonKeyword = new Json::JsonObject(file);
    } catch(...) {
        std::cerr << "Parsing json config file: " << file.string() << " failed - keyword skipped." << std::endl;
        return;
    }

    generate(of, file.filename().string(), jsonKeyword);

    delete jsonKeyword;
}

void scanAllKeywords(const boost::filesystem::path& directory , std::iostream& of, keywordGenerator generate) {
    boost::filesystem::directory_iterator end;
    for (boost::filesystem::directory_iterator iter(directory); iter != end; iter++) {
        if (boost::filesystem::is_directory(*iter))
            scanAllKeywords(*iter , of, generate);
        else
            scanKeyword(*iter , of, generate);
    }
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


void readDumpFromKeywords(boost::filesystem::path keywordPath, std::iostream& outputStream)
{
    scanAllKeywords( keywordPath , outputStream, &generateDumpForKeyword );
}

void generateSourceFile(const char* source_file_name, boost::filesystem::path keywordPath)
{
    std::fstream source_file_stream( source_file_name );
    createHeader(source_file_stream);
    startFunction(source_file_stream);
    scanAllKeywords( keywordPath , source_file_stream, &generateSourceForKeyword );
    endFunction(source_file_stream);
    source_file_stream << "}" << std::endl;
    source_file_stream.close();
}

void printUsage() {
    std::cout << "Generates source code for populating the parser's list of known keywords." << std::endl;
    std::cout << "Usage: createDefaultKeywordList <configroot> <sourcefilename> [<dumpfilename>]" << std::endl;
    std::cout << " <configroot>:     Path to keyword (JSON) files" << std::endl;
    std::cout << " <sourcefilename>: Path to source file to generate" << std::endl;
    std::cout << " <dumpfilename>:   Path to dump file containing state of keyword list at" << std::endl;
    std::cout << "                   last build (used for build triggering)." << std::endl;
}

int main(int argc , char ** argv) {
    const char * config_root = argv[1];
    const char * source_file_name = argv[2];
    const char * dump_file_name = argv[3];

    if (!argv[1]) {
        printUsage();
        return 0;
    }

    boost::filesystem::path keywordPath(config_root);
    bool needToGenerate = true;
    std::stringstream dump_stream;
    std::fstream dump_stream_on_disk(dump_file_name);

    if (dump_file_name) {
        readDumpFromKeywords( keywordPath , dump_stream );
        needToGenerate = !areStreamsEqual(dump_stream, dump_stream_on_disk);
    }

    std::cout << "needToGenerate: " << needToGenerate <<
                 ", source file: " << source_file_name <<
                 ", source file exists: " << boost::filesystem::exists(path(source_file_name)) << std::endl;

    if (needToGenerate || !boost::filesystem::exists(path(source_file_name))) {
        std::cout << "Keyword changes detected - generating keywords" << std::endl;
        generateSourceFile(source_file_name, keywordPath);
        dump_stream_on_disk.seekp(std::ios_base::beg);
        dump_stream.seekg(std::ios_base::beg);
        dump_stream_on_disk << dump_stream;
        dump_stream_on_disk.close();
    }
    else {
        std::cout << "No keyword changes detected - quitting" << std::endl;
        return 1;
    }
    return 0;
}
