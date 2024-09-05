#include "pid.h"


#pragma region constructors
// Default constructor
PID::PID() {
    _acc = 0;
    _isEnabled = true; // By default, PID controller is enabled
}

// Constructor with PID constants
PID::PID(double kp, double ki, double kd) {
    _acc = 0;
    _isEnabled = true; // By default, PID controller is enabled
    _kp = kp;
    _ki = ki;
    _kd = kd;
}

// Constructor with angular flag
PID::PID(bool isAngular) {
    _acc = 0;
    _isEnabled = true; // By default, PID controller is enabled
    _isAngular = isAngular;
}

// Constructor with angular flag and PID constants
PID::PID(bool isAngular, double kp, double ki, double kd) {
    _acc = 0;
    _isEnabled = true; // By default, PID controller is enabled
    _isAngular = isAngular;
    _kp = kp;
    _ki = ki;
    _kd = kd;
}
#pragma endregion constructors

#pragma region setters
// Set PID constants
void PID::setConstants(double kp, double ki, double kd) {
    _kp = kp;
    _ki = ki;
    _kd = kd;
}

// Set setpoint value
void PID::setSetPoint(double setpoint) {
    _setpoint = setpoint;
}

// Set output constraints
void PID::setConstrains(double lower, double upper) {
    _lower = lower;
    _upper = upper;
}

// Enable PID controller
void PID::enable() {
    _isEnabled = true;
}

// Disable PID controller
void PID::disable() {
    _isEnabled = false;
}
#pragma endregion setters

// Compute PID output
double PID::compute(double feedback) {
    if(!_isEnabled) return 0; // If PID controller is disabled, return 0

    _error = _setpoint - feedback; // Compute error

    _dt = (millis() - _prev_time) / 1000.0; // Compute time difference

    if(_isAngular) {
        while (_error > 180) _error -= 360; // Normalize angular error
        while (_error < -180) _error += 360; // Normalize angular error
    }

    if (_prev_time) {
        _diff = feedback - _prev_feedback; // Compute derivative of feedback

        if(_isAngular) {
            while (_diff > 180) _diff -= 360; // Normalize angular difference
            while (_diff < -180) _diff += 360; // Normalize angular difference
        }
        _diff /= _dt; // Compute derivative term
        _acc += _error * _dt; // Accumulate error for integral term
        _acc = constrain(_acc, _lower, _upper); // Apply output constraints
    }
    _prev_time = millis(); // Update previous time
    _prev_feedback = feedback; // Update previous feedback
    _pid = (_kp * _error) + (_ki * _acc) - (_kd * _diff); // Compute PID output

    return _pid;
}
