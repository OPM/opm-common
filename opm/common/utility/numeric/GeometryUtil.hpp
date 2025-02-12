#ifndef GEOMETRYUTIL_H
#define GEOMETRYUTIL_H

#include <vector>
#include <opm/common/utility/numeric/VectorUtil.hpp>
#include <numeric>
#include <cmath> 
namespace GeometryUtil {

constexpr double epslon = 1e-6;         
// A simple utility function to calculate area of a rectangle
double calculateRectangleArea(double width, double height);

template <typename T = double>
T  calcTetraVol(const std::array<T,4>& x, const std::array<T,4>& y, const std::array<T,4>& z ){
    T det =	x[0]*y[2]*z[1] - x[0]*y[1]*z[2] + x[1]*y[0]*z[2]
            - x[1]*y[2]*z[0] - x[2]*y[0]*z[1] + x[2]*y[1]*z[0]
            + x[0]*y[1]*z[3] - x[0]*y[3]*z[1] - x[1]*y[0]*z[3]
            + x[1]*y[3]*z[0] + x[3]*y[0]*z[1] - x[3]*y[1]*z[0]
            - x[0]*y[2]*z[3] + x[0]*y[3]*z[2] + x[2]*y[0]*z[3]
            - x[2]*y[3]*z[0] - x[3]*y[0]*z[2] + x[3]*y[2]*z[0]
            + x[1]*y[2]*z[3] - x[1]*y[3]*z[2] - x[2]*y[1]*z[3]
            + x[2]*y[3]*z[1] + x[3]*y[1]*z[2] - x[3]*y[2]*z[1];
            return std::abs(det)/6; 
}

template <typename T = double>
T calcHexaVol(const std::array<T,8>& x, const std::array<T,8>& y, const std::array<T,8>& z, 
              const T& cx,  const T& cy, const T& cz )
{
    constexpr std::array<std::array<int, 3>, 12> faceConfigurations
    {
        std::array<int, 3>{0, 1, 5}, 
                          {1, 5, 4},     // Face 0
                          {0, 4, 6},
                          {4, 6, 2},     // Face 1
                          {2, 3, 7}, 
                          {3, 7, 6},     // Face 2
                          {1, 3, 7}, 
                          {3, 7, 5},     // Face 3
                          {0, 1, 3}, 
                          {1, 3, 2},     // Face 4
                          {4, 5, 7}, 
                          {5, 7, 6}     // Face 5
    };
    auto getNodes = [](const std::array<T, 8>& X, const std::array<T, 8>& Y, const std::array<T, 8>& Z,
                                    const std::array<int,3>&  ind){
        std::array<T, 3> filtered_vectorX;
        std::array<T, 3> filtered_vectorY;
        std::array<T, 3> filtered_vectorZ;
        for (std::size_t index = 0; index < ind.size(); index++) {
            filtered_vectorX[index] = X[ind[index]];
            filtered_vectorY[index] = Y[ind[index]];
            filtered_vectorZ[index] = Z[ind[index]];
        }
        return std::make_tuple(filtered_vectorX,filtered_vectorY,filtered_vectorZ);
    };
    // note: some CPG grids may have collapsed faces that are not planar, therefore
    // the hexadron is subdivided in terahedrons.
    // calculating the volume of the pyramid with F0 as base and pc as center                         
    T totalVolume = 0.0;
    for (size_t i = 0; i < faceConfigurations.size(); i += 2) {
        auto [fX0, fY0, fZ0] = getNodes(x, y, z, faceConfigurations[i]);
        totalVolume += std::apply(calcTetraVol<T>, VectorUtil::appendNode<double>(fX0, fY0, fZ0, cx, cy, cz));

        auto [fX1, fY1, fZ1] = getNodes(x, y, z, faceConfigurations[i + 1]);
        totalVolume += std::apply(calcTetraVol<T>, VectorUtil::appendNode<double>(fX1, fY1, fZ1, cx, cy, cz));
    }
    return totalVolume;
}

template <typename T = double>
std::vector<int> isInsideElement(const std::vector<T>& tpX, const std::vector<T>& tpY, const std::vector<T>& tpZ,  
                                                const std::vector<std::array<T, 8>>& X, const std::vector<std::array<T, 8>>& Y,
                                                const std::vector<std::array<T, 8>>& Z)
{
    std::vector<int> in_elements(tpX.size(),0);
    // check if it is insde or outside boundary box
    T minX, minY, minZ, maxX, maxY, maxZ;
    T pcX, pcY, pcZ, element_volume, test_element_volume;
    bool flag;
    for (std::size_t outerIndex = 0; outerIndex < X.size(); outerIndex++) {
        minX = *std::min_element(X[outerIndex].begin(), X[outerIndex].end());
        minY = *std::min_element(Y[outerIndex].begin(), Y[outerIndex].end());
        minZ = *std::min_element(Z[outerIndex].begin(), Z[outerIndex].end());
        maxX = *std::max_element(X[outerIndex].begin(), X[outerIndex].end());
        maxY = *std::max_element(Y[outerIndex].begin(), Y[outerIndex].end());
        maxZ = *std::max_element(Z[outerIndex].begin(), Z[outerIndex].end());
        pcX = std::accumulate(X[outerIndex].begin(), X[outerIndex].end(), 0.0)/8;
        pcY = std::accumulate(Y[outerIndex].begin(), Y[outerIndex].end(), 0.0)/8;            
        pcZ = std::accumulate(Z[outerIndex].begin(), Z[outerIndex].end(), 0.0)/8;
        element_volume = calcHexaVol(X[outerIndex],Y[outerIndex],Z[outerIndex], pcX, pcY,pcZ);                         
        for (size_t innerIndex  = 0; innerIndex < tpX.size(); innerIndex++){
            // check if center of refined volume is outside the boundary box of a coarse volume.
            // Only computes volumed base test is this condition is met.
            flag  = (minX < tpX[innerIndex]) && (maxX > tpX[innerIndex]) &&
                    (minY < tpY[innerIndex]) && (maxY > tpY[innerIndex]) &&
                    (minZ < tpZ[innerIndex]) && (maxZ > tpZ[innerIndex]);                            
            if (flag && (in_elements[innerIndex] == 0)) {
                test_element_volume = calcHexaVol(X[outerIndex],Y[outerIndex],Z[outerIndex], 
                                                tpX[innerIndex], tpY[innerIndex],tpZ[innerIndex]);                         
                if (std::abs(test_element_volume - element_volume) < epslon){
                        in_elements[innerIndex] = static_cast<int>(outerIndex);
                }                             
            }
        }
    }
    return in_elements;
}


// Example for other utilities...

} // namespace GeometryUtils

#endif // GEOMETRY_UTILS_H
