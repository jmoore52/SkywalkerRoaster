import sys
sys.path.append('/usr/local/lib/python3.11/site-packages')  # Replace with actual path
import serial
import time
import csv

# Initialize serial communication with Arduino
ser = serial.Serial('/dev/cu.usbserial-1422320', 115200, timeout=1)

# Function to send a command and get a response
def send_command(command):
    ser.write((command + '\n').encode())  # Send the command to Arduino
    time.sleep(0.1)  # Give it time to process
    response = ser.readline().decode('utf-8').strip()  # Read response
    return response

# Function to log data to CSV
def log_data(heater_value, fan_value, temperature, env_temp):
    with open('roaster_data.csv', 'a') as file:
        writer = csv.writer(file)
        writer.writerow([time.time(), heater_value, fan_value, temperature, env_temp])

# Function to control heater and fan while reaching setpoints
def control_temperature_cycle():
    heater_value = 10
    fan_value = 50  # Fan speed to be kept consistent or vary as needed
    start_time = time.time()  # Mark the start time of the experiment
    last_heater_adjust_time = start_time
    observation_period = 30  # Observe temperature changes for 30 seconds before adjusting the heater

    # Stability detection
    stability_threshold = 0.3  # Lowered the temperature change range for stability
    stable_readings_count = 0  # Counter to track consecutive stable readings
    required_stable_readings = 50  # Number of consecutive stable readings needed for temperature stabilization
    previous_temperature = None

    # Set of target temperatures for cycling
    target_temperatures = [65, 70, 75, 80]
    current_stage = 0  # Start at the first target temperature

    while current_stage < len(target_temperatures):
        target_temp = target_temperatures[current_stage]
        print(f"Target temperature: {target_temp}°C")

        # Send fan speed and initial heater values
        send_command(f"OT2;{fan_value}")

        while True:
            # Send OT1 command to control heater and retrieve temperature
            send_command(f"OT1;{heater_value}")
            temperature_response = send_command("READ")

            try:
                # Parse temperature
                parts = temperature_response.split(',')
                current_temp = float(parts[2])  # Bean temperature
                env_temp = float(parts[1])  # Environment temperature
            except (ValueError, IndexError):
                print("Error reading temperature data.")
                continue

            # Log the current data
            log_data(heater_value, fan_value, current_temp, env_temp)
            print(f"Heater: {heater_value}, Fan: {fan_value}, Bean Temp: {current_temp}, Env Temp: {env_temp}")

            # Stability detection logic
            if previous_temperature is not None:
                if abs(current_temp - previous_temperature) < stability_threshold:
                    stable_readings_count += 1
                else:
                    stable_readings_count = 0  # Reset if temperature is unstable

            # Move to next stage if stable for required readings
            if stable_readings_count >= required_stable_readings:
                print(f"Stable at {target_temp}°C")
                current_stage += 1  # Move to the next target stage
                stable_readings_count = 0  # Reset stability for the next stage
                break  # Exit the inner loop to move to the next target temperature

            # Adjust the heater only after a delay and if stable
            if time.time() - last_heater_adjust_time >= observation_period:
                if current_temp < target_temp:
                    heater_value = min(heater_value + 5, 100)
                else:
                    heater_value = max(heater_value - 5, 0)
                last_heater_adjust_time = time.time()

            # Wait for the next cycle (1 second)
            time.sleep(1)
            previous_temperature = current_temp

        print(f"Moving to the next target temperature: {target_temp}°C")


# Send initial commands to Skywalker roaster
def send_initial_commands():
    send_command("CHAN;1200")
    send_command("UNITS;C")
    send_command("FILT;70,70,70,70")
    send_command("DRUM;100")

# Main function
if __name__ == "__main__":
    send_initial_commands()
    control_temperature_cycle()
