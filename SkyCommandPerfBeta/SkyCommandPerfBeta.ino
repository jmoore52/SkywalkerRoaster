//#define __DEBUG__
//#define __WARN__
#include <limits.h>

const int txPin = 3;
const int rxPin = 2;

const int preamble = 7000;
const int one_length = 1200;
const int roasterLength = 7;
const int controllerLength = 6;

uint8_t receiveBuffer[roasterLength];
uint8_t sendBuffer[controllerLength];

int ventByte = 0, drumByte = 3, coolByte = 2, filterByte = 1, heatByte = 4;

double temp = 0.0;
unsigned long time = 0;
unsigned long timeout = 10000000;
char CorF = 'F';

void setControlChecksum() {
  uint8_t sum = 0;
  for (int i = 0; i < (controllerLength - 1); i++) {
    sum += sendBuffer[i];
  }
  sendBuffer[controllerLength - 1] = sum;
}

// Always keep the checksum updated
bool setValue(uint8_t* bytePtr, uint8_t v) {
  *bytePtr = v;
  setControlChecksum();
}

//Turn everything off!
void shutdown() {
  for (int i = 0; i < controllerLength; i++) {
    sendBuffer[i] = 0;
  }
}

void pulsePin(int pin, int duration) {
  //Assuming pin is HIGH when we get it
  digitalWrite(pin, LOW);
  delayMicroseconds(duration);  // Pulse duration
  digitalWrite(pin, HIGH);
}

void sendMessage() {
  // Send Preamble
  pulsePin(txPin, 7500);
  delayMicroseconds(3050);  // Reduced preamble delay

  // Send Message
  for (int i = 0; i < controllerLength; i++) {
    for (int j = 0; j < 8; j++) {
      pulsePin(txPin, (sendBuffer[i] >> j) & 1 ? 1500 : 650);  // Send either 1500 or 650 based on bit value
      delayMicroseconds(750);                                  // Delay between bits
    }
  }
}

double calculateTemp() {
  // Precomputed constant terms
  const double c0 = 583.1509258523457;
  const double c1 = -714.0345395202813;
  const double c2 = -196.071718077524;
  const double c3 = 413.37964344228334;
  const double c4 = 2238.149675349052;
  const double c5 = -4099.91031297056;
  const double c6 = 357.49007607425233;
  const double c7 = -5001.419602972793;
  const double c8 = 8242.08618555862;
  const double c9 = 247.6124684730026;
  const double c10 = -555.8643213534281;
  const double c11 = 3879.431274654493;
  const double c12 = -6885.682277959339;
  const double c13 = 2868.4191998911865;
  const double c14 = -1349.1588373011923;

  // Extracting bytes and dividing by 1000.0
  double x = (receiveBuffer[0] << 8 | receiveBuffer[1]) * 0.001;
  double y = (receiveBuffer[2] << 8 | receiveBuffer[3]) * 0.001;

#ifdef __DEBUG__
  Serial.print(x);
  Serial.print(',');
  Serial.println(y);
#endif

  // Optimized polynomial calculation
  double v = c0 + c1 * x + c2 * y
             + c3 * x * x + c4 * x * y
             + c5 * y * y + c6 * x * x * x
             + c7 * x * x * y + c8 * x * y * y
             + c9 * y * y * y + c10 * x * x * x * x
             + c11 * x * x * x * y + c12 * x * x * y * y
             + c13 * x * y * y * y + c14 * y * y * y * y;

  // Convert to Celsius if necessary
  if (CorF == 'C') v = (v - 32) * 5 / 9;

  return v;
}

void getMessage(int bytes, int pin) {
  unsigned long timeIntervals[roasterLength * 8];
  unsigned long pulseDuration;

  // Wait for preamble
  do {
    pulseDuration = pulseIn(pin, LOW);
  } while (pulseDuration < preamble);

  // Read bits
  for (int i = 0; i < bytes; i++) {
    receiveBuffer[i] = 0;  // Zero the buffer for each byte

    for (int j = 0; j < 8; j++) {
      pulseDuration = pulseIn(pin, LOW);
      if (pulseDuration > one_length) {  // Received a 1
        receiveBuffer[i] |= (1 << j);    // Set the bit
      }
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
    count++;
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
  temp = calculateTemp();
}

void handleHEAT(uint8_t value) {
  if (value >= 0 && value <= 100) {
    setValue(&sendBuffer[heatByte], value);
  }
  time = micros();
}

void handleVENT(uint8_t value) {
  if (value >= 0 && value <= 100) {
    setValue(&sendBuffer[ventByte], value);
  }
  time = micros();
}

void handleCOOL(uint8_t value) {
  if (value >= 0 && value <= 100) {
    setValue(&sendBuffer[coolByte], value);
  }
  time = micros();
}

void handleFILTER(uint8_t value) {
  if (value >= 0 && value <= 100) {
    setValue(&sendBuffer[filterByte], value);
  }
  time = micros();
}

void handleDRUM(uint8_t value) {
  setValue(&sendBuffer[drumByte], (value != 0) ? 100 : 0);
  time = micros();
}

void handleREAD() {
  Serial.print(0.0);
  Serial.print(',');
  Serial.print(temp);
  Serial.print(',');
  Serial.print(temp);
  Serial.print(',');
  Serial.print(sendBuffer[heatByte]);
  Serial.print(',');
  Serial.print(sendBuffer[ventByte]);
  Serial.println('0');

  time = micros();
}

bool itsbeentoolong() {
  unsigned long now = micros();
  unsigned long duration = (now >= time) ? (now - time) : ((ULONG_MAX - time) + now);
  if (duration > timeout) {
    shutdown();
  }
}

void handleCHAN() {
  Serial.println("# Active channels set to 0200");
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  pinMode(txPin, OUTPUT);
  shutdown();
}

String buffer = "";  // Define buffer globally

void loop() {
  if (itsbeentoolong()) {
    shutdown();
    return;  // Stop processing further commands if shutdown
  }

  sendMessage();
  getRoasterMessage();

  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n') {  // Check for end of command
      processCommand();
    } else {
      buffer += c;  // Append character to buffer
    }
  }
}

void processCommand() {
  String command;
  uint8_t value = 0;

  int split = buffer.indexOf(';');
  if (split >= 0) {
    command = buffer.substring(0, split);
    value = buffer.substring(split + 1).toInt();
  } else {
    command = buffer;
  }

  if (command == "READ") handleREAD();
  else if (command == "OT1") handleHEAT(value);
  else if (command == "OT2") handleVENT(value);
  else if (command == "OFF") shutdown();
  else if (command == "DRUM") handleDRUM(value);
  else if (command == "FILTER") handleFILTER(value);
  else if (command == "COOL") handleCOOL(value);
  else if (command == "CHAN") handleCHAN();
  else if (command == "UNITS" && split >= 0) CorF = buffer.charAt(split + 1);

  buffer = "";  // Clear the buffer for the next command
}
