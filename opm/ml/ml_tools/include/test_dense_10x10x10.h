
#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

using namespace Opm;
template<class Evaluation>
bool test_dense_10x10x10(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST dense_10x10x10\n");

    KASSERT(load_time, "Invalid Evaluation");
    KASSERT(apply_time, "Invalid Evaluation");

    Opm::Tensor<Evaluation> in{10};
    in.data_ = {0.7104989,0.4149288,0.71831465,0.5625794,0.7118615,0.8625888,
0.35311338,0.70555335,0.19741295,0.6377552};

    Opm::Tensor<Evaluation> out{10};
    out.data_ = {-0.047795724,0.55596924,-0.08081586,-0.5206654,0.17288932,
-0.5772272,0.45383403,-0.38085136,-0.5176138,0.2733392};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/hackatonwork/opm-common/opm/ml/ml_tools/models/test_dense_10x10x10.model"), "Failed to load model");

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
