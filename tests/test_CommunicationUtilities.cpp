/*
  Copyright 2021 OPM-OP AS.

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


#include "config.h"

#define BOOST_TEST_MODULE CommunicationUtilities
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test.hpp>
#include <dune/common/parallel/mpihelper.hh>
#include <opm/common/utility/CommunicationUtils.hpp>
#include <vector>
#include <random>

class MPIError {
public:
  /** @brief Constructor. */
  MPIError(std::string s, int e) : errorstring(s), errorcode(e){}
  /** @brief The error string. */
  std::string errorstring;
  /** @brief The mpi error code. */
  int errorcode;
};

#ifdef HAVE_MPI
void MPI_err_handler(MPI_Comm *, int *err_code, ...){
  char *err_string=new char[MPI_MAX_ERROR_STRING];
  int err_length;
  MPI_Error_string(*err_code, err_string, &err_length);
  std::string s(err_string, err_length);
  std::cerr << "An MPI Error ocurred:"<<std::endl<<s<<std::endl;
  delete[] err_string;
  throw MPIError(s, *err_code);
}
#endif

struct MPIFixture
{
    MPIFixture()
    {
#if HAVE_MPI
    int m_argc = boost::unit_test::framework::master_test_suite().argc;
    char** m_argv = boost::unit_test::framework::master_test_suite().argv;
    helper = &Dune::MPIHelper::instance(m_argc, m_argv);
#ifdef MPI_2
    MPI_Comm_create_errhandler(MPI_err_handler, &handler);
   MPI_Comm_set_errhandler(MPI_COMM_WORLD, handler);
#else
        MPI_Errhandler_create(MPI_err_handler, &handler);
        MPI_Errhandler_set(MPI_COMM_WORLD, handler);
#endif
#endif
    }
    ~MPIFixture()
    {
#if HAVE_MPI
        MPI_Finalize();
#endif
    }
    Dune::MPIHelper* helper;
#if HAVE_MPI
    MPI_Errhandler handler;
#endif
};

BOOST_GLOBAL_FIXTURE(MPIFixture);

template<class T, class C>
std::tuple<std::vector<T>, std::vector<T>, std::vector<int>, std::vector<int>>
createParallelData(const C& comm, T)
{
    std::vector<int> sizes(comm.size());
    std::vector<int> displ(comm.size()+1);
    std::vector<T> allVals;
    
    if (comm.rank() == 0)
    {
        // Initialize with random numbers.
        std::random_device rndDevice;
        std::mt19937 mersenneEngine {rndDevice()};  // Generates random integers
        std::uniform_int_distribution<int> dist {1, 10};
        
        auto gen = [&dist, &mersenneEngine](){
                       return dist(mersenneEngine);
                   };
        auto begin = std::begin(sizes);
        auto end = std::end(sizes);
        std::generate(begin, end, gen);
        std::partial_sum(begin, end, std::begin(displ)+1);
        allVals.resize(displ.back());
        std::generate(std::begin(allVals), std::end(allVals), gen);
    }
    comm.broadcast(sizes.data(), sizes.size(), 0);
    comm.broadcast(displ.data(), displ.size(), 0);
    allVals.resize(displ.back()); // does nit change anything on 0
    comm.broadcast(allVals.data(), allVals.size(), 0);
    std::vector<T> myVals;
    myVals.reserve(sizes[comm.rank()]);
    auto beginAll = std::begin(allVals);

    for ( auto val = beginAll + displ[comm.rank()]; val != beginAll + displ[comm.rank()+1];
          ++val)
    {
        myVals.push_back(*val);
    }

    return { allVals, myVals, sizes, displ };
}

template<class T>
void checkGlobalData(const std::vector<T>& data, const std::vector<T>& expected,
                     const std::vector<int> displ, const std::vector<int> expectedDispl){
    using std::begin;
    using std::end;
    BOOST_CHECK_EQUAL_COLLECTIONS(begin(data), end(data), begin(expected), end(expected));
    BOOST_CHECK_EQUAL_COLLECTIONS(begin(displ), end(displ), begin(expectedDispl), end(expectedDispl));
}

template<class C>
void testAllGatherv(const C& comm)
{
    //std::vector<int> sizes(comm.size());
    std::vector<int> expectedDispl(comm.size()+1);
    std::vector<int> displ;
    std::vector<double> expectedAllVals;
    std::vector<double> allVals;
    std::vector<double> myVals;
    double d=0;
    
    std::tie(expectedAllVals, myVals, std::ignore, expectedDispl) = createParallelData(comm, d);
    std::tie(allVals, displ) = Opm::allGatherv(myVals, comm);
    checkGlobalData(allVals, expectedAllVals, displ, expectedDispl);
}

template<class C>
void testGatherv(const C& comm)
{
    //std::vector<int> sizes(comm.size());
    std::vector<int> expectedDispl(comm.size()+1);
    std::vector<int> displ;
    std::vector<double> expectedAllVals;
    std::vector<double> allVals;
    std::vector<double> myVals;
    double d;
    
    std::tie(expectedAllVals, myVals, std::ignore, expectedDispl) = createParallelData(comm, d);
    std::tie(allVals, displ) = Opm::gatherv(myVals, comm, 0);
    if ( comm.rank() != 0)
    {
        expectedDispl.clear();
        expectedAllVals.clear();
    }
    checkGlobalData(allVals, expectedAllVals, displ, expectedDispl);
}



BOOST_AUTO_TEST_CASE(FakeAllGatherv)
{
    testAllGatherv(Dune::FakeMPIHelper::getCollectiveCommunication());
}
BOOST_AUTO_TEST_CASE(FakeGatherv)
{
    testGatherv(Dune::FakeMPIHelper::getCollectiveCommunication());
}
BOOST_AUTO_TEST_CASE(AllGatherv)
{
    testAllGatherv(Dune::MPIHelper::getCollectiveCommunication());
}
BOOST_AUTO_TEST_CASE(Gatherv)
{
    testGatherv(Dune::MPIHelper::getCollectiveCommunication());
}

