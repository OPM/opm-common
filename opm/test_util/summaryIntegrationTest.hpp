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

#include <opm/test_util/summaryComparator.hpp>

//! \brief Struct for storing the total area under a graph.
//! \details Used when plotting summary vector against time. In most cases this represents a volume.
struct WellProductionVolume{
    double total=0; //!< The total area under the graph when plotting the summary vector against time. In most cases the total production volume.
    double error=0; //!< The total area under the graph when plotting the deviation vector against time. In most cases the total error volume.

    //! \brief Overloaded operator
    //! \param[in] rhs WellProductionVolume struct
    WellProductionVolume& operator+=(const WellProductionVolume& rhs){
        this->total += rhs.total;
        this->error += rhs.error;
        return *this;
    }
};

//! \details The class inherits from the SummaryComparator class, which takes care of all file reading. \n The IntegrationTest class compares values from the two different files and throws exceptions when the deviation is unsatisfying.
class IntegrationTest: public SummaryComparator {
    private:
        bool allowSpikes = false; //!< Boolean value, when true checkForSpikes is included as a sub test in the integration test. By default: false.
        bool findVolumeError = false; //!< Boolean value, when true volumeErrorCheck() is included as a sub test in the integration test. By default: false.
        bool allowDifferentAmountOfKeywords = true; //!< Boolean value, when false the integration test will check wheter the two files have the same amount of keywords. \nIf they don't, an exception will be thrown. By default: true.
        bool findVectorWithGreatestErrorRatio = false; //!< Boolean value, when true the integration test will find the vector that has the greatest error ratio. By default: false.
        bool oneOfTheMainVariables = false; //!< Boolean value, when true the integration test will only check for one of the primary variables (WOPR, WGPR, WWPR. WBHP), which will be specified by user. By default: false.
        bool throwExceptionForTooGreatErrorRatio = true; //!< Boolean value, when true any volume error ratio that exceeds the relativeTolerance will cause an exception to be thrown. By default: true.
        std::string mainVariable; //!< String variable, where the name of the main variable of interest (one of WOPR, WBHP, WWPR, WGPR) is stored. Can be empty.
        int spikeLimit = 13370; //!< The limit for how many spikes to allow in the data set of a certain keyword. By default: Set to a high number, \n should not trig the (if spikeOccurrences > spikeLimit){ // throw exception }.

        WellProductionVolume WOP; //!< WellProductionVolume struct for storing the total production volume and total error volume of all the keywords which start with WOPR
        WellProductionVolume WWP; //!< WellProductionVolume struct for storing the total production volume and total error volume of all the keywords which start with WWPR
        WellProductionVolume WGP;//!< WellProductionVolume struct for storing the total production volume and total error volume of all the keywords which start with WGPR
        WellProductionVolume WBHP; //!< WellProductionVolume struct for storing the value of the area under the graph when plotting summary vector/deviation vector against time.This is for keywords starting with WBHP. \nNote: the name of the struct may be misleading, this is not an actual volume.


        //! \brief The function gathers the correct data for comparison for a specific keyword
        //! \param[in] timeVec1 A std::vector<double> that contains the time steps of file 1.
        //! \param[in] timeVec2 A std::vector<double> that contains the time steps of file 2.
        //! \param[in] keyword The keyword of interest
        //! \details The function requires an outer loop which iterates over the keywords of the files. It prepares an integration test by gathering the data, stroing it into two vectors, \n deciding which is to be used as a reference/basis and calling the test function.
        void checkForKeyword(const std::vector<double>& timeVec1,
                             const std::vector<double>& timeVec2, const char* keyword);

        //! \brief The function compares the volume error to the total production volume of a certain type of keyword.
        //! param[in] keyword The keyword of interest.
        //! \details The function takes in a keyword and checks if it is of interest. Only keywords which say something about the well oil production, well water production, \n well gas production and the well BHP are of interest. The function sums up the total production in the cases where it is possible, \n and sums up the error volumes by a trapezoid integration method. The resulting values are stored in member variable structs of type WellProductionVolume, and double variables. For proper use of the function all the keywords of the file should be checked. This is satisfied if it is called by checkForKeyword.
        void volumeErrorCheck(const char* keyword);

        //! \brief The function calculates the total production volume and total error volume of a specific keyword
        //! \param[in] timeVec1 A std::vector<double> that contains the time steps of file 1.
        //! \param[in] timeVec2 A std::vector<double> that contains the time steps of file 2.
        //! \param[in] keyword The keyword of interest
        //! \param[out] ret Returns a WellProductionWolume struct
        //! \details The function reads the data from the two files into the member variable vectors (of the super class). It returns a WellProductionVolume struct calculated from the vectors corresponding to the keyword.
        WellProductionVolume getSpecificWellVolume(const std::vector<double>& timeVec1,
                                                   const std::vector<double>& timeVec2,
                                                   const char* keyword);

        //! \brief The function is a regression test which allows spikes.
        //! \param[in] keyword The keyword of interest, the keyword the summary vectors "belong" to.
        //! \details The function requires the protected member variables referenceVec, referenceDataVec, checkVec and checkDataVec to be stored with data, which is staisfied if it is called by checkForKeyword. \n It compares the two vectors value by value, and if the deviation is unsatisfying, the errorOccurrenceCounter is incremented. If the errorOccurrenceCounter becomes greater than the errorOccurrenceLimit, \n a exception is thrown. The function will allow spike values, however, if two values in a row exceed the deviation limit, they are no longer spikes, and an exception is thrown.
        void checkWithSpikes(const char* keyword);

        //! \brief Caluculates a deviation, throws exceptions and writes and error message.
        //! \param[in] deviation Deviation struct
        //! \param[out] int Returns 0/1, depending on wheter the deviation exceeded the limit or not.
        //! \details The function checks the values of the Deviation struct against the absolute and relative tolerance, which are private member values of the super class. \n When comparing against the relative tolerance an additional term is added, the absolute deviation has to be greater than 1e-6 for the function to throw an exception. \n When the deviations are too great, the function returns 1.
        int checkDeviation(const Deviation& deviation);

        //! \brief Calculates the keyword's total production volume and error volume
        //! \param[in] keyword The keyword of interest.
        //! \param[out] wellProductionVolume A struct containing the total production volume and the total error volume.
        //! \details The function calculates the total production volume and total error volume of a keyword, by the trapezoid integral method. \n The function throws and exception if the total error volume is negative. The function returns the results as a struct.
        WellProductionVolume getWellProductionVolume(const char* keyword);

        //! \brief The function function works properly when the private member variables are set (after running the integration test which findVolumeError = true). \n It prints out the total production volume, the total error volume and the error ratio.
        void evaluateWellProductionVolume();

        //! \brief The function calculates the total production volume and total error volume
        //! \param keyword The keyword of interest
        //! \details The function uses the data that is stored in the member variable vectors. It calculates the total production volume \n and the total error volume of the specified keyword, and adds the result to the private member WellProductionVolume variables of the class.
        void updateVolumeError(const char* keyword);

        //! \brief Finds the keyword which has the greates error volume ratio
        //! \param[in] volume WellProductionVolume struct which contains the data used for comparison
        //! \param[in] greatestRatio Double value taken in by reference. Stores the greatest error ratio value.
        //! \param[in] currentKeyword The keyword that is under evaluation
        //! \param[in] greatestErrorRatio String which contains the name of the keyword which has the greatest error ratio
        //! \details The function requires an outer loop which iterates over the keywords in the files, and calls the function for each keyword. \nThe valiables double greatestRatio and std::string keywordWithTheGreatestErrorRatio must be declared outside the loop. \nWhen the current error ratio is greater than the value stored in greatestRatio, the gratestRatio value is updated with the current error ratio.
        void findGreatestErrorRatio(const WellProductionVolume& volume,
                                    double &greatestRatio,
                                    const char* currentKeyword,
                                    std::string &greatestErrorRatio);

        //! \brief Checks whether the unit of the two data vectors is the same
        //! \param[in] keyword The keyword of interest
        //! \param[out] boolean True/false, depending on whether the units are equal or not
        bool checkUnits(const char* keyword);
    public:
        //! \brief Constructor, creates an object of IntegrationTest class.
        //! \param[in] basename1 Path to file1 without extension.
        //! \param[in] basename1 Path to file2 without extension.
        //! \param[in] atol The absolute tolerance which is to be used in the test.
        //! \param[in] rtol The relative tolerance which is to be used in the test.
        //! \details The constructor calls the constructor of the super class.
        IntegrationTest(const char* basename1, const char* basename2,
                        double atol, double rtol) :
            SummaryComparator(basename1, basename2,  atol, rtol) {}

        //! \brief This function sets the private member variable allowSpikes.
        //! \param[in] allowSpikes Boolean value
        //! \details When allowSpikes is true, the integration test checkWithSpikes is excecuted.
        void setAllowSpikes(bool allowSpikes){this->allowSpikes = allowSpikes;}

        //! \brief This function sets the private member variable findVolumeError.
        //! \param[in] findVolumeError Boolean value
        //! \details When findVolumeError is true, the integration test volumeErrorCheck and the function evaluateWellProductionVolume are excecuted.
        void setFindVolumeError(bool findVolumeError){this->findVolumeError = findVolumeError;}

        //! \brief This function sets the private member variable oneOfTheMainVariables
        //! \param[in] oneOfTheMainVariables Boolean value
        //! \details When oneOfTheMainVariables is true, the integration test runs the substest volumeErrorCheckForOneSpecificVariable.
        void setOneOfTheMainVariables(bool oneOfTheMainVariables){this->oneOfTheMainVariables = oneOfTheMainVariables;}

        //! \brief This function sets the member variable string #mainVariable
        //! \param[in] mainVar This is the string should contain one of the main variables. e.g. WOPR
        void setMainVariable(std::string mainVar){this->mainVariable = mainVar;}

        //! \brief This function sets the private member variable spikeLimit.
        //! \param[in] lim The value which the spike limit is to be given.
        void setSpikeLimit(int lim){this->spikeLimit = lim;}

        //! \brief This function sets the private member variable findVectorWithGreatestErrorRatio
        //! \param[in] findVolumeError Boolean value
        //! \details When findVectorWithGreatestErrorRatio is true, the integration test will print the vector with the greatest error ratio.
        void setFindVectorWithGreatestErrorRatio(bool boolean){this->findVectorWithGreatestErrorRatio = boolean;}

        //! \brief This function sets the private member variable allowDifferentAmountsOfKeywords
        //! \param[in] boolean Boolean value
        //! \details When allowDifferentAmountOfKeywords is false, the amount of kewyord in the two files will be compared. \nIf the number of keywords are different an exception will be thrown.
        void setAllowDifferentAmountOfKeywords(bool boolean){this->allowDifferentAmountOfKeywords = boolean;}

        //! \brief This function sets the private member variable throwExceptionForTooGreatErrorRatio
        //! \param[in] boolean Boolean value
        //! \details When throwExceptionForTooGreatErrorRatio is false, the function getWellProductionVolume will throw an exception.
        void setThrowExceptionForTooGreatErrorRatio(bool boolean){this->throwExceptionForTooGreatErrorRatio = boolean;}

        //! \brief This function executes a integration test for all the keywords. If the two files do not match in amount of keywords, an exception is thrown. \n Uses the boolean member variables to know which tests to execute.
        void getIntegrationTest();

        //! \brief This function executes a integration test for one specific keyword.  If one or both of the files do not have the keyword, an exception is thorwn. \n Uses the boolean member variables to know which tests to execute.
        void getIntegrationTest(const char* keyword);

        //! \brief This function calculates the area of an rectangle of height height and width time-timePrev
        //! \param[in] height The height of the rectangle. See important statement of use below.
        //! \param[in] width The width of the rectangle
        //! \param[out] area Returns the area of the rectangle
        //! \details This function is simple. When using it on a summary vector (data values plotted againt time), calculating the area between the two points i and i+1 note this:\nThe width is time_of_i+1 - time_of_i, the height is data_of_i+1 NOT data_of_i. The upper limit must be used.
        static double getRectangleArea(double height, double width){return height*width;}

        //! \brief This function calculates the area under a graph by doing a Riemann sum
        //! \param[in] timeVec Contains the time values
        //! \param[in] dataVec Contains the data values
        //! \details The function does a Riemann sum integration of the graph formed
        //!          by the points (timeVec[i] , dataVec[i]).
        //!          In the case of a summary vector, the summary vector of quantity
        //!          corresponding to a rate, is a piecewise continus function consisting
        //!          of unit step functions. Thus the Riemann sum will become an
        //!          exact expression for the integral of the graph.
        //!          Important: For the data values correspoding to time i and i-1,
        //!          the fixed value of the height of the rectangles in the Riemann sum
        //!          is set by the data value i. The upper limit must be used.
        static double integrate(const std::vector<double>& timeVec,
                                const std::vector<double>& dataVec);

        //! \brief This function calculates the Riemann sum of the error between two graphs.
        //! \param[in] timeVec1 Contains the time values of graph 1
        //! \param[in] dataVec1 Contains the data values of graph 1
        //! \param[in] timeVec2 Contains the time values of graph 2
        //! \param[in] dataVec2 Contains the data values of graph 2
        //! \details This function takes in two graphs and returns the integrated error.
        //!          In case of ecl summary vectors: if the vectors correspond to a
        //!          quantity which is a rate, the vectors will be piecewise
        //!          continous unit step functions. In this case the error will also
        //!          be a piecewise continous unit step function. The function uses
        //!          a Riemann sum when calculating the integral. Thus the integral
        //!          will become exact. Important: For the data values corresponding
        //!          to time i and i-1, the fixed value of the height of the rectangles
        //!          in the Riemann sum is set by the data value i.
        //!          The upper limit must be used.
        static double integrateError(const std::vector<double>& timeVec1,
                                     const std::vector<double>& dataVec1,
                                     const std::vector<double>& timeVec2,
                                     const std::vector<double>& dataVec2);
};
