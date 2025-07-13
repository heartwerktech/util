#pragma once
#include <Arduino.h>
#include <memory>
#include <vector>
#include "util.h"

namespace led {

/**
 * @brief PWM configuration structure for runtime configuration
 */
struct PWMConfig {
    uint32_t frequency = 20000;  // Hz
    uint8_t resolution_bits = 8; // 1-16 bits
    uint8_t pin = 0;
    
    PWMConfig() = default;
    PWMConfig(uint8_t pin, uint32_t freq = 20000, uint8_t res = 8) 
        : frequency(freq), resolution_bits(res), pin(pin) {}
};

/**
 * @brief Modern PWM driver with runtime configuration
 */
class PWMDriver {
public:
    explicit PWMDriver(const PWMConfig& config = {});
    ~PWMDriver() = default;
    
    // Modern C++: disable copy, enable move
    PWMDriver(const PWMDriver&) = delete;
    PWMDriver& operator=(const PWMDriver&) = delete;
    PWMDriver(PWMDriver&&) = default;
    PWMDriver& operator=(PWMDriver&&) = default;
    
    /**
     * @brief Initialize the PWM driver
     * @param config PWM configuration
     */
    void setup(const PWMConfig& config);
    
    /**
     * @brief Set PWM duty cycle (0.0 to 1.0)
     * @param percentage Duty cycle as percentage
     */
    void set(float percentage);
    
    /**
     * @brief Get current duty cycle
     * @return Current duty cycle (0.0 to 1.0)
     */
    [[nodiscard]] float get() const { return current_duty_; }
    
    /**
     * @brief Get current configuration
     * @return Current PWM configuration
     */
    [[nodiscard]] const PWMConfig& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration at runtime
     * @param new_config New PWM configuration
     */
    void updateConfig(const PWMConfig& new_config);
    
    /**
     * @brief Check if driver is properly initialized
     * @return true if initialized
     */
    [[nodiscard]] bool isInitialized() const { return initialized_; }

private:
    void initializeHardware();
    void cleanupHardware();
    
    static uint8_t getNextChannel();
    
    PWMConfig config_;
    float current_duty_ = 0.0f;
    uint8_t channel_ = 0;
    bool initialized_ = false;
    
    static uint8_t next_channel_;
    static constexpr uint8_t MAX_CHANNELS = 16;
};

// Static member declaration
static uint8_t next_channel_;

} // namespace led
