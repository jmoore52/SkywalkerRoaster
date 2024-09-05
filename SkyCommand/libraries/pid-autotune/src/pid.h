#ifndef PID_H
#define PID_H

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

/**
 * @brief Class for PID controller.
 */
class PID
{
    private:
        // Private member variables
        double _kp = 0;                 // Proportional gain
        double _ki = 0;                 // Integral gain
        double _kd = 0;                 // Derivative gain
        double _setpoint = 0;           // Setpoint value
        double _prev_feedback = 0;      // Previous feedback value
        double _error = 0;              // Error (difference between setpoint and feedback)
        double _acc = 0;                // Accumulated error (for integral term)
        double _diff = 0;               // Derivative of feedback
        double _dt = 0;                 // Time difference
        double _pid = 0;                // PID output
        double _lower = 0;              // Lower output limit
        double _upper = 255;            // Upper output limit
        uint64_t _prev_time = 0;        // Previous time for computing time difference
        bool _isAngular = false;        // Flag to indicate whether the system is angular
        bool _isEnabled = false;        // Flag to indicate whether the PID controller is enabled

    public:
        // Constructors
        #pragma region constructors
        /**
         * @brief Default PID constructor.
         */
        PID();

        /**
         * @brief Constructor with PID constants.
         * @param kp Proportional gain.
         * @param ki Integral gain.
         * @param kd Derivative gain.
         */
        PID(double kp, double ki, double kd);

        /**
         * @brief Constructor with angular flag.
         * @param isAngular Boolean flag indicating whether the system is angular.
         */
        PID(bool isAngular);

        /**
         * @brief Constructor with angular flag and PID constants.
         * @param isAngular PBoolean flag indicating whether the system is angular.
         * @param kp Proportional gain.
         * @param ki Integral gain.
         * @param kd Derivative gain.
         */
        PID(bool isAngular, double kp, double ki, double kd);
        #pragma endregion constructors

        // Setters
        #pragma region setters
        /**
         * @brief Set PID constants.
         * @param kp Proportional gain.
         * @param ki Integral gain.
         * @param kd Derivative gain.
         */
        void setConstants(double kp, double ki, double kd);
        
        /**
         * @brief Set setpoint value.
         * @param setpoint The desired setpoint value.
         */
        void setSetPoint(double setpoint);

        /**
         * @brief Set output constraints.
         * @param lower The lower output limit.
         * @param upper The upper output limit.
         */
        void setConstrains(double lower, double upper);

        /**
         * @brief Enable PID controller.
         */
        void enable();
        
        /**
         * @brief Disable PID controller.
         */
        void disable();
        #pragma endregion setters

        // Compute PID output
        /**
         * @brief Compute PID output.
         * @param feedback The feedback value from the system.
         * @return The computed PID output.
         */
        double compute(double feedback);
};

#endif /* PID_H */
