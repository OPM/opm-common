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

#include <map>
#include <vector>
#include <string>

struct ecl_file_struct; //!< Prototype for eclipse file struct, from ERT library.
typedef struct ecl_file_struct ecl_file_type;

struct ecl_grid_struct; //!< Prototype for eclipse grid struct, from ERT library.
typedef struct ecl_grid_struct ecl_grid_type;
struct ecl_kw_struct; //!< Prototype for eclipse keyword struct, from ERT library.
typedef struct ecl_kw_struct ecl_kw_type;


/*! \brief Deviation struct.
    \details The member variables are default initialized to -1,
             which is an invalid deviation value.
 */
struct Deviation {
    double abs = -1; //!< Absolute deviation
    double rel = -1; //!< Relative deviation
};


/*! \brief A class for comparing ECLIPSE files.
    \details ECLFilesComparator opens ECLIPSE files
             (unified restart, initial and RFT in addition to grid file)
             from two simulations. This class has only the functions
             printKeywords() and printKeywordsDifference(), in addition to a
             couple of get-functions: the comparison logic is implemented in
             the subclasses RegressionTest and IntegrationTest. */
class ECLFilesComparator {
    private:
        int file_type;
        double absTolerance      = 0;
        double relTolerance      = 0;
    protected:
        ecl_file_type* ecl_file1 = nullptr;
        ecl_grid_type* ecl_grid1 = nullptr;
        ecl_file_type* ecl_file2 = nullptr;
        ecl_grid_type* ecl_grid2 = nullptr;
        std::vector<std::string> keywords1, keywords2;
        bool throwOnError = true; //!< Throw on first error
        bool analysis = false; //!< Perform full error analysis
        std::map<std::string, std::vector<Deviation>> deviations;
        mutable size_t num_errors = 0;

        //! \brief Checks if the keyword exists in both cases.
        //! \param[in] keyword Keyword to check.
        //! \details If the keyword does not exist in one of the cases, the function throws an exception.
        void keywordValidForComparing(const std::string& keyword) const;
        //! \brief Stores keyword data for a given occurrence
        //! \param[out] ecl_kw1 Pointer to a ecl_kw_type, which stores keyword data for first case given the occurrence.
        //! \param[out] ecl_kw2 Pointer to a ecl_kw_type, which stores keyword data for second case given the occurrence.
        //! \param[in] keyword Which keyword to consider.
        //! \param[in] occurrence Which keyword occurrence to consider.
        //! \details This function stores keyword data for the given keyword and occurrence in #ecl_kw1 and #ecl_kw2, and returns the number of cells (for which the keyword has a value at the occurrence). If the number of cells differ for the two cases, an exception is thrown.
        unsigned int getEclKeywordData(ecl_kw_type*& ecl_kw1, ecl_kw_type*& ecl_kw2, const std::string& keyword, int occurrence1, int occurrence2) const;
        //! \brief Prints values for a given keyword, occurrence and cell
        //! \param[in] keyword Which keyword to consider.
        //! \param[in] occurrence Which keyword occurrence to consider.
        //! \param[in] cell Which cell occurrence to consider (numbered by global index).
        //! \param[in] value1 Value for first file, the data type can be bool, int, double or std::string.
        //! \param[in] value2 Value for second file, the data type can be bool, int, double or std::string.
        //! \details Templatefunction for printing values when exceptions are thrown. The function is defined for bool, int, double and std::string.
        template <typename T>
        void printValuesForCell(const std::string& keyword, int occurrence1, int occurrence2, size_t kw_size, size_t cell, const T& value1, const T& value2) const;

    public:
        //! \brief Open ECLIPSE files and set tolerances and keywords.
        //! \param[in] file_type Specifies which filetype to be compared, possible inputs are UNRSTFILE, INITFILE and RFTFILE.
        //! \param[in] basename1 Full path without file extension to the first case.
        //! \param[in] basename2 Full path without file extension to the second case.
        //! \param[in] absTolerance Tolerance for absolute deviation.
        //! \param[in] relTolerance Tolerance for relative deviation.
        //! \details The content of the ECLIPSE files specified in the input is stored in the ecl_file_type and ecl_grid_type member variables. In addition the keywords and absolute and relative tolerances (member variables) are set. If the constructor is unable to open one of the ECLIPSE files, an exception will be thrown.
        ECLFilesComparator(int file_type, const std::string& basename1, const std::string& basename2, double absTolerance, double relTolerance);
        //! \brief Closing the ECLIPSE files.
        ~ECLFilesComparator();

        //! \brief Set whether to throw on errors or not.
        void throwOnErrors(bool dothrow) { throwOnError = dothrow; }

        //! \brief Set whether to perform a full error analysis.
        void doAnalysis(bool analize) { analysis = analize; }

        //! \brief Returns the number of errors encountered in the performed comparisons.
        size_t getNoErrors() const { return num_errors; }

        //! \brief Returns the ECLIPSE filetype of this
        int getFileType() const {return file_type;}
        //! \brief Returns the absolute tolerance stored as a private member variable in the class
        double getAbsTolerance() const {return absTolerance;}
        //! \brief Returns the relative tolerance stored as a private member variable in the class
        double getRelTolerance() const {return relTolerance;}

        //! \brief Print all keywords and their respective Eclipse type for the two input cases.
        void printKeywords() const;
        //! \brief Print common and uncommon keywords for the two input cases.
        void printKeywordsDifference() const;

        //! \brief Calculate deviations for two values.
        //! \details Using absolute values of the input arguments: If one of the values are non-zero, the Deviation::abs returned is the difference between the two input values. In addition, if both values are non-zero, the Deviation::rel returned is the absolute deviation divided by the largest value.
        static Deviation calculateDeviations(double val1, double val2);
        //! \brief Calculate median of a vector.
        //! \details Returning the median of the input vector, i.e. the middle value of the sorted vector if the number of elements is odd or the mean of the two middle values if the number of elements are even.
        static double median(std::vector<double> vec);
        //! \brief Calculate average of a vector.
        //! \details Returning the average of the input vector, i.e. the sum of all values divided by the number of elements.
        static double average(const std::vector<double>& vec);
         //! \brief Obtain the volume of a cell.
        static double getCellVolume(const ecl_grid_type* ecl_grid, const int globalIndex);
};

#endif
