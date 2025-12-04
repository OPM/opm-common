/*
  Copyright (c) 2025 OPM-OP

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
*/

#define BOOST_TEST_MODULE MLLayerTest
#include <boost/test/unit_test.hpp>

#include <opm/ml/ml_model.hpp>

using namespace Opm::ML;

template <typename T>
static void check_vector_close(const Tensor<T>& t, const std::vector<double>& expected, double rel_tol = 1e-6)
{
    BOOST_REQUIRE_EQUAL((int)t.dims_.size(), 1);
    BOOST_REQUIRE_EQUAL(t.dims_[0], (int)expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        double g = t(i);
        double e = expected[i];
        BOOST_REQUIRE_CLOSE(g, e, rel_tol);
    }
}

BOOST_AUTO_TEST_SUITE(NNLayerActivationSuite)

BOOST_AUTO_TEST_CASE(test_linear_activation)
{
    NNLayerActivation<double> layer(ActivationType::kLinear);

    Tensor<double> in(3), out(3);
    in.data_ = {-1.0, 0.0, 2.5};
    out.fill(0.0);

    BOOST_CHECK(layer.apply(in, out));
    check_vector_close(out, {-1.0, 0.0, 2.5});
}

BOOST_AUTO_TEST_CASE(test_relu_activation)
{
    NNLayerActivation<double> layer(ActivationType::kRelu);

    Tensor<double> in(3), out(3);
    in.data_ = {-1.0, 0.0, 2.5};
    out.fill(0.0);

    BOOST_CHECK(layer.apply(in, out));
    check_vector_close(out, {0.0, 0.0, 2.5});
}

BOOST_AUTO_TEST_CASE(test_softplus_activation)
{
    NNLayerActivation<double> layer(ActivationType::kSoftPlus);

    Tensor<double> in(3), out(3);
    in.data_ = {-1.0, 0.0, 1.0};
    out.fill(0.0);

    BOOST_CHECK(layer.apply(in, out));
    std::vector<double> expected = {
        std::log(1.0 + std::exp(-1.0)),
        std::log(1.0 + std::exp(0.0)),
        std::log(1.0 + std::exp(1.0))
    };
    check_vector_close(out, expected);
}

BOOST_AUTO_TEST_CASE(test_sigmoid_activation)
{
    NNLayerActivation<double> layer(ActivationType::kSigmoid);

    Tensor<double> in(3), out(3);
    in.data_ = {-1.0, 0.0, 1.0};
    out.fill(0.0);

    BOOST_CHECK(layer.apply(in, out));
    std::vector<double> expected = {
        1.0 / (1.0 + std::exp(1.0)),
        0.5,
        1.0 / (1.0 + std::exp(-1.0))
    };
    check_vector_close(out, expected);
}

BOOST_AUTO_TEST_CASE(test_tanh_activation)
{
    NNLayerActivation<double> layer(ActivationType::kTanh);

    Tensor<double> in(3), out(3);
    in.data_ = {-1.0, 0.0, 1.0};
    out.fill(0.0);

    BOOST_CHECK(layer.apply(in, out));

    check_vector_close(out, std::vector{std::tanh(-1.0), 0.0, std::tanh(1.0)});
}

BOOST_AUTO_TEST_CASE(test_hard_sigmoid_activation)
{
    // Keras hard sigmoid is typically: clip(0.2*x + 0.5, 0, 1)
    NNLayerActivation<double> layer(ActivationType::kHardSigmoid);

    Tensor<double> in(3), out(3);
    in.data_ = {-3.0, 0.0, 3.0};
    out.fill(0.0);

    BOOST_CHECK(layer.apply(in, out));
    check_vector_close(out, std::vector<double>{0.0, 0.5, 1.0}, 1e-7);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(NNLayerScalingApply)
{
    using Opm::ML::Tensor;
    using Opm::ML::NNLayerScaling;

    NNLayerScaling<float> layer(0.f, 2.f, 1.f, 2.f);

    Tensor<float> in(3);
    in(0) = -1.0f; in(1) = 0.0f; in(2) = 1.0f;

    Tensor<float> out;
    BOOST_REQUIRE(layer.apply(in, out));
    check_vector_close(out, std::vector{0.5, 1.0, 1.5});
}

BOOST_AUTO_TEST_CASE(NNLayerUnScalingApplyInverse)
{
    using Opm::ML::Tensor;
    using Opm::ML::NNLayerScaling;
    using Opm::ML::NNLayerUnScaling;

    NNLayerScaling<float> scaler(0.f, 2.f, 1.f, 2.f);
    NNLayerUnScaling<float> unscaler(0.f, 2.f, 1.f, 2.f);

    Tensor<float> orig(3);
    orig(0) = -1.0f; orig(1) = 0.0f; orig(2) = 1.0f;

    Tensor<float> scaled;
    BOOST_REQUIRE(scaler.apply(orig, scaled));
    check_vector_close(scaled, std::vector{0.5, 1.0, 1.5});

    Tensor<float> recovered;
    BOOST_REQUIRE(unscaler.apply(scaled, recovered));

    check_vector_close(recovered, std::vector<double>{orig(0), orig(1), orig(2)});
}

BOOST_AUTO_TEST_CASE(NNLayerDenseApply)
{
    using Opm::ML::Tensor;
    using Opm::ML::NNLayerDense;

    // Dense: 2 outputs, 3 inputs
    Tensor<float> W(3,2);
    W(0,0) = 1.0; W(0,1) = 4.0;
    W(1,0) = 2.0; W(1,1) = 5.0;
    W(2,0) = 3.0; W(2,1) = 6.0;

    Tensor<float> b(2);
    b(0) = 0.5; b(1) = -1.0;

    NNLayerDense<double> layer(W, b);

    Tensor<double> in(3);
    in(0) =  1.0;
    in(1) =  0.0;
    in(2) = -1.0;

    Tensor<double> out;
    BOOST_REQUIRE(layer.apply(in, out));

    std::vector<double> expected = {
        1.0*1.0 + 2.0*0.0 + 3.0*(-1.0) + 0.5,   // = -1.5
        4.0*1.0 + 5.0*0.0 + 6.0*(-1.0) - 1.0    // = -3.0
    };
    check_vector_close(out, expected); // wrong?
}

BOOST_AUTO_TEST_CASE(NNLayerEmbeddingApply)
{
    using Opm::ML::Tensor;
    using Opm::ML::NNLayerEmbedding;

    // Embedding matrix: 4 tokens, embedding dim 3
    Tensor<float> embedW(4, 3);
    embedW(0, 0) = 0.1f; embedW(0, 1) = 0.2f; embedW(0, 2) = 0.3f;
    embedW(1, 0) = 1.0f; embedW(1, 1) = 1.1f; embedW(1, 2) = 1.2f;
    embedW(2, 0) = 2.0f; embedW(2, 1) = 2.1f; embedW(2, 2) = 2.2f;
    embedW(3, 0) = 3.0f; embedW(3, 1) = 3.1f; embedW(3, 2) = 3.2f;

    NNLayerEmbedding<float> embedLayer(embedW);


    // ?
}
