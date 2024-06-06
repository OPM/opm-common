
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
    in.data_ = {8.05891514e-01,4.24958289e-01,9.78650153e-01,8.33032012e-01,
6.98836505e-01,5.28312251e-02,9.86697435e-01,1.75924480e-01,
5.77136397e-01,8.90503943e-01,3.50836873e-01,8.51357281e-02,
3.52928936e-01,3.79992664e-01,4.45006251e-01,9.93837059e-01,
5.55398285e-01,8.62534046e-01,1.73030049e-01,4.98871028e-01,
5.99020064e-01,2.93842643e-01,4.75706041e-01,3.08726907e-01,
6.87034428e-01,9.52807069e-01,3.84712100e-01,5.96658170e-01,
3.96235764e-01,1.40618477e-02,1.94596589e-01,6.25770569e-01,
5.62299132e-01,5.34520566e-01,1.95316955e-01,9.29202616e-01,
8.59472275e-01,2.36784384e-01,1.90333515e-01,5.04570246e-01,
4.75366235e-01,9.33640420e-01,9.12167579e-02,7.26325393e-01,
6.58039212e-01,9.39130664e-01,3.68637234e-01,7.78640509e-02,
5.77523530e-01,3.63632172e-01,4.44532394e-01,8.23739767e-02,
3.01169813e-01,9.15617108e-01,4.01109338e-01,6.52816117e-01,
3.36535722e-01,1.83367670e-01,9.86048639e-01,2.11613506e-01,
9.39686358e-01,2.76162028e-01,8.99377912e-02,5.59440315e-01,
6.95002973e-01,6.85773134e-01,6.95965350e-01,9.38641310e-01,
3.02985966e-01,7.37742484e-01,4.92526293e-01,3.16089630e-01,
1.87618837e-01,6.22452796e-02,9.46086943e-01,2.13588208e-01,
6.07565165e-01,6.22669816e-01,6.42262280e-01,1.74358606e-01,
7.60361373e-01,6.07401252e-01,3.88030050e-04,1.06557794e-01,
5.73728263e-01,2.31160838e-02,9.74358380e-01,3.07875097e-01,
1.41230553e-01,2.09358424e-01,7.70691752e-01,8.38017046e-01,
1.21261969e-01,5.75457394e-01,6.84708729e-02,5.65244675e-01,
3.90781015e-01,5.39179444e-01,8.40706050e-01,6.09158993e-01};

    Opm::Tensor<Evaluation> out{1};
    out.data_ = {0.82167464};

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
