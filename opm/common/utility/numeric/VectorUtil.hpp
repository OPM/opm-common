#ifndef VECTORUTIL_H
#define VECTORUTIL_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <vector>
#include <iterator>

namespace VectorUtil {


template<typename T = std::size_t>
std::tuple<std::vector<T>, std::vector<T>, std::vector<T>> generate_cartesian_product(std::size_t low_nx, std::size_t up_nx,
                                                                                      std::size_t low_ny, std::size_t up_ny,
                                                                                      std::size_t low_nz, std::size_t up_nz){
    std::size_t list_size =  (up_nx - low_nx + 1) * (up_ny - low_ny + 1) * (up_nz - low_nz + 1);
    std::vector<T> i_list(list_size);
    std::vector<T> j_list(list_size);
    std::vector<T> k_list(list_size);
    std::size_t index = 0;
    for (std::size_t k_index = low_nz; k_index <= up_nz; k_index++)
    {
        for (std::size_t j_index = low_ny; j_index <= up_ny; j_index++)
        {
            for (std::size_t i_index = low_nx; i_index <= up_nx; i_index++)
            {
                i_list[index] = i_index;
                j_list[index] = j_index;
                k_list[index] = k_index;
                index++;
            }
        }
    }
    return std::make_tuple(i_list, j_list, k_list);

}


template <typename T = double>
std::tuple<std::array<T,4>, std::array<T,4>, std::array<T,4>>
appendNode(const std::array<T,3>& X, const std::array<T,3>& Y, const std::array<T,3>& Z,
           const T& xc, const T& yc,  const T& zc)
{
    std::array<T,4> tX;
    std::array<T,4> tY;
    std::array<T,4> tZ;
    std::copy(X.begin(), X.end(), tX.begin());
    tX[3]= xc;
    std::copy(Y.begin(), Y.end(), tY.begin());
    tY[3]= yc;
    std::copy(Z.begin(), Z.end(), tZ.begin());
    tZ[3]= zc;
    return std::make_tuple(tX,tY,tZ);
}

// Implementation of generation General Operation between two vectors of the same type
template <typename T, typename Operation>
std::vector<T> vectorOperation(const std::vector<T>& vecA, const std::vector<T>& vecB, Operation op) {
    if (vecA.size() != vecB.size()) {
        throw std::invalid_argument("Error: Vectors must have the same size!"); // Throwing exception
    }
    std::vector<T> result;
    result.reserve(vecA.size());
    // Use std::transform with the passed operation
    std::transform(vecA.begin(), vecA.end(), vecB.begin(), std::back_inserter(result), op);
    return result;
}

// Implementation of generation General Operation between a vector and a scalar of the same type
template <typename T, typename Operation>
std::vector<T> vectorScalarOperation(const std::vector<T>& vecA, const T& scalar, Operation op) {
    std::vector<T> result;
    result.reserve(vecA.size());
    // Use std::transform with the passed operation
    std::transform(vecA.begin(), vecA.end(), std::back_inserter(result),
        [&scalar, &op](const T& a) { return op(scalar, a); });
    return result;
}

// Implementation of generation General Operation between a scalar and a vector of the same type
template <typename T, typename Operation>
std::vector<T> scalarVectorOperation(const T& scalar, const std::vector<T>& vecA,  Operation op) {
    std::vector<T> result;
    result.reserve(vecA.size());
    // Use std::transform with the passed operation
    std::transform(vecA.begin(), vecA.end(), std::back_inserter(result),
        [&scalar, &op](const T& a) { return op(a, scalar); });
    return result;
}

std::tuple<std::array<double,4>, std::array<double,4>, std::array<double,4>> 
appendNode(const std::array<double,3>&, const std::array<double,3>&, 
           const std::array<double,3>&, const double&, const double&, 
           const double&);
template <typename T>
std::vector<T> filterArray(const std::vector<std::size_t>& X, const std::vector<int>& ind){
    std::vector<T> filtered_vectorX(ind.size(),0);
    for (std::size_t index = 0; index < ind.size(); index++) {
        filtered_vectorX[index] = X[ind[index]];
    }
    return filtered_vectorX;
}

// T type of the object
// Rout type of the object output
// Method type of method
template <typename T,  typename Rout, typename Rin = std::size_t, typename Method, typename... Args>
std::vector<Rout> callMethodForEachInputOnObject(const T& obj, Method mtd, const std::vector<Rin>& input_vector, Args&&... args) {
    std::vector<Rout> result;
    // Reserve space for each vector in the tuple
    result.reserve(input_vector.size());
    // Iterate over the input_vector and fill the tuple's vectors
    for (const auto& element : input_vector) {
        Rout value = (obj.*mtd)(element, std::forward<Args>(args)...);
        result.push_back(std::move(value));
    }
    return result;
}



template <typename T>
std::tuple<std::vector<T>,std::vector<T>, std::vector<T>> splitXYZ(std::vector<std::array<T,3>>& input_vector ){
std::vector<T> X, Y, Z;
    X.reserve(input_vector.size());
    Y.reserve(input_vector.size());
    Z.reserve(input_vector.size());
    for (auto& element : input_vector) {
            X.push_back(std::move(element[0]));  // Add to first vector
            Y.push_back(std::move(element[1]));    // Add to second vector
            Z.push_back(std::move(element[2]));   // Add to third vector
    }
    return std::make_tuple(X,Y,Z);
}


template <typename T,  typename Rout, typename Rin = std::size_t, typename Method, typename... Args>
auto callMethodForEachInputOnObjectXYZ(const T& obj, Method mtd, const std::vector<Rin>& input_vector, Args&&... args) {
    using X = typename Rout::value_type;
    auto result = callMethodForEachInputOnObject<T, Rout, Rin, Method, Args...>(obj, mtd, input_vector, std::forward<Args>(args)...);
    return splitXYZ<X>(result);
}

// Example for other utilities...

} // namespace VectorUtil

#endif // VECTORUTIL_H
