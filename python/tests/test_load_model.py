import unittest
import numpy as np
import tempfile

from opm.ml.ml_tools.dense_layers import Dense, Sequential
from opm.ml.ml_tools.scaler_layers import MinMaxScalerLayer, MinMaxUnScalerLayer
from opm.ml.ml_tools.load_model import load_model
from opm.ml.ml_tools.kerasify import export_model


class TestLoadModel(unittest.TestCase):

    def _compare_dense_layers(self, layer1, layer2):
        w1, b1 = layer1.get_weights()
        w2, b2 = layer2.get_weights()
        self.assertTrue(np.allclose(w1, w2))
        self.assertTrue(np.allclose(b1, b2))
        self.assertEqual(layer1.activation_name, layer2.activation_name)

    def test_dense_roundtrip(self):
        model = Sequential([
            Dense(2, 3, activation="relu"),
            Dense(3, 1, activation="sigmoid")
        ])

        with tempfile.NamedTemporaryFile(suffix=".bin") as tmp:
            export_model(model, tmp.name)
            loaded_model = load_model(tmp.name)

        self.assertEqual(len(model.layers), len(loaded_model.layers))
        for l1, l2 in zip(model.layers, loaded_model.layers):
            self._compare_dense_layers(l1, l2)

        x = np.random.randn(5, 2).astype(np.float32)
        y_orig = model.forward(x)
        y_loaded = loaded_model.forward(x)
        self.assertTrue(np.allclose(y_orig, y_loaded))

    def test_scaler_roundtrip(self):
        scaler = MinMaxScalerLayer(feature_range=(0, 1))
        unscaler = MinMaxUnScalerLayer(feature_range=(-1, 1))

        # Use single-feature data for scalers
        data = np.array([[0], [1], [2]], dtype=np.float32)
        scaler.adapt(data)
        unscaler.adapt(scaler.forward(data))
        scaler.data_min = float(scaler.data_min.item())
        scaler.data_max = float(scaler.data_max.item())
        unscaler.data_min = float(unscaler.data_min.item())
        unscaler.data_max = float(unscaler.data_max.item())
         
        model = Sequential([scaler, unscaler])

        with tempfile.NamedTemporaryFile(suffix=".bin") as tmp:
            export_model(model, tmp.name)
            loaded_model = load_model(tmp.name)

        self.assertEqual(len(model.layers), len(loaded_model.layers))
        for l1, l2 in zip(model.layers, loaded_model.layers):
            self.assertTrue(np.allclose(l1.data_min, l2.data_min))
            self.assertTrue(np.allclose(l1.data_max, l2.data_max))
            self.assertTrue(np.allclose(l1.feature_range, l2.feature_range))

        x = np.random.randn(4, 2).astype(np.float32)
        y_orig = model.forward(x)
        y_loaded = loaded_model.forward(x)
        self.assertTrue(np.allclose(y_orig, y_loaded))

    def test_full_model(self):
        scaler = MinMaxScalerLayer(feature_range=(0, 1))
        scaler.adapt(np.random.rand(10, 1))
        scaler.data_min = float(scaler.data_min.item())
        scaler.data_max = float(scaler.data_max.item())

        model = Sequential([
            scaler,
            Dense(2, 3, activation="relu"),
            Dense(3, 1, activation="linear")
        ])

        with tempfile.NamedTemporaryFile(suffix=".bin") as tmp:
            export_model(model, tmp.name)
            loaded_model = load_model(tmp.name)

        self.assertEqual(len(model.layers), len(loaded_model.layers))

        # Check scaler layer
        self.assertTrue(np.allclose(model.layers[0].data_min, loaded_model.layers[0].data_min))
        self.assertTrue(np.allclose(model.layers[0].data_max, loaded_model.layers[0].data_max))

        # Check Dense layers
        for l1, l2 in zip(model.layers[1:], loaded_model.layers[1:]):
            self.assertTrue(np.allclose(l1.get_weights()[0], l2.get_weights()[0]))
            self.assertTrue(np.allclose(l1.get_weights()[1], l2.get_weights()[1]))
            self.assertEqual(l1.activation_name, l2.activation_name)

        x = np.random.randn(5, 2).astype(np.float32)
        y_orig = model.forward(x)
        y_loaded = loaded_model.forward(x)
        self.assertTrue(np.allclose(y_orig, y_loaded))


if __name__ == "__main__":
    unittest.main()
