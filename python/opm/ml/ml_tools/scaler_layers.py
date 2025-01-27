#   Copyright (c) 2024 NORCE
#   Copyright (c) 2024 UiB
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

"""Provide MinMax scaler layers for tensorflow.keras."""

from __future__ import annotations

from typing import Optional, Sequence

import numpy as np
import tensorflow as tf
from numpy.typing import ArrayLike
from tensorflow import keras
from tensorflow.python.keras.engine.base_preprocessing_layer import (
    PreprocessingLayer,
)


class ScalerLayer(keras.layers.Layer):
    """MixIn to provide functionality for the Scaler Layer."""

    data_min: tf.Tensor
    data_max: tf.Tensor
    min: tf.Tensor
    scalar: tf.Tensor

    def __init__(
        self,
        data_min: Optional[float | ArrayLike] = None,
        data_max: Optional[float | ArrayLike] = None,
        feature_range: Sequence[float] | np.ndarray | tf.Tensor = (0, 1),
        **kwargs, 
    ) -> None:
        super().__init__(**kwargs)
        if feature_range[0] >= feature_range[1]:
            raise ValueError("Feature range must be strictly increasing.")
        self.feature_range: tf.Tensor = tf.convert_to_tensor(
            feature_range, dtype=tf.float32
        )
        self._is_adapted: bool = False
        if data_min is not None and data_max is not None:
            self.data_min = tf.convert_to_tensor(data_min, dtype=tf.float32)
            self.data_max = tf.convert_to_tensor(data_max, dtype=tf.float32)
            self._adapt()

    def build(self, input_shape: tuple[int, ...]) -> None:
        """Initialize ``data_min`` and ``data_max`` with the default values if they have
        not been initialized yet.

        Args:
            input_shape (tuple[int, ...]): _description_

        """
        if not self._is_adapted:
            # ``data_min`` and ``data_max`` have the same shape as one input tensor.
            self.data_min = tf.zeros(input_shape[1:])
            self.data_max = tf.ones(input_shape[1:])
            self._adapt()

    def get_weights(self) -> list[ArrayLike]:
        """Return parameters of the scaling.

        Returns:
            list[ArrayLike]: List with three elements in the following order:
            ``self.data_min``, ``self.data_max``, ``self.feature_range``

        """
        return [self.feature_range[0], self.feature_range[1], self.data_min, self.data_max, self.feature_range]

    def set_weights(self, weights: list[ArrayLike]) -> None:
        """Set parameters of the scaling.

        Args:
            weights (list[ArrayLike]): List with three elements in the following order:
            ``data_min``, ``data_max``, ``feature_range``

        Raises:
            ValueError: If ``feature_range[0] >= feature_range[1]``.

        """
        self.feature_range = tf.convert_to_tensor(weights[2], dtype=tf.float32)
        if self.feature_range[0] >= self.feature_range[1]:
            raise ValueError("Feature range must be strictly increasing.")
        self.data_min = tf.convert_to_tensor(weights[0], dtype=tf.float32)
        self.data_max = tf.convert_to_tensor(weights[1], dtype=tf.float32)

    def adapt(self, data: ArrayLike) -> None:
        """Fit the layer to the min and max of the data. This is done individually for
        each input feature.

        Note:
            So far, this is only tested for 1 dimensional input and output. For higher
            dimensional input and output some functionality might need to be added.

        Args:
            data: _description_

        """
        data = tf.convert_to_tensor(data, dtype=tf.float32)
        self.data_min = tf.math.reduce_min(data, axis=0)
        self.data_max = tf.math.reduce_max(data, axis=0)
        self._adapt()

    def _adapt(self) -> None:
        if tf.math.reduce_any(self.data_min > self.data_max):
            raise RuntimeError(
                f"""self.data_min {self.data_min} cannot be larger than self.data_max
                {self.data_max} for any element."""
            )
        self.scalar = tf.where(
            self.data_max > self.data_min,
            self.data_max - self.data_min,
            tf.ones_like(self.data_min),
        )
        self.min = tf.where(
            self.data_max > self.data_min,
            self.data_min,
            tf.zeros_like(self.data_min),
        )
        self._is_adapted = True


class MinMaxScalerLayer(ScalerLayer, PreprocessingLayer): 
    """Scales the input according to MinMaxScaling.

    See
    https://scikit-learn.org/stable/modules/generated/sklearn.preprocessing.MinMaxScaler.html
    for an explanation of the transform.

    """

    def __init__(
        self,
        data_min: Optional[float | ArrayLike] = None,
        data_max: Optional[float | ArrayLike] = None,
        feature_range: Sequence[float] | np.ndarray | tf.Tensor = (0, 1),
        **kwargs,
    ) -> None:
        super().__init__(data_min, data_max, feature_range, **kwargs)
        self._name: str = "MinMaxScalerLayer"

    def call(self, inputs: tf.Tensor) -> tf.Tensor: 
        if not self.is_adapted:
            print(np.greater_equal(self.data_min, self.data_max))
            raise RuntimeError(
                """The layer has not been adapted correctly. Call ``adapt`` before using
                the layer or set the ``data_min`` and ``data_max`` values manually.
                """
            )

        # Ensure the dtype is correct.
        inputs = tf.convert_to_tensor(inputs, dtype=tf.float32)
        scaled_data = (inputs - self.min) / self.scalar
        return (
            scaled_data * (self.feature_range[1] - self.feature_range[0])
        ) + self.feature_range[0]


class MinMaxUnScalerLayer(ScalerLayer, tf.keras.layers.Layer):
    """Unscales the input by applying the inverse transform of ``MinMaxScalerLayer``.

    See
    https://scikit-learn.org/stable/modules/generated/sklearn.preprocessing.MinMaxScaler.html
    for an explanation of the transformation.

    """

    def __init__(
        self,
        data_min: Optional[float | ArrayLike] = None,
        data_max: Optional[float | ArrayLike] = None,
        feature_range: Sequence[float] | np.ndarray | tf.Tensor = (0, 1),
        **kwargs, 
    ) -> None:
        super().__init__(data_min, data_max, feature_range, **kwargs)
        self._name: str = "MinMaxUnScalerLayer"

    def call(self, inputs: tf.Tensor) -> tf.Tensor: 
        if not self._is_adapted:
            raise RuntimeError(
                """The layer has not been adapted correctly. Call ``adapt`` before using
                the layer or set the ``data_min`` and ``data_max`` values manually."""
            )

        # Ensure the dtype is correct.
        inputs = tf.convert_to_tensor(inputs, dtype=tf.float32)
        unscaled_data = (inputs - self.feature_range[0]) / (
            self.feature_range[1] - self.feature_range[0]
        )
        return unscaled_data * self.scalar + self.min
