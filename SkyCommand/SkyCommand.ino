//#define __DEBUG__
//#define __WARN__
//#define USE_TIMER_1 true

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

bool setValue(uint8_t* bytePtr, uint8_t v) {
  *bytePtr = v;
  setControlChecksum();  // Always keep the checksum updated.
}

void shutdown() {  //Turn everything off!
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

double calculateTemp() {
  /* 
    I really hate this. 
    It seems to work.. but I feel like there must be a better way than 
    using a 4th degree polynomial to model this but I sure can't seem to figure it out. 
  */

  double x = ((receiveBuffer[0] << 8) + receiveBuffer[1]) / 1000.0;
  double y = ((receiveBuffer[2] << 8) + receiveBuffer[3]) / 1000.0;

#ifdef __DEBUG__
  Serial.print(x);
  Serial.print(',');
  Serial.println(y);
#endif

  double v = 583.1509258523457 + -714.0345395202813 * x + -196.071718077524 * y
             + 413.37964344228334 * x * x + 2238.149675349052 * x * y
             + -4099.91031297056 * y * y + 357.49007607425233 * x * x * x
             + -5001.419602972793 * x * x * y + 8242.08618555862 * x * y * y
             + 247.6124684730026 * y * y * y + -555.8643213534281 * x * x * x * x
             + 3879.431274654493 * x * x * x * y + -6885.682277959339 * x * x * y * y
             + 2868.4191998911865 * x * y * y * y + -1349.1588373011923 * y * y * y * y;

  if (CorF == 'C') v = (v - 32) * 5 / 9;

  return v;
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
  if (value != 0) {
    setValue(&sendBuffer[drumByte], 100);
  } else {
    setValue(&sendBuffer[drumByte], 0);
  }
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
  Serial.print(',');
  Serial.println('0');

  time = micros();
}

bool itsbeentoolong() {
  unsigned long now = micros();
  unsigned long duration = now - time;
  if (duration < 0) {
    duration = (ULONG_MAX - time) + now;  //I think this is right.. right?
  }
  if (duration > timeout) {
    shutdown();  //We turn everything off
  }
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
  pinMode(txPin, OUTPUT);
  shutdown();

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

    shutdown();
  }

  sendMessage();

  getRoasterMessage();

  if (Serial.available() > 0) {
    String input = Serial.readString();

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

    if (command == "READ") {
      handleREAD();
    } else if (command == "OT1") {  //Set Heater Duty
      handleHEAT(value);
    } else if (command == "OT2") {  //Set Fan Duty
      handleVENT(value);
    } else if (command == "OFF") {  //Shut it down
      shutdown();
    } else if (command == "DRUM") {  //Start the drum
      handleDRUM(value);
    } else if (command == "FILTER") {  //Turn on the filter fan
      handleFILTER(value);
    } else if (command == "COOL") {  //Cool the beans
      handleCOOL(value);
    } else if (command == "CHAN") {  //Hanlde the TC4 init message
      handleCHAN();
    } else if (command == "UNITS") {
      if (split >= 0) CorF = input.charAt(split + 1);
    }
  }
}
