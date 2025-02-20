#pragma once

#include "socket_server.h"
#include "parameter_data.h"

class ParameterServer : public SocketServer
{
public:
    ParameterServer() : SocketServer()
    {
    }

    bool setup(const char *name, Websocket_Callback callback)
    {
        Serial.println("ParameterServer::setup()");
        if (pData == nullptr)
            Serial.println("ParameterServer::PROBLEMMMMMMMMM()");
        
        pData->load();

        SocketServer::setup(name, callback);

        return true;
    }

    void loop()
    {
        SocketServer::loop();

#if 1
        static lpsd_ms timeElapsed;
        if (timeElapsed > 5)
        {
        for (auto param : pData->getParameter_changed_from_code())
            sendJson(param);
        }
#endif
    }

    
    // Sends a JSON object to all connected WebSocket clients
    void sendJson(const ParameterData::Parameter* pParam)
    {
        if (webSocket.connectedClients() > 0)
        { // Only send if there are connected clients
            StaticJsonDocument<200> doc;
            doc["name"] = pParam->name;
            doc["value"] = pParam->value;
            String jsonString;
            serializeJson(doc, jsonString);
            webSocket.broadcastTXT(jsonString);
#if DEBUG_SERVER
            Serial.println("Sent JSON: " + jsonString); // Debug output
#endif
        }
    }

    void sendJson(const ParameterData::Parameter &param) { sendJson(&param); }

    #define MAX_ARRAY_LENGTH 100 // Defines the maximum length for JSON arrays
    // Sends a JSON array to all connected WebSocket clients
    void sendJsonArray(const String &name, float * arrayValues, int length)
    {
        if (webSocket.connectedClients() > 0 && length > 0)
        { // Only send if there are connected clients and array has elements

            String jsonString = "";
            const size_t CAPACITY = JSON_ARRAY_SIZE(MAX_ARRAY_LENGTH) + 100;
            StaticJsonDocument<CAPACITY> doc;
            JsonArray valueArray = doc.createNestedArray("value");

            for (int i = 0; i < length; ++i)
                valueArray.add(arrayValues[i]);

            doc["name"] = name;
            serializeJson(doc, jsonString);
            webSocket.broadcastTXT(jsonString);
        }
    }

    void sendAllParameters()
    {
        for (auto param : pData->parameters)
            sendJson(*param);
    }

    bool parse(StaticJsonDocument<200> *pDoc, ParameterData::Parameter *parameter)
    {
        const String name = static_cast<const char *>((*pDoc)["name"]);
        const float value = (*pDoc)["value"];

        if (name == parameter->name)
        {
            parameter->value = value;

            //TODO dont save so often when parameter parsed ?!?
            pData->save();
            
            sendJson(parameter);
            
            return true;
        }
        return false;
    }


public:
    ParameterData *pData = nullptr;
};


