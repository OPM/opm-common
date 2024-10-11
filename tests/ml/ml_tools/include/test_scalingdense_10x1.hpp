
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
bool test_scalingdense_10x1(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST scalingdense_10x1\n");

    OPM_ERROR_IF(!load_time, "Invalid Evaluation");
    OPM_ERROR_IF(!apply_time, "Invalid Evaluation");

    Opm::Tensor<Evaluation> in{10};
    in.data_ = {0.696915,0.07427129,0.70194745,0.8012347,0.35779506,0.06662053,
0.05146523,0.6114587,0.40359908,0.53088933};

    Opm::Tensor<Evaluation> out{10};
    out.data_ = {747.41144,724.1453,772.69525,680.4142,689.91376,677.3659,582.6327,
817.43665,651.73566,755.77905};

    NNTimer load_timer;
    load_timer.start();

    const fs::path sub_dir = "/tests/ml/ml_tools/models/test_scalingdense_10x1.model" ;

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
        OPM_ERROR_IF ((fabs(out(i).value() - predict(i).value()) > 1e-3), fmt::format(" Expected " "{}" " got " "{}",predict(i).value(),out(i).value()));
    }

    return true;
}
