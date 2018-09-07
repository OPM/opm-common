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

#ifndef ECLREGRESSIONTEST_HPP
#define ECLREGRESSIONTEST_HPP

#include "EclFilesComparator.hpp"

/*! \brief A class for executing a regression test for two ECLIPSE files.
    \details This class inherits from ECLFilesComparator, which opens and
             closes the input cases and stores keywordnames.
             The three public functions gridCompare(), results() and
             resultsForKeyword() can be invoked to compare griddata
             or keyworddata for all keywords or a given keyword (resultsForKeyword()).
 */

class ECLRegressionTest: public ECLFilesComparator {
    private:
        // These vectors store absolute and relative deviations, respecively. Note that they are whiped clean for every new keyword comparison.
        std::vector<double> absDeviation, relDeviation;
        // Keywords which should not contain negative values, i.e. uses allowNegativeValues = false in deviationsForCell():
        const std::vector<std::string> keywordDisallowNegatives = {"SGAS", "SWAT", "PRESSURE"};

        // Only compare last occurrence
        bool onlyLastOccurrence = false;

        // Prints results stored in absDeviation and relDeviation.
        void printResultsForKeyword(const std::string& keyword) const;

        // Function which compares data at specific occurrences and for a specific keyword type. The functions takes two occurrence inputs to also be able to
        // compare keywords which are shifted relative to each other in the two files. This is for instance handy when running flow with restart from different timesteps,
        // and comparing the last timestep from the two runs.
        void boolComparisonForOccurrence(const std::string& keyword, int occurrence1, int occurrence2) const;
        void charComparisonForOccurrence(const std::string& keyword, int occurrence1, int occurrence2) const;
        void intComparisonForOccurrence(const std::string& keyword, int occurrence1, int occurrence2) const;
        void doubleComparisonForOccurrence(const std::string& keyword, int occurrence1, int occurrence2);
        // deviationsForCell throws an exception if both the absolute deviation AND the relative deviation
        // are larger than absTolerance and relTolerance, respectively. In addition,
        // if allowNegativeValues is passed as false, an exception will be thrown when the absolute value
        // of a negative value exceeds absTolerance. If no exceptions are thrown, the absolute and relative deviations are added to absDeviation and relDeviation.
        void deviationsForCell(double val1, double val2, const std::string& keyword, int occurrence1, int occurrence2, size_t kw_size, size_t cell, bool allowNegativeValues = true);
    public:
        //! \brief Sets up the regression test.
        //! \param[in] file_type Specifies which filetype to be compared, possible inputs are UNRSTFILE, INITFILE and RFTFILE.
        //! \param[in] basename1 Full path without file extension to the first case.
        //! \param[in] basename2 Full path without file extension to the second case.
        //! \param[in] absTolerance Tolerance for absolute deviation.
        //! \param[in] relTolerance Tolerance for relative deviation.
        //! \details This constructor only calls the constructor of the superclass, see the docs for ECLFilesComparator for more information.
        ECLRegressionTest(int file_type, const std::string& basename1, const std::string& basename2, double absTolerance, double relTolerance):
            ECLFilesComparator(file_type, basename1, basename2, absTolerance, relTolerance) {}

        //! \brief Option to only compare last occurrence
        void setOnlyLastOccurrence(bool onlyLastOccurrenceArg) {this->onlyLastOccurrence = onlyLastOccurrenceArg;}

        //! \brief Compares grid properties of the two cases.
        // gridCompare() checks if both the number of active and global cells in the two cases are the same. If they are, and volumecheck is true, all cells are looped over to calculate the cell volume deviation for the two cases. If the both the relative and absolute deviation exceeds the tolerances, an exception is thrown.
        void gridCompare(const bool volumecheck) const;
        //! \brief Calculates deviations for all keywords.
        // This function checks if the number of keywords of the two cases are equal, and if it is, resultsForKeyword() is called for every keyword. If not, an exception is thrown.
        void results();
        //! \brief Calculates deviations for a specific keyword.
        //! \param[in] keyword Keyword which should be compared, if this keyword is absent in one of the cases, an exception will be thrown.
        //! \details This function loops through every report step and every cell and compares the values for the given keyword from the two input cases. If the absolute or relative deviation between the two values for each step exceeds both the absolute tolerance and the relative tolerance (stored in ECLFilesComparator), an exception is thrown. In addition, some keywords are marked for "disallow negative values" -- these are SGAS, SWAT and PRESSURE. An exception is thrown if a value of one of these keywords is both negative and has an absolute value larger than the absolute tolerance. If no exceptions are thrown, resultsForKeyword() uses the private member funtion printResultsForKeyword to print the average and median deviations.
        void resultsForKeyword(const std::string& keyword);
};

#endif
