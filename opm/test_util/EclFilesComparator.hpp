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
        void printValuesForCell(const std::string& keyword, int occurrence1, int occurrence2, size_t cell, const T& value1, const T& value2) const;

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
};



/*! \brief A class for executing a regression test for two ECLIPSE files.
    \details This class inherits from ECLFilesComparator, which opens and
             closes the input cases and stores keywordnames.
             The three public functions gridCompare(), results() and
             resultsForKeyword() can be invoked to compare griddata
             or keyworddata for all keywords or a given keyword (resultsForKeyword()).
 */

class RegressionTest: public ECLFilesComparator {
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
        void deviationsForCell(double val1, double val2, const std::string& keyword, int occurrence1, int occurrence2, size_t cell, bool allowNegativeValues = true);
    public:
        //! \brief Sets up the regression test.
        //! \param[in] file_type Specifies which filetype to be compared, possible inputs are UNRSTFILE, INITFILE and RFTFILE.
        //! \param[in] basename1 Full path without file extension to the first case.
        //! \param[in] basename2 Full path without file extension to the second case.
        //! \param[in] absTolerance Tolerance for absolute deviation.
        //! \param[in] relTolerance Tolerance for relative deviation.
        //! \details This constructor only calls the constructor of the superclass, see the docs for ECLFilesComparator for more information.
        RegressionTest(int file_type, const std::string& basename1, const std::string& basename2, double absTolerance, double relTolerance):
            ECLFilesComparator(file_type, basename1, basename2, absTolerance, relTolerance) {}

        //! \brief Option to only compare last occurrence
        void setOnlyLastOccurrence(bool onlyLastOccurrence) {this->onlyLastOccurrence = onlyLastOccurrence;}

        //! \brief Compares grid properties of the two cases.
        // gridCompare() checks if both the number of active and global cells in the two cases are the same. If they are, all cells are looped over to calculate the cell volume deviation for the two cases. If the both the relative and absolute deviation exceeds the tolerances, an exception is thrown.
        void gridCompare() const;
        //! \brief Calculates deviations for all keywords.
        // This function checks if the number of keywords of the two cases are equal, and if it is, resultsForKeyword() is called for every keyword. If not, an exception is thrown.
        void results();
        //! \brief Calculates deviations for a specific keyword.
        //! \param[in] keyword Keyword which should be compared, if this keyword is absent in one of the cases, an exception will be thrown.
        //! \details This function loops through every report step and every cell and compares the values for the given keyword from the two input cases. If the absolute or relative deviation between the two values for each step exceeds both the absolute tolerance and the relative tolerance (stored in ECLFilesComparator), an exception is thrown. In addition, some keywords are marked for "disallow negative values" -- these are SGAS, SWAT and PRESSURE. An exception is thrown if a value of one of these keywords is both negative and has an absolute value larger than the absolute tolerance. If no exceptions are thrown, resultsForKeyword() uses the private member funtion printResultsForKeyword to print the average and median deviations.
        void resultsForKeyword(const std::string keyword);
};




/*! \brief A class for executing a integration test for two ECLIPSE files.
    \details This class inherits from ECLFilesComparator, which opens and closes
             the input cases and stores keywordnames. The three public functions
             equalNumKeywords(), results() and resultsForKeyword() can be invoked
             to compare griddata or keyworddata for all keywords or a given
             keyword (resultsForKeyword()).
 */
class IntegrationTest: public ECLFilesComparator {
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
        IntegrationTest(const std::string& basename1, const std::string& basename2, double absTolerance, double relTolerance);

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
        void resultsForKeyword(const std::string keyword);
};

#endif
