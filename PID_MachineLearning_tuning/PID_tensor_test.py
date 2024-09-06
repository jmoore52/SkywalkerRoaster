import tensorflow as tf
import joblib
import numpy as np

# Load the trained model
model = tf.keras.models.load_model('roaster_pid_model.keras')

# Load the saved scaler used during training
scaler = joblib.load('scaler.pkl')

# Function to predict heater value
def predict_heater_value(fan_value, current_temp, env_temp, target_temp):
    # Prepare the input data (fan, current temperature, environment temperature, target temperature)
    input_data = np.array([[fan_value, current_temp, env_temp, target_temp]])

    # Apply the scaler to the input data (it must be scaled with the same scaler used in training)
    input_data_scaled = scaler.transform(input_data)

    # Use the trained model to make a prediction
    predicted_heater_value = model.predict(input_data_scaled)

    return predicted_heater_value[0][0]  # Return the predicted heater value


# Example usage of the function with test values:
if __name__ == "__main__":
    # Define the test input values
    fan_value = 50  # Example fan value (between 0-100)
    current_temp = 45.0  # Example current bean temperature
    env_temp = 45.0  # Example environment temperature
    target_temp = 45.1  # Target temperature to reach

    # Call the prediction function
    predicted_heater_value = predict_heater_value(fan_value, current_temp, env_temp, target_temp)

    # Print the predicted heater value
    print(f"Predicted Heater Value: {predicted_heater_value}")
