/*
   Copyright 2016 Statoil ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#ifndef SUMMARYREGRESSIONTEST_HPP
#define SUMMARYREGRESSIONTEST_HPP

#include <opm/test_util/summaryComparator.hpp>

//! \details  The class inherits from the SummaryComparator class, which takes care of all file reading. \n The RegressionTest class compares the values from the two different files and throws exceptions when the deviation is unsatisfying.
class RegressionTest: public SummaryComparator {
    private:
        //! \brief Gathers the correct data for comparison for a specific keyword
        //! \param[in] timeVec1 The time steps of file 1.
        //! \param[in] timeVec2 The time steps of file 2.
        //! \param[in] keyword The keyword of interest
        //! \details The function prepares a regression test by gathering the data, stroing it into two vectors, \n deciding which is to be used as a reference/basis and calling the test function.
        void checkForKeyword(std::vector<double>& timeVec1, std::vector<double>& timeVec2, const char* keyword);

        //! \brief The regression test
        //! \param[in] keyword The keyword common for both the files. The vectors associated with the keyword are used for comparison.
        //! \details Start test uses the private member variables, pointers of std::vector<double> type, which are set to point to the correct vectors in SummaryComparison::chooseReference(...). \n The function iterates over the referenceVev/basis and for each iteration it calculates the deviation with SummaryComparison::getDeviation(..) and stors it in a Deviation struct. \n SummaryComparison::getDeviation takes the int jvar as an reference input, and using it as an iterative index for the values which are to be compared with the basis. \n Thus, by updating the jvar variable every time a deviation is calculated, one keep track jvar and do not have to iterate over already checked values.
        void startTest(const char* keyword);

        //! \brief Caluculates a deviation, throws exceptions and writes and error message.
        //! \param[in] deviation Deviation struct
        //! \param[in] keyword The keyword that the data that are being compared belongs to.
        //! \param[in] refIndex The report step of which the deviation originates from in #referenceDataVec.
        //! \param[in] checkIndex The report step of which the deviation originates from in #checkDataVec.
        //! \details The function checks the values of the Deviation struct against the absolute and relative tolerance, which are private member values of the super class. \n When comparing against the relative tolerance an additional term is added, the absolute deviation has to be greater than 1e-6 for the function to throw an exception. \n When the deviations are too great, the function writes out which keyword, and at what report step the deviation is too great before throwing an exception.
        void checkDeviation(Deviation deviation, const char* keyword, int refIndex, int checkIndex);

        bool isRestartFile = false; //!< Private member variable, when true the files that are being compared is a restart file vs a normal file
    public:
        //! \brief Constructor, creates an object of RefressionTest class.
        //! \param[in] basename1 Path to file1 without extension.
        //! \param[in] basename1 Path to file2 without extension.
        //! \param[in] relativeTol The relative tolerance which is to be used in the test.
        //! \param[in] absoluteTol The absolute tolerance which is to be used in the test.
        //! \details The constructor calls the constructor of the super class.
        RegressionTest(const char* basename1, const char* basename2, double relativeTol, double absoluteTol):
            SummaryComparator(basename1, basename2, relativeTol, absoluteTol) {}

        //! \details The function executes a regression test for all the keywords. If the two files do not match in amount of keywords, an exception is thrown.
        void getRegressionTest();

        //! \details The function executes a regression test for one specific keyword. If one or both of the files do not have the keyword, an exception is thrown.
        void getRegressionTest(const char* keyword);///< Regression test for a certain keyword of the files.

        //! \brief This function sets the private member variable isRestartFiles
        //! \param[in] boolean Boolean value
        void setIsRestartFile(bool boolean){this->isRestartFile = boolean;}
};

#endif
