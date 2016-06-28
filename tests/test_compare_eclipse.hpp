#pragma once

#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_kw_magic.h>
#include <iostream>
#include <vector>
#include <algorithm>

void printHelp();

class ReadUNRST {
    private:
        ecl_file_type* ecl_file1;
        ecl_file_type* ecl_file2;

    public:
        ReadUNRST(): ecl_file1(NULL), ecl_file2(NULL) {}
        bool open(const char* unrstFile1, const char* unrstFile2);
        void close();

        bool results(const char* keyword, std::vector<double>* absDeviaiton, std::vector<double>* relDeviation);
        static void calculateDeviations(std::vector<double> *absdev_vec,std::vector<double> *reldev_vec, double val1, double val2);

        static double max(std::vector<double>& vec);
        static double median(std::vector<double>& vec);
        static double average(std::vector<double>& vec);
};
