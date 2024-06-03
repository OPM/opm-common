from sklearn.preprocessing import MinMaxScaler
from sklearn.metrics import mean_squared_error
from keras.models import Sequential
from keras.layers import Dense
from numpy import asarray
from matplotlib import pyplot
import numpy as np
import pandas as pd


def computeBCKrw(satW, lambdaparam):
  kr = pow(satW, 2.0/lambdaparam + 3.0)
  return kr

#def computeBCKrn(satW, lambdaparam):
#  Sn = 1.0 - satW;
#  exponent = 2.0/lambdaparam + 1.0
#  kr = Sn*Sn*(1. - pow(satW, exponent))
#  return kr


# sw = np.linspace(0, 1, num=1001, endpoint=True)
sw = np.linspace(0, 1, 10001).reshape( (10001, 1) )


lambdaparam = 2

BCKrw = computeBCKrw(sw, lambdaparam)
#BCKrn = computeBCKrn(sw, lambdaparam)


# define the dataset
x = sw
y = np.array([BCKrw])

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


df = pd.DataFrame({'Id': x_plot[:, 0], 'Amount': yhat_plot[:, 0].astype(float)})


def f(a):
    a = df.loc[df['Id'] == a, 'Amount']
    #for no match
    if a.empty:
        return 'no match'
    #for multiple match
    elif len(a) > 1:
        return a
    else:
    #for match one value only
        return a.item()

# plot x vs y
pyplot.plot(x_plot,y_plot, label='Actual')
# plot x vs yhat
pyplot.plot(x_plot,yhat_plot, label='Predicted')
pyplot.title('Input (x) versus Output (y)')
pyplot.xlabel('Input Variable (x)')
pyplot.ylabel('Output Variable (y)')
pyplot.legend()
pyplot.savefig('filename.png', dpi=1200)
pyplot.show()

#save model
from kerasify import export_model
export_model(model, 'example.modelBCkrw')
