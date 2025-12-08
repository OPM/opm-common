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

"""Provide MinMax scaler layers in Numpy implementation."""

from __future__ import annotations
from typing import Optional, Sequence, Union
import numpy as np

ArrayLike = Union[np.ndarray, list, float, int]

class ScalerLayer:
    """Base layer for scaling using NumPy, providing min/max functionality."""

    def __init__(
        self,
        data_min: Optional[ArrayLike] = None,
        data_max: Optional[ArrayLike] = None,
        feature_range: Sequence[float] = (0, 1),
    ):
        if feature_range[0] >= feature_range[1]:
            raise ValueError("Feature range must be strictly increasing.")
        self.feature_range = np.array(feature_range, dtype=float)
        self._is_adapted = False

        if data_min is not None and data_max is not None:
            self.data_min = np.array(data_min, dtype=float)
            self.data_max = np.array(data_max, dtype=float)
            self._adapt()

    def adapt(self, data: ArrayLike) -> None:
        """Fit the layer to the min and max of the data."""
        data = np.asarray(data, dtype=float)
        self.data_min = np.min(data, axis=0)
        self.data_max = np.max(data, axis=0)
        self._adapt()

    def _adapt(self):
        """Compute internal scalar and min for scaling."""
        if np.any(self.data_min > self.data_max):
            raise RuntimeError(
                f"data_min {self.data_min} cannot be larger than data_max {self.data_max}"
            )

        scale = self.data_max - self.data_min
        constant_mask = scale == 0
        self.scalar = np.where(constant_mask, 1.0, scale)
        self.min = np.where(constant_mask, 0.0, self.data_min)
        self._constant_mask = constant_mask

        self._is_adapted = True

    @property
    def is_adapted(self) -> bool:
        return self._is_adapted

    def get_weights(self):
        """Return [data_min, data_max, feature_range]."""
        return [self.data_min, self.data_max, self.feature_range]

    def set_weights(self, weights):
        """Set [data_min, data_max, feature_range]."""
        self.data_min, self.data_max, self.feature_range = map(np.array, weights)
        if self.feature_range[0] >= self.feature_range[1]:
            raise ValueError("Feature range must be strictly increasing.")
        self._adapt()


class MinMaxScalerLayer(ScalerLayer):
    """Scales the input according to MinMaxScaling. 
    
    See: 
    https://scikit-learn.org/stable/modules/generated/sklearn.preprocessing.MinMaxScaler.html 
    for an explanation of the transform. 
    """

    def __init__(
        self,
        data_min: Optional[ArrayLike] = None,
        data_max: Optional[ArrayLike] = None,
        feature_range: Sequence[float] = (0, 1),
    ):
        super().__init__(data_min, data_max, feature_range)
        self._name = "MinMaxScalerLayer"

    def get_config(self):
        """Return layer configuration as a dictionary."""
        return {
            "feature_range": self.feature_range.tolist(),
            "data_min": self.data_min.tolist() if hasattr(self, "data_min") else None,
            "data_max": self.data_max.tolist() if hasattr(self, "data_max") else None,
            "is_adapted": self.is_adapted,
            "name": getattr(self, "_name", "MinMaxScalerLayer")
        }
    
    def forward(self, inputs: ArrayLike) -> np.ndarray:
        if not self.is_adapted:
            raise RuntimeError("The layer has not been adapted.")

        inputs = np.asarray(inputs, dtype=float)
        scaled_data = (inputs - self.min) / self.scalar

        scaled_data = np.where(self._constant_mask, 0.0, scaled_data)

        return scaled_data * (self.feature_range[1] - self.feature_range[0]) + self.feature_range[0]

    def __call__(self, inputs: ArrayLike) -> np.ndarray:
        """Enable calling the layer like a Keras layer: layer(x)."""
        return self.forward(inputs)


class MinMaxUnScalerLayer(ScalerLayer):
    """Unscales the input by applying the inverse transform of `MinMaxScalerLayer`.

    See:
    https://scikit-learn.org/stable/modules/generated/sklearn.preprocessing.MinMaxScaler.html
    for an explanation of the transformation.
    """

    def __init__(
        self,
        data_min: Optional[ArrayLike] = None,
        data_max: Optional[ArrayLike] = None,
        feature_range: Sequence[float] = (0, 1),
    ):
        super().__init__(data_min, data_max, feature_range)
        self._name = "MinMaxUnScalerLayer"

    def get_config(self):
        return {
            "feature_range": self.feature_range.tolist(),
            "data_min": self.data_min.tolist() if hasattr(self, "data_min") else None,
            "data_max": self.data_max.tolist() if hasattr(self, "data_max") else None,
            "is_adapted": self.is_adapted,
            "name": getattr(self, "_name", "MinMaxUnScalerLayer")
        }
    
    def forward(self, inputs: ArrayLike) -> np.ndarray:
        if not self.is_adapted:
            raise RuntimeError("The layer has not been adapted.")

        inputs = np.asarray(inputs, dtype=float)
        unscaled = (inputs - self.feature_range[0]) / (
            self.feature_range[1] - self.feature_range[0]
        )

        unscaled = np.where(self._constant_mask, 0.0, unscaled)

        return unscaled * self.scalar + self.min

    def __call__(self, inputs: ArrayLike) -> np.ndarray:
        """Enable calling the layer like a Keras layer: layer(x)."""
        return self.forward(inputs)
