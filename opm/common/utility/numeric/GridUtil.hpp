#ifndef GRIDUTIL_H
#define GRIDUTIL_H

#include <tuple>
#include <vector>
#include <opm/common/utility/numeric/VectorUtil.hpp>
#include <numeric>
#include <cmath> 
namespace GridUtil {

constexpr double epslon = 1e-6;         
std::size_t ijk_to_linear(std::size_t, std::size_t, std::size_t, std::size_t, std::size_t);
std::tuple<std::size_t, std::size_t, std::size_t> linear_to_ijk(std::size_t, std::size_t, std::size_t);

std::vector<double> pillar_to_flat_array(const std::vector<std::array<std::array<double, 3>, 2>>&);
std::tuple<std::vector<double>,std::vector<double>> convertUnsToCPG(
                     const std::vector<std::array<double, 3>>&, 
                     const std::vector<std::array<std::size_t, 8>>&, 
                     std::size_t, std::size_t, std::size_t);




// Example for other utilities...

} // namespace GeometryUtils

#endif // GEOMETRY_UTILS_H
