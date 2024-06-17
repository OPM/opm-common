#include "keras_model.hpp"

#include <iostream>
#include <stdio.h>

#include "ml_tools/include/test_conv_2x2.h"
#include "ml_tools/include/test_conv_3x3.h"
#include "ml_tools/include/test_conv_3x3x3.h"
#include "ml_tools/include/test_conv_hard_sigmoid_2x2.h"
#include "ml_tools/include/test_conv_sigmoid_2x2.h"
#include "ml_tools/include/test_conv_softplus_2x2.h"
#include "ml_tools/include/test_dense_10x1.h"
#include "ml_tools/include/test_dense_10x10.h"
#include "ml_tools/include/test_dense_10x10x10.h"
#include "ml_tools/include/test_dense_1x1.h"
#include "ml_tools/include/test_dense_2x2.h"
#include "ml_tools/include/test_dense_relu_10.h"
#include "ml_tools/include/test_dense_tanh_10.h"
#include "ml_tools/include/test_elu_10.h"
#include "ml_tools/include/test_relu_10.h"


#include "ml_tools/include/test_scalingdense_1x1.h"


namespace Opm {

template<class Evaluation>
bool tensor_test() {
  printf("TEST tensor_test\n");

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
                    KASSERT_EQ(t(ii, jj, kk), c, 1e-9);
                    KASSERT_EQ(t.data_[cc], c, 1e-9);
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
                        KASSERT_EQ(t(ii, jj, kk, ll), c, 1e-9);
                        KASSERT_EQ(t.data_[cc], c, 1e-9);
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
        KASSERT(result.data_ == std::vector<Evaluation>({3.0, 7.0, 7.0, 6.0}),
                "Vector add failed");
    }

    {
        Tensor<Evaluation> a(2, 2);
        Tensor<Evaluation> b(2, 2);

        a.data_ = {1.0, 2.0, 3.0, 5.0};
        b.data_ = {2.0, 5.0, 4.0, 1.0};

        Tensor<Evaluation> result = a.Multiply(b);
        KASSERT(result.data_ == std::vector<Evaluation>({2.0, 10.0, 12.0, 5.0}),
                "Vector multiply failed");
    }

    {
        Tensor<Evaluation> a(2, 1);
        Tensor<Evaluation> b(1, 2);

        a.data_ = {1.0, 2.0};
        b.data_ = {2.0, 5.0};

        Tensor<Evaluation> result = a.Dot(b);
        KASSERT(result.data_ == std::vector<Evaluation>({2.0, 5.0, 4.0, 10.0}),
                "Vector dot failed");
    }

    return true;
}


}

int main() {
    typedef Opm::DenseAd::Evaluation<double, 1> Evaluation;
    // typedef float Evaluation;
 
    Evaluation load_time = 0.0;
    Evaluation apply_time = 0.0;

    if (!tensor_test<Evaluation>()) {
        return 1;
    }
    
    if (!test_dense_1x1<Evaluation>(&load_time, &apply_time)) {
        return 1;
    }
    
    if (!test_dense_10x1<Evaluation>(&load_time, &apply_time)) {
        return 1;
    }
    
    if (!test_dense_2x2<Evaluation>(&load_time, &apply_time)) {
        return 1;
    }
    
    if (!test_dense_10x10<Evaluation>(&load_time, &apply_time)) {
        return 1;
    }
    
    if (!test_dense_10x10x10<Evaluation>(&load_time, &apply_time)) {
        return 1;
    }
    
    // if (!test_conv_2x2<Evaluation>(&load_time, &apply_time)) {
    //     return 1;
    // }
    // //
    // if (!test_conv_3x3<Evaluation>(&load_time, &apply_time)) {
    //     return 1;
    // }
    
    // if (!test_conv_3x3x3<Evaluation>(&load_time, &apply_time)) {
    //     return 1;
    // }
    // //
    // if (!test_elu_10<Evaluation>(&load_time, &apply_time)) {
    //     return 1;
    // }
    // //
    // if (!test_relu_10<Evaluation>(&load_time, &apply_time)) {
    //     return 1;
    // }
    
    // if (!test_dense_relu_10<Evaluation>(&load_time, &apply_time)) {
    //     return 1;
    // }
    //
    // if (!test_dense_tanh_10<Evaluation>(&load_time, &apply_time)) {
    //     return 1;
    // }
    //
    // if (!test_conv_softplus_2x2<Evaluation>(&load_time, &apply_time)) {
    //     return 1;
    // }
    //
    // if (!test_conv_hard_sigmoid_2x2<Evaluation>(&load_time, &apply_time)) {
    //     return 1;
    // }
    //
    // if (!test_conv_sigmoid_2x2<Evaluation>(&load_time, &apply_time)) {
    //     return 1;
    // }

    if (!test_scalingdense_1x1<Evaluation>(&load_time, &apply_time)) {
        return 1;
    }

    // // Run benchmark 5 times and report duration.
    // Evaluation total_load_time = 0.0;
    // Evaluation total_apply_time = 0.0;

    // for (int i = 0; i < 5; i++) {
    //     total_load_time += load_time;
    //     total_apply_time += apply_time;
    // }

    // printf("Benchmark network loads in %fs\n", total_load_time / 5);
    // printf("Benchmark network runs in %fs\n", total_apply_time / 5);

    return 0;
}
// }
