/********************************************************
   PID Basic simulated heater Example
   Reading simulated analog input 0 to control analog PWM output 3
 ********************************************************/
//  This simulates a 20W heater block driven by the PID
//  Vary the setpoint with the Pot, and watch the heater drive the temperature up
//
//  Simulation at https://wokwi.com/projects/359088752027305985
//
//  Based on
//  Wokwi https://wokwi.com/projects/357374218559137793
//  Wokwi https://wokwi.com/projects/356437164264235009

//#include "PID_v1.h" // https://github.com/br3ttb/Arduino-PID-Library
#include "PID_v1_bc.h" // https://github.com/drf5n/Arduino-PID-Library

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
//double Kp = 20, Ki = .01, Kd = 10; // works reasonably with sim heater block fo 220deg
double Kp = 25.5, Ki = 0.1, Kd = 0; // +/-10°proportional band 
//double Kp = 255, Ki = 0.05, Kd = 0; // works reasonably with sim heater block 
//double Kp = 255, Ki = .0, Kd = 0; // +/-1° proportional band works reasonably with sim heater block 
//double Kp = 10000, Ki = 0.0, Kd = 0.0; // bang-bang
//double Kp = 2, Ki = 0.0, Kd = 0.0; // P-only
//double Kp = 2, Ki = 5, Kd = 1; // commonly used defaults

PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, P_ON_E, DIRECT);

const int PWM_PIN = 3;  // UNO PWM pin for Output
const int INPUT_PIN = -1; // Analog pin for Input (set <0 for simulation)
const int SETPOINT_PIN = A1;   // Analog pin for Setpoint Potentiometer
const int AUTOMATIC_PIN = 8; // Pin for controlling manual/auto mode, NO
const int OVERRIDE_PIN = 12; // Pin for integral override, NO
const int PLUS_PIN = 4; // Pin for integral override, NO
const int MINUS_PIN = 7; // Pin for integral override, NO
const int LCD_SDA_PIN = A4; // Used by LiquidCrystal_I2C
const int LCD_SCL_PIN = A5; // Used by LiquidCrystal_I2C

#include <LiquidCrystal_I2C.h> // https://github.com/johnrickman/LiquidCrystal_I2C

#define I2C_ADDR    0x27
#define LCD_COLUMNS 20
#define LCD_LINES   4

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES); // uses SDA=A4,SCL=A5 on Uno

void setup()
{
  Serial.begin(115200);
  Serial.println(__FILE__);
  myPID.SetOutputLimits(0, 255); // -4 for 
  pinMode(OVERRIDE_PIN, INPUT_PULLUP);
  pinMode(AUTOMATIC_PIN, INPUT_PULLUP);
  pinMode(MINUS_PIN, INPUT_PULLUP);
  pinMode(PLUS_PIN, INPUT_PULLUP);
  Setpoint = 0;
  //turn the PID on
  myPID.SetMode(AUTOMATIC);
  if(INPUT_PIN>0){
    Input = analogRead(INPUT_PIN);
  }else{
    Input = simPlant(0.0,1.0); // simulate heating
  }
  lcd.init();
  lcd.backlight();

  Serial.println("Setpoint Input Output Integral");
}

void loop()
{
  // gather Input from INPUT_PIN or simulated block
  float heaterWatts = Output * 20.0 / 255; // 20W heater
  if (INPUT_PIN > 0 ) {
    Input = analogRead(INPUT_PIN);
  } else {
    float blockTemp = simPlant(heaterWatts,Output>0?1.0:1-Output); // simulate heating
    Input = blockTemp;   // read input from simulated heater block
  }

  if (myPID.Compute())
  {
    //Output = (int)Output; // Recognize that the output as used is integer
    analogWrite(PWM_PIN, Output);

  }

  Setpoint = analogRead(SETPOINT_PIN) / 4; // Read setpoint from potentiometer
  if(digitalRead(OVERRIDE_PIN)==LOW) mySetIntegral(&myPID,0); // integral override
  if(digitalRead(AUTOMATIC_PIN)==HIGH !=  myPID.GetMode()==AUTOMATIC){
    myPID.SetMode(digitalRead(AUTOMATIC_PIN)==HIGH ? AUTOMATIC :MANUAL);
  }
  static uint32_t lastButton = 0;
  if(myPID.GetMode()==MANUAL && millis() - lastButton > 250){
    if(digitalRead(PLUS_PIN)==LOW){ 
      Output += 1;
      lastButton = millis();
    }
    if(digitalRead(MINUS_PIN)==LOW){
      Output -= 1;
      lastButton = millis();
    }
  }

  report();
  reportLCD();

}

void report(void)
{
  static uint32_t last = 0;
  const int interval = 250;
  if (millis() - last > interval) {
    last += interval;
    //    Serial.print(millis()/1000.0);
    Serial.print("SP:");Serial.print(Setpoint);
    Serial.print(" PV:");
    Serial.print(Input);
    Serial.print(" CV:");
    Serial.print(Output);
    Serial.print(" Int:");
    Serial.print(myPID.outputSum);
    Serial.print(' ');
    Serial.println();
  }
}

void reportLCD(void)
{
  static uint32_t last = 0;
  const int interval = 250;
  if (millis() - last > interval) {
    last += interval;
    //    Serial.print(millis()/1000.0);
   // lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("PV:");
    lcd.print(Input,3);
    lcd.print(" CV:");
    lcd.print(Output,3);
    lcd.print("  ");
    lcd.setCursor(0,1);
    lcd.print("SP:");
    lcd.print(Setpoint,3);
    lcd.print(myPID.GetMode()==AUTOMATIC? " Automatic ":" Manual   ");
    lcd.print("  ");
    lcd.setCursor(0,3);
    lcd.print("Int:");
    lcd.print(myPID.outputSum,4);
    lcd.print(' ');
    lcd.println();
  }
}

float simPlant(float Q,float hfactor) { // heat input in W (or J/s)
  // simulate a 1x1x2cm aluminum block with a heater and passive ambient cooling
 // float C = 237; // W/mK thermal conduction coefficient for Al
  float h = 5 *hfactor ; // W/m2K thermal convection coefficient for Al passive
  float Cps = 0.89; // J/g°C
  float area = 1e-4; // m2 area for convection
  float mass = 10 ; // g
  float Tamb = 25; // °C
  static float T = Tamb; // °C
  static uint32_t last = 0;
  uint32_t interval = 100; // ms

  if (millis() - last >= interval) {
    last += interval;
    // 0-dimensional heat transfer
    T = T + Q * interval / 1000 / mass / Cps - (T - Tamb) * area * h;
  }
  return T;
}

void  mySetIntegral(PID * ptrPID,double value ){
   ptrPID->SetMode(MANUAL);
   Output = value;
   ptrPID->SetMode(AUTOMATIC);
}


