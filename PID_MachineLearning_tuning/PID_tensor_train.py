# pip install tensorflow pandas scikit-learn
#
import sys
sys.path.append('/usr/local/lib/python3.11/site-packages')  # Replace with actual path


import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler

# Load the dataset
data = pd.read_csv('roaster_data.csv', names=['timestamp', 'heater_value', 'fan_value', 'temperature', 'env_temp'])

# Drop unnecessary columns (e.g., timestamp)
data = data.drop(columns=['timestamp'])

# Split the data into features (X) and target (y)
X = data[['fan_value', 'temperature', 'env_temp']]
y = data['heater_value']

# Split into training and testing data
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Standardize the data (important for neural networks)
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_test = scaler.transform(X_test)


import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense

# Define the neural network architecture
model = Sequential([
    Dense(64, input_dim=X_train.shape[1], activation='relu'),  # Hidden layer
    Dense(32, activation='relu'),  # Second hidden layer
    Dense(1, activation='linear')  # Output layer (predict heater value)
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
model.save('roaster_pid_model.h5')

# Load the trained model
model = tf.keras.models.load_model('roaster_pid_model.h5')

# Make predictions based on new input (e.g., live data from the roaster)
new_data = [[fan_value, current_temp, env_temp]]  # Example new data
new_data_scaled = scaler.transform(new_data)
predicted_heater_value = model.predict(new_data_scaled)




