
#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

using namespace Opm;
template<class Evaluation>
bool test_relu_10(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST relu_10\n");

    KASSERT(load_time, "Invalid Evaluation");
    KASSERT(apply_time, "Invalid Evaluation");

    Opm::Tensor<Evaluation> in{10};
    in.data_ = {0.75668657,0.43547645,0.2684515,0.40864667,0.7540064,0.8699675,
0.9615154,0.21117006,0.2840515,0.6931818};

    Opm::Tensor<Evaluation> out{10};
    out.data_ = {0.41057152,0.,0.,0.3421311,1.0349326,1.9920036,
0.,0.,0.,0.};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/bikagit/opm-common/opm/ml/ml_tools/models/test_relu_10.model"), "Failed to load model");

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
