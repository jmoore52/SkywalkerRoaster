import tensorflow as tf
from sklearn.preprocessing import StandardScaler
import pandas as pd

# Load the trained model
model = tf.keras.models.load_model('roaster_pid_model.keras')

# Load and preprocess the test dataset (from roaster_data.csv or new data)
data = pd.read_csv('roaster_data.csv', names=['timestamp', 'heater_value', 'fan_value', 'temperature', 'env_temp'])

# Convert timestamp to elapsed time and calculate rate of temperature change
data['timestamp'] = pd.to_datetime(data['timestamp'], unit='s')
data['time_diff'] = data['timestamp'].diff().dt.total_seconds().fillna(0)
data['temp_rate_of_change'] = data['temperature'].diff() / data['time_diff']
data['temp_rate_of_change'] = data['temp_rate_of_change'].fillna(0)

# Drop unnecessary columns (e.g., timestamp)
data = data.drop(columns=['timestamp'])

# Select features for testing (make sure they match your training features)
X_test = data[['fan_value', 'temperature', 'env_temp', 'time_diff', 'temp_rate_of_change']]

# Standardize the data (using the scaler from training)
scaler = StandardScaler()
X_test = scaler.fit_transform(X_test)

# Predict heater values based on the features
predictions = model.predict(X_test)

# Output predictions
for i, prediction in enumerate(predictions):
    print(f"Predicted Heater Value: {prediction[0]} | Actual: {data['heater_value'].iloc[i]}")
