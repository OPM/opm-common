#  Copyright (c) 2024 NORCE
#   This file is part of the Open Porous Media project (OPM).

#   OPM is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.

#   OPM is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with OPM.  If not, see <http://www.gnu.org/licenses/>.

import numpy as np
import pprint
import os, sys
from tensorflow import keras

from keras.models import Sequential
from keras.layers import Conv2D, Dense, Flatten, Activation, MaxPooling2D, Dropout, BatchNormalization, ELU, Embedding, LSTM

sys.path.append('../../../opm/ml/ml_tools')

from kerasify import export_model
from scaler_layers import MinMaxScalerLayer, MinMaxUnScalerLayer

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

 * Copyright (c) 2016 Robert W. Rose
 * Copyright (c) 2018 Paul Maevskikh
 *
 * MIT License, see LICENSE.MIT file.
 */ 

/*
 * Copyright (c) 2024 NORCE
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
bool test_%s(Evaluation* load_time, Evaluation* apply_time)
{
    printf("TEST %s\\n");

    KASSERT(load_time, "Invalid Evaluation");
    KASSERT(apply_time, "Invalid Evaluation");

    Opm::Tensor<Evaluation> in%s;
    in.data_ = %s;

    Opm::Tensor<Evaluation> out%s;
    out.data_ = %s;

    KerasTimer load_timer;
    load_timer.Start();

    KerasModel<Evaluation> model;
    KASSERT(model.LoadModel("%s"), "Failed to load model");

    *load_time = load_timer.Stop();

    KerasTimer apply_timer;
    apply_timer.Start();

    Opm::Tensor<Evaluation> predict = out;
    KASSERT(model.Apply(&in, &out), "Failed to apply");

    *apply_time = apply_timer.Stop();

    for (int i = 0; i < out.dims_[0]; i++)
    {
        KASSERT_EQ(out(i), predict(i), %s);
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


def output_testcase(model, test_x, test_y, name, eps):
    print("Processing %s" % name)
    model.compile(loss='mean_squared_error', optimizer='adamax')
    model.fit(test_x, test_y, epochs=1, verbose=False)
    predict_y = model.predict(test_x).astype('f')
    print(model.summary())

    export_model(model, 'models/test_%s.model' % name)
    path = f'./ml/ml_tools/models/test_{name}.model'
    with open('include/test_%s.hpp' % name, 'w') as f:
        x_shape, x_data = c_array(test_x[0])
        y_shape, y_data = c_array(predict_y[0])

        f.write(TEST_CASE % (name, name, x_shape, x_data, y_shape, y_data, path, eps))


# scaling 10x1
data: np.ndarray = np.random.uniform(-500, 500, (5, 1))
feature_ranges: list[tuple[float, float]] = [(0.0, 1.0), (-3.7, 0.0)]
test_x = np.random.rand(10, 10).astype('f')
test_y = np.random.rand(10).astype('f')
data_min = 10.0
model = Sequential()
model.add(keras.layers.Input([10]))
model.add(MinMaxScalerLayer(feature_range=(0.0, 1.0)))
model.add(Dense(10,activation='tanh'))
model.add(Dense(10,activation='tanh'))
model.add(Dense(10,activation='tanh'))
model.add(Dense(10,activation='tanh'))
model.add(MinMaxUnScalerLayer(feature_range=(-3.7, -1.0)))
# #
model.get_layer(model.layers[0].name).adapt(data=data)
model.get_layer(model.layers[-1].name).adapt(data=data)
output_testcase(model, test_x, test_y, 'scalingdense_10x1', '1e-3')

# Dense 1x1
test_x = np.arange(10)
test_y = test_x * 10 + 1
model = Sequential()
model.add(Dense(1, input_dim=1))
output_testcase(model, test_x, test_y, 'dense_1x1', '1e-6')

# Dense 10x1
test_x = np.random.rand(10, 10).astype('f')
test_y = np.random.rand(10).astype('f')
model = Sequential()
model.add(Dense(1, input_dim=10))
output_testcase(model, test_x, test_y, 'dense_10x1', '1e-6')

# Dense 2x2
test_x = np.random.rand(10, 2).astype('f')
test_y = np.random.rand(10).astype('f')
model = Sequential()
model.add(Dense(2, input_dim=2))
model.add(Dense(1))
output_testcase(model, test_x, test_y, 'dense_2x2', '1e-6')

# Dense 10x10
test_x = np.random.rand(10, 10).astype('f')
test_y = np.random.rand(10).astype('f')
model = Sequential()
model.add(Dense(10, input_dim=10))
model.add(Dense(1))
output_testcase(model, test_x, test_y, 'dense_10x10', '1e-6')

# Dense 10x10x10
test_x = np.random.rand(10, 10).astype('f')
test_y = np.random.rand(10, 10).astype('f')
model = Sequential()
model.add(Dense(10, input_dim=10))
model.add(Dense(10))
output_testcase(model, test_x, test_y, 'dense_10x10x10', '1e-6')

# Activation relu
test_x = np.random.rand(1, 10).astype('f')
test_y = np.random.rand(1, 10).astype('f')
model = Sequential()
model.add(Dense(10, input_dim=10))
model.add(Activation('relu'))
output_testcase(model, test_x, test_y, 'relu_10', '1e-6')

# Dense relu
test_x = np.random.rand(1, 10).astype('f')
test_y = np.random.rand(1, 10).astype('f')
model = Sequential()
model.add(Dense(10, input_dim=10, activation='relu'))
model.add(Dense(10, input_dim=10, activation='relu'))
model.add(Dense(10, input_dim=10, activation='relu'))
output_testcase(model, test_x, test_y, 'dense_relu_10', '1e-6')

#  Dense tanh
test_x = np.random.rand(1, 10).astype('f')
test_y = np.random.rand(1, 10).astype('f')
model = Sequential()
model.add(Dense(10, input_dim=10, activation='tanh'))
model.add(Dense(10, input_dim=10, activation='tanh'))
model.add(Dense(10, input_dim=10, activation='tanh'))
output_testcase(model, test_x, test_y, 'dense_tanh_10', '1e-6')