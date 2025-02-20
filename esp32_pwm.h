#pragma once
#if ESP32
#include <Arduino.h>

struct ESP32_PWM
{
    // PWM properties
    static const int resolution = 8;
    static const int resolution_range = (1 << resolution);
    static const int MAX_CH = 17;

    static int _num_ch;
    
    struct PWM_CH
    {
        int pin; // physical pin
        int num; // ledc ch index
        int value = 0; // pwm
    };

    static PWM_CH _ch[MAX_CH];

    static void add(uint8_t pin)
    {
        _ch[_num_ch].num = _num_ch;
        _ch[_num_ch].pin = pin;
        pinMode(_ch[_num_ch].pin, OUTPUT);
        
        _num_ch++;
    }


    static void init(int freq = 20000)
    {
        for (int n = 0; n < _num_ch; n++)
        {
            ledcSetup(_ch[n].num, freq, resolution);
            ledcAttachPin(_ch[n].pin, _ch[n].num);
        }
    }

    static void set(int pin, float value) // 0-1
    {
        // find ch from pin
        int ch = -1;
        for (int n = 0; n < _num_ch; n++)
            if (_ch[n].pin == pin)
            {
                ch = _ch[n].num;
                break;
            }
        if (ch == -1)
            Serial.printf("ALARM, pin %d not found\n", pin);
            
        uint32_t duty = float(resolution_range) * value;
        ledcWrite(ch, duty);
    }
};

// Define _num_ch and _ch
int ESP32_PWM::_num_ch = 0;
ESP32_PWM::PWM_CH ESP32_PWM::_ch[MAX_CH];


#endif