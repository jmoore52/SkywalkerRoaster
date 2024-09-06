# Import necessary libraries
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import tensorflow as tf

# Load the dataset
data = pd.read_csv('roaster_data.csv', names=['timestamp', 'heater_value', 'fan_value', 'temperature', 'env_temp', 'target_temp'])

# Drop unnecessary columns (e.g., timestamp)
data = data.drop(columns=['timestamp'])

# Split the data into features (X) and target (y)
X = data[['fan_value', 'temperature', 'env_temp', 'target_temp']]  # Add target_temp as an input feature
y = data['heater_value']  # The heater value is what the model will predict

# Split into training and testing data
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Standardize the data (important for neural networks)
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_test = scaler.transform(X_test)

# Define the neural network architecture
model = tf.keras.models.Sequential([
    tf.keras.layers.Dense(64, input_dim=X_train.shape[1], activation='relu'),  # Hidden layer with 64 neurons
    tf.keras.layers.Dense(32, activation='relu'),  # Second hidden layer with 32 neurons
    tf.keras.layers.Dense(1, activation='linear')  # Output layer (predict heater value)
])

# Compile the model
model.compile(optimizer='adam', loss='mse', metrics=['mae'])

# Train the model
history = model.fit(X_train, y_train, validation_data=(X_test, y_test), epochs=50, batch_size=32)

# Evaluate the model
loss, mae = model.evaluate(X_test, y_test)
print(f"Test Mean Absolute Error: {mae}")

# Predict on test data
predictions = model.predict(X_test)

# Output predictions with context
for i, prediction in enumerate(predictions):
    print(f"Predicted Heater Value: {prediction[0]:.2f} | Actual: {y_test.iloc[i]} | Target Temp: {X_test[i, 3]} | Current Temp: {X_test[i, 1]}")

# Save the trained model to a file
model.save('roaster_pid_model.keras')

# Optional: Load the trained model
model = tf.keras.models.load_model('roaster_pid_model.keras')
