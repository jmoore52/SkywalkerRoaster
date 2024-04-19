//#define __DEBUG__
//#define __WARN__
//#define USE_TIMER_1 true
#include <Arduino.h>
#include <limits.h>

//Pin declarations 
  const int txPin = 3;
  const int rxPin = 2;

//Serial variables
  const int preamble = 7000; //microseconds
  const int one_length = 1200; // Threshold for interpreting a bit as '1'. Pulses longer than 1200 microseconds are considered '1', given an expected duration of ~1500 microseconds for a logical '1' and a duration of ~600 microseconds for a logical '0'.
  const int roasterLength = 7; //Bytes
  const int controllerLength = 6; //Bytes
  const int TIMEOUT_PULSEIN = 2000;  // Timeout (microseconds) for every pulseIn call  chatGPT "Since your longest pulse for a binary '1' is ~1.5ms, and the time between bits is 750 microseconds, you'll want your TIMEOUT_PULSEIN to be slightly longer than 1.5ms to ensure you capture the entire pulse. You might set it to around 2ms for safety."
  const int TOTAL_TIMEOUT = 800;    // Total timeout (milliseconds) for the whole receiving process
  
  uint8_t receiveBuffer[roasterLength];
  uint8_t sendBuffer[controllerLength];

  int ventByte = 0;
  int filterByte = 1;
  int coolByte = 2;
  int drumByte = 3;
  int heatByte = 4;
  int checkByte = 5;

  double temp = 0.0;
  char unitType = 'F'; //Celcius and Farenheit 

//Failsafe variables
  const int maxTemp = 300;
  unsigned long lastEventTime = 0; 
  unsigned long lastEventTimeout = 10000000; 
  bool failedToReadRoaster = false;
  int roasterReadAttempts = 0;


//Communication with roaster

  void pulsePin(int pin, int duration) {
    //Assuming pin is HIGH when we get it
    digitalWrite(pin, LOW);
    delayMicroseconds(duration);
    digitalWrite(pin, HIGH);
    //we leave it high
  }

    void receiveSerialBitsFromRoaster(int bytes, int pin) {  //Receives serial bits from the roaster and stores them in the receive buffer.
    unsigned long timeIntervals[bytes * 8]; //which would in this case be the same as roasterLength
    unsigned long pulseDuration = 0;
    unsigned long startTime = millis();
    int bits = bytes * 8;
    bool dataReceived = false;

    // while (pulseDuration < preamble) {  //Wait for it or exut
    //   pulseDuration = pulseIn(pin, LOW);
    // }


    while (!dataReceived && (millis() - startTime < TOTAL_TIMEOUT)) {
        pulseDuration = pulseIn(pin, LOW, TIMEOUT_PULSEIN);
        if (pulseDuration >= (preamble - 500) && pulseDuration <= (preamble + 500)) { // Check that the pulse is approximately 7.5ms
          dataReceived = true; // Preamble detected
          break;
        }
    }
    if (!dataReceived) {
      failedToReadRoaster = true;
      return; //ends the function early
    }

    for (int i = 0; i < bits; i++) {  //Read the proper number of bits..
      unsigned long duration = pulseIn(pin, LOW, TIMEOUT_PULSEIN);
      if (duration == 0) {
        #ifdef __DEBUG__
          Serial.print("Timeout or no pulse detected at bit ");
          Serial.println(i);
        #endif
        // Handle the error, e.g., break, set an error flag, etc.
        failedToReadRoaster = true;
        return;
      }
      timeIntervals[i] = duration;
    }

    memset(receiveBuffer, 0, bytes); //zero that buffer

    for (int i = 0; i < bits; i++) {  //Convert timings to bits
      //Bits are received in LSB order..
      if (timeIntervals[i] > one_length) {  // we received a 1
        receiveBuffer[i / 8] |= (1 << (i % 8));
      }
    }
    failedToReadRoaster = false;
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

  double calculateTemp() {  //Calculating roaster temperature from last message
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

    if (unitType == 'C') v = (v - 32) * 5 / 9;

    return v;
  }

  void sendRoasterMessage() { //Sending message to Roaster
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

  void getRoasterMessage() {
    #ifdef __DEBUG__
      Serial.print("Debug: Receiving data:");
    #endif

      bool passedChecksum = false;
      failedToReadRoaster = false;

        roasterReadAttempts += 1; //Counting number of read attempt
        if (roasterReadAttempts > 99){
        roasterReadAttempts = 99;
        }

        receiveSerialBitsFromRoaster(roasterLength, rxPin);
        passedChecksum = calculateRoasterChecksum();


      if (passedChecksum == false || failedToReadRoaster == true) {
        #ifdef __WARN__
        Serial.println(" Failed to read roaster.");
        #endif
        return;
      } else {
        roasterReadAttempts = 0;
      }

    #ifdef __DEBUG__
      printBuffer(roasterLength);
    #endif
      
    #ifdef __WARN__
      if (roasterReadAttempts > 1) {
        Serial.print("[!] WARNING: Took ");
        Serial.print(roasterReadAttempts);
        Serial.println(" tries to read roaster.");
      }
    #endif
      temp = calculateTemp();
  }


//Communication with Artisan
  void setControlChecksum() {  //Calculates and sets the checksum for the current state of the sendBuffer.
    uint8_t sum = 0;
    for (int i = 0; i < (controllerLength - 1); i++) {
      sum += sendBuffer[i];
    }
    sendBuffer[controllerLength - 1] = sum;
  }

  void setValue(uint8_t* bytePtr, uint8_t v) {  //Prepare message to send to roaster
    *bytePtr = v;
    setControlChecksum();  // Always keep the checksum updated.
  }

  void handleREAD() {
    Serial.print(0.0);  //ambient temperature
    Serial.print(',');
    Serial.print(temp);  //channel 1
    Serial.print(',');
    Serial.print(temp);  //channel 2
    Serial.print(',');
    Serial.print(sendBuffer[heatByte]);  //heater
    Serial.print(',');
    Serial.print(sendBuffer[ventByte]);  //fan
    Serial.print(',');
    Serial.println('0'); //SV - PID set value

    lastEventTime = micros();
  }

  void handleHEAT(uint8_t value) {
    if (value <= 100) {
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

  void shutdown() {  //Turn everything off!
    for (int i = 0; i < controllerLength; i++) {
      sendBuffer[i] = 0;
    }
  }

  void eStop() { //Emergency stop, heat to 0 and vent to 100
    setValue(&sendBuffer[heatByte], 0);
    setValue(&sendBuffer[ventByte], 100);
  }

  void handleDRUM(uint8_t value) {
    if (value != 0) {
      setValue(&sendBuffer[drumByte], 100);    
    } else {
      setValue(&sendBuffer[drumByte], 0);
    }
    lastEventTime = micros();
  }

  void handleFILTER(uint8_t value) {
    if (value <= 100) {
      setValue(&sendBuffer[filterByte], value);
    }
    lastEventTime = micros();
  }

  void handleCOOL(uint8_t value) {
    if (value <= 100) {
      setValue(&sendBuffer[coolByte], value);
    }
    lastEventTime = micros();
  }

  void handleCHAN() {
    Serial.println("# Active channels set to 0200");
  }

  void getArtisanMessage() {
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
      } else if (command == "ESTOP") {  //Emergency stop heat to 0 and vent to 100
        eStop();
      } else if (command == "DRUM") {  //Start the drum
        handleDRUM(value);
      } else if (command == "FILTER") {  //Turn on the filter fan
        handleFILTER(value);
      } else if (command == "COOL") {  //Cool the beans
        handleCOOL(value);
      } else if (command == "CHAN") {  //Hanlde the TC4 init message
        handleCHAN();
      } else if (command == "UNITS") {
        if (split >= 0) unitType = input.charAt(split + 1);
      }
    }
  }

//Failsafe functions
    //Don't want the roaster be uncontrolled.. By itself, if you don't send a command in 1sec it will shutdown
    //But I also want to ensure the arduino is getting commands from something.
    //I think a safeguard for this might be to ensure we're regularly receiving control messages.
    //If Artisan is on, it should be polling for temps every few seconds. This requires we get a VALID control command.
    //I also want to add some sort of over temp protection. Butthis is the wild west. Don't burn your roaster and/or house down.

  bool itsbeentoolong() {   //Checks if too much time has passed since the last control message
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

  bool isTemperatureOverLimit() {   //Checks if the current temperature exceeds the maximum allowed temperature
    if(temp > maxTemp) {
      return true;
    }
    return false;
  }

  void failSafeChecks() {

    if (itsbeentoolong()) {
      //TODO: Maybe consider moving this logic to the interrupt handler
      //That way if the arduino is having issues, the interrupt handler
      //Will stop sending messages to the roaster and it'll shut down.
      shutdown(); //Turn everything off if the last control message is too old
    }

    if (isTemperatureOverLimit()) {
      eStop(); //Possible temperature runaway? Emergency stop
    }
  }


//Main loops
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

    failSafeChecks();

    getRoasterMessage(); 

    getArtisanMessage();

    sendRoasterMessage();

    //sending message to artisan is handled by handleREAD command.

  }
