# PID_tensor_predict.py

import tensorflow as tf
import numpy as np
import pickle

# Load the trained model
model = tf.keras.models.load_model('roaster_pid_model.keras')

# Load the scaler
with open('scaler.pkl', 'rb') as f:
    scaler = pickle.load(f)

# Function to predict heater value based on inputs
def predict_heater(fan_value, current_temp, env_temp, target_temp):
    # Prepare the input data as a NumPy array
    input_data = np.array([[fan_value, current_temp, env_temp, target_temp]])

    # Standardize the input data using the same scaler as during training
    input_data_scaled = scaler.transform(input_data)

    # Make the prediction
    predicted_heater = model.predict(input_data_scaled)

    return predicted_heater[0][0]

# Example usage
fan_value = 50
current_temp = 65.0
env_temp = 43.5
target_temp = 70.0

predicted_heater = predict_heater(fan_value, current_temp, env_temp, target_temp)
print(f"Predicted Heater Value: {predicted_heater:.2f}")
