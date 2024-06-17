
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
    in.data_ = {0.1899011,0.3590567,0.284928,0.8981879,0.8274132,0.8743853,
0.025491558,0.18069586,0.19910076,0.5135733};

    Opm::Tensor<Evaluation> out{10};
    out.data_ = {0.49924585,0.54542077,-0.60667866,-0.34529728,0.15064734,-0.26957473,
0.05453661,-0.5642658,0.4621192,-0.19287723};

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
