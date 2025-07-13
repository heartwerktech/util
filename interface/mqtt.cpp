#include "mqtt.h"
#include "util.h"

MQTT::MQTT(const char* server, int port, const String& device_name)
    : _server(server), _port(port), _client(*(new WiFiClient())), _device_name(device_name) {}

MQTT::MQTT(const char* server, int port, Client& client, const String& device_name)
    : _server(server), _port(port), _client(client), _device_name(device_name) {}

void MQTT::addComponent(const String& platform, const String& name)
{
    printf("MQTT::addComponent( platform=%s, name=%s )\n", platform.c_str(), name.c_str());
    _components.push_back({platform, name});
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
        for (const auto& component : _components)
        {
            if (component.platform == "light")
            {
                printf("Subscribing to %s\n", getCommandTopicFromComponents(component).c_str());
                _client.subscribe(getCommandTopicFromComponents(component).c_str());
            }
        }
        _subscribed = true;
        printf("Subscribed to all topics\n");
    }
}

void MQTT::publishComponent(String component_name, const DynamicJsonDocument& stateDoc)
{
    for (const auto& component : _components)
        if (component.name == component_name)
        {
            publishState(getStateTopicFromComponents(component), stateDoc);
            return;
        }
    printf("Error: Component not found: %s\n", component_name.c_str());
}

void MQTT::publishLight(String component_name, float percent)
{
    if (!_isActive)
        return;

    uint8_t brightness = util::mapConstrainf(percent, 0.0f, 1.0f, 0, 255);

    DynamicJsonDocument stateDoc(200);
    stateDoc["state"] = brightness > 0 ? "ON" : "OFF";
    if (brightness > 0)
        stateDoc["brightness"] = brightness;

    printf("publishLight %s: %2.2f | homeassistant=%d | raw=%d\n",
           component_name.c_str(),
           percent,
           int(percent * 100),
           brightness);

    publishComponent(component_name, stateDoc);
    return;
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

void MQTT::publishDiscoveryMessage(Component& component)
{
    if (!_isActive)
        return;

    // 1) Build object_id == unique_id by appending device_id to component.name
    String object_id = component.name + "_" + _device_name;
    // String object_id      = component.name + "_" + _device_id;
    String discoveryTopic =
        _discovery_prefix + "/" + component.platform + "/" + object_id + "/config";

    String state_topic = getStateTopicFromComponents(component);

    DynamicJsonDocument doc(1024);

    JsonObject device = doc.createNestedObject("device");
    device["name"]    = _device_name;
    // identifiers must be an array so HA knows it's the same device
    JsonArray id_arr = device.createNestedArray("identifiers");
    id_arr.add(_device_name);
    device["mf"]  = "heartwerk.tech";
    device["mdl"] = _device_name;
    device["sw"]  = "0.1";
    device["hw"]  = "0.1";

    // 3) Top-level entity info
    doc["name"]      = component.name; // friendly object_id
    doc["unique_id"] = object_id;      // MUST match the topic object_id

    // 4) Platform-specific configuration
    if (component.platform == "light")
    {
        doc["platform"] = "mqtt";
        doc["schema"]   = "json";

        doc["state_topic"]   = state_topic;
        doc["command_topic"] = getCommandTopicFromComponents(component);

        doc["brightness"] = true;
        doc["rgb"]        = false;
        doc["white_value"] = false;
        doc["color_temp"]  = false;
        doc["effect"]      = false;
        doc["flash"]       = false;
        doc["transition"]  = false;

        doc["optimistic"] = false;
        doc["retain"]     = false;
    }

    // 5) Publish discovery message
    String jsonString;
    serializeJson(doc, jsonString);
    printf("Publishing discovery message to %s: %s\n", discoveryTopic.c_str(), jsonString.c_str());
    _client.publish(discoveryTopic.c_str(), jsonString.c_str(), true);
}

void MQTT::reconnect()
{
    if (_isActive)
    {
        printf("Attempting MQTT connection...\n");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (_client.connect(clientId.c_str()))
        {
            printf("connected\n");
            // Once connected, publish an announcement...
            _client.publish("outTopic", "hello world");
            // ... and resubscribe
            _client.subscribe("inTopic");
        }
        else
        {
            printf("failed, rc=");
            printf("%d", _client.state());
            printf(" try again in 5 seconds\n");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void MQTT::handleCallback(char* topic, byte* payload, unsigned int length)
{
    printf("Message arrived [");
    printf("%s", topic);
    printf("] ");
    for (int i = 0; i < length; i++)
    {
        printf("%c", (char)payload[i]);
    }
    printf("\n");

    DynamicJsonDocument  doc(200);
    DeserializationError error = deserializeJson(doc, payload, length);

    if (error)
    {
        printf("deserializeJson() failed: ");
        printf("%s", error.c_str());
        printf("\n");
        return;
    }

    // Extract platform and object_id from topic
    String topic_str = String(topic);
    int    lastSlash = topic_str.lastIndexOf('/');
    if (lastSlash == -1)
        return;

    String command = topic_str.substring(lastSlash + 1);
    String topic_without_command = topic_str.substring(0, lastSlash);

    lastSlash = topic_without_command.lastIndexOf('/');
    if (lastSlash == -1)
        return;

    String object_id = topic_without_command.substring(lastSlash + 1);
    String platform  = topic_without_command.substring(0, lastSlash);

    lastSlash = platform.lastIndexOf('/');
    if (lastSlash == -1)
        return;

    platform = platform.substring(lastSlash + 1);

    // Find matching component
    for (const auto& component : _components)
        if (component.platform == platform && component.name == object_id)
        {
            if (command == "set")
            {
                processLightCommand(component.name, doc);
            }
            break;
        }
}

void MQTT::publishState(const String& topic, const DynamicJsonDocument& stateDoc)
{
    if (!_isActive)
        return;

    String jsonString;
    serializeJson(stateDoc, jsonString);
    printf("Publishing state to %s: %s\n", topic.c_str(), jsonString.c_str());
    _client.publish(topic.c_str(), jsonString.c_str());
}

void MQTT::processLightCommand(const String& component_name, const DynamicJsonDocument& doc)
{
    if (doc.containsKey("state"))
    {
        String state = doc["state"];
        if (state == "ON")
        {
            float brightness = 1.0f;
            if (doc.containsKey("brightness"))
            {
                brightness = doc["brightness"].as<float>() / 255.0f;
            }
            lightChange(component_name, brightness);
        }
        else if (state == "OFF")
        {
            lightChange(component_name, 0.0f);
        }
    }
    else if (doc.containsKey("brightness"))
    {
        float brightness = doc["brightness"].as<float>() / 255.0f;
        lightChange(component_name, brightness);
    }
}

void MQTT::lightChange(const String& component_name, float percent)
{
    printf("lightChange %s: %2.2f\n", component_name.c_str(), percent);
    if (_lightChangeCallback)
        _lightChangeCallback(component_name, percent);
}

void MQTT::lightToggle(const String& component_name, bool state)
{
    printf("lightToggle %s: %s\n", component_name.c_str(), state ? "ON" : "OFF");
    if (_lightToggleCallback)
        _lightToggleCallback(component_name, state);
} 