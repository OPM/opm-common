#include <opm/io/eclipse/ERst.hpp>
#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EclOutput.hpp>

#include <cctype>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <type_traits>

#include <getopt.h>

using namespace Opm::EclIO;

using EclEntry = EclFile::EclEntry;

namespace {

template <typename T>
void write(EclOutput& outFile, EclFile& file1,
           const std::string& name, int index)
{
    auto vect = file1.get<T>(index);
    outFile.write(name, vect);
}


template<typename T>
void write(EclOutput& outFile, ERst& file1,
           const std::string& name, int index, int reportStepNumber)
{
    auto vect = file1.getRestartData<T>(index, reportStepNumber);
    outFile.write(name, vect);
}

template<typename T>
void write(EclOutput& outFile, ERst& file1,
           const std::string& name, int index)
{
    auto vect = file1.get<T>(index);
    outFile.write(name, vect);
}


template <typename T>
void writeArray(std::string name, eclArrType arrType, T& file1, int index, EclOutput& outFile) {

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
    } else if (arrType == C0NN) {
        write<std::string>(outFile, file1, name, index);
    } else if (arrType == MESS) {
        outFile.message(name);
    } else {
        std::cout << "unknown array type " << std::endl;
        exit(1);
    }
}


template <typename T>
void writeArray(std::string name, eclArrType arrType, T& file1, int index, int reportStepNumber, EclOutput& outFile) {

    if (arrType == INTE) {
        write<int>(outFile, file1, name, index, reportStepNumber);
    } else if (arrType == REAL) {
        write<float>(outFile, file1, name, index, reportStepNumber);
    } else if (arrType == DOUB) {
        write<double>(outFile, file1, name, index, reportStepNumber);
    } else if (arrType == LOGI) {
        write<bool>(outFile, file1, name, index, reportStepNumber);
    } else if (arrType == CHAR) {
        write<std::string>(outFile, file1, name, index, reportStepNumber);
    } else if (arrType == MESS) {
        outFile.message(name);
    } else {
        std::cout << "unknown array type " << std::endl;
        exit(1);
    }
}


void writeC0nnArray(const std::string& name, int elementSize, EclFile& file1, int index, EclOutput& outFile)
{
    auto vect = file1.get<std::string>(index);
    outFile.write(name, vect, elementSize);
}


void writeArrayList(std::vector<EclEntry>& arrayList,
                    const std::vector<int>& elementSizeList,
                    EclFile& file1, EclOutput& outFile)
{

    for (size_t index = 0; index < arrayList.size(); index++) {
        std::string name = std::get<0>(arrayList[index]);
        eclArrType arrType = std::get<1>(arrayList[index]);

        if (arrType == Opm::EclIO::C0NN){
            writeC0nnArray(name, elementSizeList[index], file1, index, outFile);
        } else
            writeArray(name, arrType, file1, index, outFile);
    }
}

void writeArrayList(std::vector<EclEntry>& arrayList, ERst file1, int reportStepNumber, EclOutput& outFile) {

    for (size_t index = 0; index < arrayList.size(); index++) {
        std::string name = std::get<0>(arrayList[index]);
        eclArrType arrType = std::get<1>(arrayList[index]);
        writeArray(name, arrType, file1, index , reportStepNumber, outFile);
    }
}

void printHelp() {

    std::cout << "\nconvertECL needs one argument which is the input file to be converted. If this is a binary file the output file will be formatted. If the input file is formatted the output will be binary. \n"
              << "\nIn addition, the program takes these options (which must be given before the arguments):\n\n"
              << "-h Print help and exit.\n"
              << "-l List report step numbers in the selected restart file.\n"
              << "-g Convert file to grdecl format.\n"
              << "-o Specify output file name (only valid with grdecl option).\n"
              << "-i Enforce IX standard on output file.\n"
              << "-r Extract and convert a specific report time step number from a unified restart file. \n\n";
}

struct GrdeclDataFormatParams
{
    int ncol;
    int w;
    int pre;
    bool is_string;
    bool is_int;
};

template <typename T>
GrdeclDataFormatParams getFormat()
{
    if constexpr (std::is_same<T, float>::value) {
        return {4, 11, 7, false, false};
    } else if constexpr (std::is_same<T, double>::value) {
        return {3, 21, 14, false, false};
    } else if constexpr (std::is_same<T, int>::value) {
        return {8, 6, -1, false, true};
    } else if constexpr (std::is_same<T, std::string>::value) {
        return {5, -1, -1, true, false};
    } else {
        static_assert(!std::is_same_v<T,T>, "getFormat() called for invalid type");
    }
}

template <typename T>
void writeGrdeclData(std::ofstream& ofileH, const std::string& name, const std::vector<T>& array)
{
    const auto p = getFormat<T>();

    int64_t data_size = static_cast<int64_t>(array.size());

    ofileH << "\n" << name << std::endl;

    for (int64_t n = 0; n < data_size; n++){
        if (p.is_string)
            ofileH << " " << array[n];
        else if (p.is_int)
            ofileH << " " << std::setw(p.w) << array[n];
        else
            ofileH << " " << std::setw(p.w) << std::setprecision(p.pre) << std::scientific << array[n];

        if ((n+1) % p.ncol == 0)
            ofileH << "\n";
    }

    ofileH << " /\n";
}

void open_grdecl_output(const std::string& output_fname, const std::string& input_file, std::ofstream& ofileH)
{
    if (output_fname.empty()) {
        std::filesystem::path inputfile(input_file);
        const std::filesystem::path rootName = inputfile.stem();
        const std::filesystem::path path = inputfile.parent_path();
        std::filesystem::path grdeclfile = path / rootName;
        grdeclfile.replace_extension(".grdecl");

        if (std::filesystem::exists(grdeclfile)) {
            std::cout << "\nError, cant make grdecl file " << grdeclfile.string() << ". File exists \n";
            exit(1);
        }

        ofileH.open(grdeclfile, std::ios::out);
    }
    else {
        std::filesystem::path grdeclfile(output_fname);
        std::filesystem::path path = grdeclfile.parent_path();

        if (path.empty() || std::filesystem::exists(path)) {
            if (std::filesystem::exists(grdeclfile)) {
                std::cout << "\nError, cant make grdecl file " << grdeclfile.string() << ". File exists \n";
                exit(1);
            }
        }
        else {
            std::cout << "\n!Error, output directory : '" << path.string() << "' doesn't exist \n";
            exit(1);
        }

        ofileH.open(grdeclfile.string(), std::ios::out);
    }
}

} // Anonymous namespace

int main(int argc, char **argv)
{
    int c                          = 0;
    int reportStepNumber           = -1;
    bool specificReportStepNumber  = false;
    bool listProperties            = false;
    bool enforce_ix_output         = false;
    bool to_grdecl                 = false;

    const std::map<std::string, std::string> to_formatted {
        {".GRID"  , ".FGRID"  },
        {".EGRID" , ".FEGRID" },
        {".INIT"  , ".FINIT"  },
        {".SMSPEC", ".FSMSPEC"},
        {".UNSMRY", ".FUNSMRY"},
        {".UNRST" , ".FUNRST" },
        {".RFT"   , ".FRFT"   },
        {".ESMRY" , ".FESMRY" },
        {".LGR"  , ".FLGR"},
    };

    const std::map<std::string, std::string> to_binary {
        {".FGRID"  , ".GRID"  },
        {".FEGRID" , ".EGRID" },
        {".FINIT"  , ".INIT"  },
        {".FSMSPEC", ".SMSPEC"},
        {".FUNSMRY", ".UNSMRY"},
        {".FUNRST" , ".UNRST" },
        {".FRFT"   , ".RFT"   },
        {".FESMRY" , ".ESMRY" },
        {".FLGR"  , ".LGR"},
    };

    std::string output_fname{};
    while ((c = getopt(argc, argv, "hr:ligo:")) != -1) {
        switch (c) {
        case 'h':
            printHelp();
            return 0;
        case 'l':
            listProperties = true;
            break;
        case 'g':
            to_grdecl = true;
            break;
        case 'i':
            enforce_ix_output = true;
            break;
        case 'r':
            specificReportStepNumber = true;
            reportStepNumber = atoi(optarg);
            break;
        case 'o':
            output_fname = optarg;
            break;
        default:
            return EXIT_FAILURE;
        }
    }

    int argOffset = optind;

    if (!output_fname.empty() && !to_grdecl) {
        std::cout << "\n!Error, option -o only valid whit option -g \n\n";
        exit(1);
    }

    // start reading
    auto start = std::chrono::system_clock::now();
    std::string filename = argv[argOffset];

    EclFile file1(filename);
    bool formattedOutput = file1.formattedInput() ? false : true;

    int p = filename.find_last_of(".");
    int l = filename.length();

    std::string rootN = filename.substr(0,p);
    std::string extension = filename.substr(p,l-p);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char ckey){ return std::toupper(ckey);});  
    std::string resFile;


    if (to_grdecl) {
        auto array_list = file1.getList();
        file1.loadData();

        auto start_g = std::chrono::system_clock::now();

        std::ofstream ofileH;
        open_grdecl_output(output_fname, filename, ofileH);

        for (size_t n = 0; n < array_list.size(); n ++) {
            std::string name = std::get<0>(array_list[n]);
            auto arr_type = std::get<1>(array_list[n]);

            if (arr_type == Opm::EclIO::REAL) {
                auto  data = file1.get<float>(n);
                writeGrdeclData(ofileH, name, data);
             } else if (arr_type == Opm::EclIO::DOUB) {
                auto  data = file1.get<double>(n);
                writeGrdeclData(ofileH, name, data);
            } else if (arr_type == Opm::EclIO::INTE) {
                auto  data = file1.get<int>(n);
                writeGrdeclData(ofileH, name, data);
            } else if (arr_type == Opm::EclIO::CHAR) {
                auto  data = file1.get<std::string>(n);
                writeGrdeclData(ofileH, name, data);
            } else if (arr_type == Opm::EclIO::LOGI) {
                std::cout << "\n!Warning, skipping array '" << name << " of type LOGI \n";
            } else if (arr_type == Opm::EclIO::C0NN) {
                std::cout << "\n!Warning, skipping array '" << name << " of type C0NN \n";
            } else {
                std::cout << "unknown data type for array " << name << std::endl;
                exit(1);
            }
        }

        auto end_g = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end_g-start_g;

        std::cout << "\nruntime writing grdecl file : " << elapsed_seconds.count() << " seconds\n" << std::endl;

        std::cout << std::endl;
        return 0;
    }

    if (listProperties) {
        if (extension == ".UNRST") {
            ERst rst1(filename);
            rst1.loadData("INTEHEAD");

            std::vector<int> reportStepList = rst1.listOfReportStepNumbers();

            for (auto seqn : reportStepList) {
                std::vector<int> inteh = rst1.getRestartData<int>("INTEHEAD", seqn, 0);

                std::cout << "Report step number: "
                          << std::setfill(' ') << std::setw(4) << seqn << "   Date: " << inteh[66] << "/"
                          << std::setfill('0') << std::setw(2) << inteh[65] << "/"
                          << std::setfill('0') << std::setw(2) << inteh[64] << std::endl;
            }

            std::cout << std::endl;
        }
        else {
            std::cout << "\n!ERROR, option -l only only available for unified restart files (*.UNRST) " << std::endl;
            exit(1);
        }

        return 0;
    }

    if (formattedOutput) {
        auto search = to_formatted.find(extension);
        if (search != to_formatted.end()){
            resFile = rootN + search->second;
        }
        else if (extension.substr(1,1) == "X") {
            resFile = rootN + ".F" + extension.substr(2);
        }
        else if (extension.substr(1,1) == "S") {
            resFile = rootN + ".A" + extension.substr(2);
        }
        else {
            std::cout << "\n!ERROR, unknown file type for input file '" << rootN + extension << "'\n" << std::endl;
            exit(1);
        }
    }
    else {
        auto search = to_binary.find(extension);
        if (search != to_binary.end()){
            resFile = rootN + search->second;
        }
        else if (extension.substr(1,1) == "F") {
            resFile = rootN + ".X" + extension.substr(2);
        }
        else if (extension.substr(1,1) == "A") {
            resFile = rootN + ".S" + extension.substr(2);
        }
        else {
            std::cout << "\n!ERROR, unknown file type for input file '" << rootN + extension << "'\n" << std::endl;
            exit(1);
        }
    }

    std::cout << "\033[1;31m" << "\nconverting  " << argv[argOffset] << " -> " << resFile << "\033[0m\n" << std::endl;

    EclOutput outFile(resFile, formattedOutput);

    if (file1.is_ix() || enforce_ix_output) {
        std::cout << "setting IX flag on output file \n";
        outFile.set_ix();
    }

    if (specificReportStepNumber) {
        if (extension != ".UNRST") {
            std::cout << "\n!ERROR, option -r only can only be used with unified restart files (*.UNRST) " << std::endl;
            exit(1);
        }

        ERst rst1(filename);

        if (!rst1.hasReportStepNumber(reportStepNumber)) {
            std::cout << "\n!ERROR, selected unified restart file doesn't have report step number " << reportStepNumber << "\n" << std::endl;
            exit(1);
        }

        rst1.loadReportStepNumber(reportStepNumber);

        auto arrayList = rst1.listOfRstArrays(reportStepNumber);
        writeArrayList(arrayList, rst1, reportStepNumber, outFile);
    }
    else {
        file1.loadData();
        auto arrayList = file1.getList();
        std::vector<int> elementSizeList = file1.getElementSizeList();
        writeArrayList(arrayList, elementSizeList, file1, outFile);
    }

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;

    std::cout << "runtime  : " << argv[argOffset] << ": " << elapsed_seconds.count() << " seconds\n" << std::endl;

    return 0;
}
