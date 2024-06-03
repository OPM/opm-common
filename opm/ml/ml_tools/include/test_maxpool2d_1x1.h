
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
    in.data_ = {0.25518134,0.04741953,0.30579203,0.155573,0.49132094,0.07164798,
0.6618191,0.95539516,0.60500616,0.27978984,0.5592694,0.10625745,
0.35216096,0.15352608,0.38916105,0.47961122,0.13570014,0.5139465,
0.60992926,0.48152757,0.45213497,0.74926835,0.893273,0.49284345,
0.13199767,0.40807652,0.32890376,0.92184955,0.8276756,0.4514926,
0.8575906,0.22574344,0.5647829,0.6564582,0.83293825,0.81201965,
0.16786863,0.64626974,0.73370457,0.75692856,0.056844383,0.48827544,
0.4257299,0.6403209,0.556078,0.5105229,0.3147873,0.72197866,
0.25018466,0.25356695,0.6093809,0.5989881,0.2952787,0.7309936,
0.91963696,0.03639153,0.67082775,0.74602616,0.007854396,0.35555983,
0.85999966,0.8637354,0.5419423,0.51870966,0.15975554,0.31114286,
0.2895706,0.6567837,0.49693534,0.8106974,0.3142659,0.004674668,
0.7890345,0.6929022,0.43730104,0.76309645,0.48623887,0.8451051,
0.2559583,0.8858542,0.43520355,0.5545106,0.78928936,0.19655785,
0.9229729,0.6828728,0.77383405,0.98197556,0.8564043,0.25713357,
0.46030727,0.71827936,0.34024438,0.18788093,0.8874286,0.64397246,
0.04353716,0.7736121,0.429076,0.20847954};

    Opm::Tensor<Evaluation> out{1};
    out.data_ = {-0.25252247};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/bikagit/opm-common/opm/ml/ml_tools/models/test_maxpool2d_1x1.model"), "Failed to load model");

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
