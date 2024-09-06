import sys
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense
import joblib

# Load the roaster data
data = pd.read_csv('roaster_data.csv', names=['time', 'heater_value', 'target_temp', 'bean_temp', 'env_temp'])

# Drop the time column as we won't be using it
data = data.drop(columns=['time'])

# Split the data into features (X) and target (y)
X = data[['fan_value', 'bean_temp', 'env_temp', 'target_temp']]
y = data['heater_value']

# Split into training and test data
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Scale the data (important for neural networks)
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_test = scaler.transform(X_test)

# Save the scaler for future use
joblib.dump(scaler, 'scaler.pkl')

# Define the neural network model
model = Sequential([
    Dense(64, input_dim=X_train.shape[1], activation='relu'),  # Hidden layer
    Dense(32, activation='relu'),  # Second hidden layer
    Dense(1, activation='linear')  # Output layer (predicts heater value)
])

# Compile the model
model.compile(optimizer='adam', loss='mse', metrics=['mae'])

# Train the model
history = model.fit(X_train, y_train, validation_data=(X_test, y_test), epochs=50, batch_size=32)

# Evaluate the model
loss, mae = model.evaluate(X_test, y_test)
print(f"Test Mean Absolute Error: {mae}")

# Save the trained model
model.save('roaster_pid_model.keras')

# Save the model in HDF5 format as well (legacy format)
model.save('roaster_pid_model.h5')
