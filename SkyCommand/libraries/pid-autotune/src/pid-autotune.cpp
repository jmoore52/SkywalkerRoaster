#include "pid-autotune.h"

#pragma region constructors
// Default constructor
pid_tuner::pid_tuner() {
    _mode = NO_OVERSHOOT; // By default, tining mode is set to no overshoot
}

// Constructor with PID controller reference
pid_tuner::pid_tuner(const PID &pid) {
    _pid = pid;
    _mode = NO_OVERSHOOT; // By default, tining mode is set to no overshoot
}

// Constructor with PID controller reference, number of tuning cycles, tuning interval, and tuning mode
pid_tuner::pid_tuner(const PID &pid, int32_t cycles, int64_t interval, mode_t mode) {
    _pid = pid;
    _cycles = cycles;
    _loop_interval = interval;
    _mode = mode;
}
#pragma endregion constructors

#pragma region setters
// Set PID controller
void pid_tuner::setPID(const PID &pid) {
    _pid = pid;
}

// Set target value
void pid_tuner::setTargetValue(double target) {
    _target_input = target;
}

// Set loop interval
void pid_tuner::setLoopInterval(int64_t interval) {
    _loop_interval = interval;
}

// Set output constraints
void pid_tuner::setConstrains(double lower, double upper) {
    _lower = lower;
    _upper = upper;
    _pid.setConstrains(lower, upper);
}

// Set tuning mode
void pid_tuner::setMode(mode_t mode) {
    _mode = mode;
}

// Set number of tuning cycles
void pid_tuner::setTuningCycles(int cycles) {
    _cycles = cycles;
}
#pragma endregion setters

#pragma region getters
// Get PID constants
double* pid_tuner::getConstants() {
    double constants[3];
    constants[0] = _kp;
    constants[1] = _ki;
    constants[2] = _kd;

    return constants;
}

// Get Kp
double pid_tuner::getKp() {
    return _kp;
}

// Get Ki
double pid_tuner::getKi() {
    return _ki;
}

// Get Kd
double pid_tuner::getKd() {
    return _kd;
}

// Get state
bool pid_tuner::isDone() {
    return (_cycle_count >= _cycles);
}

// Get number of completed tuning cycles
int32_t pid_tuner::getCycles() {
    return _cycle_count;
}
#pragma endregion getters

// Initialize tuning loop
void pid_tuner::startTuningLoop(uint64_t time_us) {
    _cycle_count = 0;
    _output_enabled = true;
    _output_value = _upper;
    _t1 = time_us;
    _t2 = time_us;
    _t_high = 0;
    _t_low = 0;
    _max_level = -1000000000000;
    _min_level = 1000000000000;
    _kp_mean = 0;
    _ki_mean = 0;
    _kd_mean = 0;
}

// Single tuning loop
double pid_tuner::tuningLoop(double input, uint64_t time_us) {
    // Update state level variables
    _max_level = max(_max_level, input);
    _min_level = min(_min_level, input);

    // Check for target input crossing to trigger output change
    if((_output_enabled) && (_target_input < input)) {
        _output_enabled = false;
        _output_value = _lower;
        _t1 = time_us;
        _t_high = _t1 - _t2;
        _max_level = _target_input;
    }
    else if(!(_output_enabled) && (_target_input > input)) {
        _output_enabled = true;
        _output_value = _upper;
        _t2 = time_us;
        _t_low = _t2 - _t1;

        // Calculate ultimate gain (ku) and ultimate period (tu)
        // ku = 4d / πa
        //  - d: is the amplitude of the output signal
        //  - a: is the amplitude of the input signal
        double d = (_upper - _lower) / 2.0;
        double a = (_max_level - _min_level) / 2.0;
        _ku = (4.0 * d) / (PI * a);
        _tu = _t_low + _t_high;

        // Determine tuning coefficients based on tuning mode
        if(_mode == CLASSIC_PID) {
            _kp_coefficient = 0.6;
            _ti_coefficient = 0.5;
            _td_coefficient = 0.125;
        }
        else if (_mode == PESSEN_INTEGRAL_RULE)
        {
            _kp_coefficient = 0.7;
            _ti_coefficient = 0.4;
            _td_coefficient = 0.15;
        }
        
        else if (_mode == SOME_OVERSHOOT)
        {
            _kp_coefficient = 0.33;
            _ti_coefficient = 0.5;
            _td_coefficient = 0.33;
        }
        else {
            _kp_coefficient = 0.2;
            _ti_coefficient = 0.5;
            _td_coefficient = 0.33;
        }
        
        // Calculate PID constants (Kp, Ki, Kd) using tuning coefficients
        _kp = _kp_coefficient * _ku;
        _ki = (_kp / (_ti_coefficient * _tu)) * _loop_interval;
        _kd = (_td_coefficient * _kp * _tu) / _loop_interval;

        // Update mean values for PID constants if more than one tuning cycle completed
        if(_cycle_count > 1) {
            _kp_mean += _kp;
            _ki_mean += _ki;
            _kd_mean += _kd;
        }

        // Reset the minimum level to target input level
        _min_level = _target_input;
         // Increment cycles count
        _cycle_count++;
    }

    // Finalize tuning when the specified number of cycles is reached
    if(_cycle_count >= _cycles) {
        _output_enabled = false;
        _output_value = _lower;
        _kp = _kp_mean / (_cycle_count);
        _ki = _ki_mean / (_cycle_count);
        _kd = _kd_mean / (_cycle_count);
    }

    return _output_value;
}
