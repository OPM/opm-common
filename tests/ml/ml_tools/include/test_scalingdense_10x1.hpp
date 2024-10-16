
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
    in.data_ = {0.4462127,0.8282716,0.19106287,0.46530953,0.8019146,0.6105967,
0.69855976,0.7211706,0.116201125,0.49067858};

    Opm::Tensor<Evaluation> out{10};
    out.data_ = {519.3525,665.1217,690.99927,413.76526,739.2478,407.98572,631.28186,
505.1883,526.25494,696.9934};

    NNTimer load_timer;
    load_timer.start();

    NNModel<Evaluation> model;
    OPM_ERROR_IF(!model.loadModel("./tests/ml/ml_tools/models/test_scalingdense_10x1.model"), "Failed to load model");

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
