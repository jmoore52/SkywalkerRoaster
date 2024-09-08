# import pandas as pd
# from sklearn.model_selection import train_test_split
# from sklearn.preprocessing import StandardScaler
# import tensorflow as tf
# from tensorflow.keras.models import Sequential
# from tensorflow.keras.layers import Dense
#
# # Load the dataset
# data = pd.read_csv('roaster_data.csv', names=['time', 'heater_value', 'fan_value', 'bean_temp', 'env_temp'])
#
# # Create the target column by shifting the bean_temp by one time step to simulate the "future_bean_temp"
# data['future_bean_temp'] = data['bean_temp'].shift(-1)
# data = data.dropna()  # Remove the last row since it doesn't have a future value
#
# # Split the data into features (X) and target (y)
# X = data[['bean_temp', 'fan_value', 'future_bean_temp', 'env_temp']]
# y = data['heater_value']
#
# # Split into training and testing data
# X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
#
# # Standardize the data
# scaler = StandardScaler()
# X_train = scaler.fit_transform(X_train)
# X_test = scaler.transform(X_test)
#
# # Save the scaler for later use in testing
# import joblib
# joblib.dump(scaler, 'scaler.pkl')
#
# # Define the neural network architecture
# model = Sequential([
#     Dense(128, input_dim=X_train.shape[1], activation='relu'),  # Increased complexity
#     Dense(64, activation='relu'),  # Second hidden layer
#     Dense(1, activation='linear')  # Linear output layer
# ])
#
# # Compile the model
# model.compile(optimizer='adam', loss='mse', metrics=['mae'])
#
# # Train the model
# model.fit(X_train, y_train, validation_data=(X_test, y_test), epochs=200, batch_size=32)
#
# # Evaluate the model
# loss, mae = model.evaluate(X_test, y_test)
# print(f"Test Mean Absolute Error: {mae}")
#
# # Save the trained model to a file
# model.save('roaster_pid_model.keras')

import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense
import joblib

# Load the dataset
data = pd.read_csv('roaster_data.csv', names=['time', 'heater_value', 'fan_value', 'bean_temp', 'env_temp'])

# Detect gaps in time and segment batches
data['time_diff'] = data['time'].diff().fillna(1)
data['batch'] = (data['time_diff'] > 1).cumsum()  # Each gap larger than 1 second is considered a new batch
data = data.drop(columns='time_diff')

# Shift bean temperature by one step to create the "future_bean_temp"
data['future_bean_temp'] = data['bean_temp'].shift(-1)
data = data.dropna()

# Feature engineering: create time-lagged features for bean temperature
lag_seconds = 10  # Example: look back 10 seconds
for lag in range(1, lag_seconds + 1):
    data[f'bean_temp_lag_{lag}'] = data['bean_temp'].shift(lag)

data = data.dropna()  # Drop rows with NaN after creating lagged features

# Select features (bean temp, fan, and environment temp) and target (heater value)
X = data[['fan_value', 'env_temp', 'future_bean_temp'] + [f'bean_temp_lag_{i}' for i in range(1, lag_seconds + 1)]]
y = data['heater_value']

# Split the data into training and testing sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Standardize the data
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_test = scaler.transform(X_test)

# Save the scaler for later use in testing
joblib.dump(scaler, 'scaler.pkl')

# Define a simplified neural network architecture for microcontroller deployment
model = Sequential([
    Dense(32, input_dim=X_train.shape[1], activation='relu'),  # Reduced size for microcontroller
    Dense(16, activation='relu'),  # Reduced second layer
    Dense(1, activation='linear')  # Linear output for predicting heater value
])

# Compile the model
model.compile(optimizer='adam', loss='mse', metrics=['mae'])

# Train the model
model.fit(X_train, y_train, validation_data=(X_test, y_test), epochs=100, batch_size=16)  # Reduced epochs and batch size

# Evaluate the model
loss, mae = model.evaluate(X_test, y_test)
print(f"Test Mean Absolute Error: {mae}")

# Save the trained model to a file
model.save('roaster_pid_model.keras')

# Convert to TensorFlow Lite model for microcontroller deployment
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()

# Save the TensorFlow Lite model
with open('roaster_pid_model.tflite', 'wb') as f:
    f.write(tflite_model)

print("Model has been converted and saved as roaster_pid_model.tflite")

