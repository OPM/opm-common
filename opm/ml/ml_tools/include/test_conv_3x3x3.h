
#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

using namespace Opm;
template<class Evaluation>
bool test_conv_3x3x3(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST conv_3x3x3\n");

    KASSERT(load_time, "Invalid Evaluation");
    KASSERT(apply_time, "Invalid Evaluation");

    Opm::Tensor<Evaluation> in{3,3,3};
    in.data_ = {0.99366415,0.8214218,0.78640515,0.2603204,0.16119346,0.33790612,
0.003531503,0.8499332,0.45968544,0.4627597,0.13754487,0.10913391,
0.14221385,0.6451374,0.84302205,0.4868773,0.083437696,0.27630988,
0.22067706,0.11169723,0.3520237,0.58716524,0.05329963,0.26573667,
0.4124315,0.18437439,0.74106956};

    Opm::Tensor<Evaluation> out{1};
    out.data_ = {-0.32770187};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/hackatonwork/opm-common/opm/ml/ml_tools/models/test_conv_3x3x3.model"), "Failed to load model");

    *load_time = load_timer.Stop();

    KerasTimer apply_timer;
    apply_timer.Start();

    Opm::Tensor<Evaluation> predict = out;
    KASSERT(model.Apply(&in, &out), "Failed to apply");

    *apply_time = apply_timer.Stop();

    for (int i = 0; i < out.dims_[0]; i++)
    {
        KASSERT_EQ(out(i), predict(i), 1e-6);
    }

    return true;
}
