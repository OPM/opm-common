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

#include "keras_model.hpp"

#include <cmath>
#include <fstream>
#include <limits>
#include <stdio.h>
#include <utility>

#include <iostream>

namespace Opm {


bool ReadUnsignedInt(std::ifstream* file, unsigned int* i) {
    KASSERT(file, "Invalid file stream");
    KASSERT(i, "Invalid pointer");

    file->read((char*)i, sizeof(unsigned int));
    KASSERT(file->gcount() == sizeof(unsigned int), "Expected unsigned int");

    return true;
}

bool ReadFloat(std::ifstream* file, float* f) {
    KASSERT(file, "Invalid file stream");
    KASSERT(f, "Invalid pointer");

    file->read((char*)f, sizeof(float));
    KASSERT(file->gcount() == sizeof(float), "Expected Evaluation");

    return true;
}

bool ReadFloats(std::ifstream* file, float* f, size_t n) {
    KASSERT(file, "Invalid file stream");
    KASSERT(f, "Invalid pointer");

    file->read((char*)f, sizeof(float) * n);
    KASSERT(((unsigned int)file->gcount()) == sizeof(float) * n,
            "Expected Evaluations");

    return true;
}

template<class Evaluation>
bool KerasLayerActivation<Evaluation>::LoadLayer(std::ifstream* file) {
    KASSERT(file, "Invalid file stream");

    unsigned int activation = 0;
    KASSERT(ReadUnsignedInt(file, &activation),
            "Failed to read activation type");

    switch (activation) {
    case kLinear:
        activation_type_ = kLinear;
        break;
    case kRelu:
        activation_type_ = kRelu;
        break;
    case kSoftPlus:
        activation_type_ = kSoftPlus;
        break;
    case kHardSigmoid:
        activation_type_ = kHardSigmoid;
        break;
    case kSigmoid:
        activation_type_ = kSigmoid;
        break;
    case kTanh:
        activation_type_ = kTanh;
        break;
    default:
        KASSERT(false, "Unsupported activation type %d", activation);
    }

    return true;
}

template<class Evaluation>
bool KerasLayerActivation<Evaluation>::Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out) {
    KASSERT(in, "Invalid input");
    KASSERT(out, "Invalid output");

    *out = *in;

    switch (activation_type_) {
    case kLinear:
        break;
    case kRelu:
        for (size_t i = 0; i < out->data_.size(); i++) {
            if (out->data_[i] < 0.0) {
                out->data_[i] = 0.0;
            }
        }
        break;
    case kSoftPlus:
        for (size_t i = 0; i < out->data_.size(); i++) {
            out->data_[i] = log(1.0 + exp(out->data_[i]));
        }
        break;
    case kHardSigmoid:
        for (size_t i = 0; i < out->data_.size(); i++) {
            Evaluation x = (out->data_[i] * 0.2) + 0.5;

            if (x <= 0) {
                out->data_[i] = 0.0;
            } else if (x >= 1) {
                out->data_[i] = 1.0;
            } else {
                out->data_[i] = x;
            }
        }
        break;
    case kSigmoid:
        for (size_t i = 0; i < out->data_.size(); i++) {
            Evaluation& x = out->data_[i];

            if (x >= 0) {
                out->data_[i] = 1.0 / (1.0 + exp(-x));
            } else {
                Evaluation z = exp(x);
                out->data_[i] = z / (1.0 + z);
            }
        }
        break;
    case kTanh:
        for (size_t i = 0; i < out->data_.size(); i++) {
            out->data_[i] = sinh(out->data_[i])/cosh(out->data_[i]);
        }
        break;
    default:
        break;
    }

    return true;
}


template<class Evaluation>
bool KerasLayerScaling<Evaluation>::LoadLayer(std::ifstream* file) {
    KASSERT(file, "Invalid file stream");

    KASSERT(ReadFloat(file, &data_min), "Failed to read min");
    KASSERT(ReadFloat(file, &data_max), "Failed to read max");
    KASSERT(ReadFloat(file, &feat_inf), "Failed to read max");
    KASSERT(ReadFloat(file, &feat_sup), "Failed to read max");
    return true;
}

template<class Evaluation>
bool KerasLayerScaling<Evaluation>::Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out) {
    KASSERT(in, "Invalid input");
    KASSERT(out, "Invalid output");

    Tensor<Evaluation> temp_in, temp_out;

    *out = *in;

    for (size_t i = 0; i < out->data_.size(); i++) {
        auto tempscale = (out->data_[i] - data_min)/(data_max - data_min);
        out->data_[i] = tempscale * (feat_sup - feat_inf) + feat_inf;
    }

    return true;
}

template<class Evaluation>
bool KerasLayerUnScaling<Evaluation>::LoadLayer(std::ifstream* file) {
    KASSERT(file, "Invalid file stream");

    KASSERT(ReadFloat(file, &data_min), "Failed to read min");
    KASSERT(ReadFloat(file, &data_max), "Failed to read max");
    KASSERT(ReadFloat(file, &feat_inf), "Failed to read inf");
    KASSERT(ReadFloat(file, &feat_sup), "Failed to read sup");

    return true;
}

template<class Evaluation>
bool KerasLayerUnScaling<Evaluation>::Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out) {
    KASSERT(in, "Invalid input");
    KASSERT(out, "Invalid output");

    *out = *in;

    for (size_t i = 0; i < out->data_.size(); i++) {
        auto tempscale = (out->data_[i] - feat_inf)/(feat_sup - feat_inf);

        out->data_[i] = tempscale * (data_max - data_min) + data_min;
    }


    return true;
}


template<class Evaluation>
bool KerasLayerDense<Evaluation>::LoadLayer(std::ifstream* file) {
    KASSERT(file, "Invalid file stream");

    unsigned int weights_rows = 0;
    KASSERT(ReadUnsignedInt(file, &weights_rows), "Expected weight rows");
    KASSERT(weights_rows > 0, "Invalid weights # rows");

    unsigned int weights_cols = 0;
    KASSERT(ReadUnsignedInt(file, &weights_cols), "Expected weight cols");
    KASSERT(weights_cols > 0, "Invalid weights shape");

    unsigned int biases_shape = 0;
    KASSERT(ReadUnsignedInt(file, &biases_shape), "Expected biases shape");
    KASSERT(biases_shape > 0, "Invalid biases shape");

    weights_.Resize(weights_rows, weights_cols);
    KASSERT(
        ReadFloats(file, weights_.data_.data(), weights_rows * weights_cols),
        "Expected weights");

    biases_.Resize(biases_shape);
    KASSERT(ReadFloats(file, biases_.data_.data(), biases_shape),
            "Expected biases");

    KASSERT(activation_.LoadLayer(file), "Failed to load activation");

    return true;
}

template<class Evaluation>
bool KerasLayerDense<Evaluation>::Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out) {
    KASSERT(in, "Invalid input");
    KASSERT(out, "Invalid output");
    KASSERT(in->dims_.size() <= 2, "Invalid input dimensions");

    if (in->dims_.size() == 2) {
        KASSERT(in->dims_[1] == weights_.dims_[0], "Dimension mismatch %d %d",
                in->dims_[1], weights_.dims_[0]);
    }

    Tensor<Evaluation> tmp(weights_.dims_[1]);

    for (int i = 0; i < weights_.dims_[0]; i++) {
        for (int j = 0; j < weights_.dims_[1]; j++) {
            tmp(j) += (*in)(i)*weights_(i, j);
        }
    }

    for (int i = 0; i < biases_.dims_[0]; i++) {
        tmp(i) += biases_(i);
    }

    KASSERT(activation_.Apply(&tmp, out), "Failed to apply activation");

    return true;
}

template<class Evaluation>
bool KerasLayerFlatten<Evaluation>::LoadLayer(std::ifstream* file) {
    KASSERT(file, "Invalid file stream");
    return true;
}

template<class Evaluation>
bool KerasLayerFlatten<Evaluation>::Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out) {
    KASSERT(in, "Invalid input");
    KASSERT(out, "Invalid output");

    *out = *in;
    out->Flatten();

    return true;
}



template<class Evaluation>
bool KerasLayerEmbedding<Evaluation>::LoadLayer(std::ifstream* file) {
    KASSERT(file, "Invalid file stream");

    unsigned int weights_rows = 0;
    KASSERT(ReadUnsignedInt(file, &weights_rows), "Expected weight rows");
    KASSERT(weights_rows > 0, "Invalid weights # rows");

    unsigned int weights_cols = 0;
    KASSERT(ReadUnsignedInt(file, &weights_cols), "Expected weight cols");
    KASSERT(weights_cols > 0, "Invalid weights shape");

    weights_.Resize(weights_rows, weights_cols);
    KASSERT(
        ReadFloats(file, weights_.data_.data(), weights_rows * weights_cols),
        "Expected weights");

    return true;
}

template<class Evaluation>
bool KerasLayerEmbedding<Evaluation>::Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out) {
    int output_rows = in->dims_[1];
    int output_cols = weights_.dims_[1];
    out->dims_ = {output_rows, output_cols};
    out->data_.reserve(output_rows * output_cols);

    std::for_each(in->data_.begin(), in->data_.end(), [=](Evaluation i) {
        typename std::vector<Evaluation>::const_iterator first =
            this->weights_.data_.begin() + (getValue(i) * output_cols);
        typename std::vector<Evaluation>::const_iterator last =
            this->weights_.data_.begin() + (getValue(i) + 1) * output_cols;

        out->data_.insert(out->data_.end(), first, last);
    });

    return true;
}


template<class Evaluation>
bool KerasModel<Evaluation>::LoadModel(const std::string& filename) {
    std::ifstream file(filename.c_str(), std::ios::binary);
    KASSERT(file.is_open(), "Unable to open file %s", filename.c_str());

    unsigned int num_layers = 0;
    KASSERT(ReadUnsignedInt(&file, &num_layers), "Expected number of layers");

    for (unsigned int i = 0; i < num_layers; i++) {
        unsigned int layer_type = 0;
        KASSERT(ReadUnsignedInt(&file, &layer_type), "Expected layer type");

        KerasLayer<Evaluation>* layer = NULL;

        switch (layer_type) {
        case kFlatten:
            layer = new KerasLayerFlatten<Evaluation>();
            break;
        case kScaling:
            layer = new KerasLayerScaling<Evaluation>();
            break;
        case kUnScaling:
            layer = new KerasLayerUnScaling<Evaluation>();
            break;
        case kDense:
            layer = new KerasLayerDense<Evaluation>();
            break;
        case kActivation:
            layer = new KerasLayerActivation<Evaluation>();
            break;
        default:
            break;
        }

        KASSERT(layer, "Unknown layer type %d", layer_type);

        bool result = layer->LoadLayer(&file);
        if (!result) {
            printf("Failed to load layer %d", i);
            delete layer;
            return false;
        }

        layers_.push_back(layer);
    }

    return true;
}

template<class Evaluation>
bool KerasModel<Evaluation>::Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out) {
    Tensor<Evaluation> temp_in, temp_out;

    for (unsigned int i = 0; i < layers_.size(); i++) {
        if (i == 0) {
            temp_in = *in;
        }

        KASSERT(layers_[i]->Apply(&temp_in, &temp_out),
                "Failed to apply layer %d", i);

        temp_in = temp_out;
    }

    *out = temp_out;

    return true;
}

template class KerasModel<float>;

template class KerasModel<double>;
template class KerasModel<Opm::DenseAd::Evaluation<double, 6>>;
template class KerasModel<Opm::DenseAd::Evaluation<double, 5>>;
template class KerasModel<Opm::DenseAd::Evaluation<double, 4>>;
template class KerasModel<Opm::DenseAd::Evaluation<double, 3>>;
template class KerasModel<Opm::DenseAd::Evaluation<double, 2>>;
template class KerasModel<Opm::DenseAd::Evaluation<double, 1>>;

} // namespace Opm
