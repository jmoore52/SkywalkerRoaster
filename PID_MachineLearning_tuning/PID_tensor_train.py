import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense

# Load the dataset
data = pd.read_csv('roaster_data.csv', names=['time', 'heater_value', 'fan_value', 'bean_temp', 'env_temp'])

# Create the target column by shifting the bean_temp by one time step to simulate the "future_bean_temp"
data['future_bean_temp'] = data['bean_temp'].shift(-1)
data = data.dropna()  # Remove the last row since it doesn't have a future value

# Split the data into features (X) and target (y)
X = data[['bean_temp', 'fan_value', 'future_bean_temp', 'env_temp']]
y = data['heater_value']

# Split into training and testing data
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Standardize the data
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_test = scaler.transform(X_test)

# Save the scaler for later use in testing
import joblib
joblib.dump(scaler, 'scaler.pkl')

# Define the neural network architecture
model = Sequential([
    Dense(64, input_dim=X_train.shape[1], activation='relu'),  # Hidden layer
    Dense(32, activation='relu'),  # Second hidden layer
    Dense(1, activation='sigmoid')  # Output layer with sigmoid activation
])

# Compile the model
model.compile(optimizer='adam', loss='mse', metrics=['mae'])

# Train the model
model.fit(X_train, y_train / 100.0, validation_data=(X_test, y_test / 100.0), epochs=50, batch_size=32)

# Evaluate the model
loss, mae = model.evaluate(X_test, y_test / 100.0)
print(f"Test Mean Absolute Error: {mae * 100}")

# Save the trained model to a file
model.save('roaster_pid_model.keras')
