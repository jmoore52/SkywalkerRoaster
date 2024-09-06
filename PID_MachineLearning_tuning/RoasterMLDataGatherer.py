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
    response = ser.readline().decode('utf-8').strip()  # Read response
    return response

# Function to log data to CSV
def log_data(heater_value, fan_value, temperature, env_temp):
    with open('roaster_data.csv', 'a') as file:
        writer = csv.writer(file)
        writer.writerow([time.time(), heater_value, fan_value, temperature, env_temp])

# Function to convert "minutes:seconds" to total seconds
def convert_time_to_seconds(time_str):
    minutes, seconds = map(int, time_str.split(':'))
    return minutes * 60 + seconds

# Function to control heater and fan while reaching setpoints
def control_temperature_cycle():
    heater_value = 10
    fan_value = 50  # Fan speed to be kept consistent or vary as needed
    start_time = time.time()  # Mark the start time of the experiment
    last_heater_adjust_time = start_time

    # Stability detection
    stability_threshold = 0.3  # Lowered the temperature change range for stability
    stable_readings_count = 0  # Counter to track consecutive stable readings
    required_stable_readings = 50  # Number of consecutive stable readings needed for temperature stabilization
    previous_temperature = None

    # Set of target temperatures and times for cycling
    target_times_and_temps = [
        {"time": "1:00", "temp": 65},
        {"time": "2:00", "temp": 70},
        {"time": "3:00", "temp": 75},
        {"time": "4:00", "temp": 80}
    ]

    for stage in target_times_and_temps:
        target_time_in_seconds = convert_time_to_seconds(stage["time"])
        target_temp = stage["temp"]
        print(f"Target temperature: {target_temp}°C at {stage['time']}")

        # Send fan speed and initial heater values
        send_command(f"OT2;{fan_value}")

        while time.time() - start_time < target_time_in_seconds:
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

            # Adjust the heater only after a delay and if stable
            if time.time() - last_heater_adjust_time >= 30:
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
    time.sleep(0.5)  # Give it time to process
    send_command("UNITS;C")
    time.sleep(0.5)  # Give it time to process
    send_command("FILT;70,70,70,70")
    time.sleep(0.5)  # Give it time to process
    send_command("DRUM;100")
    time.sleep(0.5)  # Give it time to process

# Main function
if __name__ == "__main__":
    send_initial_commands()
    control_temperature_cycle()
