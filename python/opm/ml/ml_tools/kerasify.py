
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
# Note: This file is based on kerasify/kerasify.py

import numpy as np
import struct

LAYER_SCALING = 1
LAYER_UNSCALING = 2
LAYER_DENSE = 3
LAYER_ACTIVATION = 4

ACTIVATION_LINEAR = 1
ACTIVATION_RELU = 2
ACTIVATION_SOFTPLUS = 3
ACTIVATION_SIGMOID = 4
ACTIVATION_TANH = 5
ACTIVATION_HARD_SIGMOID = 6

def write_scaling(f):
    f.write(struct.pack('I', LAYER_SCALING))


def write_unscaling(f):
    f.write(struct.pack('I', LAYER_UNSCALING))


def write_tensor(f, data, dims=1):
    """
    Writes tensor as flat array of floats to file in 1024 chunks,
    prevents memory explosion writing very large arrays to disk
    when calling struct.pack().
    """
    f.write(struct.pack('I', dims))

    for stride in data.shape[:dims]:
        f.write(struct.pack('I', stride))

    data = data.ravel()
    step = 1024
    written = 0

    for i in np.arange(0, len(data), step):
        remaining = min(len(data) - i, step)
        written += remaining
        f.write(struct.pack(f'={remaining}f', *data[i: i + remaining]))

    assert written == len(data)


def write_floats(file, floats):
    '''
    Writes floats to file in 1024 chunks.. prevents memory explosion
    writing very large arrays to disk when calling struct.pack().
    '''
    step = 1024
    written = 0

    for i in np.arange(0, len(floats), step):
        remaining = min(len(floats) - i, step)
        written += remaining
        file.write(struct.pack('=%sf' % remaining, *floats[i:i+remaining]))

    assert written == len(floats)

def export_model(model, filename):
    with open(filename, 'wb') as f:

        def write_activation(activation):
            if activation == 'linear':
                f.write(struct.pack('I', ACTIVATION_LINEAR))
            elif activation == 'relu':
                f.write(struct.pack('I', ACTIVATION_RELU))
            elif activation == 'softplus':
                f.write(struct.pack('I', ACTIVATION_SOFTPLUS))
            elif activation == 'tanh':
                f.write(struct.pack('I', ACTIVATION_TANH))
            elif activation == 'sigmoid':
                f.write(struct.pack('I', ACTIVATION_SIGMOID))
            elif activation == 'hard_sigmoid':
                f.write(struct.pack('I', ACTIVATION_HARD_SIGMOID))
            else:
                assert False, f"Unsupported activation type:{activation}" 

        model_layers = [l for l in model.layers]

        num_layers = len(model_layers)
        f.write(struct.pack('I', num_layers))

        for layer in model_layers:
            layer_type = type(layer).__name__

            if layer_type == 'MinMaxScalerLayer':
                write_scaling(f)
                feat_inf = layer.get_weights()[0]
                feat_sup = layer.get_weights()[1]
                f.write(struct.pack('f', layer.data_min))
                f.write(struct.pack('f', layer.data_max))
                f.write(struct.pack('f', feat_inf))
                f.write(struct.pack('f', feat_sup))


            elif layer_type == 'MinMaxUnScalerLayer':
                write_unscaling(f)
                feat_inf = layer.get_weights()[0]
                feat_sup = layer.get_weights()[1]
                f.write(struct.pack('f', layer.data_min))
                f.write(struct.pack('f', layer.data_max))
                f.write(struct.pack('f', feat_inf))
                f.write(struct.pack('f', feat_sup))

            elif layer_type == 'Dense':
                weights = layer.get_weights()[0]
                biases = layer.get_weights()[1]
                activation = layer.get_config()['activation']

                f.write(struct.pack('I', LAYER_DENSE))
                f.write(struct.pack('I', weights.shape[0]))
                f.write(struct.pack('I', weights.shape[1]))
                f.write(struct.pack('I', biases.shape[0]))

                weights = weights.flatten()
                biases = biases.flatten()

                write_floats(f, weights)
                write_floats(f, biases)

                write_activation(activation)


            elif layer_type == 'Activation':
                activation = layer.get_config()['activation']

                f.write(struct.pack('I', LAYER_ACTIVATION))
                write_activation(activation)

            else:
                assert False, f"Unsupported layer type:{layer_type}" 
