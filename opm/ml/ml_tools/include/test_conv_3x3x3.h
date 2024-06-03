
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
    in.data_ = {0.87271255,0.93225205,0.454738,0.7743667,0.07311473,0.43917352,
0.06562502,0.10534243,0.84919333,0.05485726,0.26453573,0.43184462,
0.31198752,0.3685557,0.33569786,0.5550908,0.30508468,0.5998162,
0.5240912,0.084725335,0.68967754,0.99210244,0.99117345,0.9305709,
0.9118958,0.84186167,0.566753};

    Opm::Tensor<Evaluation> out{1};
    out.data_ = {0.26097965};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/bikagit/opm-common/opm/ml/ml_tools/models/test_conv_3x3x3.model"), "Failed to load model");

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
