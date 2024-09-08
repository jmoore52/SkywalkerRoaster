import numpy as np
import joblib
import tensorflow as tf

# Load the trained model and scaler
model = tf.keras.models.load_model('roaster_pid_model.keras')
scaler = joblib.load('scaler.pkl')


# Function to predict the heater value
def predict_heater_value(current_bean_temp, fan_value, target_bean_temp, current_env_temp):
    # Prepare input data
    input_data = np.array([[current_bean_temp, fan_value, target_bean_temp, current_env_temp]])

    # Standardize the input data
    input_data_scaled = scaler.transform(input_data)

    # Predict the heater value using the model
    predicted_heater_value = model.predict(input_data_scaled)[0][0]

    # Clip the predicted heater value to the range 0-100
    predicted_heater_value = np.clip(predicted_heater_value, 0, 100)

    return predicted_heater_value


# Example usage with sample input
if __name__ == "__main__":
    # Define the current state and target for prediction
    current_bean_temp = 27.22  # Example current bean temperature
    fan_value = 50  # Example fan speed (0-100%)
    target_bean_temp = 27.43  # Desired future bean temperature (target temp)
    current_env_temp = 26.72  # Example current environmental temperature

    # Make the prediction
    predicted_heater_value = predict_heater_value(current_bean_temp, fan_value, target_bean_temp, current_env_temp)

    # Print the prediction
    print(f"Predicted Heater Value: {predicted_heater_value:.2f}")
