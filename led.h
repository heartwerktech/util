#pragma once
#include <Arduino.h>
#include <elapsedMillis.h>
#include <functional>
#include <memory>
#include <vector>
#include "pwm.h"
#include "util.h"

namespace util {

namespace led {

// Import PWM types from the led namespace
using ::led::PWMConfig;
using ::led::PWMDriver;

/**
 * @brief Animation mode enumeration
 */
enum class AnimationMode
{
    STATIC, // Static brightness
    BREATH, // Breathing effect
    PULSE,  // Pulsing effect
    WAVE,   // Wave pattern
    RANDOM  // Random brightness changes
};

/**
 * @brief Animation configuration
 */
struct AnimationConfig
{
    AnimationMode mode           = AnimationMode::STATIC;
    float         speed          = 1.0f; // Animation speed multiplier
    float         min_brightness = 0.0f; // Minimum brightness (0.0-1.0)
    float         max_brightness = 1.0f; // Maximum brightness (0.0-1.0)
    uint32_t      period_ms      = 2000; // Animation period in milliseconds

    AnimationConfig() = default;
    AnimationConfig(AnimationMode m,
                    float         spd    = 1.0f,
                    float         min    = 0.0f,
                    float         max    = 1.0f,
                    uint32_t      period = 2000)
        : mode(m)
        , speed(spd)
        , min_brightness(min)
        , max_brightness(max)
        , period_ms(period)
    {
    }
};

/**
 * @brief Base animation driver class
 */
class AnimationDriver
{
public:
    virtual ~AnimationDriver() = default;

    /**
     * @brief Update animation and return current brightness
     * @param elapsed_ms Time elapsed since last update
     * @return Current brightness value (0.0-1.0)
     */
    virtual float update(uint32_t elapsed_ms) = 0;

    /**
     * @brief Reset animation to initial state
     */
    virtual void reset() = 0;

    /**
     * @brief Get animation configuration
     */
    [[nodiscard]] const AnimationConfig& getConfig() const { return config_; }

    /**
     * @brief Update animation configuration
     */
    virtual void setConfig(const AnimationConfig& config) { config_ = config; }

protected:
    AnimationConfig config_;
    uint32_t        animation_time_ = 0;
};

/**
 * @brief Static animation driver (no animation)
 */
class StaticAnimation : public AnimationDriver
{
public:
    explicit StaticAnimation(float brightness = 1.0f)
    {
        config_.mode       = AnimationMode::STATIC;
        static_brightness_ = brightness;
    }

    float update(uint32_t elapsed_ms) override { return static_brightness_; }

    void reset() override
    {
        // No state to reset for static animation
    }

    void setBrightness(float brightness)
    {
        static_brightness_ = util::clipf(brightness, 0.0f, 1.0f);
    }

private:
    float static_brightness_ = 1.0f;
};

/**
 * @brief Breathing animation driver
 */
class BreathAnimation : public AnimationDriver
{
public:
    BreathAnimation() { config_.mode = AnimationMode::BREATH; }

    float update(uint32_t elapsed_ms) override
    {
        animation_time_ += elapsed_ms;
        const float cycle_time = config_.period_ms / config_.speed;
        const float phase      = (animation_time_ % static_cast<uint32_t>(cycle_time)) / cycle_time;

        // Use sine wave for smooth breathing effect
        const float sine_value = (sinf(phase * 2.0f * PI) + 1.0f) * 0.5f;
        return util::mapConstrainf(sine_value,
                                   0.0f,
                                   1.0f,
                                   config_.min_brightness,
                                   config_.max_brightness);
    }

    void reset() override { animation_time_ = 0; }
};

/**
 * @brief Pulse animation driver
 */
class PulseAnimation : public AnimationDriver
{
public:
    PulseAnimation() { config_.mode = AnimationMode::PULSE; }

    float update(uint32_t elapsed_ms) override
    {
        animation_time_ += elapsed_ms;
        const float cycle_time = config_.period_ms / config_.speed;
        const float phase      = (animation_time_ % static_cast<uint32_t>(cycle_time)) / cycle_time;

        // Use square wave for pulsing effect
        const float pulse_value = (phase < 0.5f) ? 1.0f : 0.0f;
        return util::mapConstrainf(pulse_value,
                                   0.0f,
                                   1.0f,
                                   config_.min_brightness,
                                   config_.max_brightness);
    }

    void reset() override { animation_time_ = 0; }
};

/**
 * @brief Wave animation driver
 */
class WaveAnimation : public AnimationDriver
{
public:
    WaveAnimation() { config_.mode = AnimationMode::WAVE; }

    float update(uint32_t elapsed_ms) override
    {
        animation_time_ += elapsed_ms;
        const float cycle_time = config_.period_ms / config_.speed;
        const float phase      = (animation_time_ % static_cast<uint32_t>(cycle_time)) / cycle_time;

        // Use triangle wave for wave effect
        const float triangle_value = (phase < 0.5f) ? phase * 2.0f : (1.0f - phase) * 2.0f;
        return util::mapConstrainf(triangle_value,
                                   0.0f,
                                   1.0f,
                                   config_.min_brightness,
                                   config_.max_brightness);
    }

    void reset() override { animation_time_ = 0; }
};

/**
 * @brief Random animation driver
 */
class RandomAnimation : public AnimationDriver
{
public:
    RandomAnimation()
    {
        config_.mode = AnimationMode::RANDOM;
        last_change_time_ = 0;
        current_random_   = 0.5f;
    }

    float update(uint32_t elapsed_ms) override
    {
        animation_time_ += elapsed_ms;
        const uint32_t change_interval = static_cast<uint32_t>(config_.period_ms / config_.speed);

        if (animation_time_ - last_change_time_ >= change_interval)
        {
            // Generate new random value
            current_random_ = static_cast<float>(random(config_.min_brightness * 1000,
                                                       config_.max_brightness * 1000)) / 1000.0f;
            last_change_time_ = animation_time_;
        }

        return current_random_;
    }

    void reset() override
    {
        animation_time_    = 0;
        last_change_time_  = 0;
        current_random_    = 0.5f;
    }

private:
    uint32_t last_change_time_;
    float    current_random_;
};

/**
 * @brief Main LED driver class
 */
class Driver
{
public:
    explicit Driver(const PWMConfig& pwm_config);

    void setup();
    void loop();

    void set(float percentage);
    void setDirectly(float percentage);
    void toggle(bool state);

    [[nodiscard]] float get() const { return target_brightness_; }
    [[nodiscard]] float getCurrent() const { return current_brightness_; }

    void setAnimation(AnimationMode mode, const AnimationConfig& config = {});
    [[nodiscard]] const AnimationConfig& getAnimationConfig() const;

    /**
     * @brief Set gamma correction
     * @param gamma Gamma value (typically 2.2-2.8)
     */
    void setGamma(float gamma) { gamma_ = gamma; }

    /**
     * @brief Set filter value for smooth transitions
     * @param value Filter value (0.0-1.0, lower = smoother)
     */
    void setFilterValue(float value) { filter_value_ = value; }

    /**
     * @brief Set callback for brightness changes
     * @param callback Function to call when brightness changes
     */
    void setOnChangeCallback(std::function<void(float)> callback)
    {
        on_change_callback_ = std::move(callback);
    }

    void updatePWMConfig(const PWMConfig& config);

private:
    PWMDriver pwm_driver_;
    std::unique_ptr<AnimationDriver> animation_;
    
    float target_brightness_ = 0.0f;
    float current_brightness_ = 0.0f;
    float last_target_brightness_ = 1.0f;
    
    float gamma_ = 2.2f;
    float filter_value_ = 0.1f;
    
    bool initialized_ = false;
    
    elapsedMillis since_loop_;
    elapsedMillis since_set_;
    
    std::function<void(float)> on_change_callback_;
    
    void applyBrightness();
    float applyGamma(float value) const;
    std::unique_ptr<AnimationDriver> createAnimation(AnimationMode mode);
};

} // namespace led

} // namespace util

// Type alias for convenience
using LedDrivers = std::vector<std::unique_ptr<util::led::Driver>>; 