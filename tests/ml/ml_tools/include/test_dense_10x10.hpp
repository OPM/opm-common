
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
bool test_dense_10x10(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST dense_10x10\n");

    OPM_ERROR_IF(!load_time, "Invalid Evaluation");
    OPM_ERROR_IF(!apply_time, "Invalid Evaluation");

    Opm::ML::Tensor<Evaluation> in{10};
    in.data_ = {0.17322063,0.03807505,0.043235425,0.026106816,0.5015539,0.12986024,
0.2096403,0.9004162,0.08708454,0.1511973};

    Opm::ML::Tensor<Evaluation> out{1};
    out.data_ = {0.34216738};

    Opm::ML::NNTimer load_timer;
    load_timer.start();

    Opm::ML::NNModel<Evaluation> model;
    OPM_ERROR_IF(!model.loadModel(std::filesystem::current_path() / "ml/ml_tools/models/test_dense_10x10.model"), "Failed to load model");

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
