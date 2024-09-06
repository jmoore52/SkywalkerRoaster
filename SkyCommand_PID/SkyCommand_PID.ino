//#define __DEBUG__
//#define __WARN__
//#define USE_TIMER_1 true

// #define __DEBUG2__


#include <limits.h>

const int txPin = 3;
const int rxPin = 2;

const int preamble = 7000;
const int one_length = 1200;
const int roasterLength = 7;
const int controllerLength = 6;

uint8_t receiveBuffer[roasterLength];
uint8_t sendBuffer[controllerLength];

int ventByte = 0;
int drumByte = 3;
int coolByte = 2;
int filterByte = 1;
int heatByte = 4;
int checkByte = 5;

double beanTemp = 0.0;
double envTemp = 0.0;

unsigned long lastEventTime = 0;
unsigned long lastEventTimeout = 10000000;
char CorF = 'F';

// Serial Debug via CH340

#include <SoftwareSerial.h>

SoftwareSerial debugSerial(6, 7); // RX, TX on pins 10 and 11

// MAX31865
#include <Adafruit_MAX31865.h>

Adafruit_MAX31865 beanThermo = Adafruit_MAX31865(10, 11, 12, 13);
Adafruit_MAX31865 envThermo = Adafruit_MAX31865(9, 11, 12, 13);

#define RREF 430.0
#define RNOMINAL 100.0
const unsigned long tempReadInterval = 1000;  // 1000ms = 1 second delay
unsigned long lastBeanTempReadTime = 0;
unsigned long lastEnvTempReadTime = 0;
double lastBeanTemp = 0.0;
double lastEnvTemp = 0.0;


// PID
#include <AutoPID.h>

bool tuning = false;         // To check if tuning is in progress
bool PIDFirmwareStarted = false; //PID firmware requests started
unsigned long tuningStartTime;
unsigned long lastTuneTime = 0;   // To control the frequency of each tuning step
double tuningOutput = 0;  // To store the output of each step
unsigned long lastTuningUpdate = 0;
const unsigned long tuningInterval = 2000;  // 1-second interval for tuning updates

unsigned long lastPIDFirmwareUpdate = 0;
const unsigned long firmwarePIDInterval = 2000;  // 1-second interval for tuning updates

// PID constants (initial values)

double setpoint = 0;  // Target temperature
double input, output;
double Kp = 2.0, Ki = 0.03, Kd = 0.4;

// Set up the heater output range (e.g., 0-100 for percentage control)
AutoPID myPID(&input, &setpoint, &output, 0, 100, Kp, Ki, Kd);

// Variables to store error values
double prevError = 0.0;       // Previous error for derivative term
double integral = 0.0;        // Accumulated error for integral term

//
void setControlChecksum() {
  uint8_t sum = 0;
  for (int i = 0; i < (controllerLength - 1); i++) {
    sum += sendBuffer[i];
  }
  sendBuffer[controllerLength - 1] = sum;
}

void setValue(uint8_t* bytePtr, uint8_t v) {
  *bytePtr = v;
  setControlChecksum();  // Always keep the checksum updated.
}

void shutdown() {  //Turn everything off!
  PIDFirmwareStarted = false;
  for (int i = 0; i < controllerLength; i++) {
    sendBuffer[i] = 0;
  }
}

void pulsePin(int pin, int duration) {
  //Assuming pin is HIGH when we get it
  digitalWrite(pin, LOW);
  delayMicroseconds(duration);
  digitalWrite(pin, HIGH);
  //we leave it high
}

void sendMessage() {
  //send Preamble
  pulsePin(txPin, 7500);
  delayMicroseconds(3800);

  //send Message
  for (int i = 0; i < controllerLength; i++) {
    for (int j = 0; j < 8; j++) {
      if (bitRead(sendBuffer[i], j) == 1) {
        pulsePin(txPin, 1500);  //delay for a 1
      } else {
        pulsePin(txPin, 650);  //delay for a 0
      }
      delayMicroseconds(750);  //delay between bits
    }
  }
}

// double calculateBeanTemp() {

//   //MAX31865 TEMP
//   double v = 0;
//   uint8_t fault = beanThermo.readFault();
//   if (fault) {
//     // Serial.print("Fault 0x");
//     // Serial.println(fault, HEX);
//     beanThermo.clearFault();
//   } else {
//     v = beanThermo.temperature(RNOMINAL, RREF);
//   }
//   // Serial.print( "Bean Temp: ");
//   // Serial.println( v);
//   return v;
// }

// double calculateEnvTemp() {

//   //MAX31865 TEMP
//   double v = 0;
//   uint8_t fault = envThermo.readFault();
//   if (fault) {
//     // Serial.print("Fault 0x");
//     // Serial.println(fault, HEX);
//     envThermo.clearFault();
//   } else {
//     v = envThermo.temperature(RNOMINAL, RREF);
//   }

//   return v;
// }

double calculateBeanTemp() {
  // Current time
  unsigned long currentMillis = millis();
  
  // Check if enough time has passed since the last reading
  if (currentMillis - lastBeanTempReadTime >= tempReadInterval) {
    lastBeanTempReadTime = currentMillis;  // Update the last read time
    // Serial.println("calculateBeanTemp");
    // MAX31865 TEMP
    uint8_t fault = beanThermo.readFault();
    if (fault) {
      // Serial.print("Fault 0x");
      // Serial.println(fault, HEX);
      beanThermo.clearFault();
    } else {
      lastBeanTemp = beanThermo.temperature(RNOMINAL, RREF);
    }
  }

  // Return the last temperature reading
  return lastBeanTemp;
}

double calculateEnvTemp() {
  // Current time
  unsigned long currentMillis = millis();
  
  // Check if enough time has passed since the last reading
  if (currentMillis - lastEnvTempReadTime >= tempReadInterval) {
    lastEnvTempReadTime = currentMillis;  // Update the last read time
    // Serial.println("calculateEnvTemp");

    // MAX31865 TEMP
    uint8_t fault = envThermo.readFault();
    if (fault) {
      // Serial.print("Fault 0x");
      // Serial.println(fault, HEX);
      envThermo.clearFault();
    } else {
      lastEnvTemp = envThermo.temperature(RNOMINAL, RREF);
    }
  }

  // Return the last temperature reading
  return lastEnvTemp;
}

void getMessage(int bytes, int pin) {
  unsigned long timeIntervals[roasterLength * 8];
  unsigned long pulseDuration = 0;
  int bits = bytes * 8;

  while (pulseDuration < preamble) {  //Wait for it or exut
    pulseDuration = pulseIn(pin, LOW);
  }

  for (int i = 0; i < bits; i++) {  //Read the proper number of bits..
    timeIntervals[i] = pulseIn(pin, LOW);
  }

  for (int i = 0; i < 7; i++) {  //zero that buffer
    receiveBuffer[i] = 0;
  }

  for (int i = 0; i < bits; i++) {  //Convert timings to bits
    //Bits are received in LSB order..
    if (timeIntervals[i] > one_length) {  // we received a 1
      receiveBuffer[i / 8] |= (1 << (i % 8));
    }
  }
}

bool calculateRoasterChecksum() {
  uint8_t sum = 0;
  for (int i = 0; i < (roasterLength - 1); i++) {
    sum += receiveBuffer[i];
  }

#ifdef __DEBUG__
  Serial.print("sum: ");
  Serial.print(sum, HEX);
  Serial.print(" Checksum Byte: ");
  Serial.println(receiveBuffer[roasterLength - 1], HEX);
#endif
  return sum == receiveBuffer[roasterLength - 1];
}

void printBuffer(int bytes) {
  for (int i = 0; i < bytes; i++) {
    Serial.print(sendBuffer[i], HEX);
    Serial.print(',');
  }
  Serial.print("\n");
}

void getRoasterMessage() {
#ifdef __DEBUG__
  Serial.print("R ");
#endif

  bool passedChecksum = false;
  int count = 0;

  while (!passedChecksum) {
    count += 1;
    getMessage(roasterLength, rxPin);
    passedChecksum = calculateRoasterChecksum();
  }
#ifdef __DEBUG__
  printBuffer(roasterLength);
#endif

#ifdef __WARN__
  if (count > 1) {
    Serial.print("[!] WARN: Took ");
    Serial.print(count);
    Serial.println(" tries to read roaster.");
  }
#endif

beanTemp = calculateBeanTemp();
envTemp = calculateEnvTemp();
  // Serial.print("Temp:");
  // Serial.println(temp);

}
void handleHEAT(uint8_t value) {
  if (value <= 100) {
    Serial.print("handleHEAT - setting Heater to : ");
    Serial.println(value);  // Debug message for heater control

    setValue(&sendBuffer[heatByte], value);
  }
  lastEventTime = micros();
}

void handleVENT(uint8_t value) {
  if (value <= 100) {
    setValue(&sendBuffer[ventByte], value);
  }
  lastEventTime = micros();
}

void handleCOOL(uint8_t value) {
  if (value <= 100) {
    setValue(&sendBuffer[coolByte], value);
  }
  lastEventTime = micros();
}

void handleFILTER(uint8_t value) {
  if (value <= 100) {
    setValue(&sendBuffer[filterByte], value);
  }
  lastEventTime = micros();
}

void handleDRUM(uint8_t value) {
  if (value != 0) {
    setValue(&sendBuffer[drumByte], 100);
  } else {
    setValue(&sendBuffer[drumByte], 0);
  }
  lastEventTime = micros();
}

void handleREAD() {
  Serial.print(0.0);
  Serial.print(',');
  Serial.print(envTemp);
  Serial.print(',');
  Serial.print(beanTemp);
  Serial.print(',');
  Serial.print(sendBuffer[heatByte]);
  Serial.print(',');
  Serial.print(sendBuffer[ventByte]);
  Serial.print(',');
  Serial.println('0');

  lastEventTime = micros();
}

bool itsbeentoolong() {
  unsigned long now = micros();
  unsigned long duration = now - lastEventTime;
  //if (duration < 0) {       // Commented out because compiler states comparison of unsigned expression < 0 is always false
  //  duration = (ULONG_MAX - lastEventTime) + now;  //I think this is right.. right?
  //}
  if (duration > lastEventTimeout) {
    return true;
  }
  return false;
}

void handleCHAN() {
  Serial.println("# Active channels set to 0200");
}

void setup() {
  //ok.. Talking to myself here.. but lets do a sanity check.
  //The idea is that the loop will handle any requests from serial.
  //While the timer which runs every 10ms will send the control message to the roaster.
  Serial.begin(115200);
  Serial.setTimeout(100);

  // serial debug CH340
  // debugSerial.begin(9600);   // Second serial for CH340G debugging

  pinMode(txPin, OUTPUT);
  shutdown();

  //MAX31865
  beanThermo.begin(MAX31865_4WIRE);  // 2WIRE, 3WIRE, 4WIRE 
  envThermo.begin(MAX31865_4WIRE);  // 2WIRE, 3WIRE, 4WIRE 
  //
  //additional serial
  // debugSerial.begin(9600);            // Software serial for debugging
  // debugSerial.println("Debugging Port Ready!");  // Message on the debug port

  

  //ITimer1.init();
  //ITimer1.attachInterruptInterval(750, sendMessage);
}

void loop() {
  //Don't want the roaster be uncontrolled.. By itself, if you don't send a command in 1sec it will shutdown
  //But I also want to ensure the arduino is getting commands from something.
  //I think a safeguard for this might be to ensure we're regularly receiving control messages.
  //If Artisan is on, it should be polling for temps every few seconds. This requires we get a VALID control command.
  //I also want to add some sort of over temp protection. Butthis is the wild west. Don't burn your roaster and/or house down.

  if (itsbeentoolong()) {
    //TODO: Maybe consider moving this logic to the interrupt handler
    //That way if the arduino is having issues, the interrupt handler
    //Will stop sending messages to the roaster and it'll shut down.
    shutdown(); //We turn everything off
  }

  sendMessage();

  getRoasterMessage();

  if (Serial.available() > 0) {
    String input = Serial.readString();

    #ifdef __DEBUG2__
      debugSerial.print("Artisan received : ");
      debugSerial.print(input);
    #endif

    uint8_t value = 0;
    input.trim();
    int split = input.indexOf(';');
    String command = "";

    if (split >= 0) {
      command = input.substring(0, split);
      value = input.substring(split + 1).toInt();
    } else {
      command = input;
    }
    
    if (!tuning) {

      if (command == "READ") {
        handleREAD();   
      } 
      else if (command == "OT1") {  //Set Heater Duty
        handleHEAT(value);
      } 
      else if (command == "OT2") {  //Set Fan Duty
        handleVENT(value);
      } 
      else if (command == "OFF") {  //Shut it down
        shutdown();
      } 
      else if (command == "DRUM") {  //Start the drum
        // Serial.print("DRUM");
        handleDRUM(value);
      } 
      else if (command == "FILTER") {  //Turn on the filter fan
        handleFILTER(value);
      } 
      else if (command == "COOL") {  //Cool the beans
        handleCOOL(value);
      } 
      else if (command == "CHAN") {  //Hanlde the TC4 init message
        handleCHAN();
      } 
      else if (command == "UNITS") {
        if (split >= 0) CorF = input.charAt(split + 1);
      }
      else if (command == "SV") {  //Hanlde the TC4 init message
        handlePIDFirmware(value);
      } 

      else if (command == "PID_CALIBRATE") {  //Hanlde the TC4 init message
        handlePidCalibration(value);
      } 
    } else  
    {
      if (command == "STOP_PID_CALIBRATE") {  
         tuning = false;  // Set tuning active

      }
    }
  } //if (Serial.available() > 0)

  if (PIDFirmwareStarted) {
    executePIDFirmware();

  }



// //PID CALIBRATE
//   if (tuning) {
//     if (millis() - lastTuningUpdate >= tuningInterval) {

//       Serial.print (" in tuning ... ")      ;
//       // Automatically compute the PID output
//       input = beanTemp;
//       myPID.run();
//       // Send the computed output to the heater
//       handleHEAT((uint8_t)output);
//       lastTuningUpdate = millis();
//     }

//   }


}


void handlePIDFirmware(uint8_t targetTemperature) {
    // Set the target temperature for PID tuning
    setpoint = targetTemperature;
    PIDFirmwareStarted = true;  // Set tuning active

    Serial.print("SV request: ");
    Serial.println(targetTemperature);

}

void executePIDFirmware() {
    if (millis() - lastPIDFirmwareUpdate >= firmwarePIDInterval) {

      Serial.print (" executePIDFirmware ... ")      ;
      // Automatically compute the PID output
      input = beanTemp;
      myPID.run();
      // Send the computed output to the heater
      handleHEAT((uint8_t)output);
      lastPIDFirmwareUpdate = millis();
    }
}


void handlePidCalibration(uint8_t targetTemperature) {
    // Set the target temperature for PID tuning
    setpoint = targetTemperature;
    tuning = true;  // Set tuning active
    tuningStartTime = millis();

    Serial.print("Starting PID calibration for setpoint: ");
    Serial.println(targetTemperature);
}



