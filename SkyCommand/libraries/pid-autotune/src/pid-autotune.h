#ifndef PID_AUTOTUNE_H
#define PID_AUTOTUNE_H


#include "pid.h"

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

/**
 * @brief Class for PID autotuning.
 * 
 * This class provides functionality for tuning the PID constants automatically
 * based on the system response. It implements several methods for autotuning 
 * PID controllers, including classic PID, Pessen integral rule, some overshoot, 
 * and no overshoot methods.
 */

class pid_tuner
{
  public:
    typedef enum {
        CLASSIC_PID,                        // Classic PID tuning method
        PESSEN_INTEGRAL_RULE,               // Pessen integral rule tuning method
        SOME_OVERSHOOT,                     // Some overshoot tuning method
        NO_OVERSHOOT                        // No overshoot tuning method
    } mode_t;
private:
    double _kp = 0;                         // Proportional gain
    double _ki = 0;                         // Integral gain
    double _kd = 0;                         // Derivative gain
    double _ku = 0;                         // Ultimate gain
    double _tu = 0;                         // Ultimate period
    double _kp_coefficient = 0;             // Proportional gain coefficient
    double _ti_coefficient = 0;             // Integral gain coefficient
    double _td_coefficient = 0;             // Derivative gain coefficient
    double _target_input = 0;               // Target input for tuning
    double _loop_interval = 0;              // Interval for the tuning loop
    double _lower = 0;                      // Lower output limit
    double _upper = 255;                    // Upper output limit
    mode_t _mode;                           // Tuning mode
    uint8_t _cycles = 10;                   // Number of tuning cycles
    uint32_t _cycle_count;                  // Counter for tuning cycles
    bool _output_enabled;                   // Flag to control output
    double _output_value;                   // Output value
    uint64_t _time = 0;                     // Current time
    uint64_t _t1 = 0;                       // Time cursor 1
    uint64_t _t2 = 0;                       // Time cursor 2
    uint64_t _t_high = 0;                   // High state time
    uint64_t _t_low = 0;                    // Low state time
    double _max_level = -1000000000000;     // Maximum input level
    double _min_level = 1000000000000;      // Maximum input level
    double _kp_mean;                        // Kp mean value
    double _ki_mean;                        // Ki mean value
    double _kd_mean;                        // Kd mean value
    PID _pid = NULL;                        // Pointer to the PID controller

    
public:

    // Constructors
    #pragma region constructors
    /**
     * @brief Default constructor initializes a PID tuner instance.
     * 
     * This constructor initializes a new instance of the PID tuner with default values.
     */
    pid_tuner();

    /**
     * @brief Constructor initializes a PID tuner with a PID controller reference.
     * 
     * This constructor initializes a new instance of the PID tuner with the provided PID controller reference.
     * 
     * @param pid Reference to the PID controller object to be tuned
     */
    pid_tuner(const PID &pid);

    /**
     * @brief Constructor initializes a PID tuner with custom parameters.
     * 
     * This constructor initializes the PID tuner with the provided PID controller, number of tuning cycles,
     * tuning interval, and tuning mode.
     * 
     * @param pid Reference to the PID controller object
     * @param cycles Number of tuning cycles to perform
     * @param interval Interval between tuning iterations in microseconds
     * @param mode Tuning mode (classic PID, Pessen integral rule, some overshoot, no overshoot)
     */
    pid_tuner(const PID &pid, int32_t cycles, int64_t interval, mode_t mode);
    #pragma endregion constructors

    // Setters
    #pragma region setters
    /**
     * @brief Set the PID controller object for tuning.
     * 
     * This method sets the PID controller object to be used for tuning.
     * 
     * @param pid Reference to the PID controller object
     */
    void setPID(const PID &pid);
    /**
     * @brief Set the target input for tuning.
     * 
     * Sets the target input value for the tuning process.
     * 
     * @param target Target input value
     */
    void setTargetValue(double target);
    /**
     * @brief Set the loop interval for tuning.
     * 
     * Sets the interval between tuning iterations.
     * 
     * @param interval Interval between tuning iterations in microseconds
     */
    void setLoopInterval(int64_t interval);

    /**
     * @brief Set the output constraints for tuning.
     * 
     * Sets the lower and upper bounds for the output value during tuning.
     * 
     * @param lower Lower output limit
     * @param upper Upper output limit
     */
    void setConstrains(double lower, double upper);

    /**
     * @brief Set the tuning mode.
     * 
     * Sets the tuning mode for the PID autotuner.
     * 
     * @param mode Tuning mode (classic PID, Pessen integral rule, some overshoot, no overshoot)
     */
    void setMode(mode_t mode);

    /**
     * @brief Set the number of tuning cycles.
     * 
     * Sets the number of tuning cycles to perform.
     * 
     * @param cycles Number of tuning cycles
     */
    void setTuningCycles(int cycles);
    #pragma endregion setters

    // Getters
    #pragma region getters
    /**
     * @brief Get the tuned PID constants.
     * 
     * Returns an array containing the tuned PID constants (Kp, Ki, Kd).
     * 
     * @return Array containing the tuned PID constants
     */
    double* getConstants();
    /**
     * @brief Get the tuned proportional gain (Kp).
     * 
     * @return Tuned proportional gain (Kp)
     */
    double getKp();
    /**
     * @brief Get the tuned integral gain (Ki).
     * 
     * @return Tuned integral gain (Ki)
     */
    double getKi();
    /**
     * @brief Get the tuned derivative gain (Kd).
     * 
     * @return Tuned derivative gain (Kd)
     */
    double getKd();
    /**
     * @brief Check if tuning process is complete.
     * 
     * @return True if tuning process is complete, otherwise false
     */
    bool isDone();

    /**
     * @brief Get the number of completed tuning cycles.
     * 
     * @return Number of completed tuning cycles
     */
    int32_t getCycles();
    #pragma endregion getters

    /**
     * @brief Initialize the tuning loop.
     * 
     * Initializes the variables and flags required for the tuning process.
     * 
     * @param time_us Current time in microseconds
     */
    void startTuningLoop(uint64_t time_us);

    /**
     * @brief Perform one iteration of the tuning loop.
     * 
     * This method is called for each iteration of the tuning process. It calculates
     * the output value based on the current input, updates internal variables, and
     * computes the PID constants when tuning is complete.
     * 
     * @param input Current input value
     * @param time_us Current time in microseconds
     * @return Output value for the tuning iteration
     */
    double tuningLoop(double input, uint64_t time_us);


    /**
     * @brief Tune method for PID autotuning.
     * 
     * This method initiates the PID autotuning process using the specified input and output functions.
     * It continuously adjusts the PID parameters based on the system response until the tuning process
     * is completed. 
     * 
     * @tparam Func Type of the input and output functions
     * @tparam ArgType Type of the input argument
     * @param __input_function Input function that provides the current system input
     * @param __input_arg Argument passed to the input function
     * @param __output_function Output function that receives the computed output value
     */
    template<typename inputFunc_t, typename ArgType, typename outputFunc_t>
    void tune(inputFunc_t __input_function, ArgType __input_arg, outputFunc_t __output_function) {
        // Initialize the tuning loop
        startTuningLoop(micros());

        // Main tuning loop
        while(!isDone()) {
            _time = micros();
            // Obtain input value from the input function
            double input = __input_function(__input_arg);
            // Compute output value using the tuning loop
            double output = tuningLoop(input, _time);
            // Send output value to the output function
            __output_function(output);
            // Wait until the loop interval elapses
            while(micros() - _time < _loop_interval) delayMicroseconds(1);
        }
        // Send final output value of 0 to the output function
        __output_function(0);
        // Set the tuned PID constants in the PID controller
        _pid.setConstants(_kp, _ki, _kd);
    }
};

#endif /* PID_AUTOTUNE_H */
