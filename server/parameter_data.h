#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "server/spiffs_helper.h"
#include <vector>

#define PARAMETER_FILE_NAME "/parameter.json"

#define DEBUG_DATA 1

// TODO:
// parameter_data should be totaly independant of server!!!

// ---------------------------------------------------------------------------------------
// used by Server which utilizes WiFi, WebSockets, and JSON for configuration and control.
//
// Requires SPIFFS for file system operations. For setup, refer to:
// https://github.com/me-no-dev/arduino-esp32fs-plugin/releases/
// https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/
//
// Icon from: https://icons8.com/icons/set/favicon
// Based on code by mo thunderz, updated last on 11.09.2022.
// https://github.com/mo-thunderz/Esp32WifiPart4
// ---------------------------------------------------------------------------------------

#define CREATE_PARAMETER(name, defaultValue) \
    Parameter name{this, #name, int(defaultValue)};
    
// ==============================
class ParameterData
{
public:
    struct Parameter
    {
        String name;
        float value;
        ParameterData *_parent;

        Parameter(
            ParameterData *parent,
            const String &name,
            float default_value) : _parent(parent),
                                 name(name),
                                 value(default_value)
        {
            _parent->register_parameter(this);
        }

        Parameter &operator=(float v)
        {
            value = v;
            _parent->mark_parameter_changed_from_code(this);
            return *this;
        }
        operator float() const { return value; }

        float mapConstrainf(float fromLow, float fromHigh, float toLow, float toHigh)
        {
            return util::mapConstrainf(float(value), fromLow, fromHigh, toLow, toHigh);
        }
    };

    // TODO really define this twice ?!
    using ParameterList = std::vector<ParameterData::Parameter *>;

    ParameterList parameters;
    void register_parameter(Parameter *param) { parameters.push_back(param); }

    bool save() const
    {
#if DEBUG_DATA
        Serial.println("ParameterData::save() !!! ");
#endif

        File parameterFile = SPIFFS.open(PARAMETER_FILE_NAME, "w");
        if (!parameterFile)
        {
            Serial.println("Failed to open parameter file for writing");
            return false;
        }
        DynamicJsonDocument doc(1024);

        for (auto param : parameters)
            doc[param->name] = param->value;

        if (serializeJson(doc, parameterFile) == 0)
        {
            Serial.println("Failed to write file");
            parameterFile.close();
            return false;
        }

        parameterFile.close();
        return true;
    }

    bool load()
    {
        initFS();
#if DEBUG_DATA
        Serial.println("ParameterData::load() !!! ");
#endif
        File parameterFile = SPIFFS.open(PARAMETER_FILE_NAME, "r");
        if (!parameterFile)
        {
            Serial.println("Failed to open file");
            return false;
        }

        size_t size = parameterFile.size();
        if (size > 1024)
        {
            Serial.println("file size is too large");
            parameterFile.close();
            return false;
        }

        std::unique_ptr<char[]> buf(new char[size]);
        parameterFile.readBytes(buf.get(), size);
        parameterFile.close();

        DynamicJsonDocument doc(1024);
        auto error = deserializeJson(doc, buf.get());
        if (error)
        {
            Serial.println("Failed to parse file");
            return false;
        }

        for (auto param : parameters)
            param->value = doc[param->name];

#if DEBUG_DATA
        Serial.printf("Loaded user data: (%d) \n", parameters.size());
        for (auto param : parameters)
            Serial.printf("%s: %d\n", param->name.c_str(), param->value);
#endif

        return true;
    }

    void didUpdate()
    {
        _wasUpdated = true;
    }

    bool wasUpdated()
    {
        if (_wasUpdated)
        {
            _wasUpdated = false;
            save();
            return true;
        }
        return false;
    }

    bool parseAll(uint8_t *payload)
    {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return false;
        }

        const String name = static_cast<const char *>(doc["name"]);
        const int value = doc["value"];

        for (auto param : parameters)
        {
            if (name == param->name)
            {
                if (param->value != value)
                {
                    param->value = value;
                    _wasUpdated = true;
                    mark_parameter_changed_from_server(param);
                }
            }
        }
        return _wasUpdated;
    }

private:
    bool _wasUpdated = false;

public:
    ParameterList _parameters_changed_from_server;
    void mark_parameter_changed_from_server(Parameter *param)
    {
        // printf("mark_parameter_changed_from_server: %s = %d \n", param->name.c_str(), param->value);
        if (std::find(_parameters_changed_from_server.begin(), _parameters_changed_from_server.end(), param) == _parameters_changed_from_server.end())
            _parameters_changed_from_server.push_back(param);
    }

    ParameterList getParameter_changed_from_server()
    {
        ParameterList parameter = _parameters_changed_from_server;
        _parameters_changed_from_server.clear();
        return parameter;
    }

public:
    ParameterList _parameters_changed_from_code;
    void mark_parameter_changed_from_code(Parameter *param)
    {
        // printf("mark_parameter_changed_from_code: %s = %d \n", param->name.c_str(), param->value);
        if (std::find(_parameters_changed_from_code.begin(), _parameters_changed_from_code.end(), param) == _parameters_changed_from_code.end())
            _parameters_changed_from_code.push_back(param);
    }

    ParameterList getParameter_changed_from_code()
    {
        ParameterList parameter = _parameters_changed_from_code;
        _parameters_changed_from_code.clear();
        return parameter;
    }
};

using ParameterList = std::vector<ParameterData::Parameter *>;