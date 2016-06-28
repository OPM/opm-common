/*
   Copyright 2016 Statoil ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#ifndef TEST_COMPAREECLIPSERESTART_HPP
#define TEST_COMPAREECLIPSERESTART_HPP

#include <iostream>
#include <vector>
#include <algorithm>
#include <opm/common/ErrorMacros.hpp>
#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_kw_magic.h>

void printHelp();




struct Deviation {
    double abs = -1;
    double rel = -1;
};




class ReadUNRST {
    private:
        ecl_file_type* ecl_file1;
        ecl_file_type* ecl_file2;

    public:
        ReadUNRST(): ecl_file1(nullptr), ecl_file2(nullptr) {}
        bool open(const char* unrstFile1, const char* unrstFile2);
        void close();

        bool results(const char* keyword, std::vector<double>& absDeviaiton, std::vector<double>& relDeviation) const;
        Deviation calculateDeviations(double val1, double val2) const;

        static double median(std::vector<double> vec);
        static double average(const std::vector<double>& vec);
};

#endif
