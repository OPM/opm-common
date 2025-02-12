#ifndef GRIDUTIL_H
#define GRIDUTIL_H

#include <cmath> 
#include <tuple>
#include <vector>
namespace Opm::GridUtil {


std::tuple<std::vector<double>,std::vector<double>> convertUnsToCPG(
                     const std::vector<std::array<double, 3>>&, 
                     const std::vector<std::array<std::size_t, 8>>&, 
                     std::size_t, std::size_t, std::size_t);




// Example for other utilities...

} // namespace GeometryUtils

#endif // GEOMETRY_UTILS_H
