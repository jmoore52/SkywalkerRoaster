import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler

# Load the dataset
data = pd.read_csv('roaster_data.csv', names=['timestamp', 'heater_value', 'fan_value', 'temperature', 'env_temp'])
data = data.drop(columns=['timestamp'])  # Drop unnecessary columns

# Split the data into features (X) and target (y)
X = data[['fan_value', 'temperature', 'env_temp']]
y = data['heater_value']

# Split into training and testing data
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Standardize the data
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_test = scaler.transform(X_test)

# Define the neural network architecture
model = Sequential([
    Dense(64, input_dim=X_train.shape[1], activation='relu'),
    Dense(32, activation='relu'),
    Dense(1, activation='linear')
])

# Define custom loss function if needed
def custom_mse(y_true, y_pred):
    return tf.reduce_mean(tf.square(y_true - y_pred))

# Compile the model with custom loss or standard loss
model.compile(optimizer='adam', loss='mse', metrics=['mae'])

# Train the model
history = model.fit(X_train, y_train, validation_data=(X_test, y_test), epochs=50, batch_size=32)

# Evaluate the model
loss, mae = model.evaluate(X_test, y_test)
print(f"Test Mean Absolute Error: {mae}")

# Save the trained model in the .keras format
model.save('roaster_pid_model.keras')

# Load the model with the custom loss function
model = tf.keras.models.load_model('roaster_pid_model.keras', custom_objects={'custom_mse': custom_mse})
