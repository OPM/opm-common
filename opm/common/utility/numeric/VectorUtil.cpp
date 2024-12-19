#include <opm/common/utility/numeric/VectorUtil.hpp>
#include <array>
#include <tuple>

namespace VectorUtil {

std::tuple<std::array<double,4>, std::array<double,4>, std::array<double,4>> 
appendNode(const std::array<double,3>& X, const std::array<double,3>& Y, 
           const std::array<double,3>& Z, const double& xc, const double& yc, 
           const double& zc) 
{
    std::array<double,4> tX;
    std::array<double,4> tY;
    std::array<double,4> tZ;
    std::copy(X.begin(), X.end(), tX.begin());
    tX[3]= xc;
    std::copy(Y.begin(), Y.end(), tY.begin());
    tY[3]= yc;            
    std::copy(Z.begin(), Z.end(), tZ.begin());
    tZ[3]= zc; 
    return std::make_tuple(tX,tY,tZ);
}

} // namespace VectorUtil
