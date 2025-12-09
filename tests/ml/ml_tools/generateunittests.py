#   Copyright (c) 2016 Robert W. Rose
#   Copyright (c) 2018 Paul Maevskikh
#   Copyright (c) 2024 NORCE
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Note: This file is based on     kerasify/make_tests.py

import numpy as np
import os, sys

sys.path.insert(0, '../../../python')

from opm.ml.ml_tools import export_model
from opm.ml.ml_tools import  MinMaxScalerLayer, MinMaxUnScalerLayer
from opm.ml.ml_tools import Dense, Sequential

np.set_printoptions(precision=25, threshold=10000000)


def c_array(a):
    def to_cpp(ndarray):
        text = np.array2string(ndarray, separator=',', threshold=np.inf,
                               floatmode='unique')
        return text.replace('[', '{').replace(']', '}').replace(' ', '')

    s = to_cpp(a.ravel())
    shape = to_cpp(np.asarray(a.shape)) if a.shape else '{1}'
    return shape, s


TEST_CASE = '''
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
bool test_%s(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST %s\\n");

    OPM_ERROR_IF(!load_time, "Invalid Evaluation");
    OPM_ERROR_IF(!apply_time, "Invalid Evaluation");

    Opm::ML::Tensor<Evaluation> in%s;
    in.data_ = %s;

    Opm::ML::Tensor<Evaluation> out%s;
    out.data_ = %s;

    Opm::ML::NNTimer load_timer;
    load_timer.start();

    Opm::ML::NNModel<Evaluation> model;
    OPM_ERROR_IF(!model.loadModel(std::filesystem::current_path() / "%s"), "Failed to load model");

    *load_time = load_timer.stop();

    Opm::ML::NNTimer apply_timer;
    apply_timer.start();

    Opm::ML::Tensor<Evaluation> predict = out;
    OPM_ERROR_IF(!model.apply(in, out), "Failed to apply");

    *apply_time = apply_timer.stop();

    for (int i = 0; i < out.dims_[0]; i++)
    {
        OPM_ERROR_IF ((fabs(out(i).value() - predict(i).value()) > %s), fmt::format(" Expected " "{}" " got " "{}",predict(i).value(),out(i).value()));
    }

    return true;
}
'''

directory = os.getcwd()
directory1 = "models"
directory2 = "include"

if os.path.isdir(directory1):
    print(f"{directory1} exists.")
else:
    print(f"{directory1} does not exist.")
    path1 = os.path.join(directory, directory1)
    os.makedirs(path1)

if os.path.isdir(directory2):
    print(f"{directory2} exists.")
else:
    path2 = os.path.join(directory, directory2)
    os.makedirs(path2)


def output_testcase(model, test_x, name, eps):
    print("Processing %s" % name)
    predict_y = model.forward(test_x).astype('f')
    print(model.summary())

    export_model(model, 'models/test_%s.model' % name)

    path = f'ml/ml_tools/models/test_{name}.model'
    with open('include/test_%s.hpp' % name, 'w') as f:
        x_shape, x_data = c_array(test_x[0])
        y_shape, y_data = c_array(predict_y[0])
        f.write(TEST_CASE % (name, name, x_shape, x_data, y_shape, y_data, path, eps))


# scaling 10x1
test_x = np.random.uniform(0.0, 1.0, (10, 1)).astype('f')
test_y = np.random.uniform(-3.7, 0.0, (10, 1)).astype('f')
model = Sequential([
    MinMaxScalerLayer(feature_range=[0.0, 1.0]),
    Dense(input_dim=1, output_dim=10, activation='tanh'),
    Dense(input_dim=10, output_dim=10, activation='tanh'),
    Dense(input_dim=10, output_dim=10, activation='tanh'),
    Dense(input_dim=10, output_dim=1, activation='tanh'),
    MinMaxUnScalerLayer(feature_range=[-3.7, 0.0])
])
model.layers[0].adapt(data=test_x)
model.layers[-1].adapt(data=test_y)
output_testcase(model, test_x, 'scalingdense_10x1', '1e-3')

# Dense 1x1
test_x = np.array([10.])
model = Sequential([
    Dense(input_dim=1, output_dim=1, activation='linear')
])
output_testcase(model, test_x, 'dense_1x1', '1e-6')

# Dense 10x1
test_x = np.random.rand(10, 10).astype('f')
model = Sequential([
    Dense(input_dim=10, output_dim=1, activation='linear')
])
output_testcase(model, test_x, 'dense_10x1', '1e-6')

# Dense 2x2
test_x = np.random.rand(10, 2).astype('f')
model = Sequential([
    Dense(input_dim=2, output_dim=2, activation='linear'),
    Dense(input_dim=2, output_dim=1)
])
output_testcase(model, test_x, 'dense_2x2', '1e-6')

# Dense 10x10
test_x = np.random.rand(10, 10).astype('f')
model = Sequential([
    Dense(input_dim=10, output_dim=10, activation='linear'),
    Dense(input_dim=10, output_dim=1)
])
output_testcase(model, test_x, 'dense_10x10', '1e-6')

# Dense 10x10x10
test_x = np.random.rand(10, 10).astype('f')
model = Sequential([
    Dense(input_dim=10, output_dim=10, activation='linear'),
    Dense(input_dim=10, output_dim=10)
])
output_testcase(model, test_x, 'dense_10x10x10', '1e-6')

# Dense activations
test_x = np.random.rand(1, 10).astype('f')
model = Sequential([
    Dense(input_dim=10, output_dim=10, activation='linear'),
    Dense(input_dim=10, output_dim=10, activation='relu'),
    Dense(input_dim=10, output_dim=10, activation='softplus'),
    Dense(input_dim=10, output_dim=10, activation='sigmoid'),
    Dense(input_dim=10, output_dim=10, activation='tanh'),
    Dense(input_dim=10, output_dim=10, activation='hard_sigmoid'),
])
output_testcase(model, test_x, 'dense_activation_10', '1e-6')
