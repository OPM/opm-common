
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
    in.data_ = {0.32681695,0.88247293,0.9610101,0.7766642,0.90783143,0.95788574,
0.25465804,0.14405063,0.30747658,0.29391462,0.88617074,0.7574131,
0.7840184,0.13056566,0.17718191,0.51664996,0.68511343,0.2752934,
0.3730155,0.7273659,0.070773244,0.6111477,0.55316126,0.14030892,
0.9425838,0.78347766,0.7334307};

    Opm::Tensor<Evaluation> out{1};
    out.data_ = {0.34363604};

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
