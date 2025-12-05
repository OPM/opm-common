#   Copyright (c) 2025 NORCE
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

from opm.ml.ml_tools.scaler_layers import (
    MinMaxScalerLayer,
    MinMaxUnScalerLayer,
)

# Activation functions
def linear(x):
    return x


def relu(x):
    return np.maximum(0, x)


def softplus(x):
    return np.log1p(np.exp(x))


def sigmoid(x):
    return 1 / (1 + np.exp(-x))


def tanh(x):
    return np.tanh(x)

def hard_sigmoid(x):
    return np.clip(0.2 * x + 0.5, 0, 1)

# Mapping of activation names to functions
ACTIVATIONS = {
    "linear": linear,
    "relu": relu,
    "softplus": softplus,
    "sigmoid": sigmoid,
    "tanh": tanh,
    "hard_sigmoid": hard_sigmoid
}


class Dense:
    def __init__(self, input_dim, output_dim, activation="linear"):
        # Random initialization for generality
        self.weights = np.random.randn(input_dim, output_dim).astype(np.float32) * 0.01
        self.biases = np.zeros((output_dim,), dtype=np.float32)
        self.activation_name = activation
        self.activation_func = ACTIVATIONS.get(activation, linear)

    def set_weights(self, w_b):
        self.weights = w_b[0].astype(np.float32)
        self.biases = w_b[1].astype(np.float32)

    def get_weights(self):
        return [self.weights, self.biases]

    def get_config(self):
        return {
            "input_dim": self.weights.shape[0],
            "output_dim": self.weights.shape[1],
            "activation": self.activation_name,
        }

    def forward(self, x):
        z = np.dot(x, self.weights) + self.biases
        return self.activation_func(z)

    def __call__(self, x) -> np.ndarray:
        return self.forward(x)


class Sequential:
    def __init__(self, layers):
        self.layers = layers

    def forward(self, x):
        """Forward propagate input through all layers."""
        for layer in self.layers:
            x = layer.forward(x)
        return x

    def summary(self):
        print("Model Summary:")
        for i, layer in enumerate(self.layers):
            cfg = layer.get_config()
            # Determine type-specific info
            if isinstance(layer, Dense):
                print(
                    f" Layer {i+1}: Dense({cfg['input_dim']} -> {cfg['output_dim']}), activation={cfg['activation']}"
                )
            elif isinstance(layer, (MinMaxScalerLayer, MinMaxUnScalerLayer)):
                print(
                    f" Layer {i+1}: {cfg.get('name', layer.__class__.__name__)}, "
                    f"feature_range={cfg.get('feature_range')}, "
                    f"is_adapted={cfg.get('is_adapted')}"
                )
            else:
                # fallback for unknown layers
                print(f" Layer {i+1}: {layer.__class__.__name__}, config={cfg}")

    def get_weights(self):
        return [layer.get_weights() for layer in self.layers]

    def set_weights(self, weights_list):
        for layer, w in zip(self.layers, weights_list):
            layer.set_weights(w)

    def __call__(self, x) -> np.ndarray:
        return self.forward(x)