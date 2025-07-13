#pragma once

/**
 * @file mqtt.h
 * @brief MQTT interface for ESP32
 *
 * This file contains the MQTT class which provides methods to setup, manage, and communicate with
 * an MQTT server. It includes functionality to send and receive messages, handle light component
 * commands, and publish discovery messages.
 *
 * Usage:
 *
 * void setup() {
 *   if (mqtt.isRechableAndActive()) {
 *     mqtt.setup();
 *     mqtt.setLightChangeCallback([](const String &name, float percent) {
 *       // Handle light change
 *     });
 *   }
 * }
 *
 * void loop() {
 *   mqtt.loop();
 *   // Other loop code
 * }
 */

// CURRENTLY ONLY TESTED with "light" components

#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#endif

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>

#define USE_NODE_ID 0

// should always be used like:
//
// void setup() {
//   if (mqtt.isRechableAndActive())
//   {
//     mqtt.setup();
//     mqtt.setLightChangeCallback( [.....]
//    }
// }
//
// void loop() {
//   mqtt.loop();
// [...]
// }

class MQTT
{
public:
    using LightChangeCallback = std::function<void(const String&, float)>;
    using LightToggleCallback = std::function<void(const String&, bool)>;

    MQTT(const char* server, int port, const String& device_name);
    MQTT(const char* server, int port, Client& client, const String& device_name);

    void setup();
    bool isRechableAndActive(); // call this once in the beginning

    void loop();
    void publishComponent(String component_name, const DynamicJsonDocument& stateDoc);
    void publishLight(String component_name, float percent);

    bool _isActive = false;

    void setLightChangeCallback(LightChangeCallback callback);

    void setLightToggleCallback(LightToggleCallback callback);

    struct Component
    {
        String platform;
        String name; // != object_id as the object_id contains device_name to be unique in
                     // homeassistant
    };

    // Usage:
    // addLight("some_light");
    // addComponent("sensor", "position"); // theoretically , but not implemented
    void addLight(const String& name) { addComponent("light", name); }
    void addComponent(const String& platform, const String& name);

    std::vector<Component> _components;

private:
    PubSubClient _client;
    const char*  _server;
    int          _port;

    bool _subscribed = false;

    void reconnect();
    void publishDiscoveryMessage(Component& component);
    void handleCallback(char* topic, byte* payload, unsigned int length);

    LightChangeCallback _lightChangeCallback;
    LightToggleCallback _lightToggleCallback;

    const String _discovery_prefix = "homeassistant";
    String      _device_name;

    String _device_id;

    String getStateTopicFromComponents(Component cmp) { return getBaseTopic(cmp) + "/state"; }
    String getCommandTopicFromComponents(Component cmp) { return getBaseTopic(cmp) + "/set"; }

    String getBaseTopic(Component cmp)
    {
#if USE_NODE_ID
        return _device_name + "/" + cmp.platform + "/" + _device_name + "/" + cmp.name;
#else
        return _device_name + "/" + cmp.platform + "/" + cmp.name;
#endif
    }

    void publishState(const String& topic, const DynamicJsonDocument& stateDoc);

    void processLightCommand(const String& component_name, const DynamicJsonDocument& doc);

    void lightChange(const String& component_name, float percent);
    void lightToggle(const String& component_name, bool state);

protected:
    void publishCustomTopic(const char* topic, const char* payload) { _client.publish(topic, payload); }
};

