import tensorflow as tf
import joblib
import numpy as np

# Load the trained model
model = tf.keras.models.load_model('roaster_pid_model.keras')

# Load the scaler
scaler = joblib.load('scaler.pkl')

# Function to predict heater value
def predict_heater_value(fan_value, current_temp, env_temp, target_temp):
    # Prepare the input data
    input_data = np.array([[fan_value, current_temp, env_temp, target_temp]])

    # Scale the input data
    input_data_scaled = scaler.transform(input_data)

    # Predict using the model
    predicted_heater_value = model.predict(input_data_scaled)

    return predicted_heater_value[0][0]

# Example usage
if __name__ == "__main__":
    # Example input values
    fan_value = 50  # Fan value (adjust this as needed)
    current_temp = 45.0  # Current bean temperature
    env_temp = 43.0  # Environment temperature
    target_temp = 60.0  # Desired target temperature

    # Predict heater value
    predicted_heater_value = predict_heater_value(fan_value, current_temp, env_temp, target_temp)

    # Output the prediction
    print(f"Predicted Heater Value: {predicted_heater_value}")
