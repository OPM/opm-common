from sklearn.preprocessing import MinMaxScaler
from sklearn.metrics import mean_squared_error
from keras.models import Sequential
from keras.layers import Dense
from numpy import asarray
from matplotlib import pyplot
import numpy as np
import pandas as pd


def computeVGKrw(satW, lambdaparam):
    r = 1.0 - pow(1.0 - pow(satW, 1/lambdaparam), lambdaparam);
    return pow(satW,0.5)*r*r;

#def computeVGKrn(satW, lambdaparam):
#  Sn = 1.0 - satW;
#  exponent = 2.0/lambdaparam + 1.0
#  kr = Sn*Sn*(1. - pow(satW, exponent))
#  return kr


# sw = np.linspace(0, 1, num=1001, endpoint=True)
sw = np.linspace(0, 1, 10001).reshape( (10001, 1) )


lambdaparam = 0.78

VGKrw = computeVGKrw(sw, lambdaparam)
#VGKrn = computeVGKrn(sw, lambdaparam)


# define the dataset
x = sw
y = np.array([VGKrw])

print(x.min(), x.max(), y.min(), y.max())
# reshape arrays into into rows and cols
x = x.reshape((len(x), 1))
y = y.reshape((10001, 1))
# separately scale the input and output variables
scale_x = MinMaxScaler()
x = scale_x.fit_transform(x)
scale_y = MinMaxScaler()
y = scale_y.fit_transform(y)
print(x.min(), x.max(), y.min(), y.max())

# design the neural network model
model = Sequential()
model.add(Dense(3, input_dim=1, activation='relu', kernel_initializer='he_uniform'))
# model.add(Dense(10, activation='relu', kernel_initializer='he_uniform'))
model.add(Dense(10, activation='relu', kernel_initializer='he_uniform'))
model.add(Dense(10, activation='relu', kernel_initializer='he_uniform'))
model.add(Dense(10, activation='relu', kernel_initializer='he_uniform'))
model.add(Dense(10, activation='relu', kernel_initializer='he_uniform'))
model.add(Dense(10, activation='relu', kernel_initializer='he_uniform'))
# model.add(Dense(1000, activation='relu', kernel_initializer='he_uniform'))
model.add(Dense(1))

# define the loss function and optimization algorithm
model.compile(loss='mse', optimizer='adam')
# ft the model on the training dataset
model.fit(x, y, epochs=1000, batch_size=100, verbose=0)
# make predictions for the input data
yhat = model.predict(x)
# inverse transforms
x_plot = scale_x.inverse_transform(x)
y_plot = scale_y.inverse_transform(y)
yhat_plot = scale_y.inverse_transform(yhat)
# report model error
print('MSE: %.3f' % mean_squared_error(y_plot, yhat_plot))
print(yhat_plot)
print('blah: %.3f' % mean_squared_error(y_plot, yhat_plot))


# plot x vs y
pyplot.plot(x_plot,y_plot, label='Actual')
# plot x vs yhat
pyplot.plot(x_plot,yhat_plot, label='Predicted')
pyplot.title('Input (x) versus Output (y)')
pyplot.xlabel('Input Variable (x)')
pyplot.ylabel('Output Variable (y)')
pyplot.legend()
pyplot.savefig('filenamekrw.png', dpi=1200)
pyplot.show()
#save model
from kerasify import export_model
export_model(model, 'example.modelVGkrw')
