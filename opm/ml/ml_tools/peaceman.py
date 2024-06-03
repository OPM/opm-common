import csv
import logging
import math

import numpy as np
import pandas as pd
import tensorflow as tf
from keras.layers import Dense
from keras.models import Sequential
from matplotlib import pyplot
from numpy import asarray

# from sklearn.metrics import mean_squared_error
from sklearn.preprocessing import MinMaxScaler

logger = logging.getLogger(__name__)


def computePeaceman(h: float, k: float, r_e: float, r_w: float) -> float:
    r"""Compute the well productivity index (adjusted for density and viscosity) from the
    Peaceman well model.

    .. math::
        WI\cdot\frac{\mu}{\rho} = \frac{2\pi hk}{\ln (r_e/r_w)}

    Parameters:
        h: Thickness of the well block.
        k: Permeability.
        r_e: Equivalent well-block radius.
        r_w: Wellbore radius.

    Returns:
        :math:`WI\cdot\frac{\mu}{\rho}`

    """
    WI = (2 * math.pi * h * k) / (math.log(r_e / r_w))
    return WI


computePeaceman_np = np.vectorize(computePeaceman)

logger.info("Prepare dataset")

# scale h
h_plot = np.linspace(1, 20, 20)
h3 = 6.096 
k3 = 9.86923e-14
#h_plot = np.array([h3])
scale_h = MinMaxScaler()
# h = scale_h.fit_transform(h_plot.reshape(-1, 1)).squeeze(axis=0)
h = scale_h.fit_transform(h_plot.reshape(-1, 1)).squeeze()
#param_h = scale_h.get_params()
#print(scale_h.data_max_)
# scale k
k_plot = np.linspace(1e-13, 1e-11, 20)
#k_plot = np.array([k3])
scale_k = MinMaxScaler()
k = scale_k.fit_transform(k_plot.reshape(-1, 1)).squeeze()
#param_k = scale_k.get_params()
#print(param_k)
# # scale r_e
# r_e_plot = np.logspace(math.log(10), math.log(400), 300)
r_e_plot = np.linspace(10, 300, 600)
#scale_r_e = MinMaxScaler((0.02, 0.5))
scale_r_e = MinMaxScaler()
r_e = scale_r_e.fit_transform(r_e_plot.reshape(-1, 1)).squeeze()
#param_r_e = scale_r_e.get_params()
r_w = np.array([0.0762])
h_v, k_v, r_e_v, r_w_v = np.meshgrid(h, k, r_e, r_w)
h_plot_v, k_plot_v, r_e_plot_v, r_w_v = np.meshgrid(h_plot, k_plot, r_e_plot, r_w)

x = np.stack([h_v.flatten(), k_v.flatten(), r_e_v.flatten(), r_w_v.flatten()], axis=-1)

y = computePeaceman_np(
    h_plot_v.flatten()[..., None],
    k_plot_v.flatten()[..., None],
    r_e_plot_v.flatten()[..., None],
    r_w_v.flatten()[..., None],
)
scale_y = MinMaxScaler()
y_scaled = scale_y.fit_transform(y)

logger.info("Done")

# Write scaling info to file
with open("scales.csv", "w", newline="") as csvfile:
    writer = csv.DictWriter(csvfile, fieldnames=["variable", "min", "max"])
    writer.writeheader()
    writer.writerow(
        {"variable": "h", "min": f"{h_plot.min()}", "max": f"{h_plot.max()}"}
    )
    writer.writerow(
        {"variable": "k", "min": f"{k_plot.min()}", "max": f"{k_plot.max()}"}
    )
    writer.writerow(
        {"variable": "r_e", "min": f"{r_e_plot.min()}", "max": f"{r_e_plot.max()}"}
    )
    writer.writerow({"variable": "r_w", "min": f"{r_w.min()}", "max": f"{r_w.max()}"})
    writer.writerow({"variable": "y", "min": f"{y.min()}", "max": f"{y.max()}"})


# design the neural network model
model = Sequential(
    [
        tf.keras.Input(shape=(4,)),
        # tf.keras.layers.BatchNormalization(),
        Dense(10, activation="sigmoid", kernel_initializer="glorot_normal"),
        Dense(10, activation="sigmoid", kernel_initializer="glorot_normal"),
        Dense(10, activation="sigmoid", kernel_initializer="glorot_normal"),
        Dense(10, activation="sigmoid", kernel_initializer="glorot_normal"),
        Dense(10, activation="sigmoid", kernel_initializer="glorot_normal"),
        Dense(1),
    ]
)

# define the loss function and optimization algorithm
# lr_schedule = tf.keras.optimizers.schedules.ExponentialDecay(
#     0.1, decay_steps=1000, decay_rate=0.96, staircase=False
# )
# model.compile(loss="mse", optimizer=tf.keras.optimizers.Adam(learning_rate=lr_schedule))

reduce_lr = tf.keras.callbacks.ReduceLROnPlateau(
    monitor="loss", factor=0.1, patience=10, verbose=1, min_delta=1e-10
)
model.compile(loss="mse", optimizer=tf.keras.optimizers.Adam(learning_rate=0.1))


# ft the model on the training dataset
logger.info("Train model")
model.fit(x, y_scaled, epochs=5, batch_size=100, verbose=1, callbacks=reduce_lr)

# make predictions for the input data
yhat = model.predict(x)
# inverse transforms
# r_e_plot = scale_r_e.inverse_transform(r_e.reshape(-1, 1)).squeeze()
# y_plot = scale_y.inverse_transform(y)
# yhat_plot = scale_y.inverse_transform(yhat)
# report model error
mse = tf.keras.losses.MeanSquaredError()
logger.info(f"MSE: {mse(y, yhat).numpy():.3f}")


re3 = 60.3473 
rw3 = 0.0762
h3 = 15.24
k3 = 1.97385e-13

#60.3473 0.0762 15.24 1.97385e-13


wi = computePeaceman(h3 , k3 , re3, rw3)

# scale h
h_plot2 = np.array(h3)
# h_plot = np.array([1e-12])
#scale_h = MinMaxScaler()
#scale_h.set_params(param_h)
# h = scale_h.fit_transform(h_plot.reshape(-1, 1)).squeeze(axis=0)
h2 = scale_h.transform(h_plot2.reshape(-1, 1)).squeeze()

# scale k
k_plot2 = np.array(k3)
#scale_k = MinMaxScaler()
#scale_k.set_params(param_k)
k2 = scale_k.transform(k_plot2.reshape(-1, 1)).squeeze()

# # scale r_e
# r_e_plot = np.logspace(math.log(10), math.log(400), 300)
r_e_plot2 = np.array(re3)
#scale_r_e = MinMaxScaler((0.02, 0.5))
#scale_r_e.set_params(param_r_e)
r_e2 = scale_r_e.transform(r_e_plot2.reshape(-1, 1)).squeeze()
r_w2 = np.array(rw3)
h_v2, k_v2, r_e_v2, r_w_v2 = np.meshgrid(h2, k2, r_e2, r_w2)
h_plot_v2, k_plot_v2, r_e_plot_v2, r_w_v2 = np.meshgrid(h_plot2, k_plot2, r_e_plot2, r_w2)
x2 = np.stack([h_v2.flatten(), k_v2.flatten(), r_e_v2.flatten(), r_w_v2.flatten()], axis=-1)
yhat2 = model.predict(x2)
print(r_e2, r_w2, h2, k2)
print(wi, yhat2, scale_y.inverse_transform(yhat2))
# df = pd.DataFrame({"Id": x_plot[:, 0], "Amount": yhat_plot[:, 0].astype(float)})


# def f(a):
#     a = df.loc[df["Id"] == a, "Amount"]
#     # for no match
#     if a.empty:
#         return "no match"
#     # for multiple match
#     elif len(a) > 1:
#         return a
#     else:
#         # for match one value only
#         return a.item()


# Plot w.r.t. r_e
# plot x vs y
for i in [0, 5, 10, 15]:
    try:
        pyplot.figure()
        pyplot.plot(
            r_e_plot,
            computePeaceman_np(
                np.full_like(r_e, h_plot[i]),
                np.full_like(r_e, k_plot[i]),
                r_e_plot,
                np.full_like(r_e, r_w[0]),
            ),
            label="Actual",
        )
        # plot x vs yhat
        pyplot.plot(
            r_e_plot,
            scale_y.inverse_transform(
                model(
                    np.stack(
                        [
                            np.full_like(r_e, h[i]),
                            np.full_like(r_e, k[i]),
                            r_e,
                            np.full_like(r_e, r_w[0]),
                        ],
                        axis=-1,
                    )
                )
            ),
            label="Predicted",
        )
        pyplot.title("Input (x) versus Output (y)")
        pyplot.xlabel("$r_e$")
        pyplot.ylabel(r"$WI\cdot\frac{\mu}{\rho}$")
        pyplot.legend()
        pyplot.savefig(f"plt_r_e_vs_WI_{i}.png", dpi=1200)
        pyplot.show()
        pyplot.close()
    except Exception as e:
        print(e)
        pass

# Plot w.r.t. h
# plot x vs y
try:
    pyplot.figure()
    pyplot.plot(
        h_plot,
        computePeaceman_np(
            h_plot,
            np.full_like(h, k_plot[0]),
            np.full_like(h, r_e_plot[0]),
            np.full_like(h, r_w[0]),
        ),
        label="Actual",
    )
    # plot x vs yhat
    pyplot.plot(
        h_plot,
        scale_y.inverse_transform(
            model(
                np.stack(
                    [
                        h,
                        np.full_like(h, k[0]),
                        np.full_like(h, r_e[0]),
                        np.full_like(h, r_w[0]),
                    ],
                    axis=-1,
                )
            )
        ),
        label="Predicted",
    )
    pyplot.title("Input (x) versus Output (y)")
    pyplot.xlabel("$h$")
    pyplot.ylabel(r"$WI\cdot\frac{\mu}{\rho}$")
    pyplot.legend()
    pyplot.savefig("plt_h_vs_WI.png", dpi=1200)
    pyplot.show()
    pyplot.close()
except Exception as e:
    pass

# Plot w.r.t. k
# plot x vs y
try:
    pyplot.figure()
    pyplot.plot(
        k_plot,
        computePeaceman_np(
            np.full_like(k, h_plot[0]),
            k_plot,
            np.full_like(k, r_e_plot[0]),
            np.full_like(k, r_w[0]),
        ),
        label="Actual",
    )
    # plot x vs yhat
    pyplot.plot(
        k_plot,
        scale_y.inverse_transform(
            model(
                np.stack(
                    [
                        np.full_like(k, h[0]),
                        k,
                        np.full_like(k, r_e[0]),
                        np.full_like(k, r_w[0]),
                    ],
                    axis=-1,
                )
            )
        ),
        label="Predicted",
    )
    pyplot.title("Input (x) versus Output (y)")
    pyplot.xlabel("$k$")
    pyplot.ylabel(r"$WI\cdot\frac{\mu}{\rho}$")
    pyplot.legend()
    pyplot.savefig("plt_k_vs_WI.png", dpi=1200)
    pyplot.show()
    pyplot.close()
except Exception as e:
    pass

# Plot w.r.t. r_w
# plot x vs y
try:
    pyplot.figure()
    pyplot.plot(
        r_w,
        computePeaceman_np(
            np.full_like(r_w, h_plot[0]),
            np.full_like(r_w, k_plot[0]),
            np.full_like(r_w, r_e_plot[0]),
            r_w,
        ),
        label="Actual",
    )
    # plot x vs yhat
    pyplot.plot(
        r_w,
        scale_y.inverse_transform(
            model(
                np.stack(
                    [
                        np.full_like(r_w, h[0]),
                        np.full_like(r_w, k[0]),
                        np.full_like(r_w, r_e[0]),
                        r_w,
                    ],
                    axis=-1,
                )
            )
        ),
        label="Predicted",
    )
    pyplot.title("Input (x) versus Output (y)")
    pyplot.xlabel("$r_w$")
    pyplot.ylabel(r"$WI\cdot\frac{\mu}{\rho}$")
    pyplot.legend()
    pyplot.savefig("plt_r_w_vs_WI.png", dpi=1200)
    pyplot.show()
    pyplot.close()
except Exception as e:
    pass

# save model
#model.save_weights("modelPeaceman.tf")

from kerasify import export_model

export_model(model, "example.modelPeaceman")