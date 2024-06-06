
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
    in.data_ = {0.26271367,0.57310253,0.4322339,0.018163363,0.42448965,0.6118086,
0.20635886,0.38553724,0.97148365,0.2353917};

    Opm::Tensor<Evaluation> out{10};
    out.data_ = {-0.5350144,-0.36773947,-0.015882624,0.020122323,0.11873825,
0.42103818,-0.27925694,0.060885787,0.5043703,0.029599192};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/hackatonwork/opm-common/opm/ml/ml_tools/models/test_dense_tanh_10.model"), "Failed to load model");

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
