import joblib
import pandas as pd
import tensorflow as tf

# Load the trained model
model = tf.keras.models.load_model('roaster_pid_model.keras')

# Load the saved scaler
scaler = joblib.load('scaler.pkl')


# Example function to predict heater value
def predict_heater_value(fan_value, current_temp, env_temp, target_temp):
    # Prepare input data, now including 'target_temp' in the input
    input_data = pd.DataFrame([[fan_value, current_temp, env_temp, target_temp]],
                              columns=['fan_value', 'temperature', 'env_temp', 'target_temp'])

    # Scale input data using the loaded scaler
    input_data_scaled = scaler.transform(input_data)

    # Make prediction
    predicted_heater_value = model.predict(input_data_scaled)

    return predicted_heater_value[0][0]  # Return the predicted heater value


# Example usage
fan_value = 50  # Example fan value
current_temp = 40  # Example bean temperature
env_temp = 45  # Example environment temperature
target_temp = 65  # Example target temperature

# Make a prediction
predicted_heater_value = predict_heater_value(fan_value, current_temp, env_temp, target_temp)
print(f"Predicted Heater Value: {predicted_heater_value}")
