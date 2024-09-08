import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Dropout
from tensorflow.keras.regularizers import l2
import joblib

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

# Create lagged features based on 20-second or 30-second history
lag_seconds = 20  # You can change this to 30 if needed
data = create_lagged_features(data, lag_seconds)
data = data.dropna()  # Drop rows with NaN after creating lagged features

# Select features including the new delta_bean_temp
X = data[['fan_value', 'env_temp', 'future_bean_temp', 'delta_bean_temp'] + 
         [f'bean_temp_lag_{i}' for i in range(1, lag_seconds + 1)]]
y = data['heater_value']

# Split the data into training and testing sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Standardize the data
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_test = scaler.transform(X_test)

# Save the scaler for later use in testing
joblib.dump(scaler, 'scaler.pkl')

# Define the neural network architecture with L2 Regularization and Dropout
model = Sequential([
    Dense(32, input_dim=X_train.shape[1], activation='relu', kernel_regularizer=l2(0.01)),  # L2 regularization
    Dropout(0.2),  # Dropout layer to prevent overfitting
    Dense(16, activation='relu', kernel_regularizer=l2(0.01)),  # Second layer with L2 regularization
    Dropout(0.2),  # Dropout to prevent overfitting
    Dense(1, activation='linear')  # Linear output for predicting heater value
])

# Compile the model with L2 regularization and dropout
model.compile(optimizer='adam', loss='mse', metrics=['mae'])

# Train the model with early stopping to prevent overfitting
early_stopping = tf.keras.callbacks.EarlyStopping(monitor='val_loss', patience=10, restore_best_weights=True)

model.fit(
    X_train, y_train,
    validation_data=(X_test, y_test),
    epochs=200,
    batch_size=16,
    callbacks=[early_stopping]  # Apply early stopping
)

# Evaluate the model
loss, mae = model.evaluate(X_test, y_test)
print(f"Test Mean Absolute Error: {mae}")

# Save the trained model to a file
model.save('roaster_pid_model_with_dropout_l2_long_history.keras')

# Convert to TensorFlow Lite model for microcontroller deployment
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()

# Save the TensorFlow Lite model
with open('roaster_pid_model_with_dropout_l2_long_history.tflite', 'wb') as f:
    f.write(tflite_model)

print("Model has been converted and saved as roaster_pid_model_with_dropout_l2_long_history.tflite")

# Prediction and Comparison
# Load the scaler and model for prediction
scaler = joblib.load('scaler.pkl')
model = tf.keras.models.load_model('roaster_pid_model_with_dropout_l2_long_history.keras')

# Scale the features for prediction
X_scaled = scaler.transform(X)

# Make predictions
y_pred = model.predict(X_scaled)

# Compare the predicted heater values with the actual values
comparison_df = pd.DataFrame({'Actual Heater Value': y, 'Predicted Heater Value': y_pred.flatten()})

# Calculate Mean Absolute Error for comparison
mae = np.mean(np.abs(comparison_df['Actual Heater Value'] - comparison_df['Predicted Heater Value']))
print(f"Mean Absolute Error: {mae}")

# Plot comparison of actual vs predicted heater values
import matplotlib.pyplot as plt
plt.figure(figsize=(10, 6))
plt.plot(comparison_df['Actual Heater Value'].values, label='Actual Heater Value')
plt.plot(comparison_df['Predicted Heater Value'].values, label='Predicted Heater Value', linestyle='--')
plt.title('Actual vs Predicted Heater Values')
plt.xlabel('Sample Index')
plt.ylabel('Heater Value')
plt.legend()
plt.show()

