#include <opm/parser/eclipse/Generator/KeywordGenerator.hpp>
#include <opm/parser/eclipse/Generator/KeywordLoader.hpp>



int main(int argc , char ** argv) {
    if (argc == 5) {
        const char * config_root = argv[1];
        const char * source_file_name = argv[2];
        const char * header_file_name = argv[3];
        const char * test_file_name = argv[4];
        bool verboseLoader = false;
        bool verboseGenerator = true;

        Opm::KeywordLoader loader(verboseLoader);
        Opm::KeywordGenerator generator(verboseGenerator);
        loader.loadMultipleKeywordDirectories( config_root );

        generator.updateSource(loader , source_file_name );
        generator.updateHeader(loader , header_file_name );
        generator.updateTest(loader , test_file_name );

        exit(0);
    } else {
        std::cerr << "Error calling keyword generator: Expected arguments: <config_root> <source_file_name> <header_file_name> <test_file_name>" << std::endl;
        exit(1);
    }
}
