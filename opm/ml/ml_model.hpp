/*

 * Copyright (c) 2016 Robert W. Rose
 * Copyright (c) 2018 Paul Maevskikh
 *
 * MIT License, see LICENSE.MIT file.
 */ 

/*
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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <string>
#include <vector>
#include <iostream>

#include <opm/material/densead/Math.hpp>
#include <opm/common/ErrorMacros.hpp>
#include <sstream>

namespace Opm {

#define KASSERT(x, ...)                                                        \
    if (!(x)) {                                                                \
        std::printf("KASSERT: %s(%d): ", __FILE__, __LINE__);                       \
        std::printf(__VA_ARGS__);                                                   \
        std::printf("\n");                                                          \
        return false;                                                          \
    }

#define KASSERT_EQ(x, y, eps)                                                  \
    if (fabs(x.value() - y.value()) > eps) {                                   \
        std::printf("KASSERT: Expected %f, got %f\n", y.value(), x.value());        \
        return false;                                                          \
    }

#ifdef DEBUG
#define KDEBUG(x, ...)                                                         \
    if (!(x)) {                                                                \
        std::printf("%s(%d): ", __FILE__, __LINE__);                                \
        std::printf(__VA_ARGS__);                                                   \
        std::printf("\n");                                                          \
        exit(-1);                                                              \
    }
#else
#define KDEBUG(x, ...) ;
#endif

template<class T>
class Tensor {
  public:
    Tensor() {}

     explicit Tensor(int i) { resize(i); }
     
    Tensor(int i, int j) { resize(i, j); }

    Tensor(int i, int j, int k) { resize(i, j, k); }

    Tensor(int i, int j, int k, int l) { resize(i, j, k, l); }

    void resize(int i) {
        dims_ = {i};
        data_.resize(i);
    }

    void resize(int i, int j) {
        dims_ = {i, j};
        data_.resize(i * j);
    }

    void resize(int i, int j, int k) {
        dims_ = {i, j, k};
        data_.resize(i * j * k);
    }

    void resize(int i, int j, int k, int l) {
        dims_ = {i, j, k, l};
        data_.resize(i * j * k * l);
    }

    inline void flatten() {
        KDEBUG(dims_.size() > 0, "Invalid tensor");

        int elements = dims_[0];
        for (unsigned int i = 1; i < dims_.size(); i++) {
            elements *= dims_[i];
        }
        dims_ = {elements};
    }

    inline T& operator()(int i) {
        KDEBUG(dims_.size() == 1, "Invalid indexing for tensor");
        KDEBUG(i < dims_[0] && i >= 0, "Invalid i: %d (max %d)", i, dims_[0]);

        return data_[i];
    }

    inline T& operator()(int i, int j) {
        KDEBUG(dims_.size() == 2, "Invalid indexing for tensor");
        KDEBUG(i < dims_[0] && i >= 0, "Invalid i: %d (max %d)", i, dims_[0]);
        KDEBUG(j < dims_[1] && j >= 0, "Invalid j: %d (max %d)", j, dims_[1]);

        return data_[dims_[1] * i + j];
    }

    const T& operator()(int i, int j) const {
        KDEBUG(dims_.size() == 2, "Invalid indexing for tensor");
        KDEBUG(i < dims_[0] && i >= 0, "Invalid i: %d (max %d)", i, dims_[0]);
        KDEBUG(j < dims_[1] && j >= 0, "Invalid j: %d (max %d)", j, dims_[1]);

        return data_[dims_[1] * i + j];
    }

    inline T& operator()(int i, int j, int k) {
        KDEBUG(dims_.size() == 3, "Invalid indexing for tensor");
        KDEBUG(i < dims_[0] && i >= 0, "Invalid i: %d (max %d)", i, dims_[0]);
        KDEBUG(j < dims_[1] && j >= 0, "Invalid j: %d (max %d)", j, dims_[1]);
        KDEBUG(k < dims_[2] && k >= 0, "Invalid k: %d (max %d)", k, dims_[2]);

        return data_[dims_[2] * (dims_[1] * i + j) + k];
    }
        const T& operator()(int i, int j, int k) const {
        KDEBUG(dims_.size() == 3, "Invalid indexing for tensor");
        KDEBUG(i < dims_[0] && i >= 0, "Invalid i: %d (max %d)", i, dims_[0]);
        KDEBUG(j < dims_[1] && j >= 0, "Invalid j: %d (max %d)", j, dims_[1]);
        KDEBUG(k < dims_[2] && k >= 0, "Invalid k: %d (max %d)", k, dims_[2]);

        return data_[dims_[2] * (dims_[1] * i + j) + k];
    }

    inline T& operator()(int i, int j, int k, int l) {
        KDEBUG(dims_.size() == 4, "Invalid indexing for tensor");
        KDEBUG(i < dims_[0] && i >= 0, "Invalid i: %d (max %d)", i, dims_[0]);
        KDEBUG(j < dims_[1] && j >= 0, "Invalid j: %d (max %d)", j, dims_[1]);
        KDEBUG(k < dims_[2] && k >= 0, "Invalid k: %d (max %d)", k, dims_[2]);
        KDEBUG(l < dims_[3] && l >= 0, "Invalid l: %d (max %d)", l, dims_[3]);

        return data_[dims_[3] * (dims_[2] * (dims_[1] * i + j) + k) + l];
    }

    const T& operator()(int i, int j, int k, int l) const{
        KDEBUG(dims_.size() == 4, "Invalid indexing for tensor");
        KDEBUG(i < dims_[0] && i >= 0, "Invalid i: %d (max %d)", i, dims_[0]);
        KDEBUG(j < dims_[1] && j >= 0, "Invalid j: %d (max %d)", j, dims_[1]);
        KDEBUG(k < dims_[2] && k >= 0, "Invalid k: %d (max %d)", k, dims_[2]);
        KDEBUG(l < dims_[3] && l >= 0, "Invalid l: %d (max %d)", l, dims_[3]);

        return data_[dims_[3] * (dims_[2] * (dims_[1] * i + j) + k) + l];
    }
     void fill(const T & value) {
        std::fill(data_.begin(), data_.end(), value);
    }

    // Tensor Unpack(int row) const {
    //     KASSERT(dims_.size() >= 2, "Invalid tensor");
    //     std::vector<int> pack_dims =
    //         std::vector<int>(dims_.begin() + 1, dims_.end());
    //     int pack_size = std::accumulate(pack_dims.begin(), pack_dims.end(), 0);

    //     typename std::vector<T>::const_iterator first =
    //         data_.begin() + (row * pack_size);
    //     typename std::vector<T>::const_iterator last =
    //         data_.begin() + (row + 1) * pack_size;

    //     Tensor x = Tensor();
    //     x.dims_ = pack_dims;
    //     x.data_ = std::vector<T>(first, last);

    //     return x;
    // }

    // Tensor Select(int row) const {
    //     Tensor x = Unpack(row);
    //     x.dims_.insert(x.dims_.begin(), 1);

    //     return x;
    // }

    Tensor operator+(const Tensor& other) {
        // KASSERT(dims_ == other.dims_,
                // "Cannot add tensors with different dimensions");
	    OPM_ERROR_IF(dims_.size()  != other.dims_.size() ,
                "Cannot add tensors with different dimensions");
// std::cout<<"dims_ "<<dims_.size()<<std::endl;
// std::cout<<"other.dims_ "<<other.dims_.size()<<std::endl;

        Tensor result;
        result.dims_ = dims_;
        result.data_.reserve(data_.size());

        std::transform(data_.begin(), data_.end(), other.data_.begin(),
                       std::back_inserter(result.data_),
                       [](T x, T y) { return x + y; });

        return result;
    }

    Tensor multiply(const Tensor& other) {
        // KASSERT(dims_ == other.dims_,
        //         "Cannot multiply elements with different dimensions");
	    OPM_ERROR_IF(dims_.size()  != other.dims_.size() ,
                      "Cannot multiply elements with different dimensions");

        Tensor result;
        result.dims_ = dims_;
        result.data_.reserve(data_.size());

        std::transform(data_.begin(), data_.end(), other.data_.begin(),
                       std::back_inserter(result.data_),
                       [](T x, T y) { return x * y; });

        return result;
    }

    Tensor dot(const Tensor& other) {
        KDEBUG(dims_.size() == 2, "Invalid tensor dimensions");
        KDEBUG(other.dims_.size() == 2, "Invalid tensor dimensions");
        // KASSERT(dims_[1] == other.dims_[0],
        //         "Cannot multiply with different inner dimensions");
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

template<class Evaluation>
class KerasLayer {
  public:
    KerasLayer() {}

    virtual ~KerasLayer() {}

    virtual bool loadLayer(std::ifstream& file) = 0;

    virtual bool apply(Tensor<Evaluation>& in, Tensor<Evaluation>& out) = 0;
};

template<class Evaluation>
class KerasLayerActivation : public KerasLayer<Evaluation> {
  public:
    enum ActivationType {
        kLinear = 1,
        kRelu = 2,
        kSoftPlus = 3,
        kSigmoid = 4,
        kTanh = 5,
        kHardSigmoid = 6
    };

    KerasLayerActivation() : activation_type_(ActivationType::kLinear) {}

    virtual ~KerasLayerActivation() {}

    virtual bool loadLayer(std::ifstream& file);

    virtual bool apply(Tensor<Evaluation>& in, Tensor<Evaluation>& out);

  private:
    ActivationType activation_type_;
};

template<class Evaluation>
class KerasLayerScaling : public KerasLayer<Evaluation> {
  public:
    KerasLayerScaling(): data_min(1.0f), data_max(1.0f), feat_inf(1.0f), feat_sup(1.0f) {}

    virtual ~KerasLayerScaling() {}

    virtual bool loadLayer(std::ifstream& file);

    virtual bool apply(Tensor<Evaluation>& in, Tensor<Evaluation>& out);

  private:
    Tensor<float> weights_;
    Tensor<float> biases_;
    float data_min;
    float data_max;
    float feat_inf;
    float feat_sup;
};

template<class Evaluation>
class KerasLayerUnScaling : public KerasLayer<Evaluation> {
  public:
    KerasLayerUnScaling(): data_min(1.0f), data_max(1.0f), feat_inf(1.0f), feat_sup(1.0f) {}

    virtual ~KerasLayerUnScaling() {}

    virtual bool loadLayer(std::ifstream& file);

    virtual bool apply(Tensor<Evaluation>& in, Tensor<Evaluation>& out);

  private:
    Tensor<float> weights_;
    Tensor<float> biases_;
    float data_min;
    float data_max;
    float feat_inf;
    float feat_sup;
};


template<class Evaluation>
class KerasLayerDense : public KerasLayer<Evaluation> {
  public:
    KerasLayerDense() {}

    virtual ~KerasLayerDense() {}

    virtual bool loadLayer(std::ifstream& file);

    virtual bool apply(Tensor<Evaluation>& in, Tensor<Evaluation>& out);

  private:
    Tensor<float> weights_;
    Tensor<float> biases_;

    KerasLayerActivation<Evaluation> activation_;
};

template<class Evaluation>
class KerasLayerFlatten : public KerasLayer<Evaluation> {
  public:
    KerasLayerFlatten() {}

    virtual ~KerasLayerFlatten() {}

    virtual bool loadLayer(std::ifstream& file);

    virtual bool apply(Tensor<Evaluation>& in, Tensor<Evaluation>& out);

  // private:
};


template<class Evaluation>
class KerasLayerEmbedding : public KerasLayer<Evaluation> {
  public:
    KerasLayerEmbedding() {}

    virtual ~KerasLayerEmbedding() {}

    virtual bool loadLayer(std::ifstream& file);

    virtual bool apply(Tensor<Evaluation>& in, Tensor<Evaluation>& out);

  private:
    Tensor<float> weights_;
};

template<class Evaluation>
class KerasModel {
  public:
    enum LayerType {
        kFlatten = 1,
        kScaling = 2,
        kUnScaling = 3,
        kDense = 4,
        kActivation = 5
    };

    KerasModel() {}

    virtual ~KerasModel() {
        // for (unsigned int i = 0; i < layers_.size(); i++) {
        //     delete layers_[i];
        // }
    }

    virtual bool loadModel(const std::string& filename);

    virtual bool apply(Tensor<Evaluation>& in, Tensor<Evaluation>& out);

  private:
    // std::vector<KerasLayer<Evaluation>*> layers_;
    std::vector<std::unique_ptr<KerasLayer<Evaluation>>> layers_;
};

class KerasTimer {
  public:
    KerasTimer() {}

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

} // namespace Opm

#endif // ML_MODEL_H_
