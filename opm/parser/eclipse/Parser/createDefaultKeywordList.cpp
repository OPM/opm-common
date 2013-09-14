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


void inlineKeyword(const boost::filesystem::path& file , std::ofstream& of) {
    std::string keyword(file.filename().string());
    if (ParserKeyword::validName(keyword)) {
        Json::JsonObject jsonKeyword(file);
        ParserKeywordConstPtr parserKeyword(new ParserKeyword(jsonKeyword));
        of << "{" << std::endl;
        of << "ParserKeyword *";
        parserKeyword->inlineNew(of , keyword);
        of << "addKeyword( ParserKeywordConstPtr(" << keyword << "));" << std::endl;
        of << "}" << std::endl << std::endl;
    }
}

void inlineAllKeywords(const boost::filesystem::path& directory , std::ofstream& of) {
    boost::filesystem::directory_iterator end;
    for (boost::filesystem::directory_iterator iter(directory); iter != end; iter++) {
        if (boost::filesystem::is_directory(*iter))
            inlineAllKeywords(*iter , of);
        else
            inlineKeyword(*iter , of);
    }
}




int main(int argc , char ** argv) {
    const char * config_root = argv[1];
    const char * target_file = argv[2];
    boost::filesystem::path directory(config_root);
    std::ofstream of( target_file );

    createHeader(of);
    startFunction(of);
    inlineAllKeywords( directory , of );
    endFunction(of);
    of << "}" << std::endl;

    of.close();
}
