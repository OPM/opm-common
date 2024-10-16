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

#include <opm/ml/ml_model.hpp>

#include <cmath>
#include <fstream>
#include <limits>
#include <cstdio>
#include <utility>

#include <opm/common/ErrorMacros.hpp>
#include <fmt/format.h>

namespace Opm {


template <typename T>
bool readFile(std::ifstream& file, T& data, size_t n=1)
{
    file.read(reinterpret_cast<char*>(&data), sizeof (T)* n);
    return !file.fail();
}

template<class Evaluation>
bool NNLayerActivation<Evaluation>::loadLayer(std::ifstream& file) {

    unsigned int activation = 0;
    OPM_ERROR_IF(!readFile<unsigned int>(file, activation),
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
        OPM_ERROR_IF(true,fmt::format("\n Unsupported activation type " "{}", activation));

    }

    return true;
}

template<class Evaluation>
bool NNLayerActivation<Evaluation>::apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out) {
    constexpr double sigmoid_scale = 0.2;
    out = in;

    switch (activation_type_) {
    case kLinear:
        break;
    case kRelu:
        for (size_t i = 0; i < out.data_.size(); i++) {
            if (out.data_[i] < 0.0) {
                out.data_[i] = 0.0;
            }
        }
        break;
    case kSoftPlus:
        for (size_t i = 0; i < out.data_.size(); i++) {
            out.data_[i] = log(1.0 + exp(out.data_[i]));
        }
        break;
    case kHardSigmoid:
        for (size_t i = 0; i < out.data_.size(); i++) {
            Evaluation x = (out.data_[i] * sigmoid_scale) + 0.5;

            if (x <= 0) {
                out.data_[i] = 0.0;
            } else if (x >= 1) {
                out.data_[i] = 1.0;
            } else {
                out.data_[i] = x;
            }
        }
        break;
    case kSigmoid:
        for (size_t i = 0; i < out.data_.size(); i++) {
            Evaluation& x = out.data_[i];

            if (x >= 0) {
                out.data_[i] = 1.0 / (1.0 + exp(-x));
            } else {
                Evaluation z = exp(x);
                out.data_[i] = z / (1.0 + z);
            }
        }
        break;
    case kTanh:
        for (size_t i = 0; i < out.data_.size(); i++) {
            out.data_[i] = sinh(out.data_[i])/cosh(out.data_[i]);
        }
        break;
    default:
        break;
    }

    return true;
}


template<class Evaluation>
bool NNLayerScaling<Evaluation>::loadLayer(std::ifstream& file) {
    OPM_ERROR_IF(!readFile<float>(file, data_min), "Failed to read min");
    OPM_ERROR_IF(!readFile<float>(file, data_max), "Failed to read min");
    OPM_ERROR_IF(!readFile<float>(file, feat_inf), "Failed to read min");
    OPM_ERROR_IF(!readFile<float>(file, feat_sup), "Failed to read min");
    return true;
}

template<class Evaluation>
bool NNLayerScaling<Evaluation>::apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out) {

    for (size_t i = 0; i < out.data_.size(); i++) {
        auto tempscale = (in.data_[i] - data_min)/(data_max - data_min);
        out.data_[i] = tempscale * (feat_sup - feat_inf) + feat_inf;
    }

    return true;
}

template<class Evaluation>
bool NNLayerUnScaling<Evaluation>::loadLayer(std::ifstream& file) {
    OPM_ERROR_IF(!readFile<float>(file, data_min), "Failed to read min");
    OPM_ERROR_IF(!readFile<float>(file, data_max), "Failed to read max");
    OPM_ERROR_IF(!readFile<float>(file, feat_inf), "Failed to read inf");
    OPM_ERROR_IF(!readFile<float>(file, feat_sup), "Failed to read sup");

    return true;
}

template<class Evaluation>
bool NNLayerUnScaling<Evaluation>::apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out) {

    for (size_t i = 0; i < out.data_.size(); i++) {
        auto tempscale = (in.data_[i] - feat_inf)/(feat_sup - feat_inf);

        out.data_[i] = tempscale * (data_max - data_min) + data_min;
    }

    return true;
}


template<class Evaluation>
bool NNLayerDense<Evaluation>::loadLayer(std::ifstream& file) {
    unsigned int weights_rows = 0;
    OPM_ERROR_IF(!readFile<unsigned int>(file, weights_rows), "Expected weight rows");
    OPM_ERROR_IF(!(weights_rows > 0), "Invalid weights # rows");

    unsigned int weights_cols = 0;
    OPM_ERROR_IF(!readFile<unsigned int>(file, weights_cols), "Expected weight cols");
    OPM_ERROR_IF(!(weights_cols > 0), "Invalid weights shape");

    unsigned int biases_shape = 0;
    OPM_ERROR_IF(!readFile<unsigned int>(file, biases_shape), "Expected biases shape");
    OPM_ERROR_IF(!(biases_shape > 0), "Invalid biases shape");

    weights_.resizeI<unsigned int>({weights_rows, weights_cols});
    OPM_ERROR_IF(!readFile<float>(file, *weights_.data_.data(), weights_rows * weights_cols),
        "Expected weights");

    biases_.resizeI<unsigned int>({biases_shape});
    OPM_ERROR_IF(!readFile<float>(file, *biases_.data_.data(), biases_shape),
            "Expected biases");

    OPM_ERROR_IF(!activation_.loadLayer(file), "Failed to load activation");

    return true;
}

template<class Evaluation>
bool NNLayerDense<Evaluation>::apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out) {

   Tensor<Evaluation> tmp(weights_.dims_[1]);
   Tensor<Evaluation> temp_in(in);
    for (int i = 0; i < weights_.dims_[0]; i++) {
        for (int j = 0; j < weights_.dims_[1]; j++) {
            tmp(j) += (temp_in)(i)*weights_(i, j);
        }
    }

    for (int i = 0; i < biases_.dims_[0]; i++) {
        tmp(i) += biases_(i);
    }

    OPM_ERROR_IF(!activation_.apply(tmp, out), "Failed to apply activation");

    return true;
}



template<class Evaluation>
bool NNLayerEmbedding<Evaluation>::loadLayer(std::ifstream& file) {

    unsigned int weights_rows = 0;
    OPM_ERROR_IF(!readFile<unsigned int>(file, weights_rows), "Expected weight rows");
    OPM_ERROR_IF(!(weights_rows > 0), "Invalid weights # rows");

    unsigned int weights_cols = 0;
    OPM_ERROR_IF(!readFile<unsigned int>(file, weights_cols), "Expected weight cols");
    OPM_ERROR_IF(!(weights_cols > 0), "Invalid weights shape");

    weights_.resizeI<unsigned int>({weights_rows, weights_cols});
    OPM_ERROR_IF(!readFile<float>(file, *weights_.data_.data(), weights_rows * weights_cols),
        "Expected weights");

    return true;
}

template<class Evaluation>
bool NNLayerEmbedding<Evaluation>::apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out) {
    int output_rows = in.dims_[1];
    int output_cols = weights_.dims_[1];
    out.dims_ = {output_rows, output_cols};
    out.data_.reserve(output_rows * output_cols);

    std::for_each(in.data_.begin(), in.data_.end(), [=](Evaluation i) {
        typename std::vector<Evaluation>::const_iterator first =
            this->weights_.data_.begin() + (getValue(i) * output_cols);
        typename std::vector<Evaluation>::const_iterator last =
            this->weights_.data_.begin() + (getValue(i) + 1) * output_cols;

        out.data_.insert(out.data_.end(), first, last);
    });

    return true;
}


template<class Evaluation>
bool NNModel<Evaluation>::loadModel(const std::string& filename) {
    std::ifstream file(filename.c_str(), std::ios::binary);
    OPM_ERROR_IF(!file.is_open(),fmt::format("\n Unable to open file " "{}", filename.c_str()));

    unsigned int num_layers = 0;
    OPM_ERROR_IF(!readFile<unsigned int>(file, num_layers), "Expected number of layers");

    for (unsigned int i = 0; i < num_layers; i++) {
        unsigned int layer_type = 0;
        OPM_ERROR_IF(!readFile<unsigned int>(file, layer_type), "Expected layer type");

        std::unique_ptr<NNLayer<Evaluation>> layer = nullptr;

        switch (layer_type) {
        case kScaling:
            layer = std::make_unique<NNLayerScaling<Evaluation>>() ;
            break;
        case kUnScaling:
            layer = std::make_unique<NNLayerUnScaling<Evaluation>>() ;
            break;
        case kDense:
            layer = std::make_unique<NNLayerDense<Evaluation>>() ;
            break;
        case kActivation:
            layer = std::make_unique<NNLayerActivation<Evaluation>>() ;
            break;
        default:
            break;
        }

        OPM_ERROR_IF(!layer,fmt::format("\n Unknown layer type " "{}", layer_type));
        OPM_ERROR_IF(!layer->loadLayer(file),fmt::format("\n Failed to load layer " "{}", i));

        layers_.emplace_back(std::move(layer));

    }

    return true;
}


template<class Evaluation>
bool NNModel<Evaluation>::apply(const  Tensor<Evaluation>& in, Tensor<Evaluation>& out) {
    Tensor<Evaluation> temp_in(in);

    for (unsigned int i = 0; i < layers_.size(); i++) {

            OPM_ERROR_IF(!(layers_[i]->apply(temp_in, out)),fmt::format("\n Failed to apply layer " "{}", i));
            temp_in = out;
}
    return true;

}


template class NNModel<float>;

template class NNModel<double>;
template class NNModel<Opm::DenseAd::Evaluation<double, 6>>;
template class NNModel<Opm::DenseAd::Evaluation<double, 5>>;
template class NNModel<Opm::DenseAd::Evaluation<double, 4>>;
template class NNModel<Opm::DenseAd::Evaluation<double, 3>>;
template class NNModel<Opm::DenseAd::Evaluation<double, 2>>;
template class NNModel<Opm::DenseAd::Evaluation<double, 1>>;

} // namespace Opm
