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
#include <opm/parser/eclipse/Parser/Parser.hpp>

using namespace Opm;

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




int main(int argc , char ** argv) {
    const char * config_root = argv[1];
    const char * source_file_to_generate = argv[2];
    const char * keyword_dump_file = argv[3];
    boost::filesystem::path directory(config_root);

    if (keyword_dump_file) {
        std::ofstream dump_file_stream( keyword_dump_file );
        scanAllKeywords( directory , dump_file_stream, &generateDumpForKeyword );
        dump_file_stream.close();
    }

    std::ofstream of( source_file_to_generate );
    createHeader(of);
    startFunction(of);
    scanAllKeywords( directory , of, &generateSourceForKeyword );
    endFunction(of);
    of << "}" << std::endl;
    of.close();
}
