import unittest
import numpy as np

from opm.ml.ml_tools.scaler_layers import (
    ScalerLayer,
    MinMaxScalerLayer,
    MinMaxUnScalerLayer,
)


class TestScalerLayer(unittest.TestCase):

    def test_init_feature_range_error(self):
        with self.assertRaises(ValueError):
            ScalerLayer(feature_range=(1, 0))

        with self.assertRaises(ValueError):
            ScalerLayer(feature_range=(0, 0))

    def test_adapt_sets_min_max_and_scalar(self):
        data = np.array([[1, 10], [3, 30], [2, 20]], dtype=float)
        layer = ScalerLayer()
        layer.adapt(data)

        self.assertTrue(layer.is_adapted)
        self.assertTrue(np.allclose(layer.data_min, np.array([1, 10])))
        self.assertTrue(np.allclose(layer.data_max, np.array([3, 30])))

        # scalar = max - min
        self.assertTrue(np.allclose(layer.scalar, np.array([2, 20])))
        self.assertTrue(np.allclose(layer.min, np.array([1, 10])))

    def test_zero_range_dimension_uses_scalar_1(self):
        data = np.array([[5, 10], [5, 20]]) 
        layer = ScalerLayer()
        layer.adapt(data)

        self.assertTrue(np.allclose(layer.scalar, np.array([1.0, 10.0])))
        self.assertTrue(np.allclose(layer.min, np.array([0.0, 10.0])))

    def test_set_get_weights_roundtrip(self):
        data_min = np.array([1, 2], dtype=float)
        data_max = np.array([5, 10], dtype=float)
        fr = np.array([0.0, 1.0], dtype=float)

        layer = ScalerLayer(data_min, data_max, fr)
        w = layer.get_weights()

        layer2 = ScalerLayer()
        layer2.set_weights(w)

        w2 = layer2.get_weights()

        for a, b in zip(w, w2):
            self.assertTrue(np.allclose(a, b))


class TestMinMaxScalerLayer(unittest.TestCase):

    def test_call_without_adapt_raises(self):
        layer = MinMaxScalerLayer()
        with self.assertRaises(RuntimeError):
            _ = layer.call([1, 2, 3])

    def test_scaling_simple(self):
        data = np.array([0, 5, 10], dtype=float)
        layer = MinMaxScalerLayer()
        layer.adapt(data)

        x = np.array([0, 5, 10], dtype=float)
        y = layer(x)

        # Expect 0 → 0, 5 → 0.5, 10 → 1.0
        self.assertTrue(np.allclose(y, np.array([0.0, 0.5, 1.0])))

    def test_scaling_custom_range(self):
        data = np.array([0, 10], dtype=float)
        layer = MinMaxScalerLayer(feature_range=(2, 4))
        layer.adapt(data)

        # 0 → 2, 5 → 3, 10 → 4
        x = np.array([0, 5, 10])
        y = layer(x)

        self.assertTrue(np.allclose(y, np.array([2.0, 3.0, 4.0])))

    def test_inverse_scaling_roundtrip(self):
        data = np.linspace(0, 100, 6)
        scaler = MinMaxScalerLayer()
        scaler.adapt(data)

        unscaler = MinMaxUnScalerLayer()
        unscaler.set_weights(scaler.get_weights())

        x = np.array([0, 25, 50, 75, 100], dtype=float)
        y = scaler(x)
        z = unscaler(y)

        self.assertTrue(np.allclose(x, z))

    def test_scaler_min_equals_max(self):
        data = np.array([3, 3, 3], dtype=float)
        layer = MinMaxScalerLayer()
        layer.adapt(data)

        x = np.array([3], dtype=float)
        y = layer(x)

        self.assertEqual(y[0], layer.feature_range[0])


class TestMinMaxUnScalerLayer(unittest.TestCase):

    def test_call_without_adapt_raises(self):
        layer = MinMaxUnScalerLayer()
        with self.assertRaises(RuntimeError):
            _ = layer.call([0.1, 0.2])

    def test_unscaling(self):
        data_min = np.array([0.0])
        data_max = np.array([10.0])
        layer = MinMaxUnScalerLayer(data_min, data_max)

        # Inverse of [0 → 0, 0.5 → 5, 1 → 10]
        x = np.array([0.0, 0.5, 1.0])
        y = layer(x)

        self.assertTrue(np.allclose(y, np.array([0.0, 5.0, 10.0])))

    def test_custom_range_inverse(self):
        data = np.array([0, 10])
        scaler = MinMaxScalerLayer(feature_range=(-2, 2))
        scaler.adapt(data)

        unscaler = MinMaxUnScalerLayer()
        unscaler.set_weights(scaler.get_weights())

        scaled = scaler([0, 10, 5])
        unscaled = unscaler(scaled)

        self.assertTrue(np.allclose(unscaled, np.array([0.0, 10.0, 5.0])))


if __name__ == "__main__":
    unittest.main()
