
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
    in.data_ = {0.37697044,0.6427236,0.89172053,0.73344576,0.11428697,0.40963235,
0.81641096,0.6176922,0.6046866,0.9238176};

    Opm::Tensor<Evaluation> out{10};
    out.data_ = {0.44721743,0.,0.61811334,0.5108469,0.73356915,0.,
0.,0.,0.5264998,0.026750417};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/hackatonwork/opm-common/opm/ml/ml_tools/models/test_relu_10.model"), "Failed to load model");

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
