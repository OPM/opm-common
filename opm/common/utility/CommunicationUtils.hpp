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

namespace Opm
{
template<class T, class A, class C>
std::tuple<std::vector<T, A>,
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

template<class T, class A, class C>
std::tuple<std::vector<T, A>,
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
