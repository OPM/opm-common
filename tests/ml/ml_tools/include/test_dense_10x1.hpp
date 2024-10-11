
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
bool test_dense_10x1(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST dense_10x1\n");

    OPM_ERROR_IF(!load_time, "Invalid Evaluation");
    OPM_ERROR_IF(!apply_time, "Invalid Evaluation");

    Opm::Tensor<Evaluation> in{10};
    in.data_ = {0.2956252,0.3948974,0.2736113,0.147597,0.33848935,0.48078042,
0.46976265,0.93288493,0.9137014,0.25778493};

    Opm::Tensor<Evaluation> out{1};
    out.data_ = {0.68196857};

    NNTimer load_timer;
    load_timer.start();

    const fs::path sub_dir = "/tests/ml/ml_tools/models/test_dense_10x1.model" ;

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
