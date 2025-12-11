#   Copyright (C) 2025 NORCE Research AS.

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

import struct
import numpy as np

from opm.ml.ml_tools.kerasify import (
    LAYER_SCALING, LAYER_UNSCALING, LAYER_DENSE, LAYER_ACTIVATION,
    ACTIVATION_LINEAR, ACTIVATION_RELU, ACTIVATION_SOFTPLUS, 
    ACTIVATION_TANH, ACTIVATION_SIGMOID, ACTIVATION_HARD_SIGMOID,
)

from opm.ml.ml_tools.scaler_layers import (
    MinMaxScalerLayer,
    MinMaxUnScalerLayer,
)

from opm.ml.ml_tools.dense_layers import (
    Dense, Sequential
)

def read_floats(f, count):
    """Read `count` float32 values from file."""
    data = f.read(4 * count)
    return np.array(struct.unpack(f"{count}f", data), dtype=np.float32)

def read_activation(f):
    """Reverse of write_activation."""
    act_id = struct.unpack('I', f.read(4))[0]

    if act_id == ACTIVATION_LINEAR:
        return "linear"
    elif act_id == ACTIVATION_RELU:
        return "relu"
    elif act_id == ACTIVATION_SOFTPLUS:
        return "softplus"
    elif act_id == ACTIVATION_TANH:
        return "tanh"
    elif act_id == ACTIVATION_SIGMOID:
        return "sigmoid"
    elif act_id == ACTIVATION_HARD_SIGMOID:
        return "hard_sigmoid"
    else:
        raise ValueError(f"Unknown activation ID: {act_id}")


def load_model(filename):
    """
    Load a Sequential model previously saved with `export_model`.

    This function reads a binary file containing layer information,
    weights, biases, and scaler parameters, and reconstructs a
    Python `Sequential` model with `Dense`, `MinMaxScalerLayer`,
    and `MinMaxUnScalerLayer` layers.

    Parameters
    ----------
    filename : str or os.PathLike
        Path to the binary file created by `export_model`.

    Returns
    -------
    model : Sequential
        A Sequential model with layers and weights restored from the file.

    Notes
    -----
    - Dense layers with activations are saved as separate Dense + Activation entries
      in the file, so this function automatically merges them into a single Dense layer
      with the correct activation.
    - MinMaxScalerLayer and MinMaxUnScalerLayer require the stored data_min, data_max,
      and feature_range to properly reconstruct the internal scaling parameters.
    - The function currently supports only scalar or single-feature scaling.
    """
    layers = []

    with open(filename, "rb") as f:
        num_layers = struct.unpack('I', f.read(4))[0]

        for _ in range(num_layers):

            layer_type_id = struct.unpack('I', f.read(4))[0]

            # ---- MinMaxScalerLayer ----
            if layer_type_id == LAYER_SCALING:
                data_min = struct.unpack('f', f.read(4))[0]
                data_max = struct.unpack('f', f.read(4))[0]
                feat_inf = struct.unpack('f', f.read(4))[0]
                feat_sup = struct.unpack('f', f.read(4))[0]

                layer = MinMaxScalerLayer(feature_range=(feat_inf, feat_sup))
                layer.data_min = data_min
                layer.data_max = data_max
                layer._adapt() 
                layers.append(layer)

            # ---- MinMaxUnScalerLayer ----
            elif layer_type_id == LAYER_UNSCALING:
                data_min = struct.unpack('f', f.read(4))[0]
                data_max = struct.unpack('f', f.read(4))[0]
                feat_inf = struct.unpack('f', f.read(4))[0]
                feat_sup = struct.unpack('f', f.read(4))[0]

                layer = MinMaxUnScalerLayer(feature_range=(feat_inf, feat_sup))
                layer.data_min = data_min
                layer.data_max = data_max
                layer._adapt()
                layers.append(layer)

            # ---- Dense ----
            elif layer_type_id == LAYER_DENSE:
                in_dim = struct.unpack('I', f.read(4))[0]
                out_dim = struct.unpack('I', f.read(4))[0]
                bias_dim = struct.unpack('I', f.read(4))[0]

                w_count = in_dim * out_dim
                weights = read_floats(f, w_count).reshape((in_dim, out_dim))
                biases = read_floats(f, bias_dim)

                activation = read_activation(f)

                layer = Dense(in_dim, out_dim, activation=activation)
                layer.set_weights([weights, biases])

                layers.append(layer)

            # ---- Standalone Activation (ignored) ----
            elif layer_type_id == LAYER_ACTIVATION:
                _ = read_activation(f)
                continue

            else:
                raise ValueError(f"Unsupported layer type id: {layer_type_id}")

    return Sequential(layers)
