
#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

using namespace Opm;
template<class Evaluation>
bool test_dense_tanh_10(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST dense_tanh_10\n");

    KASSERT(load_time, "Invalid Evaluation");
    KASSERT(apply_time, "Invalid Evaluation");

    Opm::Tensor<Evaluation> in{10};
    in.data_ = {0.03345425,0.78990555,0.33485138,0.09934332,0.74528086,0.6406323,
0.78257537,0.17336933,0.06393018,0.94946647};

    Opm::Tensor<Evaluation> out{10};
    out.data_ = {-0.10979327,0.31542704,0.3152341,0.42501545,0.31062323,-0.18132454,
-0.763928,0.15322652,0.73196095,-0.19889684};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/bikagit/opm-common/opm/ml/ml_tools/models/test_dense_tanh_10.model"), "Failed to load model");

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
