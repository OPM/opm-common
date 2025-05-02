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

  Note: This file is based on kerasify/keras_model_test.cc
*/

#include <opm/common/ErrorMacros.hpp>
#include <opm/ml/ml_model.hpp>

#include <tests/ml/ml_tools/include/test_dense_10x1.hpp>
#include <tests/ml/ml_tools/include/test_dense_10x10.hpp>
#include <tests/ml/ml_tools/include/test_dense_10x10x10.hpp>
#include <tests/ml/ml_tools/include/test_dense_1x1.hpp>
#include <tests/ml/ml_tools/include/test_dense_2x2.hpp>
#include <tests/ml/ml_tools/include/test_dense_relu_10.hpp>
#include <tests/ml/ml_tools/include/test_dense_tanh_10.hpp>
#include <tests/ml/ml_tools/include/test_relu_10.hpp>
#include <tests/ml/ml_tools/include/test_scalingdense_10x1.hpp>

#include <cstdio>

#include <fmt/format.h>

namespace Opm {

using namespace ML;

template <class Evaluation>
bool tensor_test()
{
    std::printf("TEST tensor_test\n");

    {
        const int i = 3;
        const int j = 5;
        const int k = 10;
        Tensor<Evaluation> t(i, j, k);

        Evaluation c = 1.f;
        for (int ii = 0; ii < i; ii++) {
            for (int jj = 0; jj < j; jj++) {
                for (int kk = 0; kk < k; kk++) {
                    t(ii, jj, kk) = c;
                    c += 1.f;
                }
            }
        }

        c = 1.f;
        int cc = 0;
        for (int ii = 0; ii < i; ii++) {
            for (int jj = 0; jj < j; jj++) {
                for (int kk = 0; kk < k; kk++) {
                    OPM_ERROR_IF(fabs(t(ii, jj, kk).value() - c.value()) > 1e-9,
                                 fmt::format("\n Expected "
                                             "{}"
                                             "got "
                                             "{}",
                                             c.value(),
                                             t(ii, jj, kk).value()));
                    OPM_ERROR_IF(fabs(t.data_[cc].value() - c.value()) > 1e-9,
                                 fmt::format("\n Expected "
                                             "{}"
                                             "got "
                                             "{}",
                                             c.value(),
                                             t.data_[cc].value()));

                    c += 1.f;
                    cc++;
                }
            }
        }
    }

    {
        const int i = 2;
        const int j = 3;
        const int k = 4;
        const int l = 5;
        Tensor<Evaluation> t(i, j, k, l);

        Evaluation c = 1.f;
        for (int ii = 0; ii < i; ii++) {
            for (int jj = 0; jj < j; jj++) {
                for (int kk = 0; kk < k; kk++) {
                    for (int ll = 0; ll < l; ll++) {
                        t(ii, jj, kk, ll) = c;
                        c += 1.f;
                    }
                }
            }
        }

        c = 1.f;
        int cc = 0;
        for (int ii = 0; ii < i; ii++) {
            for (int jj = 0; jj < j; jj++) {
                for (int kk = 0; kk < k; kk++) {
                    for (int ll = 0; ll < l; ll++) {
                        OPM_ERROR_IF(fabs(t(ii, jj, kk, ll).value() - c.value()) > 1e-9,
                                     fmt::format("\n Expected "
                                                 "{}"
                                                 "got "
                                                 "{}",
                                                 c.value(),
                                                 t(ii, jj, kk, ll).value()));
                        OPM_ERROR_IF(fabs(t.data_[cc].value() - c.value()) > 1e-9,
                                     fmt::format("\n Expected "
                                                 "{}"
                                                 "got "
                                                 "{}",
                                                 c.value(),
                                                 t.data_[cc].value()));
                        c += 1.f;
                        cc++;
                    }
                }
            }
        }
    }

    {
        Tensor<Evaluation> a(2, 2);
        Tensor<Evaluation> b(2, 2);

        a.data_ = {1.0, 2.0, 3.0, 5.0};
        b.data_ = {2.0, 5.0, 4.0, 1.0};

        Tensor<Evaluation> result = a + b;
        OPM_ERROR_IF(result.data_ != std::vector<Evaluation>({3.0, 7.0, 7.0, 6.0}),
                     "Vector add failed");
    }

    {
        Tensor<Evaluation> a(2, 2);
        Tensor<Evaluation> b(2, 2);

        a.data_ = {1.0, 2.0, 3.0, 5.0};
        b.data_ = {2.0, 5.0, 4.0, 1.0};

        Tensor<Evaluation> result = a.multiply(b);
        OPM_ERROR_IF(result.data_ != std::vector<Evaluation>({2.0, 10.0, 12.0, 5.0}),
                     "Vector multiply failed");
    }

    {
        Tensor<Evaluation> a(2, 1);
        Tensor<Evaluation> b(1, 2);

        a.data_ = {1.0, 2.0};
        b.data_ = {2.0, 5.0};

        Tensor<Evaluation> result = a.dot(b);
        OPM_ERROR_IF(result.data_ != std::vector<Evaluation>({2.0, 5.0, 4.0, 10.0}),
                     "Vector dot failed");
    }

    return true;
}

} // namespace Opm

int main()
{
    using Evaluation = Opm::DenseAd::Evaluation<double, 1>;

    Evaluation load_time = 0.0;
    Evaluation apply_time = 0.0;

    try {
        tensor_test<Evaluation>();
        test_dense_1x1<Evaluation>(&load_time, &apply_time);
        test_dense_10x1<Evaluation>(&load_time, &apply_time);
        test_dense_2x2<Evaluation>(&load_time, &apply_time);
        test_dense_10x10<Evaluation>(&load_time, &apply_time);
        test_dense_10x10x10<Evaluation>(&load_time, &apply_time);
        test_relu_10<Evaluation>(&load_time, &apply_time);
        test_dense_relu_10<Evaluation>(&load_time, &apply_time);
        test_dense_tanh_10<Evaluation>(&load_time, &apply_time);
        test_scalingdense_10x1<Evaluation>(&load_time, &apply_time);
    }
    catch(...) {
        return 1;
    }

    return 0;
}
