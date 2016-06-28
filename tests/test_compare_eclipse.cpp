#include "test_compare_eclipse.hpp"

int main(int argc, char** argv) {
    char* gridFile1 = argv[1];
    char* gridFile2 = argv[2];
    char* unrstFile1 = argv[3];
    char* unrstFile2 = argv[4];

    if (argc != 5) {
        printHelp();
    }
    else {
        // Comparing grid sizes from .EGRID-files
        {
            ecl_grid_type* ecl_grid1 = ecl_grid_alloc(gridFile1);
            ecl_grid_type* ecl_grid2 = ecl_grid_alloc(gridFile2);

            std::cout << "\nName of grid1: " << ecl_grid_get_name(ecl_grid1) << std::endl;
            int gridCount1 = ecl_grid_get_global_size(ecl_grid1);
            std::cout << "Grid1 count = " << gridCount1 << std::endl;
            std::cout << "Name of grid2: " << ecl_grid_get_name(ecl_grid2) << std::endl;
            int gridCount2 = ecl_grid_get_global_size(ecl_grid2);
            std::cout << "Grid2 count = " << gridCount2 << std::endl;

            ecl_grid_free(ecl_grid1);
            ecl_grid_free(ecl_grid2);
        }

        //Comparing keyword values from .UNRST-files:

        ReadUNRST read;
        if (read.open(unrstFile1, unrstFile2)) {
            for (const char* keyword : {"SGAS", "SWAT", "PRESSURE"}) {
                std::cout << "\nKeyword " << keyword << ":\n\n";

                std::vector<double> absDeviation, relDeviation;
                read.results(keyword, &absDeviation, &relDeviation);

                std::cout << "absDeviation size = " << absDeviation.size() << std::endl;
                std::cout << "relDeviation size = " << relDeviation.size() << std::endl;
                std::cout << "Average absolute deviation = " << ReadUNRST::average(absDeviation) << std::endl;
                std::cout << "Median absolute deviation = " << ReadUNRST::median(absDeviation) << std::endl;
                std::cout << "Average relative deviation = " << ReadUNRST::average(relDeviation) << std::endl;
                std::cout << "Median relative deviation = " << ReadUNRST::median(relDeviation) << std::endl;
            }
            std::cout << std::endl;
            read.close();
        }
    }
    return 0;
}

//------------------------------------------------//

void printHelp() {
    std::cout << "The program takes four arguments:\n"
        << "1. .EGRID-file number 1\n" 
        << "2. .EGRID-file number 2\n" 
        << "2. .UNRST-file number 1\n" 
        << "3. .UNRST-file number 2\n";
}

bool ReadUNRST::open(const char* unrstFile1, const char* unrstFile2) {
    if (ecl_file1 == NULL) {
        ecl_file1 = ecl_file_open(unrstFile1, ECL_FILE_CLOSE_STREAM);
    }
    if (ecl_file2 == NULL) {
        ecl_file2 = ecl_file_open(unrstFile2, ECL_FILE_CLOSE_STREAM);
    }

    if (ecl_file1 == NULL || ecl_file2 == NULL) {
        return false;
    }
    else {
        return true;
    }
}

void ReadUNRST::close() {
    ecl_file_close(ecl_file1);
    ecl_file_close(ecl_file2);
}

bool ReadUNRST::results(const char* keyword, std::vector<double>* absDeviation, std::vector<double>* relDeviation) {
    if (!ecl_file_has_kw(ecl_file1, keyword) || !ecl_file_has_kw(ecl_file2, keyword)) {
        std::cout << "The file does not have this keyword." << std::endl;
        return false;
    }
    unsigned int occurrences1 = ecl_file_get_num_named_kw(ecl_file1, keyword);
    unsigned int occurrences2 = ecl_file_get_num_named_kw(ecl_file2, keyword);
    if (occurrences1 != occurrences2) {
        std::cout << "Keyword occurrences are not equal." << std::endl;
        return false;
    }

    for (unsigned int index = 0; index < occurrences1; ++index) {
        ecl_kw_type* ecl_kw1 = ecl_file_iget_named_kw(ecl_file1, keyword, index);
        unsigned int numActiveCells1 = ecl_kw_get_size(ecl_kw1);
        ecl_kw_type* ecl_kw2 = ecl_file_iget_named_kw(ecl_file2, keyword, index);
        unsigned int numActiveCells2 = ecl_kw_get_size(ecl_kw2);
        if (numActiveCells1 != numActiveCells2) {
            std::cout << "Number of active cells are different." << std::endl;
            return false;
        }

        std::vector<double> values1; // Elements in the vector corresponds to active cells
        std::vector<double> values2; // Elements in the vector corresponds to active cells
        values1.resize(numActiveCells1);
        values2.resize(numActiveCells2);
        ecl_kw_get_data_as_double(ecl_kw1, values1.data());
        ecl_kw_get_data_as_double(ecl_kw2, values2.data());

        for (unsigned int i = 0; i < values1.size(); i++) {
            calculateDeviations(absDeviation, relDeviation, values1[i], values2[i]);
        }
    }
    return true;
}


void ReadUNRST::calculateDeviations(std::vector<double> *absDeviationVector,std::vector<double> *relDeviationVector, double val1, double val2){
    double absDeviation = 0;
    double relDeviation = 0;
    if (val1 < 0) {
        val1 = 0;
        //std::cout << "Value 1 has a negative quantity." << std::endl; 
    }
    if (val2 < 0) {
        val2 = 0;
        //std::cout << "Value 2 has a negative quantity." << std::endl; 
    }

    if (!(val1 == 0 && val2 == 0)) {
        absDeviation = std::max(val1, val2) - std::min(val1, val2);
        absDeviationVector->push_back(absDeviation);
        if (val1 != 0 && val2 != 0) {
            relDeviation = absDeviation/static_cast<double>(std::max(val1, val2));
            relDeviationVector->push_back(relDeviation);
        }
    }
}

double ReadUNRST::max(std::vector<double>& vec){
    double maxValue = 0;
    for(unsigned int i = 0; i < vec.size(); ++i){
        if (vec[i] > maxValue) {
            maxValue = vec[i];
        }
    }
    return maxValue;
}

double ReadUNRST::median(std::vector<double>& vec) {
    if(vec.empty()) return 0;
    else {
        std::sort(vec.begin(), vec.end());
        if(vec.size() % 2 == 0)
            return (vec[vec.size()/2 - 1] + vec[vec.size()/2]) / 2;
        else
            return vec[vec.size()/2];
    }
}

double ReadUNRST::average(std::vector<double>& vec) {
    if(vec.empty()) return 0;
    else {
        double sum = 0;
        for (unsigned int i = 0; i < vec.size(); ++i) {
            sum += vec[i];
        }
        return sum/vec.size();
    }
}
