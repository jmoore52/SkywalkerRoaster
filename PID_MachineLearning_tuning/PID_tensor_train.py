import sys
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense

# Load the dataset
data = pd.read_csv('roaster_data.csv', names=['timestamp', 'heater_value', 'fan_value', 'temperature', 'env_temp'])

# Convert timestamp to elapsed time
data['timestamp'] = pd.to_datetime(data['timestamp'], unit='s')  # Assuming the timestamp is in seconds
data['time_diff'] = data['timestamp'].diff().dt.total_seconds().fillna(0)  # Time difference in seconds

# Calculate the rate of temperature change
data['temp_rate_of_change'] = data['temperature'].diff() / data['time_diff']
data['temp_rate_of_change'] = data['temp_rate_of_change'].fillna(0)  # Fill NaN values

# Drop unnecessary columns (e.g., timestamp)
data = data.drop(columns=['timestamp'])

# Features (X) include fan, temperature, environment temp, time difference, and rate of temperature change
X = data[['fan_value', 'temperature', 'env_temp', 'time_diff', 'temp_rate_of_change']]

# Target (y) is the heater value we want to predict
y = data['heater_value']

# Split the data into training and testing datasets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Standardize the data (important for neural networks)
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_test = scaler.transform(X_test)

# Define the neural network architecture
model = Sequential([
    Dense(64, input_dim=X_train.shape[1], activation='relu'),  # First hidden layer
    Dense(32, activation='relu'),  # Second hidden layer
    Dense(1, activation='linear')  # Output layer (predicting heater value)
])

# Compile the model
model.compile(optimizer='adam', loss='mse', metrics=['mae'])

# Train the model
history = model.fit(X_train, y_train, validation_data=(X_test, y_test), epochs=50, batch_size=32)

# Evaluate the model
loss, mae = model.evaluate(X_test, y_test)
print(f"Test Mean Absolute Error: {mae}")

# Predict on new data
predictions = model.predict(X_test)

# Save the trained model to a file
model.save('roaster_pid_model.keras')

# Example: Loading the trained model
# model = tf.keras.models.load_model('roaster_pid_model.keras')
