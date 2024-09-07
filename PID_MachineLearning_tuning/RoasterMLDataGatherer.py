import sys
sys.path.append('/usr/local/lib/python3.11/site-packages')  # Replace with actual path
import serial
import time
import csv

# Initialize serial communication with Arduino
ser = serial.Serial('/dev/cu.usbserial-1430', 115200, timeout=1)

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
    heater_value = 20
    fan_value = 60  # Fan speed to be kept consistent or vary as needed
    start_time = time.time()  # Mark the start time of the experiment
    last_heater_adjust_time = start_time

    # Stability detection
    stability_threshold = 0.25  # Lowered the temperature change range for stability
    stable_readings_count = 0  # Counter to track consecutive stable readings
    required_stable_readings = 10  # Number of consecutive stable readings needed for temperature stabilization
    previous_temperature = None

    # Set of target temperatures and times for cycling
    # Set of target temperatures and times for cycling
    target_times_and_temps = [
        {'time': '1:45', 'temp': 120.0},
        {'time': '1:51', 'temp': 121.0},
        {'time': '1:57', 'temp': 122.0},
        {'time': '2:02', 'temp': 123.0},
        {'time': '2:08', 'temp': 124.0},
        {'time': '2:15', 'temp': 125.0},
        {'time': '2:17', 'temp': 125.0},
        {'time': '2:21', 'temp': 125.0},
        {'time': '2:23', 'temp': 125.0},
        {'time': '2:27', 'temp': 125.0},
        {'time': '2:30', 'temp': 125.0},
        {'time': '2:42', 'temp': 132.0},
        {'time': '2:53', 'temp': 139.0},
        {'time': '3:06', 'temp': 146.0},
        {'time': '3:17', 'temp': 153.0},
        {'time': '3:30', 'temp': 160.0},
        {'time': '3:36', 'temp': 160.0},
        {'time': '3:42', 'temp': 160.0},
        {'time': '3:47', 'temp': 160.0},
        {'time': '3:53', 'temp': 160.0},
        {'time': '4:00', 'temp': 160.0},
        {'time': '4:24', 'temp': 163.4},
        {'time': '4:47', 'temp': 166.8},
        {'time': '5:12', 'temp': 170.2},
        {'time': '5:35', 'temp': 173.6},
        {'time': '6:00', 'temp': 177.0},
        {'time': '6:24', 'temp': 177.6},
        {'time': '6:47', 'temp': 178.2},
        {'time': '7:12', 'temp': 178.8},
        {'time': '7:35', 'temp': 179.4},
        {'time': '8:00', 'temp': 180.0},
        {'time': '8:11', 'temp': 181.0},
        {'time': '8:24', 'temp': 182.0},
        {'time': '8:35', 'temp': 183.0},
        {'time': '8:48', 'temp': 184.0},
        {'time': '9:00', 'temp': 185.0},
        {'time': '9:11', 'temp': 186.0},
        {'time': '9:24', 'temp': 187.0},
        {'time': '9:35', 'temp': 188.0},
        {'time': '9:48', 'temp': 189.0},
        {'time': '10:00', 'temp': 190.0},
        {'time': '10:05', 'temp': 191.0},
        {'time': '12:11', 'temp': 190.0},
        {'time': '12:18', 'temp': 180.0},
        {'time': '12:24', 'temp': 170.0},
        {'time': '12:30', 'temp': 160.0},
        {'time': '12:35', 'temp': 159.0},
        {'time': '12:41', 'temp': 158.0},
        {'time': '12:48', 'temp': 157.0},
        {'time': '12:54', 'temp': 156.0},
        {'time': '13:00', 'temp': 155.0},
        {'time': '13:11', 'temp': 152.0},
        {'time': '13:24', 'temp': 149.0},
        {'time': '13:35', 'temp': 146.0},
        {'time': '13:48', 'temp': 143.0},
        {'time': '14:00', 'temp': 140.0},
        {'time': '14:11', 'temp': 138.0},
        {'time': '14:24', 'temp': 136.0},
        {'time': '14:35', 'temp': 134.0},
        {'time': '14:48', 'temp': 132.0},
        {'time': '15:00', 'temp': 130.0},
        {'time': '15:11', 'temp': 128.0},
        {'time': '15:24', 'temp': 126.0},
        {'time': '15:35', 'temp': 124.0},
        {'time': '15:48', 'temp': 122.0},
        {'time': '16:00', 'temp': 120}
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
            if time.time() - last_heater_adjust_time >= 3:
                if current_temp < target_temp:
                    heater_value = min(heater_value + 5, 70)
                else:
                    heater_value = max(heater_value - 5, 0)
                last_heater_adjust_time = time.time()

            # Wait for the next cycle (1 second)
            time.sleep(.25)
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
