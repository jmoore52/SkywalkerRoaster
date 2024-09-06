import sys
import numpy as np
import pandas as pd
from sklearn.preprocessing import StandardScaler
import tensorflow as tf

# Load the saved model
model = tf.keras.models.load_model('roaster_pid_model.keras')

# Load the scaler used during training
scaler = StandardScaler()


# Assume the scaler was saved, so we load the fitted scaler
# In practice, you would have saved the scaler used during training and load it here

# Placeholder for now
# Manually fitted values, or in practice, load from saved scaler fit on training data
def load_scaler():
    # Dummy placeholder values, replace with actual training data values
    dummy_data = np.array([[50, 60, 70], [50, 60, 70]])  # Example data
    scaler.fit(dummy_data)
    return scaler


scaler = load_scaler()


def predict_heater_value(fan_value, current_temp, env_temp, target_temp):
    # Prepare input data
    input_data = np.array([[fan_value, current_temp, env_temp, target_temp]])

    # Scale the input data using the scaler from training
    input_data_scaled = scaler.transform(input_data)

    # Make the prediction
    predicted_heater = model.predict(input_data_scaled)

    # Ensure that the predicted value is a reasonable number
    if np.isnan(predicted_heater):
        print("Prediction resulted in NaN, please check your input values.")
        return None

    return predicted_heater[0][0]  # Return the predicted value


if __name__ == "__main__":
    # Example inputs
    fan_value = 50
    current_temp = 45.0  # Current bean temperature
    env_temp = 30.0  # Environment temperature
    target_temp = 60.0  # Target temperature to reach

    # Predict the heater value
    predicted_heater_value = predict_heater_value(fan_value, current_temp, env_temp, target_temp)

    if predicted_heater_value is not None:
        print(f"Predicted Heater Value: {predicted_heater_value}")
    else:
        print("Failed to predict the heater value.")
