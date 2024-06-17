
#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

using namespace Opm;
template<class Evaluation>
bool test_conv_hard_sigmoid_2x2(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST conv_hard_sigmoid_2x2\n");

    KASSERT(load_time, "Invalid Evaluation");
    KASSERT(apply_time, "Invalid Evaluation");

    Opm::Tensor<Evaluation> in{2,2,1};
    in.data_ = {0.86426544,0.8477615,0.5583865,0.5447023};

    Opm::Tensor<Evaluation> out{1};
    out.data_ = {-0.8503885};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/hackatonwork/opm-common/opm/ml/ml_tools/models/test_conv_hard_sigmoid_2x2.model"), "Failed to load model");

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
