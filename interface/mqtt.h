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
    MQTT(const char* server, int port)
        : _server(server)
        , _port(port)
        , _client(*(new WiFiClient()))
    {
    }

    MQTT(const char* server, int port, Client& client)
        : _server(server)
        , _port(port)
        , _client(client)
    {
    }

    void setup();
    bool isRechableAndActive(); // call this once in the beginning

    void loop();
    void sendComponent(String component_name, String value);
    void publishLight(String component_name, float percent);

    bool _isActive = false;

    using LightChangeCallback = std::function<void(const String& name, float percent)>;
    void setLightChangeCallback(LightChangeCallback callback);

    using LightToggleCallback = std::function<void(const String& name, bool state)>;
    void setLightToggleCallback(LightToggleCallback callback);

    struct Component
    {
        String type;
        String name;
    };

    // Usage:
    // addComponent("sensor", "position");
    // addComponent("switch", "relay");
    // addComponent("light", "rotation_CW");
    void addComponent(const String& type, const String& name);

private:
    PubSubClient _client;
    const char*  _server;
    int          _port;

    bool _subscribed = false;

    void reconnect();
    void publishDiscoveryMessage(Component component);
    void publishDiscoveryMessageDevice();
    void handleCallback(char* topic, byte* payload, unsigned int length);

    LightChangeCallback _lightChangeCallback;
    LightToggleCallback _lightToggleCallback;

    const String _discovery_prefix = "homeassistant";
    const String _device_name      = DEVICE_NAME;

    String _device_id;

    std::vector<Component> _components;

    String getStateTopicFromComponents(Component component)
    {
        return getBaseFromComponent(component) + "/state";
    }
    String getCommandTopicFromComponents(Component component)
    {
        return getBaseFromComponent(component) + "/set";
    }
    String getBaseFromComponent(Component component)
    {
        return _device_name + "/" + component.type + "/" DEVICE_NAME + "/" + component.name;
    }

    void publishState(const String& topic, const DynamicJsonDocument& stateDoc);
    void processLightCommand(const String& component_name, const DynamicJsonDocument& doc);

    void lightChange(const String& component_name, float percent);
    void lightToggle(const String& component_name, bool state);
};

void MQTT::addComponent(const String& type, const String& name)
{
    _components.push_back({type, name});
}

void MQTT::setup()
{
    _client.setBufferSize(1024);
    _client.setServer(_server, _port);
    _client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->handleCallback(topic, payload, length);
    });

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

        for (const auto& component : _components)
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
    for (const auto& component : _components)
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

void MQTT::publishLight(String component_name, float percent)
{

    if (!_isActive)
        return;

    uint8_t brightness = util::mapConstrainf(percent, 0.0f, 1.0f, 0, 255);

    for (const auto& component : _components)
    {
        if (component.name == component_name && component.type == "light")
        {
            String topic = getStateTopicFromComponents(component);

            DynamicJsonDocument stateDoc(200);
            stateDoc["state"]      = brightness > 0 ? "ON" : "OFF";
            stateDoc["brightness"] = brightness;

            printf("publishLight %s: %2.2f | homeassistant=%d | raw=%d\n",
                   component_name.c_str(),
                   percent,
                   int(percent * 100),
                   brightness);

            publishState(topic, stateDoc);
            return;
        }
    }
    printf("Error: Component not found\n");
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

void MQTT::setLightChangeCallback(LightChangeCallback callback) { _lightChangeCallback = callback; }

void MQTT::setLightToggleCallback(LightToggleCallback callback) { _lightToggleCallback = callback; }

void MQTT::publishDiscoveryMessage(Component component)
{
    if (!_isActive)
        return;

    // TODO: the node_id is an extra field to make sure homeassistant properly creates the device
    // and can parse correctly. before this, a new device with same component.name (==object_id)
    // would "use" the ones from before.
    String node_id = DEVICE_NAME;
    String discovery_topic =
        _discovery_prefix + "/" + component.type + "/" + node_id + "/" + component.name + "/config";
    String state_topic = getStateTopicFromComponents(component);

    DynamicJsonDocument doc(1024);

    JsonObject device = doc.createNestedObject("device");
    device["name"]    = _device_name;
    device["ids"]     = _device_id;
    device["mf"]      = "heartwerk.tech";
    device["mdl"]     = _device_name;
    device["sw"]      = "0.1";
    device["hw"]      = "0.1";

    String name      = component.name;
    doc["name"]      = name;
    doc["unique_id"] = name + "_" + _device_id;
    if (component.type == "light")
    {
        doc["~"]          = getBaseFromComponent(component);
        doc["cmd_t"]      = "~/set";
        doc["stat_t"]     = "~/state";
        doc["schema"]     = "json";
        doc["brightness"] = true;
    }
    else
    {
        doc["state_topic"] = state_topic;
    }

    doc["unit_of_measurement"] = "";
    doc["value_template"]      = "{{ value_json.value }}";

    char buffer[1024];
    serializeJson(doc, buffer);

    if (_client.publish(discovery_topic.c_str(), buffer, true))
        printf("Config published for auto-discovery\n");
    else
        printf("Failed to publish config\n");
}

#if 0
#define MAX_SIZE 1024*32
// could be theoretically that this is nicer 
void MQTT::publishDiscoveryMessageDevice()
{
    if (!_isActive)
        return;

    // Format: <discovery_prefix>/device/<object_id>/config
    String discovery_topic = _discovery_prefix + "/device/" + _device_id + "/config";

    // Try publishing components individually if the device approach fails
    bool published = false;

    // First, try with minimal device-based approach
    {
        DynamicJsonDocument doc(MAX_SIZE); // Even smaller size

        // Absolute minimum device info
        JsonObject dev = doc.createNestedObject("dev");
        dev["ids"]     = _device_id;   // Only required field
        dev["name"]    = _device_name; // Helpful for display

        // Minimal origin info
        JsonObject o = doc.createNestedObject("o");
        o["name"]    = "esp"; // Super short
        o["sw"]      = "1";   // Even shorter

        // Minimal components info
        JsonObject cmps = doc.createNestedObject("cmps");

        // Add only the essential parameters for each component
        for (const auto& component : _components)
        {
            JsonObject comp = cmps.createNestedObject(component.name);

            // Only required field
            comp["p"] = component.type;

            // Shortest unique ID possible
            comp["uniq_id"] = component.name + _device_id;

            // Add only the most essential config
            if (component.type == "light")
            {

                doc["~"]          = getBaseFromComponent(component);
                doc["cmd_t"]      = "~/set";
                doc["stat_t"]     = "~/state";
                doc["schema"]     = "json";
                doc["brightness"] = true;

                // comp["bri"] = true;                                         // Enable brightness
                // comp["stat_t"] = getStateTopicFromComponents(component);    // State topic
                // comp["cmd_t"] = getCommandTopicFromComponents(component);   // Command topic
            }
        }

        char   buffer[MAX_SIZE];
        size_t n = serializeJson(doc, buffer);

        printf("Discovery message size: %d bytes\n", n);

        // Try to publish the device discovery
        if (_client.publish(discovery_topic.c_str(), buffer, true))
        {
            printf("Device discovery config published (%d bytes)\n", n);
            published = true;
        }
        else
        {
            printf("Failed to publish device config (%d bytes)\n", n);
        }
    }

    // If device-based approach fails, fall back to individual component discovery
    if (!published)
    {
        printf("Falling back to individual component discovery\n");
        for (const auto& component : _components)
        {
            publishDiscoveryMessage(component);
        }
    }
}
#endif

void MQTT::handleCallback(char* topic, byte* payload, unsigned int length)
{
    String message;
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
    printf("MQTT RX [%s]=%s\n", topic, message.c_str());

    DynamicJsonDocument  doc(200);
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        printf("Failed to parse JSON: %s\n", error.c_str());
        return;
    }

    String topicStr = String(topic); // format = deviceName/type/component/command - eg.:
                                     // led-note-01/light/driver_ch1/set

    int firstSlash  = topicStr.indexOf("/");
    int secondSlash = topicStr.indexOf("/", firstSlash + 1);
    int thirdSlash  = topicStr.indexOf("/", secondSlash + 1);
    int fourthSlash = topicStr.indexOf("/", thirdSlash + 1);

    String deviceName = topicStr.substring(0, firstSlash);

    String type      = topicStr.substring(firstSlash + 1, secondSlash);
    String node_id   = topicStr.substring(secondSlash + 1, thirdSlash);
    String object_id = topicStr.substring(thirdSlash + 1, fourthSlash);
    String command   = topicStr.substring(fourthSlash + 1);

    printf("deviceName=%s, type=%s, node_id=%s, object_id=%s, command=%s\n",
           deviceName.c_str(),
           type.c_str(),
           node_id.c_str(),
           object_id.c_str(),
           command.c_str());

    if (deviceName != _device_name)
        return;
    if (node_id != DEVICE_NAME)
        return;
    if (type == "light")
        for (const auto& component : _components)
            if (component.type == type && component.name == object_id)
            {
                if (command == "set")
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

#if 0
            for (const auto& component : _components)
                publishDiscoveryMessage(component);

                // publishDiscoveryMessageDevice();
#endif

            _subscribed = false;
        }
        else
        {
            printf("failed, rc=%d try again in 5 seconds\n", _client.state());
            delay(2000);
        }
    }
}

void MQTT::publishState(const String& topic, const DynamicJsonDocument& stateDoc)
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

void MQTT::processLightCommand(const String& component_name, const DynamicJsonDocument& doc)
{
    if (doc.containsKey("state"))
    {
        String state = doc["state"];
        printf("state: %s\n", state.c_str());

        if (state == "ON")
        {
            if (doc.containsKey("brightness"))
                lightChange(component_name, float(doc["brightness"]) / 255.0f);
            else
                lightToggle(component_name, true);
        }
        else if (state == "OFF")
        {
            lightToggle(component_name, false);
        }
        else if (state != "ON")
            printf("Invalid state value\n");
    }
    else if (doc.containsKey("brightness"))
        lightChange(component_name, float(doc["brightness"]) / 255.0f);
    else
        printf("state and brightness key not found in JSON\n");
}

void MQTT::lightChange(const String& component_name, float percent)
{
    if (percent <= (4.0f / 255.0f))
        percent = 0;

    if (_lightChangeCallback)
        _lightChangeCallback(component_name, percent);
}

void MQTT::lightToggle(const String& component_name, bool state)
{
    if (_lightToggleCallback)
        _lightToggleCallback(component_name, state);
}