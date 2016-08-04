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

#ifndef SUMMARYCOMPARATOR_HPP
#define SUMMARYCOMPARATOR_HPP

#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <string>

//! \brief Prototyping struct, encapsuling the stringlist libraries.
struct stringlist_struct;
typedef struct stringlist_struct stringlist_type;

//! \brief Prototyping struct, encapsuling the ert libraries.
struct ecl_sum_struct;
typedef struct ecl_sum_struct ecl_sum_type;


//! \brief Struct for storing the deviation between two values.
struct Deviation {
    double abs = 0;//!< The absolute deviation
    double rel = 0; //!< The relative deviation
};


class SummaryComparator {
    private:
        double absoluteTolerance = 0; //!< The maximum absolute deviation that is allowed between two values.
        double relativeTolerance = 0; //!< The maximum relative deviation that is allowed between twi values.
    protected:
        ecl_sum_type * ecl_sum1                = nullptr; //!< Struct that contains file1
        ecl_sum_type * ecl_sum2                = nullptr; //!< Struct that contains file2
        ecl_sum_type * ecl_sum_fileShort       = nullptr; //!< For keeping track of the file with most/fewest timesteps
        ecl_sum_type * ecl_sum_fileLong        = nullptr; //!< For keeping track of the file with most/fewest timesteps
        stringlist_type* keys1                 = nullptr; //!< For storing all the keywords of file1
        stringlist_type* keys2                 = nullptr; //!< For storing all the keywords of file2
        stringlist_type * keysShort            = nullptr; //!< For keeping track of the file with most/fewest keywords
        stringlist_type * keysLong             = nullptr; //!< For keeping track of the file with most/fewest keywords
        const std::vector<double> * referenceVec     = nullptr; //!< For storing the values of each time step for the file containing the fewer time steps.
        const std::vector<double> * referenceDataVec = nullptr; //!< For storing the data corresponding to each time step for the file containing the fewer time steps.
        const std::vector<double> * checkVec         = nullptr; //!< For storing the values of each time step for the file containing the more time steps.
        const std::vector<double> * checkDataVec     = nullptr; //!< For storing the data values corresponding to each time step for the file containing the more time steps.
        bool printKeyword = false; //!< Boolean value for choosing whether to print the keywords or not
        bool printSpecificKeyword = false; //!< Boolean value for choosing whether to print the vectors of a keyword or not

        //! \brief Calculate deviation between two data values and stores it in a Deviation struct.
        //! \param[in] refIndex Index in reference data
        //! \param[in] checkindex Index in data to be checked.
        //! \param[out] dev Holds the result from the comparison on return.
        //! \details Uses the #referenceVec as basis, and checks its values against the values in #checkDataVec. The function is reccursive, and will update the iterative index j of the #checkVec until #checkVec[j] >= #referenceVec[i]. \n When #referenceVec and #checkVec have the same time value (i.e. #referenceVec[i] == #checkVec[j]) a direct comparison is used, \n when this is not the case, when #referenceVec[i] do not excist as an element in #checkVec, a value is generated, either by the principle of unit step or by interpolation.
        void getDeviation(size_t refIndex, size_t &checkIndex, Deviation &dev);

        //! \brief Figure out which data file contains the most / less timesteps and assign member variable pointers accordingly.
        //! \param[in] timeVec1 Data from first file
        //! \param[in] timeVec2 Data from second file
        //! \details Figure out which data file that contains the more/fewer time steps and assigns the private member variable pointers #ecl_sum_fileShort / #ecl_sum_fileLong to the correct data sets #ecl_sum1 / #ecl_sum2.
        void setDataSets(const std::vector<double>& timeVec1,
                         const std::vector<double>& timeVec2);

        //! \brief  Reads in the time values of each time step.
        //! \param[in] timeVec1 Vector for storing the time steps from file1
        //! \param[in] timeVec2 Vector for storing the time steps from file2
        void setTimeVecs(std::vector<double> &timeVec1,std::vector<double> &timeVec2);

        //! \brief Read the data for one specific keyword into two separate vectors.
        //! \param[in] dataVec1 Vector for storing the data for one specific keyword from file1
        //! \param[in] dataVec2 Vector for storing the data for one specific keyword from file2
        //! \details The two data files do not necessarily have the same amount of data values, but the values must correspond to the same interval in time. Thus possible to interpolate values.
        void getDataVecs(std::vector<double> &dataVec1,
                         std::vector<double> &dataVec2, const char* keyword);

        //! \brief Sets one data set as a basis and the other as values to check against.
        //! \param[in] timeVec1 Used to figure out which dataset that have the more/fewer time steps.
        //! \param[in] timeVec2 Used to figure out which dataset that have the more/fewer time steps.
        //! \param[in] dataVec1 For assiging the the correct pointer to the data vector.
        //! \param[in] dataVec2 For assiging the the correct pointer to the data vector.
        //! \details Figures out which time vector that contains the fewer elements. Sets this as #referenceVec and its corresponding data as #referenceDataVec. \n The remaining data set is set as #checkVec (the time vector) and #checkDataVec.
        void chooseReference(const std::vector<double> &timeVec1,
                             const std::vector<double> &timeVec2,
                             const std::vector<double> &dataVec1,
                             const std::vector<double> &dataVec2);

        //! \brief Returns the relative tolerance.
        double getRelTolerance(){return this->relativeTolerance;}

        //! \brief Returns the absolute tolerance.
        double getAbsTolerance(){return this->absoluteTolerance;}

        //! \brief Returns the unit of the values of a keyword
        //! \param[in] keyword The keyword of interest.
        //! \param[out] ret The unit of the keyword as a const char*.
        const char* getUnit(const char* keyword);

        //! \brief Prints the units of the files.
        void printUnits();

        //! \brief Prints the keywords of the files.
        //! \details The function prints first the common keywords, than the keywords that are different.
        void printKeywords();

        //! \brief Prints the summary vectors from the two files.
        //! \details The function requires that the summary vectors of the specific file have been read into the member variables referenceVec etc.
        void printDataOfSpecificKeyword(const std::vector<double>& timeVec1,
                                        const std::vector<double>& timeVec2,
                                        const char* keyword);

    public:
        //! \brief Creates an SummaryComparator class object
        //! \param[in] basename1 Path to file1 without extension.
        //! \param[in] basename1 Path to file2 without extension.
        //! \param[in] absoluteTolerance The absolute tolerance which is to be used in the test.
        //! \param[in] relativeTolerance The relative tolerance which is to be used in the test.
        //! \details The constructor creates an object of the class, and openes the files, an exception is thrown if the opening of the files fails. \n It creates stringlists, in which keywords are to be stored, and figures out which keylist that contains the more/less keywords. \n Also the private member variables aboluteTolerance and relativeTolerance are set.
        SummaryComparator(const char* basename1, const char* basename2, double absoluteTolerance, double relativeTolerance);

        //! \brief Destructor
        //! \details The destructor takes care of the allocated memory in which data has been stored.
        ~SummaryComparator();

        //! \brief Calculates the deviation between two values
        //! \param[in] val1 The first value of interest.
        //! \param[in] val2 The second value if interest.
        //! \param[out] ret Returns a Deviation struct.
        //! \details The function takes two values, calculates the absolute and relative deviation and returns the result as a Deviation struct.
        static Deviation calculateDeviations( double val1, double val2);

        //! \brief Sets the private member variable printKeywords
        //! \param[in] boolean Boolean value
        //! \details The function sets the private member variable printKeywords. When it is true the function printKeywords will be called.
        void setPrintKeywords(bool boolean){this->printKeyword = boolean;}

        //! \brief Sets the private member variable printSpecificKeyword
        //! \param[in] boolean Boolean value
        //! \details The function sets the private member variable printSpecificKeyword. When true, the summary vector of the keyword for both files will be printed.
        void setPrintSpecificKeyword(bool boolean){this->printSpecificKeyword = boolean;}

        //! \brief Unit step function
        //! \param[in] value The input value should be the last know value
        //! \param[out] ret Return the unit-step-function value.
        //! \details In this case: The unit step function is used when the data from the two data set, which is to be compared, don't match in time. \n The unit step function is then to be called on the #checkDataVec 's value at the last time step which is before the time of comparison.

        //! \brief Returns a value based on the unit step principle.
        static double unitStep(double value){return value;}

};

#endif
