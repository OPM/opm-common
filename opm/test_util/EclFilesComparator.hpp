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

#ifndef ECLFILESCOMPARATOR_HPP
#define ECLFILESCOMPARATOR_HPP

#include <vector>
#include <string>

/// Prototype for eclipse file struct, defined in the ERT library
struct ecl_file_struct;
typedef struct ecl_file_struct ecl_file_type;
/// Prototype for eclipse grid struct, defined in the ERT library
struct ecl_grid_struct;
typedef struct ecl_grid_struct ecl_grid_type;

enum eclFileEnum {RESTARTFILE, INITFILE};



//! Deviation struct.
/*! The member variables are default initialized to -1, which is an invalid deviation value.*/
struct Deviation {
    double abs = -1;
    double rel = -1;
};



//! A class for handeling ECLIPSE files.
/*! ECLFilesComparator opens ECLIPSE files (.UNRST or .INIT in addition to .EGRID or .GRID) from two simulations.
 *  Then the functions gridCompare, results and resultsForKeyword can be invoked to compare the files from the simulations.
 *  Note that when either absolute or relative deviation is larger than the member variables (double) absTolerance and (double) relTolerance respectively,
 *  an exception is thrown. This is also the case if a value is negative and its absolute value is larger than absTolerance. */
class ECLFilesComparator {
    private:
        eclFileEnum fileType;
        ecl_file_type* ecl_file1 = nullptr;
        ecl_grid_type* ecl_grid1 = nullptr;
        ecl_file_type* ecl_file2 = nullptr;
        ecl_grid_type* ecl_grid2 = nullptr;
        double absTolerance      = 0;
        double relTolerance      = 0;
        bool showValues          = false;
        std::vector<std::string> keywords1, keywords2;
        std::vector<double>      absDeviation, relDeviation;

        // Keywords which should not contain negative values, i.e. uses allowNegativeValues = false in deviationsForCell():
        const std::vector<std::string> keywordWhitelist = {"SGAS", "SWAT", "PRESSURE"};

        bool keywordValidForComparing(const std::string& keyword) const;
        void printResultsForKeyword(const std::string& keyword) const;
        void deviationsForOccurence(const std::string& keyword, int occurence);
        void deviationsForCell(double val1, double val2, const std::string& keyword, int occurence, int cell, bool allowNegativeValues = true);
    public:
        //! \brief Open ECLIPSE files and set tolerances and keywords.
        //! \param[in] fileType Specifies which filetype to be compared, possible inputs are "RESTARTFILE" and "INITFILE".
        //! \param[in] basename1 Full path without file extension to the first case.
        //! \param[in] basename2 Full path without file extension to the second case.
        //! \param[in] absTolerance Tolerance for absolute deviation.
        //! \param[in] relTolerance Tolerance for relative deviation.
        //! \details The content of the ECLIPSE files specified in the input is stored in the ecl_file_type and ecl_grid_type member variables. In addition the keywords and absolute and relative tolerances (member variables) are set. If the constructor is unable to open one of the ECLIPSE files, an exception will be thrown.
        ECLFilesComparator(eclFileEnum fileType, const std::string& basename1, const std::string& basename2, double absTolerance, double relTolerance);
        //! \brief Closing the ECLIPSE files.
        ~ECLFilesComparator();

        //! \brief Setting the member variable showValues.
        //! \param[in] values Variable which is copied to #showValues
        //! \details When showValues is set to true, resultsForKeyword() will print all values side by side from the two files.
        void setShowValues(bool values) {this->showValues = values;}
        //! \brief Print all keywords stored in #keywords1 and #keywords2
        void printKeywords() const;
        //! \brief Comparing grids from the to gridfiles.
        //! \details This functions compares the number of active and global cells of the two grids. If the number of cells differ, an exception is thrown.
        bool gridCompare() const;
        //! \brief Calculating deviations for all keywords.
        //! \details results() loops over all keywords, checks if they are supported for comparison, and calls resultsForKeyword() for each keyword.
        void results();
        //! \brief Calculating deviations for a specific keyword.
        //! \param[in] keyword Keyword which should be compared.
        //! \details This function loops through every report step and every cell and compares the values for the given keyword from the two input cases. If the absolute or relative deviation between the two values for each step exceeds #absTolerance or #relTolerance respectively, or a value is both negative and has a larger absolute value than #absTolerance, an exception is thrown. The function also temporarily stores the absolute and relative deviation in #absDeviation and #relDeviation, and prints average and median values.
        void resultsForKeyword(const std::string keyword);

        //! \brief Calculate deviations for two values.
        //! \details Using absolute values of the input arguments: If one of the values are non-zero, the Deviation::abs returned is the difference between the two input values. In addition, if both values are non-zero, the Deviation::rel returned is the absolute deviation divided by the largest value.
        static Deviation calculateDeviations(double val1, double val2);
        //! \brief Calculate median of a vector.
        //! \details Returning the median of the input vector, i.e. the middle value of the sorted vector if the number of elements is odd or the mean of the two middle values if the number of elements are even.
        static double median(std::vector<double> vec);
        //! \brief Calculate average of a vector.
        //! \details Returning the average of the input vector, i.e. the sum of all values divided by the number of elements.
        static double average(const std::vector<double>& vec);
};

#endif
