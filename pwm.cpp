#include "pwm.h"

namespace led {

// Static member initialization
uint8_t PWMDriver::next_channel_ = 0;

PWMDriver::PWMDriver(const PWMConfig& config) : config_(config) {}

void PWMDriver::setup(const PWMConfig& config) {
    config_ = config;
    initializeHardware();
}

void PWMDriver::initializeHardware() {
    if (initialized_) {
        cleanupHardware();
    }
    
    Serial.printf("PWM: Setting up pin %d, freq=%u Hz, res=%d bits\n", 
                  config_.pin, config_.frequency, config_.resolution_bits);
    
    pinMode(config_.pin, OUTPUT);
    
#ifdef ARDUINO_ARCH_ESP32
    channel_ = getNextChannel();
    ledcAttach(config_.pin, config_.frequency, config_.resolution_bits);
#else
    analogWriteFreq(config_.frequency);
    analogWriteResolution(config_.resolution_bits);
#endif
    
    initialized_ = true;
    set(0.0f); // Initialize to off
}

void PWMDriver::cleanupHardware() {
    if (!initialized_) return;
    
#ifdef ARDUINO_ARCH_ESP32
    ledcDetach(config_.pin);
#endif
    
    initialized_ = false;
}

void PWMDriver::set(float percentage) {
    if (!initialized_) return;
    
    current_duty_ = util::clipf(percentage, 0.0f, 1.0f);
    
    const uint32_t max_value = (1U << config_.resolution_bits) - 1;
    const uint32_t pwm_value = static_cast<uint32_t>(current_duty_ * max_value);
    
#ifdef ARDUINO_ARCH_ESP32
    ledcWrite(config_.pin, pwm_value);
#else
    analogWrite(config_.pin, pwm_value);
#endif
}

void PWMDriver::updateConfig(const PWMConfig& new_config) {
    if (config_.pin != new_config.pin) {
        // Different pin, need full reinitialization
        setup(new_config);
    } else {
        // Same pin, can update frequency/resolution
        config_ = new_config;
        initializeHardware();
    }
}

uint8_t PWMDriver::getNextChannel() {
    if (next_channel_ >= MAX_CHANNELS) {
        Serial.println("WARNING: PWM channel limit reached!");
        return 0;
    }
    return next_channel_++;
}

} // namespace led 