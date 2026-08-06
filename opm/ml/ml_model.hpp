/*
  Copyright (c) 2016 Robert W. Rose
  Copyright (c) 2018 Paul Maevskikh
  Copyright (c) 2024 NORCE

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  Note: This file is based on kerasify/keras_model.hh
*/

#ifndef ML_MODEL_H_
#define ML_MODEL_H_

#include <chrono>
#include <iosfwd>
#include <string>
#include <vector>

namespace Opm::ML
{
    // NN layer
    // ---------------------
    /** \class Tensor class
     * Implements mathematical tensor (Max 4d)
     */
    template <class T>
    class Tensor
    {
    public:
        Tensor() = default;

        explicit Tensor(int i)
        {
            resizeI(std::vector{i});
        }

        Tensor(int i, int j)
        {
            resizeI(std::vector{i, j});
        }

        Tensor(int i, int j, int k)
        {
            resizeI(std::vector{i, j, k});
        }

        Tensor(int i, int j, int k, int l)
        {
            resizeI(std::vector{i, j, k, l});
        }

        void resizeI(const std::vector<int>& sizes);

        void flatten();

        T& operator()(int i);
        const T& operator()(int i) const;
        T& operator()(int i, int j);
        const T& operator()(int i, int j) const;
        T& operator()(int i, int j, int k);
        const T& operator()(int i, int j, int k) const;
        T& operator()(int i, int j, int k, int l);
        const T& operator()(int i, int j, int k, int l) const;

        void fill(const T& value);

        // Tensor addition
        Tensor operator+(const Tensor& other);

        // Tensor multiplication
        Tensor multiply(const Tensor& other);

        // Tensor dot for 2d tensor
        Tensor dot(const Tensor& other);

        void swap(Tensor& other);

        std::vector<int> dims_;
        std::vector<T> data_;
    };

    // NN layer
    // ---------------------
    /** \class Neural Network  Layer base class.
     * Objects of type Evaluation can be AD object Opm::DenseAd::Evaluation,
    double or float types.
     */
    template <class Evaluation>
    class NNLayer
    {
    public:
        virtual ~NNLayer() = default;

        // Loads the ML trained file, returns true if the file exists
        virtual bool loadLayer(std::ifstream& file) = 0;
        // Apply the NN layers
        virtual bool apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out) = 0;
    };

    //! Activation types
    enum class ActivationType {
        kLinear = 1,
        kRelu = 2,
        kSoftPlus = 3,
        kSigmoid = 4,
        kTanh = 5,
        kHardSigmoid = 6
    };

    /** \class Activation  Layer class
     * Applies an activation function
     */
    template <class Evaluation>
    class NNLayerActivation : public NNLayer<Evaluation>
    {
    public:
        NNLayerActivation(ActivationType activation_type = ActivationType::kLinear)
            : activation_type_(activation_type)
        {
        }

        bool loadLayer(std::ifstream& file) override;

        bool apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out) override;

    private:
        ActivationType activation_type_;
    };

    /** \class Scaling Layer class
     * A preprocessing layer which rescales input values to a new range.
     */
    template <class Evaluation>
    class NNLayerScaling : public NNLayer<Evaluation>
    {
    public:
        NNLayerScaling(float data_min = 1.0f,
                       float data_max = 1.0f,
                       float feat_inf = 1.0f,
                       float feat_sup = 1.0f)
            : data_min_(data_min)
            , data_max_(data_max)
            , feat_inf_(feat_inf)
            , feat_sup_(feat_sup)
        {}

        bool loadLayer(std::ifstream& file) override;

        bool apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out) override;

    private:
        float data_min_;
        float data_max_;
        float feat_inf_;
        float feat_sup_;
    };

    /** \class Unscaling Layer class
     * A postprocessing layer to undo the scaling according to feature_range.
     */
    template <class Evaluation>
    class NNLayerUnScaling : public NNLayer<Evaluation>
    {
    public:
        NNLayerUnScaling(float data_min = 1.0f,
                         float data_max = 1.0f,
                         float feat_inf = 1.0f,
                         float feat_sup = 1.0f)
            : data_min_(data_min)
            , data_max_(data_max)
            , feat_inf_(feat_inf)
            , feat_sup_(feat_sup)
        {}

        bool loadLayer(std::ifstream& file) override;

        bool apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out) override;

    private:
        float data_min_;
        float data_max_;
        float feat_inf_;
        float feat_sup_;
    };

    /** \class Dense Layer class
     * Densely-connected NN layer.
     */
    template <class Evaluation>
    class NNLayerDense : public NNLayer<Evaluation>
    {
    public:
        NNLayerDense(Tensor<float> weights = {}, Tensor<float> biases = {}, ActivationType activation_type = ActivationType::kLinear)
            : weights_(weights)
            , biases_(biases)
            , activation_(activation_type)
        {}

        bool loadLayer(std::ifstream& file) override;

        /**
         * @brief Applies the forward pass of a dense (fully connected) neural-network layer.
         *
         * This method performs a matrix–vector multiplication between the layer's weight
         * matrix and the input tensor, adds the bias vector, and then applies the
         * configured activation function.
         *
         * ### Shape conventions
         * - `in` is treated as a 1D row vector of length `weights_.dims_[0]`.
         * - `weights_` has shape `(input_dim, output_dim)`:
         *      - rows = input features
         *      - columns = output neurons
         * - `biases_` is a vector of length `output_dim`.
         * - `out` is a 1D vector of length `output_dim`.
         *
         * This implements:
         * @f[
         *     \text{tmp}_j = \sum_i \text{in}_i \cdot W_{i,j} + b_j,
         *     \qquad \text{out} = \text{activation}(\text{tmp})
         * @f]
         *
         * ### Note on row-major vs column-major
         * The current implementation assumes row-major access to `W` and is efficient
         * when using larger batch sizes. For inference with very small batches
         * (especially `(1 × input_dim)`), a column-major layout or transposed multiply
         * could improve cache locality because each output neuron would read contiguous
         * memory. Whether to switch depends on expected inference batch sizes and the
         * storage layout of `Tensor<Evaluation>`. This will depend on future applications
         * of ML.
         * Current applications and best related convention:
         * - Hybrid Newton:
         *      - input (1, N_cells x N_in_feat)  --> output(1, N_cells x N_out_feat)
         *      - Best convention: column-major
         */
        bool apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out) override;

    private:
        Tensor<float> weights_;
        Tensor<float> biases_;

        NNLayerActivation<Evaluation> activation_;
    };

    /** \class Neural Network Model class
     * A model grouping layers into an object
     */
    template <class Evaluation>
    class NNModel
    {
    public:
        enum class LayerType { kScaling = 1, kUnScaling = 2, kDense = 3, kActivation = 4 };

        virtual ~NNModel() = default;

        // loads models (.model files) generated by Kerasify
        virtual bool loadModel(const std::string& filename);

        virtual bool apply(const Tensor<Evaluation>& in, Tensor<Evaluation>& out);

    private:
        std::vector<std::unique_ptr<NNLayer<Evaluation>>> layers_;
    };

    /** \class Neural Network Timer class
     */
    class NNTimer
    {
    public:
        //! Start the timer
        void start();
        //! Stop the timer and return elapsed time in milliseconds
        float stop();

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> start_;
    };
} // namespace Opm::ML

#endif // ML_MODEL_H_
