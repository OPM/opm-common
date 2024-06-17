
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
    in.data_ = {0.9307595,0.623219,0.26583958,0.58003414,0.030215342,0.06587499,
0.9004415,0.03221161,0.4206354,0.8563274};


    Opm::Tensor<Evaluation> out{10};
    out.data_ = {0.068457894,0.03991737,-0.30497518,-0.19265954,-0.32966894,
-0.6756333,0.4275365,1.121752,0.14004697,0.30182698};

    KerasTimer load_timer;
    load_timer.Start();
    std::cout<<"in->dims_.size() "<<in.dims_.size()<<std::endl;


    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/hackatonwork/opm-common/opm/ml/ml_tools/models/test_dense_10x10x10.model"), "Failed to load model");

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
