
#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

using namespace Opm;
template<class Evaluation>
bool test_scalingdense_1x1(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST scalingdense_1x1\n");

    KASSERT(load_time, "Invalid Evaluation");
    KASSERT(apply_time, "Invalid Evaluation");

    Opm::Tensor<Evaluation> in{10};
    in.data_ = {0.35745713,0.5183338,0.43530357,0.7207405,0.96301425,0.47163334,
0.10632531,0.49405062,0.48787624,0.23182626};

    Opm::Tensor<Evaluation> out{10};
    out.data_ = {80.98062,133.76828,113.56729,180.86313,126.80325,148.15707,
111.01114,41.78473,51.552124,153.05319};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/hackatonwork/opm-common/opm/ml/ml_tools/models/test_scalingdense_1x1.model"), "Failed to load model");

    *load_time = load_timer.Stop();

    KerasTimer apply_timer;
    apply_timer.Start();

    Opm::Tensor<Evaluation> predict = out;
    KASSERT(model.Apply(&in, &out), "Failed to apply");

    *apply_time = apply_timer.Stop();

    for (int i = 0; i < out.dims_[0]; i++)
    {
        KASSERT_EQ(out(i), predict(i), 1e-3);
    }

    return true;
}
