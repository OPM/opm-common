
#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

using namespace Opm;
template<class Evaluation>
bool test_dense_10x1(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST dense_10x1\n");

    KASSERT(load_time, "Invalid Evaluation");
    KASSERT(apply_time, "Invalid Evaluation");

    Opm::Tensor<Evaluation> in{10};
    in.data_ = {0.6118447,0.770052,0.16679357,0.8346833,0.7955753,0.25932008,
0.3465901,0.13505514,0.35986346,0.716514};

    Opm::Tensor<Evaluation> out{1};
    out.data_ = {0.91411453};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/hackatonwork/opm-common/opm/ml/ml_tools/models/test_dense_10x1.model"), "Failed to load model");

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
