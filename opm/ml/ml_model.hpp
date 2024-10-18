/*
  Copyright (c) 2016 Robert W. Rose
  Copyright (c) 2018 Paul Maevskikh
  Copyright (c) 2024 NORCE
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

#ifndef ML_MODEL_H_
#define ML_MODEL_H_

#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <opm/common/ErrorMacros.hpp>
#include <opm/material/densead/Math.hpp>
#include <string>
#include <vector>

namespace Opm {

// NN layer
// ---------------------
/** \class Tensor class
 * Implements mathematical tensor (Max 4d)
 */
template <class T> class Tensor {
 public:
  Tensor() {}

  explicit Tensor(int i) { resizeI<int>({i}); }

  Tensor(int i, int j) { resizeI<int>({i, j}); }

  Tensor(int i, int j, int k) { resizeI<int>({i, j, k}); }

  Tensor(int i, int j, int k, int l) { resizeI<int>({i, j, k, l}); }

  template <typename Type> void resizeI(std::vector<Type> c) {
    if (c.size() == 1) dims_ = {(int)c[0]};
    if (c.size() == 2) dims_ = {(int)c[0], (int)c[1]};
    if (c.size() == 3) dims_ = {(int)c[0], (int)c[1], (int)c[2]};
    if (c.size() == 4) dims_ = {(int)c[0], (int)c[1], (int)c[2], (int)c[3]};

    data_.resize(
        std::accumulate(begin(c), end(c), 1.0, std::multiplies<Type>()));
  }

  inline void flatten() {
    OPM_ERROR_IF(dims_.size() <= 0, "Invalid tensor");

    int elements = dims_[0];
    for (unsigned int i = 1; i < dims_.size(); i++) {
      elements *= dims_[i];
    }
    dims_ = {elements};
  }

  inline T& operator()(int i) {
    OPM_ERROR_IF(dims_.size() != 1, "Invalid indexing for tensor");

    OPM_ERROR_IF(!(i < dims_[0] && i >= 0), fmt::format(" Invalid i:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        i, dims_[0]));

    return data_[i];
  }

  inline T& operator()(int i, int j) {
    OPM_ERROR_IF(dims_.size() != 2, "Invalid indexing for tensor");
    OPM_ERROR_IF(!(i < dims_[0] && i >= 0), fmt::format(" Invalid i:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        i, dims_[0]));
    OPM_ERROR_IF(!(j < dims_[1] && j >= 0), fmt::format(" Invalid j:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        j, dims_[1]));

    return data_[dims_[1] * i + j];
  }

  const T& operator()(int i, int j) const {
    OPM_ERROR_IF(dims_.size() != 2, "Invalid indexing for tensor");
    OPM_ERROR_IF(!(i < dims_[0] && i >= 0), fmt::format(" Invalid i:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        i, dims_[0]));
    OPM_ERROR_IF(!(j < dims_[1] && j >= 0), fmt::format(" Invalid j:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        j, dims_[1]));
    return data_[dims_[1] * i + j];
  }

  inline T& operator()(int i, int j, int k) {
    OPM_ERROR_IF(dims_.size() != 3, "Invalid indexing for tensor");
    OPM_ERROR_IF(!(i < dims_[0] && i >= 0), fmt::format(" Invalid i:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        i, dims_[0]));
    OPM_ERROR_IF(!(j < dims_[1] && j >= 0), fmt::format(" Invalid j:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        j, dims_[1]));
    OPM_ERROR_IF(!(k < dims_[2] && k >= 0), fmt::format(" Invalid k:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        k, dims_[2]));

    return data_[dims_[2] * (dims_[1] * i + j) + k];
  }
  const T& operator()(int i, int j, int k) const {
    OPM_ERROR_IF(dims_.size() != 3, "Invalid indexing for tensor");
    OPM_ERROR_IF(!(i < dims_[0] && i >= 0), fmt::format(" Invalid i:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        i, dims_[0]));
    OPM_ERROR_IF(!(j < dims_[1] && j >= 0), fmt::format(" Invalid j:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        j, dims_[1]));
    OPM_ERROR_IF(!(k < dims_[2] && k >= 0), fmt::format(" Invalid k:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        k, dims_[2]));

    return data_[dims_[2] * (dims_[1] * i + j) + k];
  }

  inline T& operator()(int i, int j, int k, int l) {
    OPM_ERROR_IF(dims_.size() != 4, "Invalid indexing for tensor");
    OPM_ERROR_IF(!(i < dims_[0] && i >= 0), fmt::format(" Invalid i:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        i, dims_[0]));
    OPM_ERROR_IF(!(j < dims_[1] && j >= 0), fmt::format(" Invalid j:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        j, dims_[1]));
    OPM_ERROR_IF(!(k < dims_[2] && k >= 0), fmt::format(" Invalid k:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        k, dims_[2]));
    OPM_ERROR_IF(!(l < dims_[3] && l >= 0), fmt::format(" Invalid l:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        l, dims_[3]));

    return data_[dims_[3] * (dims_[2] * (dims_[1] * i + j) + k) + l];
  }

  const T& operator()(int i, int j, int k, int l) const {
    OPM_ERROR_IF(dims_.size() != 4, "Invalid indexing for tensor");
    OPM_ERROR_IF(!(i < dims_[0] && i >= 0), fmt::format(" Invalid i:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        i, dims_[0]));
    OPM_ERROR_IF(!(j < dims_[1] && j >= 0), fmt::format(" Invalid j:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        j, dims_[1]));
    OPM_ERROR_IF(!(k < dims_[2] && k >= 0), fmt::format(" Invalid k:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        k, dims_[2]));
    OPM_ERROR_IF(!(l < dims_[3] && l >= 0), fmt::format(" Invalid l:  "
                                                        "{}"
                                                        "  max: "
                                                        "{}",
                                                        l, dims_[3]));

    return data_[dims_[3] * (dims_[2] * (dims_[1] * i + j) + k) + l];
  }
  void fill(const T& value) { std::fill(data_.begin(), data_.end(), value); }

  // Tensor addition
  Tensor operator+(const Tensor& other) {
    OPM_ERROR_IF(dims_.size() != other.dims_.size(),
                 "Cannot add tensors with different dimensions");
    Tensor result;
    result.dims_ = dims_;
    result.data_.reserve(data_.size());

    std::transform(data_.begin(), data_.end(), other.data_.begin(),
                   std::back_inserter(result.data_),
                   [](T x, T y) { return x + y; });

    return result;
  }

  // Tensor multiplication
  Tensor multiply(const Tensor& other) {
    OPM_ERROR_IF(dims_.size() != other.dims_.size(),
                 "Cannot multiply elements with different dimensions");

    Tensor result;
    result.dims_ = dims_;
    result.data_.reserve(data_.size());

    std::transform(data_.begin(), data_.end(), other.data_.begin(),
                   std::back_inserter(result.data_),
                   [](T x, T y) { return x * y; });

    return result;
  }

  // Tensor dot for 2d tensor
  Tensor dot(const Tensor& other) {
    OPM_ERROR_IF(dims_.size() != 2, "Invalid tensor dimensions");
    OPM_ERROR_IF(other.dims_.size() != 2, "Invalid tensor dimensions");

    OPM_ERROR_IF(dims_[1] != other.dims_[0],
                 "Cannot multiply with different inner dimensions");

    Tensor tmp(dims_[0], other.dims_[1]);

    for (int i = 0; i < dims_[0]; i++) {
      for (int j = 0; j < other.dims_[1]; j++) {
        for (int k = 0; k < dims_[1]; k++) {
          tmp(i, j) += (*this)(i, k) * other(k, j);
        }
      }
    }

    return tmp;
  }

  std::vector<int> dims_;
  std::vector<T> data_;
};

// NN layer
// ---------------------
/** \class Neural Network  Layer base class
 */
template <class Evaluation> class NNLayer {
 public:
  NNLayer() {}

  virtual ~NNLayer() {}

  virtual bool loadLayer(std::ifstream& file) = 0;

  virtual bool apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out) = 0;
};

/** \class Activation  Layer class
 * Applies an activation function
 */
template <class Evaluation>
class NNLayerActivation : public NNLayer<Evaluation> {
 public:
  enum ActivationType {
    kLinear = 1,
    kRelu = 2,
    kSoftPlus = 3,
    kSigmoid = 4,
    kTanh = 5,
    kHardSigmoid = 6
  };

  NNLayerActivation() : activation_type_(ActivationType::kLinear) {}

  virtual ~NNLayerActivation() {}

  virtual bool loadLayer(std::ifstream& file);

  virtual bool apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out);

 private:
  ActivationType activation_type_;
};

/** \class Scaling Layer class
 * A preprocessing layer which rescales input values to a new range.
 */
template <class Evaluation> class NNLayerScaling : public NNLayer<Evaluation> {
 public:
  NNLayerScaling()
      : data_min(1.0f), data_max(1.0f), feat_inf(1.0f), feat_sup(1.0f) {}

  virtual ~NNLayerScaling() {}

  virtual bool loadLayer(std::ifstream& file);

  virtual bool apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out);

 private:
  Tensor<float> weights_;
  Tensor<float> biases_;
  float data_min;
  float data_max;
  float feat_inf;
  float feat_sup;
};

/** \class Unscaling Layer class
 * A postprocessing layer to undo the scaling according to feature_range.
 */
template <class Evaluation>
class NNLayerUnScaling : public NNLayer<Evaluation> {
 public:
  NNLayerUnScaling()
      : data_min(1.0f), data_max(1.0f), feat_inf(1.0f), feat_sup(1.0f) {}

  virtual ~NNLayerUnScaling() {}

  virtual bool loadLayer(std::ifstream& file);

  virtual bool apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out);

 private:
  Tensor<float> weights_;
  Tensor<float> biases_;
  float data_min;
  float data_max;
  float feat_inf;
  float feat_sup;
};

/** \class Dense Layer class
 * Densely-connected NN layer.
 */
template <class Evaluation> class NNLayerDense : public NNLayer<Evaluation> {
 public:
  NNLayerDense() {}

  virtual ~NNLayerDense() {}

  virtual bool loadLayer(std::ifstream& file);

  virtual bool apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out);

 private:
  Tensor<float> weights_;
  Tensor<float> biases_;

  NNLayerActivation<Evaluation> activation_;
};

/** \class Embedding Layer class
 * Turns nonnegative integers (indexes) into dense vectors of fixed size.
 */
template <class Evaluation>
class NNLayerEmbedding : public NNLayer<Evaluation> {
 public:
  NNLayerEmbedding() {}

  virtual ~NNLayerEmbedding() {}

  virtual bool loadLayer(std::ifstream& file);

  virtual bool apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out);

 private:
  Tensor<float> weights_;
};

/** \class Neural Network Model class
 * A model grouping layers into an object
 */
template <class Evaluation> class NNModel {
 public:
  enum LayerType { kScaling = 1, kUnScaling = 2, kDense = 3, kActivation = 4 };

  NNModel() {}

  virtual ~NNModel() {}
  // loads models generated by Kerasify
  virtual bool loadModel(const std::string& filename);

  virtual bool apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out);

 private:
  std::vector<std::unique_ptr<NNLayer<Evaluation>>> layers_;
};

/** \class Neural Network Timer class
 */
class NNTimer {
 public:
  NNTimer() {}

  void start() { start_ = std::chrono::high_resolution_clock::now(); }

  float stop() {
    std::chrono::time_point<std::chrono::high_resolution_clock> now =
        std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff = now - start_;

    return diff.count();
  }

 private:
  std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

}  // namespace Opm

#endif  // ML_MODEL_H_
