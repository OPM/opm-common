
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

    Opm::Tensor<Evaluation> in{10};
    in.data_ = {0.57784367,0.25253502,0.56445366,0.7543359,0.5095673,0.14378926,
0.58591866,0.02342105,0.028024895,0.905349};

    Opm::Tensor<Evaluation> out{10};
    out.data_ = {-0.029181654,0.3627418,-0.052402735,0.50181013,-0.5348898,
-0.061557874,-0.29160437,0.055488374,0.054263014,0.20701094};

    NNTimer load_timer;
    load_timer.start();

    const fs::path sub_dir = "/tests/ml/ml_tools/models/test_dense_tanh_10.model" ;

    fs::path p = fs::current_path();

    fs::path curr_path = fs::absolute(p)+=sub_dir;

    NNModel<Evaluation> model;
    OPM_ERROR_IF(!model.loadModel(curr_path), "Failed to load model");

    *load_time = load_timer.stop();

    NNTimer apply_timer;
    apply_timer.start();

    Opm::Tensor<Evaluation> predict = out;
    OPM_ERROR_IF(!model.apply(in, out), "Failed to apply");

    *apply_time = apply_timer.stop();

    for (int i = 0; i < out.dims_[0]; i++)
    {
        OPM_ERROR_IF ((fabs(out(i).value() - predict(i).value()) > 1e-6), fmt::format(" Expected " "{}" " got " "{}",predict(i).value(),out(i).value()));
    }

    return true;
}
