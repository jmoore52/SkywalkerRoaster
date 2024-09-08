float bean_temp = 22.0;  // Constant bean temperature
float env_temp = 22.0;   // Constant environment temperature
float fan_value = 40.0;  // Constant fan value

void setup() {
  Serial.begin(9600);  // Start serial communication at 9600 baud rate
  while (!Serial) {
    ;  // Wait for serial connection
  }
  Serial.println("Arduino is ready and waiting for PID;SV;VALUE...");
}

void loop() {
  // Check if data is available from the serial input
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');  // Read the incoming string until newline
    input.trim();  // Remove any leading or trailing whitespace

    // Check if the input format is correct (PID;SV;25)
    if (input.startsWith("PID;SV;")) {
      String setValueStr = input.substring(7);  // Extract the set value (target temperature)
      float setValue = setValueStr.toFloat();   // Convert the set value to a float

      // Now send the data (SV, bean_temp, env_temp, fan_value) to a Python script or another device
      // to predict the heater value based on the trained neural network. Here we'll simulate a response.
      // For demonstration purposes, we assume the heater value returned is a simple proportional function
      // (In reality, you'd get this from the Python program on your computer or another device)

      float heater_value = calculateHeaterValue(setValue, bean_temp, env_temp, fan_value);

      // Send the heater value back over the serial port
      Serial.print("Heater Value: ");
      Serial.println(heater_value);
    } else {
      // If the input format is incorrect
      Serial.println("Invalid command. Please send in the format PID;SV;VALUE");
    }
  }
}

float calculateHeaterValue(float setValue, float bean_temp, float env_temp, float fan_value) {
  // Simulate a heater value calculation based on the target set value
  // In the actual implementation, this would come from the Python neural network model
  // Example: proportional control (for demo purposes)
  float error = setValue - bean_temp;
  float heater_value = error * 10;  // Simple proportional control (adjust the gain as needed)
  
  if (heater_value > 100) heater_value = 100;  // Limit heater value to a maximum of 100
  if (heater_value < 0) heater_value = 0;      // Limit heater value to a minimum of 0
  
  return heater_value;
}
