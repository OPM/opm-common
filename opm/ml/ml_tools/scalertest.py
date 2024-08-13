#  Copyright (c) 2024 NORCE
#  Copyright (c) 2024 UiB

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

from __future__ import annotations

import pathlib

import numpy as np

from tensorflow import keras

from kerasify import export_model

from scaler_layers import MinMaxScalerLayer, MinMaxUnScalerLayer

from keras.models import Sequential
from keras.layers import Conv2D, Dense, Flatten, Activation, MaxPooling2D, Dropout, BatchNormalization, ELU, Embedding, LSTM

savepath = pathlib.Path(__file__).parent / "model_with_scaler_layers.opm"

feature_ranges: list[tuple[float, float]] = [(0.0, 1.0), (-3.7, 0.0)]

data: np.ndarray = np.random.uniform(-500, 500, (5, 1))

model = Sequential()
model.add(keras.layers.Input([1]))
model.add(MinMaxScalerLayer(feature_range=(0.0, 1.0)))
model.add(Dense(1, input_dim=1))
model.add(Dense(1, input_dim=1))
model.add(Dense(1, input_dim=1))
model.add(Dense(1, input_dim=1))

model.add(MinMaxUnScalerLayer(feature_range=(-3.7, -1.0)))

export_model(model, str(savepath))
