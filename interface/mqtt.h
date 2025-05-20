#pragma once

/**
 * @file mqtt.h
 * @brief MQTT interface for ESP32
 *
 * This file contains the MQTT class which provides methods to setup, manage, and communicate with an MQTT server.
 * It includes functionality to send and receive messages, handle light component commands, and publish discovery messages.
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

#if ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>
#include "config.h"

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
    MQTT(const char *server, int port)
        : _server(server), _port(port), _client(*(new WiFiClient())) {}

    MQTT(const char *server, int port, Client &client)
        : _server(server), _port(port), _client(client) {}

    void setup();
    bool isRechableAndActive(); // call this once in the beginning

    void loop();
    void sendComponent(String component_name, String value);
    void sendLightBrightness(String component_name, uint8_t brightness);
    void sendLight(String component_name, int value);

    bool _isActive = false;

    using LightChangeCallback = std::function<void(const String &name, float percent)>;
    void setLightChangeCallback(LightChangeCallback callback);

    struct Component
    {
        String type;
        String name;
    };

    // Usage:
    // addComponent("sensor", "position");
    // addComponent("switch", "relay");
    // addComponent("light", "rotation_CW");
    void addComponent(const String &type, const String &name);

private:
    PubSubClient _client;
    const char *_server;
    int _port;

    bool _subscribed = false;

    void reconnect();
    void publishDiscoveryMessage(Component component);
    void handleCallback(char *topic, byte *payload, unsigned int length);

    LightChangeCallback _lightChangeCallback;

    const String _discovery_prefix = "homeassistant";
    const String _device_name = DEVICE_NAME;

    String _device_id;

    std::vector<Component> _components;

    String getStateTopicFromComponents(Component component) { return getBaseFromComponent(component) + "/state"; }
    String getCommandTopicFromComponents(Component component) { return getBaseFromComponent(component) + "/set"; }
    String getBaseFromComponent(Component component) { return _device_name + "/" + component.type + "/" + component.name; }

    void publishState(const String &topic, const DynamicJsonDocument &stateDoc);
    void processLightCommand(const String &component_name, const DynamicJsonDocument &doc);
    void reactToLightChange(const String &component_name, uint8_t brightness);
};

void MQTT::addComponent(const String &type, const String &name)
{
    _components.push_back({type, name});
}

void MQTT::setup()
{
    _client.setBufferSize(1024);
    _client.setServer(_server, _port);
    _client.setCallback([this](char *topic, byte *payload, unsigned int length)
                        { this->handleCallback(topic, payload, length); });

    String mac = WiFi.macAddress();
    mac.replace(":", "");
    _device_id = _device_name + String("_") + mac;

    reconnect();
}

void MQTT::loop()
{
    if (!_isActive)
        return;

    if (!_client.connected())
        reconnect();

    _client.loop();

    if (_client.connected() && !_subscribed)
    {
        printf("Subscribing to topics\n");

        for (const auto &component : _components)
        {
            if (component.type == "light")
            {
                printf("Subscribing to %s\n", getCommandTopicFromComponents(component).c_str());
                _client.subscribe(getCommandTopicFromComponents(component).c_str());
            }
        }
        _subscribed = true;
        printf("Subscribed to all topics\n");
    }
}

void MQTT::sendComponent(String component_name, String value)
{
    for (const auto &component : _components)
    {
        if (component.name == component_name)
        {
            String topic = getStateTopicFromComponents(component);

            DynamicJsonDocument stateDoc(200);
            stateDoc["value"] = value;

            publishState(topic, stateDoc);
            return;
        }
    }
    printf("Error: Component not found\n");
}

void MQTT::sendLightBrightness(String component_name, uint8_t brightness)
{
    for (const auto &component : _components)
    {
        if (component.name == component_name && component.type == "light")
        {
            String topic = getStateTopicFromComponents(component);

            DynamicJsonDocument stateDoc(200);
            stateDoc["state"] = brightness > 0 ? "ON" : "OFF";
            stateDoc["brightness"] = brightness;

            printf("sendLight %s: %d\n", component_name.c_str(), brightness);

            publishState(topic, stateDoc);
            return;
        }
    }
    printf("Error: Component not found\n");
}

void MQTT::sendLight(String component_name, int value)
{
    if (_isActive)
        sendLightBrightness(component_name, util::mapConstrainf(value, 0, 100, 0, 255));
}

bool MQTT::isRechableAndActive()
{
    printf("Trying to reach MQTT server...\n");
    WiFiClient client;
    for (int i = 0; i < 3; ++i)
    {
        if (client.connect(_server, _port))
        {
            _isActive = true;
            return _isActive;
        }
        delay(500);
    }
    _isActive = false;
    if (!_isActive)
        printf("MQTT server not reachable, skipping MQTT setup.");
    return _isActive;
}

void MQTT::setLightChangeCallback(LightChangeCallback callback)
{
    _lightChangeCallback = callback;
}

void MQTT::publishDiscoveryMessage(Component component)
{
    if (!_isActive)
        return;

    String discovery_topic = _discovery_prefix + "/" + component.type + "/" + component.name + "/config";
    String state_topic = getStateTopicFromComponents(component);

    DynamicJsonDocument doc(1024);

    JsonObject device = doc.createNestedObject("device");
    device["name"] = _device_name;
    device["ids"] = _device_id;
    device["mf"] = "heartwerk.tech";
    device["mdl"] = _device_name;
    device["sw"] = "0.1";
    device["hw"] = "0.1";

    String name = component.name;
    doc["name"] = name;
    doc["unique_id"] = name + "_" + WiFi.macAddress();
    if (component.type == "light")
    {
        doc["~"] = getBaseFromComponent(component);
        doc["cmd_t"] = "~/set";
        doc["stat_t"] = "~/state";
        doc["schema"] = "json";
        doc["brightness"] = true;
    }
    else
    {
        doc["state_topic"] = state_topic;
    }

    doc["unit_of_measurement"] = "";
    doc["value_template"] = "{{ value_json.value }}";

    char buffer[1024];
    serializeJson(doc, buffer);

    if (_client.publish(discovery_topic.c_str(), buffer, true))
        printf("Config published for auto-discovery\n");
    else
        printf("Failed to publish config\n");
}

void MQTT::handleCallback(char *topic, byte *payload, unsigned int length)
{
    String message;
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
    printf("Message arrived [%s]: %s\n", topic, message.c_str());

    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        printf("Failed to parse JSON: %s\n", error.c_str());
        return;
    }

    String topicStr = String(topic);

    if (topicStr.indexOf("/light/") != -1)
        for (const auto &component : _components)
            if (component.type == "light")
            {
                String componentNameInTopic = topicStr.substring(topicStr.indexOf("/light/") + 7);
                componentNameInTopic = componentNameInTopic.substring(0, componentNameInTopic.indexOf("/set"));

                // printf("Comparing %s with %s\n", componentNameInTopic.c_str(), component.name.c_str());
                if (componentNameInTopic == component.name)
                {
                    processLightCommand(component.name, doc);
                    return;
                }
            }
}

void MQTT::reconnect()
{
    while (!_client.connected())
    {
        printf("Attempting MQTT connection...");
        if (_client.connect(_device_id.c_str())) // THIS HAS TO BE UNIQUE per device
        {
            printf("connected\n");

            for (const auto &component : _components)
                publishDiscoveryMessage(component);

            _subscribed = false;
        }
        else
        {
            printf("failed, rc=%d try again in 5 seconds\n", _client.state());
            delay(2000);
        }
    }
}

void MQTT::publishState(const String &topic, const DynamicJsonDocument &stateDoc)
{
    if (!_isActive)
        return;

    String stateJson;
    serializeJson(stateDoc, stateJson);

    if (_client.publish(topic.c_str(), stateJson.c_str()))
    {
        // printf("State published successfully\n");
    }
    else
        printf("Failed to publish state\n");
}

void MQTT::processLightCommand(const String &component_name, const DynamicJsonDocument &doc)
{
    if (doc.containsKey("state"))
    {
        String state = doc["state"];
        printf("State: %s\n", state.c_str());

        if (state == "OFF")
        {
            reactToLightChange(component_name, 0);
        }
        else if (state != "ON")
            printf("Invalid state value\n");
    }
    else
        printf("State key not found in JSON\n");

    if (doc.containsKey("brightness"))
    {
        uint8_t brightness = doc["brightness"];

        printf("Brightness: %d\n", brightness);

        reactToLightChange(component_name, brightness);
    }
    else
        printf("Brightness key not found in JSON\n");
}

void MQTT::reactToLightChange(const String &component_name, uint8_t brightness)
{
    // loopback to mqtt
    sendLightBrightness(component_name, brightness);

    // use elsewhere via callback
    float percent = brightness / 255.0;
    if (_lightChangeCallback)
        _lightChangeCallback(component_name, percent);
}