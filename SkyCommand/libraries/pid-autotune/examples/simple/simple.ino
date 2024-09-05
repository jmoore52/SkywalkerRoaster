#include "pid-autotune.h"

PID pid = PID();
pid_tuner tuner = pid_tuner(pid, 10, 1000000, pid_tuner::CLASSIC_PID);

void outputFunc(double x) {
  analogWrite(11, x);
}

void setup() {
    Serial.begin(115200);

    tuner.setConstrains(0, 255);
    tuner.setTargetValue(100);

    tuner.tune(analogRead, A0, outputFunc);

    Serial.println(tuner.getKp());
    Serial.println(tuner.getKi());
    Serial.println(tuner.getKd());
}

void loop() {
    // Your loop code here
}
