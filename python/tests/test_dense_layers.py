import io
import sys

import unittest
import numpy as np

from opm.ml.ml_tools.dense_layers import (
    linear, relu, softplus, sigmoid, tanh, hard_sigmoid,
    Dense, Sequential, ACTIVATIONS
)


class TestActivations(unittest.TestCase):

    def test_linear(self):
        x = np.array([1, -2, 3])
        y = linear(x)
        self.assertTrue(np.allclose(y, x))

    def test_relu(self):
        x = np.array([-1.0, 0.0, 2.0])
        y = relu(x)
        self.assertTrue(np.allclose(y, np.array([0.0, 0.0, 2.0])))

    def test_softplus(self):
        x = np.array([0.0])
        y = softplus(x)
        self.assertTrue(np.allclose(y, np.log1p(np.exp(x))))

    def test_sigmoid(self):
        x = np.array([0.0])
        y = sigmoid(x)
        self.assertTrue(np.allclose(y, np.array([0.5])))

    def test_tanh(self):
        x = np.array([0.0])
        y = tanh(x)
        self.assertTrue(np.allclose(y, np.array([0.0])))

    def test_hard_sigmoid(self):
        x = np.array([-10.0, 0.0, 10.0])
        y = hard_sigmoid(x)
        exp = np.clip(0.2 * x + 0.5, 0.0, 1.0)
        self.assertTrue(np.allclose(y, exp))

    def test_activation_mapping(self):
        for _, fn in ACTIVATIONS.items():
            self.assertTrue(callable(fn))
        self.assertEqual(ACTIVATIONS["linear"], linear)
        self.assertEqual(ACTIVATIONS["relu"], relu)


class TestDense(unittest.TestCase):

    def test_forward_shape(self):
        layer = Dense(input_dim=4, output_dim=3, activation="linear")
        x = np.random.randn(2, 4).astype(np.float32)
        y = layer.forward(x)
        self.assertEqual(y.shape, (2, 3))

    def test_forward_relu(self):
        layer = Dense(2, 2, activation="relu")
        layer.weights = np.array([[1, -1], [1, -1]], dtype=np.float32)
        layer.biases = np.array([0.0, 0.0], dtype=np.float32)

        x = np.array([[1.0, 1.0]], dtype=np.float32)
        y = layer.forward(x)

        # dot = [2, -2], relu → [2, 0]
        self.assertTrue(np.allclose(y, np.array([[2.0, 0.0]])))

    def test_set_get_weights(self):
        layer = Dense(3, 2)
        w = np.array([[1, 2], [3, 4], [5, 6]], dtype=np.float32)
        b = np.array([0.1, 0.2], dtype=np.float32)

        layer.set_weights([w, b])
        w2, b2 = layer.get_weights()

        self.assertTrue(np.allclose(w, w2))
        self.assertTrue(np.allclose(b, b2))

    def test_get_config(self):
        layer = Dense(5, 7, activation="softplus")
        cfg = layer.get_config()
        self.assertEqual(cfg["input_dim"], 5)
        self.assertEqual(cfg["output_dim"], 7)
        self.assertEqual(cfg["activation"], "softplus")


class TestSequential(unittest.TestCase):

    def test_forward(self):
        model = Sequential([
            Dense(3, 4, activation="relu"),
            Dense(4, 2, activation="linear")
        ])
        x = np.random.randn(5, 3).astype(np.float32)
        y = model.forward(x)
        self.assertEqual(y.shape, (5, 2))

    def test_weights_roundtrip(self):
        model = Sequential([
            Dense(2, 3),
            Dense(3, 1)
        ])

        original = model.get_weights()
        model.set_weights(original)
        new = model.get_weights()

        for (w1, b1), (w2, b2) in zip(original, new):
            self.assertTrue(np.allclose(w1, w2))
            self.assertTrue(np.allclose(b1, b2))

    def test_summary_output(self):
        model = Sequential([
            Dense(2, 3, activation="relu"),
            Dense(3, 1, activation="sigmoid")
        ])

        buf = io.StringIO()
        sys_stdout_backup = sys.stdout
        sys.stdout = buf

        model.summary()

        sys.stdout = sys_stdout_backup
        out = buf.getvalue()

        self.assertIn("Dense(2 -> 3), activation=relu", out)
        self.assertIn("Dense(3 -> 1), activation=sigmoid", out)


if __name__ == "__main__":
    unittest.main()
