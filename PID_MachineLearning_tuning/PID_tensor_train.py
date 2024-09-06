import sys
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense

# Load the dataset (assuming the CSV has these columns in the correct order)
data = pd.read_csv('roaster_data.csv', names=['time', 'heater_value', 'fan_value', 'bean_temp', 'env_temp'])

# Create the target temperature by shifting the 'bean_temp' column by one row (representing the future bean temp)
data['future_bean_temp'] = data['bean_temp'].shift(-1)

# Drop the last row (as it won't have a future temp value)
data = data.dropna()

# Define features (X) and target (y)
X = data[['bean_temp', 'fan_value', 'future_bean_temp', 'env_temp']]  # Features: current bean temp, fan speed, target temp, and env temp
y = data['heater_value']  # Target: heater value

# Split the data into training and testing sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Standardize the data
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_test = scaler.transform(X_test)

# Build the neural network model
model = Sequential([
    Dense(64, input_dim=X_train.shape[1], activation='relu'),  # First hidden layer
    Dense(32, activation='relu'),  # Second hidden layer
    Dense(1, activation='linear')  # Output layer (predict heater value)
])

# Compile the model
model.compile(optimizer='adam', loss='mse', metrics=['mae'])

# Train the model with more epochs for deeper training
history = model.fit(X_train, y_train, validation_data=(X_test, y_test), epochs=100, batch_size=32)

# Evaluate the model on the test set
loss, mae = model.evaluate(X_test, y_test)
print(f"Test Mean Absolute Error: {mae}")

# Save the trained model as a .tflite file for deployment on microcontrollers
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()

# Save the model to a file
with open('roaster_pid_model.tflite', 'wb') as f:
    f.write(tflite_model)
