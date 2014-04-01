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

typedef void (*keywordGenerator)(std::ofstream& of, const std::string keywordName, Json::JsonObject* jsonKeyword);

void createHeader(std::ofstream& of ) {
    of << "#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/ParserRecord.hpp>" << std::endl;
    of << "#include <opm/parser/eclipse/Parser/Parser.hpp>" << std::endl;
    of << "namespace Opm {"  << std::endl << std::endl;
}

void startFunction(std::ofstream& of) {
    of << "void Parser::addDefaultKeywords() { " << std::endl;
}


void endFunction(std::ofstream& of) {
    of << "}" << std::endl;
}


void generateSourceForKeyword(std::ofstream& of, const std::string keywordName, Json::JsonObject* jsonKeyword)
{
    if (ParserKeyword::validName(keywordName)) {
        ParserKeywordConstPtr parserKeyword(new ParserKeyword(*jsonKeyword));
        std::string indent("   ");
        of << "{" << std::endl;
        of << indent << "ParserKeyword *";
        parserKeyword->inlineNew(of , keywordName , indent);
        of << indent << "addKeyword( ParserKeywordConstPtr(" << keywordName << "));" << std::endl;
        of << "}" << std::endl << std::endl;

        std::cout << "Creating keyword: " << keywordName << std::endl;
    }
}

void generateDumpForKeyword(std::ofstream& of, const std::string keywordName, Json::JsonObject* jsonKeyword)
{
    of << keywordName << std::endl;
    of << jsonKeyword->get_content() << std::endl;
}

void scanKeyword(const boost::filesystem::path& file , std::ofstream& of, keywordGenerator generate) {
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

void scanAllKeywords(const boost::filesystem::path& directory , std::ofstream& of, keywordGenerator generate) {
    boost::filesystem::directory_iterator end;
    for (boost::filesystem::directory_iterator iter(directory); iter != end; iter++) {
        if (boost::filesystem::is_directory(*iter))
            scanAllKeywords(*iter , of, generate);
        else
            scanKeyword(*iter , of, generate);
    }
}

bool areFilesEqual (path file1, path file2) {
    const int BUFFER_SIZE = 1000*1024;
    std::ifstream file1stream(file1.c_str(), std::ifstream::in | std::ifstream::binary);
    std::ifstream file2stream(file2.c_str(), std::ifstream::in | std::ifstream::binary);

    if(!file1stream.is_open() || !file2stream.is_open())
    {
        return false;
    }

    char *buffer1 = new char[BUFFER_SIZE]();
    char *buffer2 = new char[BUFFER_SIZE]();

    do {
        file1stream.read(buffer1, BUFFER_SIZE);
        file2stream.read(buffer2, BUFFER_SIZE);

        if (std::memcmp(buffer1, buffer2, BUFFER_SIZE) != 0)
        {
            delete[] buffer1;
            delete[] buffer2;
            return false;
        }
    } while (file1stream.good() || file2stream.good());

    delete[] buffer1;
    delete[] buffer2;
    return true;
}




path generateDumpFile(boost::filesystem::path keywordPath)
{
    path root = unique_path("/tmp/%%%%-%%%%");
    create_directories(root);
    path temp_dump_file = root / "opm_createDefaultKeywordList_dumpfile";
    std::string temp_dump_file_name = temp_dump_file.string().c_str();
    std::ofstream temp_dump_file_stream( temp_dump_file_name );
    scanAllKeywords( keywordPath , temp_dump_file_stream, &generateDumpForKeyword );
    temp_dump_file_stream.close();

    return temp_dump_file;
}

void generateSourceFile(const char* source_file_name, boost::filesystem::path keywordPath)
{
    std::ofstream source_file_stream( source_file_name );
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

    if (dump_file_name) {
        path temp_dump_file = generateDumpFile(keywordPath);
        path dump_file_reference = dump_file_name;
        bool areEqual = areFilesEqual(dump_file_reference, temp_dump_file);
        copy_file(temp_dump_file,dump_file_reference,copy_option::overwrite_if_exists);

        if (areEqual) {
            std::cout << "No keyword changes detected - quitting" << std::endl;
            return 1;
        }
        else {
            std::cout << "Keyword changes detected - generating keywords" << std::endl;
        }
    }

    generateSourceFile(source_file_name, keywordPath);
}
