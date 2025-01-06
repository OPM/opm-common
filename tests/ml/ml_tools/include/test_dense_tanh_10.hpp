
/*
  Copyright (c) 2016 Robert W. Rose
  Copyright (c) 2018 Paul Maevskikh
  Copyright (c) 2024 NORCE
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <filesystem>
#include <iostream>
#include <opm/common/ErrorMacros.hpp>
#include <fmt/format.h>

namespace fs = std::filesystem;

using namespace Opm;

template<class Evaluation>
bool test_dense_tanh_10(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST dense_tanh_10\n");

    OPM_ERROR_IF(!load_time, "Invalid Evaluation");
    OPM_ERROR_IF(!apply_time, "Invalid Evaluation");

    Opm::ML::Tensor<Evaluation> in{10};
    in.data_ = {0.6977599,0.93939304,0.3949355,0.090981744,0.3517743,0.37817308,
0.7245931,0.7449905,0.13598745,0.024666037};

    Opm::ML::Tensor<Evaluation> out{10};
    out.data_ = {0.19516285,0.18422243,0.042072553,-0.03372099,-0.17953515,
-0.3563359,0.114287786,-0.4759317,0.12825572,0.05502218};

    Opm::ML::NNTimer load_timer;
    load_timer.start();

    Opm::ML::NNModel<Evaluation> model;
    OPM_ERROR_IF(!model.loadModel("./tests/ml/ml_tools/models/test_dense_tanh_10.model"), "Failed to load model");

    *load_time = load_timer.stop();

    Opm::ML::NNTimer apply_timer;
    apply_timer.start();

    Opm::ML::Tensor<Evaluation> predict = out;
    OPM_ERROR_IF(!model.apply(in, out), "Failed to apply");

    *apply_time = apply_timer.stop();

    for (int i = 0; i < out.dims_[0]; i++)
    {
        OPM_ERROR_IF ((fabs(out(i).value() - predict(i).value()) > 1e-6), fmt::format(" Expected " "{}" " got " "{}",predict(i).value(),out(i).value()));
    }

    return true;
}
