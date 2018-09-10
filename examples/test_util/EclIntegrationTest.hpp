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

#ifndef ECLINTEGRATIONTEST_HPP
#define ECLINTEGRATIONTEST_HPP

#include "EclFilesComparator.hpp"

/*! \brief A class for executing a integration test for two ECLIPSE files.
    \details This class inherits from ECLFilesComparator, which opens and closes
             the input cases and stores keywordnames. The three public functions
             equalNumKeywords(), results() and resultsForKeyword() can be invoked
             to compare griddata or keyworddata for all keywords or a given
             keyword (resultsForKeyword()).
*/
class ECLIntegrationTest: public ECLFilesComparator {
    private:
        std::vector<double> cellVolumes; //!< Vector of cell volumes in second input case (indexed by global index)
        std::vector<double> initialCellValues; //!< Keyword values for all cells at first occurrence (index by global index)

        // These are the only keywords which are compared, since SWAT should be "1 - SOIL - SGAS", this keyword is omitted.
        const std::vector<std::string> keywordWhitelist = {"SGAS", "SWAT", "PRESSURE"};

        void setCellVolumes();
        void initialOccurrenceCompare(const std::string& keyword);
        void occurrenceCompare(const std::string& keyword, int occurrence) const;
    public:
        //! \brief Sets up the integration test.
        //! \param[in] basename1 Full path without file extension to the first case.
        //! \param[in] basename2 Full path without file extension to the second case.
        //! \param[in] absTolerance Tolerance for absolute deviation.
        //! \param[in] relTolerance Tolerance for relative deviation.
        //! \details This constructor calls the constructor of the superclass, with input filetype unified restart. See the docs for ECLFilesComparator for more information.
        ECLIntegrationTest(const std::string& basename1, const std::string& basename2, double absTolerance, double relTolerance);

        //! \brief Checks if a keyword is supported for comparison.
        //! \param[in] keyword Keyword to check.
        bool elementInWhitelist(const std::string& keyword) const;
        //! \brief Checks if the number of keywords equal in the two input cases.
        //! \param[in] keyword Keyword to check.
        void equalNumKeywords() const;

        //! \brief Finds deviations for all supported keywords.
        //! \details results() loops through all supported keywords for integration test (defined in keywordWhitelist -- this is SGAS, SWAT and PRESSURE) and calls resultsForKeyword() for each keyword.
        void results();
        //! \brief Finds deviations for a specific keyword.
        //! \param[in] keyword Keyword to check.
        /*! \details First, resultsForKeyword() checks if the keyword exits in both cases, and if the number of keyword occurrences in the two cases differ. If these tests fail, an exception is thrown. Then deviaitons are calculated as described below for each occurrence, and an exception is thrown if the relative error ratio \f$E\f$ is larger than the relative tolerance.
        * Calculation:\n
        * Let the keyword values for occurrence \f$n\f$ and cell \f$i\f$ be \f$p_{n,i}\f$ and \f$q_{n,i}\f$ for input case 1 and 2, respectively.
        * Consider first the initial occurrence (\f$n=0\f$). The function uses the second cases as reference, and calculates the volume weighted sum of \f$q_{0,i}\f$ over all cells \f$i\f$:
        * \f[ S_0 = \sum_{i} q_{0,i} v_i \f]
        * where \f$v_{i}\f$ is the volume of cell \f$i\f$ in case 2. Then, the deviations between the cases for each cell are calculated:
        * \f[ \Delta = \sum_{i} |p_{0,i} - q_{0,i}| v_i.\f]
        * The error ratio is then \f$E = \Delta/S_0\f$.\n
        * For all other occurrences \f$n\f$, the deviation value \f$\Delta\f$ is calculated the same way, but the total value \f$S\f$ is calculated relative to the initial occurrence total \f$S_0\f$:
        * \f[ S = \sum_{i} |q_{n,i} - q_{0,i}| v_i. \f]
        * The error ratio is \f$ E = \Delta/S\f$. */
        void resultsForKeyword(const std::string& keyword);
};

#endif
