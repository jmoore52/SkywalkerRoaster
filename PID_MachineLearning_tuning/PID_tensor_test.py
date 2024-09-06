import tensorflow as tf
import joblib
import numpy as np

# Load the trained model
model = tf.keras.models.load_model('roaster_pid_model.keras')

# Load the scaler
scaler = joblib.load('scaler.pkl')

# Function to predict future bean temperature
def predict_future_bean_temp(heater_value, fan_value, current_temp, env_temp):
    # Prepare the input data
    input_data = np.array([[heater_value, fan_value, current_temp, env_temp]])

    # Scale the input data
    input_data_scaled = scaler.transform(input_data)

    # Predict using the model
    predicted_temp = model.predict(input_data_scaled)

    return predicted_temp[0][0]

# Example usage
if __name__ == "__main__":
    # Example input values
    heater_value = 10  # Heater value
    fan_value = 50     # Fan value
    current_temp = 45.0  # Current bean temperature
    env_temp = 43.0  # Environment temperature

    # Predict future bean temperature
    predicted_temp = predict_future_bean_temp(heater_value, fan_value, current_temp, env_temp)

    # Output the prediction
    print(f"Predicted Future Bean Temperature: {predicted_temp}")
