
#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

using namespace Opm;
template<class Evaluation>
bool test_conv_3x3(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST conv_3x3\n");

    KASSERT(load_time, "Invalid Evaluation");
    KASSERT(apply_time, "Invalid Evaluation");

    Opm::Tensor<Evaluation> in{3,3,1};
    in.data_ = {0.8302591,0.7126048,0.049354695,0.065839484,0.022077054,0.8151628,
0.8770473,0.93242997,0.5808531};

    Opm::Tensor<Evaluation> out{1};
    out.data_ = {0.1281208};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/bikagit/opm-common/opm/ml/ml_tools/models/test_conv_3x3.model"), "Failed to load model");

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
