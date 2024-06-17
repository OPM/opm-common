
#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

using namespace Opm;
template<class Evaluation>
bool test_maxpool2d_1x1(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST maxpool2d_1x1\n");

    KASSERT(load_time, "Invalid Evaluation");
    KASSERT(apply_time, "Invalid Evaluation");

    Opm::Tensor<Evaluation> in{10,10,1};
    in.data_ = {0.3584981,0.2958925,0.7605831,0.53394246,0.61106086,
0.0044219894,0.5463487,0.25235063,0.18157023,0.06756325,
0.8124267,0.8730054,0.0947123,0.80920976,0.8023841,
0.18305726,0.1803083,0.044107396,0.8450164,0.5707187,
0.627845,0.49776414,0.7567636,0.22644873,0.17683746,
0.09286818,0.16009343,0.5313456,0.95800453,0.32112077,
0.91887784,0.57785136,0.38100433,0.30305266,0.4294771,
0.74898136,0.24068417,0.24434112,0.4277804,0.66749835,
0.2807149,0.5275129,0.64340687,0.2915763,0.032093775,
0.7258795,0.5505952,0.68428564,0.41406462,0.91326606,
0.22276446,0.8068874,0.80964404,0.26798266,0.96306396,
0.18655908,0.91054565,0.005801419,0.9993052,0.51641726,
0.8414052,0.45861587,0.9145516,0.51948214,0.88117427,
0.22991507,0.053036828,0.6385797,0.68001044,0.739364,
0.56692076,0.36897555,0.39697468,0.5334803,0.19781172,
0.7582431,0.1215363,0.1281851,0.5813795,0.98001736,
0.84356993,0.70857924,0.6370623,0.54709303,0.9008569,
0.73727363,0.53235346,0.7178515,0.8857483,0.80863845,
0.26460695,0.19305061,0.3812545,0.018899608,0.7839583,
0.9368291,0.87295556,0.17056823,0.46442586,0.91270417};

    Opm::Tensor<Evaluation> out{1};
    out.data_ = {0.79461604};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/hackatonwork/opm-common/opm/ml/ml_tools/models/test_maxpool2d_1x1.model"), "Failed to load model");

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
