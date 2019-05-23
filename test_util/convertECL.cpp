#include <chrono>
#include <iomanip>
#include <iostream>
#include <tuple>

#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EclOutput.hpp>

using namespace Opm::ecl;

template<typename T>
void write(EclOutput& outFile, EclFile& file1,
           const std::string& name, int index)
{
    auto vect = file1.get<T>(index);
    outFile.write(name, vect);
}


int main(int argc, char **argv) {

    if (argc != 2) {
       std::cout << "\nInvalid input, need 1 argument which should be the eclipse output file to be converted \n" << std::endl;
       exit(1);
    }

    // start reading
    auto start = std::chrono::system_clock::now();
    std::string filename = argv[1];

    EclFile file1(filename);

    file1.loadData();

    auto end1 = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds1 = end1 - start;

    bool formattedOutput = file1.formattedInput() ? false : true;

    int p = filename.find_last_of(".");
    int l = filename.length();

    std::string rootN = filename.substr(0,p);
    std::string extension = filename.substr(p,l-p);
    std::string resFile;

    if (formattedOutput) {
        resFile = rootN + ".F" + extension.substr(1, extension.length() - 1);
    } else {
        resFile = rootN + "." + extension.substr(2, extension.length() - 2);
    }

    std::cout << "\033[1;31m" << "\nconverting  " << argv[1] << " -> " << resFile << "\033[0m\n" << std::endl;

    EclOutput outFile(resFile, formattedOutput);

    auto arrayList = file1.getList();

    for (size_t index = 0; index < arrayList.size(); index++) {
        std::string name = std::get<0>(arrayList[index]);
        eclArrType arrType = std::get<1>(arrayList[index]);

        if (arrType == INTE) {
            write<int>(outFile, file1, name, index);
        } else if (arrType == REAL) {
            write<float>(outFile, file1, name,index);
        } else if (arrType == DOUB) {
            write<double>(outFile, file1, name, index);
        } else if (arrType == LOGI) {
            write<bool>(outFile, file1, name, index);
        } else if (arrType == CHAR) {
            write<std::string>(outFile, file1, name, index);
        } else if (arrType == MESS) {
            // shold not be any associated data
            outFile.write(name,std::vector<char>());
        } else {
            std::cout << "unknown array type " << std::endl;
            exit(1);
        }
    }

    auto end2 = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds2 = end2-end1;

    std::cout << "\ntime to load from file : " << argv[1] << ": " << elapsed_seconds1.count() << " seconds" << std::endl;
    std::cout << "time to write to file  : " << resFile << ": " << elapsed_seconds2.count() << " seconds\n" << std::endl;

    return 0;
}
