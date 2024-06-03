
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
    in.data_ = {0.8998526,0.42643917,0.2649691,0.101825155,0.4860865,0.35637248,
0.7880747,0.5331231,0.90746176,0.3913051};

    Opm::Tensor<Evaluation> out{10};
    out.data_ = {0.097687565,0.30221617,-0.24571078,-0.051162135,-0.24975275,
-0.3039312,-0.14934357,-0.62535065,-0.34585848,-1.3160709};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/bikagit/opm-common/opm/ml/ml_tools/models/test_dense_10x10x10.model"), "Failed to load model");

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
