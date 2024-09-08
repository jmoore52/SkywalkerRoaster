import pandas as pd
import joblib
import tensorflow as tf
import numpy as np
import matplotlib.pyplot as plt

# Load the dataset
data = pd.read_csv('roaster_data.csv', names=['time', 'heater_value', 'fan_value', 'bean_temp', 'env_temp'])

# Convert time from Unix timestamp to seconds
data['time'] = pd.to_datetime(data['time'], unit='s')
data['time'] = data['time'].astype(int) / 10**9  # Convert Unix time to seconds

# Detect gaps in time and segment batches
data['time_diff'] = data['time'].diff().fillna(1)
data['batch'] = (data['time_diff'] > 1).cumsum()  # Each gap larger than 1 second is considered a new batch

# Shift bean temperature by one step to create the "future_bean_temp"
data['future_bean_temp'] = data['bean_temp'].shift(-1)

# Calculate delta_bean_temp: the difference in bean_temp from the previous time step
data['delta_bean_temp'] = data.groupby('batch')['bean_temp'].diff().fillna(0)  # Reset for each batch

# Drop rows with missing future_bean_temp (since they can't be predicted)
data = data.dropna()

# Create lagged features for bean temperature (based on actual time, not just row shifts)
def create_lagged_features(df, lag_seconds):
    for lag in range(1, lag_seconds + 1):
        df[f'bean_temp_lag_{lag}'] = df.groupby('batch')['bean_temp'].shift(lag)
    return df

# Make sure the lag_seconds here matches the value used during training (e.g., 20 or 30)
lag_seconds = 20  # Use 20 or 30 based on the training configuration
data = create_lagged_features(data, lag_seconds)
data = data.dropna()  # Drop rows with NaN after creating lagged features

# Select features (including lagged features) to match the training setup
X = data[['fan_value', 'env_temp', 'future_bean_temp', 'delta_bean_temp'] + 
         [f'bean_temp_lag_{i}' for i in range(1, lag_seconds + 1)]]
y = data['heater_value']

# Load the scaler and scale the features (ensure this is the same scaler used during training)
scaler = joblib.load('scaler.pkl')
X_scaled = scaler.transform(X)

# Load the trained model (ensure this matches the model used during training)
model = tf.keras.models.load_model('roaster_pid_model_with_dropout_l2_long_history.keras')

# Make predictions
y_pred = model.predict(X_scaled)

# Compare the predicted heater values with the actual values
comparison_df = pd.DataFrame({'Actual Heater Value': y, 'Predicted Heater Value': y_pred.flatten()})

# Calculate Mean Absolute Error for comparison
mae = np.mean(np.abs(comparison_df['Actual Heater Value'] - comparison_df['Predicted Heater Value']))
print(f"Mean Absolute Error: {mae}")

# Plot comparison of actual vs predicted heater values
plt.figure(figsize=(10, 6))
plt.plot(comparison_df['Actual Heater Value'].values, label='Actual Heater Value')
plt.plot(comparison_df['Predicted Heater Value'].values, label='Predicted Heater Value', linestyle='--')
plt.title('Actual vs Predicted Heater Values')
plt.xlabel('Sample Index')
plt.ylabel('Heater Value')
plt.legend()
plt.show()

