// #define __DEBUG__
// #define __WARN__

const int controllerPin = 2;
const int roasterPin = 3;

unsigned long timeIntervals[56];
uint8_t bytebuffer[7];

const int preamble = 7000;
const int one_length = 1200;
const int roasterLength = 7;
const int controllerLength = 6;

bool toggle = 0;
double temp = 0.0;
int heat = 0;
int air = 0;

void setup() {
  Serial.begin(9600);
}

double calculateTemp() {

  /* 
    I really hate this. 
    It seems to work nicely.. but I feel like there must be a better way than 
    using a 4th degree polynomial to model this but I sure can't seem to figure it out. 
  */
  double x = ((bytebuffer[0] << 8) + bytebuffer[1]) / 1000.0;
  double y = ((bytebuffer[2] << 8) + bytebuffer[3]) / 1000.0;

#ifdef __DEBUG__
  Serial.print(x);
  Serial.print(',');
  Serial.println(y);
#endif

  return 583.1509258523457 + -714.0345395202813 * x + -196.071718077524 * y
         + 413.37964344228334 * x * x + 2238.149675349052 * x * y
         + -4099.91031297056 * y * y + 357.49007607425233 * x * x * x
         + -5001.419602972793 * x * x * y + 8242.08618555862 * x * y * y
         + 247.6124684730026 * y * y * y + -555.8643213534281 * x * x * x * x
         + 3879.431274654493 * x * x * x * y + -6885.682277959339 * x * x * y * y
         + 2868.4191998911865 * x * y * y * y + -1349.1588373011923 * y * y * y * y;
}

void getMessage(int bytes, int pin) {
  unsigned long pulseDuration = 0;
  int bits = bytes * 8;

  while (pulseDuration < preamble) {  //Wait for it...
    pulseDuration = pulseIn(pin, LOW);
  }

  for (int i = 0; i < bits; i++) {  //Read the proper number of bits..
    timeIntervals[i] = pulseIn(pin, LOW);
  }

  for (int i = 0; i < 7; i++) {  //zero that buffer
    bytebuffer[i] = 0;
  }

  for (int i = 0; i < bits; i++) {  //Convert timings to bits
    //Bits are received in LSB order..
    if (timeIntervals[i] > one_length) {  // we received a 1
      bytebuffer[i / 8] |= (1 << (i % 8));
    }
  }
}

bool calculateChecksum(int length) {
  uint8_t sum = 0;
  for (int i = 0; i < (length - 1); i++) {
    sum += bytebuffer[i];
  }
#ifdef __DEBUG__
  Serial.print("sum: ");
  Serial.print(sum, HEX);
  Serial.print(" Checksum Byte: ");
  Serial.println(bytebuffer[length - 1], HEX);
#endif
  return sum == bytebuffer[length - 1];
}

void printBuffer(int bytes) {
  for (int i = 0; i < bytes; i++) {
    Serial.print(bytebuffer[i], HEX);
    Serial.print(',');
  }
  Serial.print("\n");
}

void getControllerMessage() {
#ifdef __DEBUG__
  Serial.print("C");
#endif

  bool passedChecksum = false;
  int count = 0;

  while (!passedChecksum) {
    count += 1;
    getMessage(controllerLength, controllerPin);
    passedChecksum = calculateChecksum(controllerLength);
  }

#ifdef __DEBUG__
  printBuffer(controllerLength);
#endif

  heat = bytebuffer[4];
  air = bytebuffer[0];

#ifdef __WARN__
  if (count > 1) {
    Serial.print("[!] WARN: Took ");
    Serial.print(count);
    Serial.println(" tries to read controller.");
  }
#endif
}


void getRoasterMessage() {
#ifdef __DEBUG__
  Serial.print("R ");
#endif

  bool passedChecksum = false;
  int count = 0;

  while (!passedChecksum) {
    count += 1;
    getMessage(roasterLength, roasterPin);
    passedChecksum = calculateChecksum(roasterLength);
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

void loop() {
  toggle = !toggle;
  if (toggle == 0) {
    getControllerMessage();
  } else {
    getRoasterMessage();
  }

  if (Serial.available() > 0) {
    Serial.print("0.0,");
    Serial.print((float)temp);
    Serial.print(",");
    Serial.print((float)heat);
    Serial.print(",");
    Serial.print((float)air);
    Serial.print("\n");
    while (Serial.available() > 0) {
      Serial.read();
    }
  }
}
