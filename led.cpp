#include "led.h"
#include <cmath>

namespace util {
namespace led {

Driver::Driver(const PWMConfig& pwm_config) 
    : pwm_driver_(pwm_config) {
}

void Driver::setup() {
    pwm_driver_.setup(pwm_driver_.getConfig());
    
    // Set default animation to static
    setAnimation(AnimationMode::STATIC);
    
    initialized_ = true;
    Serial.printf("LED Driver: Initialized with pin %d\n", pwm_driver_.getConfig().pin);
}

void Driver::loop() {
    if (!initialized_) return;
    
    if (since_loop_ > 2) {
        since_loop_ = 0;
        applyBrightness();
    }
}

void Driver::set(float percentage) {
    since_set_ = 0;
    target_brightness_ = util::clipf(percentage, 0.0f, 1.0f);
    
    // If we have a static animation, update its brightness
    if (animation_ && animation_->getConfig().mode == AnimationMode::STATIC) {
        // Create a new static animation with the new brightness
        animation_ = std::make_unique<StaticAnimation>(target_brightness_);
    }
}

void Driver::setDirectly(float percentage) {
    set(percentage);
    current_brightness_ = target_brightness_;
    applyBrightness();
}

void Driver::toggle(bool state) {
    if (!state) {
        // Turn off
        last_target_brightness_ = target_brightness_;
        set(0.0f);
    } else {
        // Turn on to last brightness
        set(last_target_brightness_);
    }
}

void Driver::setAnimation(AnimationMode mode, const AnimationConfig& config) {
    AnimationConfig new_config = config;
    new_config.mode = mode;
    
    // Create new animation
    animation_ = createAnimation(mode);
    if (animation_) {
        animation_->setConfig(new_config);
        animation_->reset();
    }
}

const AnimationConfig& Driver::getAnimationConfig() const {
    static const AnimationConfig default_config;
    return animation_ ? animation_->getConfig() : default_config;
}

void Driver::updatePWMConfig(const PWMConfig& config) {
    pwm_driver_.updateConfig(config);
}

void Driver::applyBrightness() {
    if (!animation_) return;
    
    // Get animated brightness
    float animated_brightness = animation_->update(since_set_);
    
    // Apply filtering for smooth transitions
    simpleFilterf(current_brightness_, animated_brightness, filter_value_);
    
    // Apply gamma correction
    float corrected_brightness = applyGamma(current_brightness_);
    
    // Set PWM output
    pwm_driver_.set(corrected_brightness);
    
    // Call change callback if provided
    if (on_change_callback_) {
        on_change_callback_(current_brightness_);
    }
}

float Driver::applyGamma(float value) const {
    return powf(util::clipf(value, 0.0f, 1.0f), gamma_);
}

std::unique_ptr<AnimationDriver> Driver::createAnimation(AnimationMode mode) {
    switch (mode) {
        case AnimationMode::STATIC:
            return std::make_unique<StaticAnimation>(target_brightness_);
        case AnimationMode::BREATH:
            return std::make_unique<BreathAnimation>();
        case AnimationMode::PULSE:
            return std::make_unique<PulseAnimation>();
        case AnimationMode::WAVE:
            return std::make_unique<WaveAnimation>();
        case AnimationMode::RANDOM:
            return std::make_unique<RandomAnimation>();
        default:
            return std::make_unique<StaticAnimation>(target_brightness_);
    }
}

} // namespace led
} // namespace util 