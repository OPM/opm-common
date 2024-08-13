
/*

 * Copyright (c) 2016 Robert W. Rose
 * Copyright (c) 2018 Paul Maevskikh
 *
 * MIT License, see LICENSE.OLD file.
 */ 

/*
 * Copyright (c) 2024 Birane Kane
 * Copyright (c) 2024 Tor Harald Sandve
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
namespace fs = std::filesystem;

using namespace Opm;
template<class Evaluation>
bool test_dense_tanh_10(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST dense_tanh_10\n");

    KASSERT(load_time, "Invalid Evaluation");
    KASSERT(apply_time, "Invalid Evaluation");

    Opm::Tensor<Evaluation> in{10};
    in.data_ = {0.5133757,0.8992964,0.018602798,0.8300745,0.43004134,0.16437091,
0.13667183,0.7081913,0.39895463,0.53926337};

    Opm::Tensor<Evaluation> out{10};
    out.data_ = {-0.16390416,-0.11630989,0.37623432,-0.65770155,0.1101914,
-0.097250186,0.5037399,-0.2223558,0.3810121,0.6196858};

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("/Users/macbookn/hackatonwork/opm-common/opm/ml/ml_tools/models/test_dense_tanh_10.model"), "Failed to load model");

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
