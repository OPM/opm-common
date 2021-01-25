/*
  Copyright 2020,2021 OPM-OP AS

  This file is part of the Open Porous Media Project (OPM).

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
#ifndef OPM_COMMUNICATION_UTILS_HPP
#define OPM_COMMUNICATION_UTILS_HPP

#include<vector>
#include<numeric>
#include<tuple>

namespace Opm
{
/// \brief Gathers vectors from all processes on all processes
///
/// In parallel this will call MPI_Allgatherv. Has to be called on all
/// ranks.
///
/// \param input The input vector to gather from the rank.
/// \param comm The Dune::Communication object.
/// \return A pair of a vector with all the values gathered (first
///         the ones from rank 0, then the ones from rank 1, ...)
///         and a vector with the offsets of the first value from each
///         rank (values[offset[rank]] will be the first value from rank).
///         This vector is one bigger than the number of processes and
///         the last entry is the size of the first array.
template<class T, class A, class C>
std::pair<std::vector<T, A>,
          std::vector<int>>
allGatherv(const std::vector<T,A>& input, const C& comm)
{
    std::vector<int> sizes(comm.size());
    std::vector<int> displ(comm.size() + 1, 0);
    int mySize = input.size();
    comm.allgather(&mySize, 1, sizes.data());
    std::partial_sum(sizes.begin(), sizes.end(), displ.begin()+1);
    std::vector<T,A> output(displ.back());
    comm.allgatherv(input.data(), input.size(), output.data(), sizes.data(), displ.data());
    return {output, displ};
}
/// \brief Gathers vectors from all processes on a root process.
///
/// In parallel this will call MPI_Gatherv. Has to be called on all
/// ranks.
///
/// \param input The input vector to gather from the rank.
/// \param comm The Dune::Communication object.
/// \param root The rank of the processes to gather the values-
/// \return On non-root ranks a pair of empty vectors. On the root rank
///         a pair of a vector with all the values gathered (first
///         the ones from rank 0, then the ones from rank 1, ...)
///         and a vector with the offsets of the first value from each
///         rank (values[offset[rank]] will be the first value from rank).
///         This vector is one bigger than the number of processes and
///         the last entry is the size of the first array.

template<class T, class A, class C>
std::pair<std::vector<T, A>,
          std::vector<int>>
gatherv(const std::vector<T,A>& input, const C& comm, int root)
{
    bool isRoot = (comm.rank() == root);
    std::vector<int> sizes;
    std::vector<int> displ;
    if (isRoot)
    {
        sizes.resize(comm.size());
        displ.resize(comm.size() + 1);
    }
    int mySize = input.size();
    comm.gather(&mySize, sizes.data(), 1, root);
    std::partial_sum(sizes.begin(), sizes.end(), displ.begin()+1);
    std::vector<T,A> output( isRoot ? displ.back() : 0);
    comm.gatherv(input.data(), input.size(), output.data(), sizes.data(), displ.data(), root);
    return {output, displ};
}
}
#endif
