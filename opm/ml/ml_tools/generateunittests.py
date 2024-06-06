import numpy as np
import pprint
import os
from tensorflow import keras

from keras.models import Sequential
from keras.layers import Conv2D, Dense, Flatten, Activation, MaxPooling2D, Dropout, BatchNormalization, ELU, Embedding, LSTM
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

# path1=os.path.abspath(directory)
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
    path = os.path.abspath(f'models/test_{name}.model')
    with open('include/test_%s.h' % name, 'w') as f:
        x_shape, x_data = c_array(test_x[0])
        y_shape, y_data = c_array(predict_y[0])

        f.write(TEST_CASE % (name, name, x_shape, x_data, y_shape, y_data, path, eps))



# scaling 1x1
data: np.ndarray = np.random.uniform(-500, 500, (5, 1))
feature_ranges: list[tuple[float, float]] = [(0.0, 1.0), (-3.7, 0.0)]
test_x = np.random.rand(1).astype('f')
test_y = np.random.rand(1).astype('f')
data_min = 10.0
model = Sequential()
model.add(keras.layers.Input([1]))
# model.add(Flatten())
# model.add(Flatten())
model.add(MinMaxScalerLayer(data_min=10,feature_range=(0.0, 1.0)))
model.add(Dense(1,activation='tanh'))
model.add(Dense(10,activation='tanh'))
model.add(Dense(1,activation='tanh'))
model.add(MinMaxUnScalerLayer(feature_range=(1.0, 5.0)))
# model.add(Dense(10))
# model.add(Dense(10))
# model.add(Dense(10))
# model.add(MinMaxScalerLayer(feature_range=(0.0, 1.0)))
# model.add(Flatten())
# model.add(MinMaxUnScalerLayer())
# #
model.get_layer(model.layers[0].name).adapt(data=data)
model.get_layer(model.layers[-1].name).adapt(data=data)

# model.add(Dense(1, input_dim=1))

# model: keras.Model = keras.Sequential(
#
#     [
#
#         keras.layers.Input([1]),
#
#         MinMaxScalerLayer(feature_range=(0.0, 1.0)),
#
#         # keras.layers.Dense(1, input_dim=1),
#
#         # MinMaxUnScalerLayer(feature_range=(0.0, 1.0)),
#
#     ]
#
# )
output_testcase(model, test_x, test_y, 'scalingdense_1x1', '1e-3')

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

# Conv 2x2
test_x = np.random.rand(10, 2, 2,1).astype('f')
test_y = np.random.rand(10, 1).astype('f')
model = Sequential()
model.add(Conv2D(1,(2, 2), padding='valid', input_shape=(2, 2, 1)))
model.add(Flatten())
model.add(Dense(1))
output_testcase(model, test_x, test_y, 'conv_2x2', '1e-6')

# Conv 3x3
test_x = np.random.rand(10, 3, 3, 1).astype('f').astype('f')
test_y = np.random.rand(10, 1).astype('f')
model = Sequential([
    Conv2D(1, (3, 3), input_shape=(3, 3, 1)),
    Flatten(),
    Dense(1)
])
output_testcase(model, test_x, test_y, 'conv_3x3', '1e-6')

# Conv 3x3x3
test_x = np.random.rand(10, 3, 3, 3).astype('f')
test_y = np.random.rand(10, 1).astype('f')
model = Sequential([
    Conv2D(3, (3, 3), input_shape=(3, 3, 3)),
    Flatten(),
    # BatchNormalization(),
    Dense(1)
])
output_testcase(model, test_x, test_y, 'conv_3x3x3', '1e-6')

# Activation ELU
test_x = np.random.rand(1, 10).astype('f')
test_y = np.random.rand(1, 1).astype('f')
model = Sequential([
    Dense(10, input_dim=10),
    ELU(alpha=0.5),
    Dense(1)
])
output_testcase(model, test_x, test_y, 'elu_10', '1e-6')

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

# Conv softplus
test_x = np.random.rand(10, 2, 2, 1).astype('f')
test_y = np.random.rand(10, 1).astype('f')
model = Sequential([
    Conv2D(1, (2, 2), input_shape=(2, 2, 1), activation='softplus'),
    Flatten(),
    Dense(1)
])
output_testcase(model, test_x, test_y, 'conv_softplus_2x2', '1e-6')

# Conv hardsigmoid
test_x = np.random.rand(10, 2, 2, 1).astype('f')
test_y = np.random.rand(10, 1).astype('f')
model = Sequential([
    Conv2D(1, (2, 2), input_shape=(2, 2, 1), activation='hard_sigmoid'),
    Flatten(),
    Dense(1)
])
output_testcase(model, test_x, test_y, 'conv_hard_sigmoid_2x2', '1e-6')

# Conv sigmoid
test_x = np.random.rand(10, 2, 2, 1).astype('f')
test_y = np.random.rand(10, 1).astype('f')
model = Sequential([
    Conv2D(1, (2, 2), input_shape=(2, 2, 1), activation='sigmoid'),
    Flatten(),
    Dense(1)
])
output_testcase(model, test_x, test_y, 'conv_sigmoid_2x2', '1e-6')

# Maxpooling2D 1x1
test_x = np.random.rand(10, 10, 10, 1).astype('f')
test_y = np.random.rand(10, 1).astype('f')
model = Sequential([
    MaxPooling2D(pool_size=(1, 1), input_shape=(10, 10, 1)),
    Flatten(),
    Dense(1)
])
output_testcase(model, test_x, test_y, 'maxpool2d_1x1', '1e-6')
