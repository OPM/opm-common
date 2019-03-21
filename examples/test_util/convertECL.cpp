#include <iostream>
#include <chrono>
#include <tuple>
#include <iomanip>

#include <examples/test_util/EclFile.hpp>
#include <examples/test_util/EclOutput.hpp>


int main(int argc, char **argv) {

    if (argc!=2){
       std::cout << "\nInvalid input, need 1 argument which should be the eclipse output file to be converted \n" << std::endl;
       exit(1);
    }

    // start reading
    auto start = std::chrono::system_clock::now();
    std::string filename=argv[1];

    EclFile file1(filename);

    file1.loadData();

    auto end1 = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds1 = end1-start;

    bool formattedOutput=file1.formattedInput()==true ? false : true;

    int p=filename.find_last_of(".");
    int l=filename.length();

    std::string rootN=filename.substr(0,p);
    std::string extension=filename.substr(p,l-p);
    std::string resFile;


    std::ofstream outFileH;

    if (formattedOutput){
        resFile=rootN+".F"+extension.substr(1,extension.length()-1);
        outFileH.open(resFile, std::ios::out |  std::ios::binary  );
    } else {
        resFile=rootN+"."+extension.substr(2,extension.length()-2);
        outFileH.open(resFile, std::ios::out  );
    }

    std::cout << "\033[1;31m" << "\nconverting  " << argv[1] << " -> " << resFile << "\033[0m\n" << std::endl;

    EclOutput outFile(outFileH);

    std::vector<std::tuple<std::string, EIOD::eclArrType, int>> arrayList;

    arrayList=file1.getList();

    if (formattedOutput){

        //outFile.setManualFlush();

        for (unsigned int index=0;index<arrayList.size();index++){

            std::string name=std::get<0>(arrayList[index]);
            EIOD::eclArrType arrType=std::get<1>(arrayList[index]);
            int num=std::get<2>(arrayList[index]);

            outFile.writeFormattedHeader(name,num,arrType);

            if (arrType==EIOD::INTE){
                std::vector<int> vect=file1.get<int>(index);
                outFile.writeFormattedArray(vect);
            } else if (arrType==EIOD::REAL){
                std::vector<float> vect=file1.get<float>(index);
                outFile.writeFormattedArray(vect);
            } else if (arrType==EIOD::DOUB){
                std::vector<double> vect=file1.get<double>(index);
                outFile.writeFormattedArray(vect);
            } else if (arrType==EIOD::LOGI){
                std::vector<bool> vect=file1.get<bool>(index);
                outFile.writeFormattedArray(vect);
            } else if (arrType==EIOD::CHAR){
                std::vector<std::string> vect=file1.get<std::string>(index);
                outFile.writeFormattedCharArray(vect);
            } else if (arrType==EIOD::MESS){
                // shold not be any associated data
            } else {
                std::cout << "unknown array type " << std::endl;
                exit(1);
            }
        }

        // outFile.flushStrBuffer();

    } else {

        for (unsigned int index=0;index<arrayList.size();index++){

            std::string name=std::get<0>(arrayList[index]);
            EIOD::eclArrType arrType=std::get<1>(arrayList[index]);
            int num=std::get<2>(arrayList[index]);

            outFile.writeBinaryHeader(name,num,arrType);

            if (arrType==EIOD::INTE){
                std::vector<int> vect=file1.get<int>(index);
                outFile.writeBinaryArray(vect);
            } else if (arrType==EIOD::REAL){
                std::vector<float> vect=file1.get<float>(index);
               outFile.writeBinaryArray(vect);
            } else if (arrType==EIOD::DOUB){
                std::vector<double> vect=file1.get<double>(index);
                outFile.writeBinaryArray(vect);
            } else if (arrType==EIOD::LOGI){
                std::vector<bool> vect=file1.get<bool>(index);
                outFile.writeBinaryArray(vect);
            } else if (arrType==EIOD::CHAR){
                std::vector<std::string> vect=file1.get<std::string>(index);
                outFile.writeBinaryCharArray(vect);
            } else if (arrType==EIOD::MESS){
                // shold not be any associated data
            } else {
                std::cout << "unknown array type " << std::endl;
                exit(1);
            }
        }

    }

    outFileH.close();

    auto end2 = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds2 = end2-end1;

    std::cout << "\ntime to load from file : " << argv[1] << ": " << elapsed_seconds1.count() << " seconds" << std::endl;
    std::cout << "time to write to file  : " << resFile << ": " << elapsed_seconds2.count() << " seconds\n" << std::endl;

    return 0;
}
