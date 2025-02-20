#pragma once

#include "managed_server.h"
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

typedef void (*Websocket_Callback)(uint8_t num, WStype_t type, uint8_t *payload, size_t length);

class SocketServer : public ManagedServer
{
public:
    SocketServer() : ManagedServer(), webSocket(81)
    {
    }

    bool setup(const char *name, Websocket_Callback callback)
    {
        Serial.println("SocketServer::setup()");

        if (ManagedServer::setup(name) || 1)
        {
            ManagedServer::serveStatic("/", SPIFFS, "/");
            ManagedServer::on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                              { request->send(SPIFFS, "/index.html", "text/html"); });
            ManagedServer::onNotFound([](AsyncWebServerRequest *request)
                                      { request->send(404, "text/plain", "File not found"); });

            webSocket.onEvent(callback); // define a callback function -> what does the ESP32 need to do when an event from the websocket is received? -> run function "webSocketEvent()"
            webSocket.begin();           // start websocket

            // ManagedServer::begin(); // start server after the websocket
        }
        else {
            Serial.println("ManagedServer::setup failed()");
        }
        return true;
    }

    void loop()
    {
        ManagedServer::loop();
        webSocket.loop();

// TODO: mechanism to only send when control values changed
#if 0 // Send wake control values every second
        static elapsedMillis timeElapsed;
        if (timeElapsed > 1000)
        {
            timeElapsed = 0;
            if (webSocket.connectedClients() > 0)
                send_wake_control();
        }
#endif
    }

public:
    WebSocketsServer webSocket;
};
