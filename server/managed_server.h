#pragma once
#include <ESPAsyncWebServer.h>

// TODO initialize dns server totally dynamically for AP use-
#define ENABLE_DNS_SERVER 1 // for use in access point ?! 

#if ENABLE_DNS_SERVER
#include <DNSServer.h>
#endif
#include <ESPmDNS.h> // for name.local resolution

#include "minimal_wifimanager.h"
#include "spiffs_helper.h"

#define USE_WIFIMANAGER_ON_ROOT 0

#define DEBUG_SERVER 1

class CaptiveRequestHandler : public AsyncWebHandler
{
public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request)
    {
        // request->addInterestingHeader("ANY");
        if (request->method() == HTTP_POST)
            return false;
        else
            return true;
    }

    void handleRequest(AsyncWebServerRequest *request)
    {
#if 1
        if (request->method() == HTTP_GET)
        {
Serial.println("captive handled GET request");
#if 0
#if USE_WIFIMANAGER_ON_ROOT
            request->send(SPIFFS, "/wifimanager.html", "text/html");
#else
            request->send(SPIFFS, "/index.html", "text/html");
#endif
#endif
        }
        else if (request->method() == HTTP_POST)
        {
            // Serial.println("captive handled POST request");
            for (int i = 0; i < request->params(); i++)
            {
                const AsyncWebParameter *p = request->getParam(i);
                if (p->isPost())
                {
                    String filePath = "/" + p->name() + ".txt";
                    writeFile(filePath.c_str(), p->value().c_str());
#if DEBUG_SERVER
                    Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
#endif
                }
            }
            request->send(200, "text/plain", "Done, will restart! connect to your wifi! ");
            delay(100);
            ESP.restart();
        }
#else
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        response->print("<!DOCTYPE html><html><head><title>Captive Portal</title></head><body>");
        response->print("<p>This is out captive portal front page.</p>");
        response->printf("<p>You were trying to reach: http://%s%s</p>", request->host().c_str(), request->url().c_str());
        response->printf("<p>Try opening <a href='http://%s'>this link</a> instead</p>", WiFi.softAPIP().toString().c_str());
        response->print("</body></html>");
        request->send(response);
#endif
    }
};

class ManagedServer : public AsyncWebServer
{
public:
    ManagedServer() : AsyncWebServer(80)
    {
    }

    void begin()
    {
        AsyncWebServer::begin();
        Serial.printf("Find server here: http://%s\n", WiFi.localIP().toString().c_str());
    }

    bool setup(const char *name)
    {
        Serial.println("ManagedServer::setup()");

        initFS();
        if (!initWiFi()) // start basic captive wifi manager if not connected
        {
            Serial.println("Starting Access Point");

            WiFi.softAP("AP: " + String(name));

#if ENABLE_DNS_SERVER
            // dnsServer.start(53, "*", WiFi.softAPIP());
            pDnsServer = new DNSServer();
            pDnsServer->start(53, "*", WiFi.softAPIP());
#endif
            Serial.printf("with IP address: %s\n", WiFi.softAPIP().toString().c_str());

            AsyncWebServer::serveStatic("/", SPIFFS, "/");
            AsyncWebServer::addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER); // only when requested from AP

#if 1
#if USE_WIFIMANAGER_ON_ROOT
            // Web Server Root URL
            AsyncWebServer::on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                               { request->send(SPIFFS, "/wifimanager.html", "text/html"); });
#else
            AsyncWebServer::on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                               { request->send(SPIFFS, "/index.html", "text/html"); });
#endif
#endif
#if 1
            AsyncWebServer::on("/", HTTP_POST, [](AsyncWebServerRequest *request)
                               {
                                Serial.println("normal handled POST request");
                for (int i = 0; i < request->params(); i++)
                {
                    const AsyncWebParameter *p = request->getParam(i);
#if DEBUG_SERVER
                    Serial.printf("%s set to: %s\n", p->name().c_str(), p->value().c_str());
#endif

                    if (p->isPost())
                    {
                        // use param name for fs path
                        String filePath = "/" + p->name() + ".txt";
                        writeFile(filePath.c_str(), p->value().c_str());
#if DEBUG_SERVER
                        Serial.printf("%s set to: %s\n", p->name().c_str(), p->value().c_str());
#endif

                        // Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
                    }
                }
                request->send(200, "text/plain", "Done, will restart! connect to your wifi! ");
                delay(1000);
                ESP.restart(); });

#endif
            AsyncWebServer::begin();
        }
        else
        {
            _soft_AP_active = false;
        }

        return (!_soft_AP_active);
    }

    String getIP()
    {
        return WiFi.localIP().toString();
    }

    void loop()
    {
#if ENABLE_DNS_SERVER
        static lpsd_ms timeElapsed;
        if (timeElapsed > 2)
        {
            timeElapsed = 0;
            if (_soft_AP_active)
                pDnsServer->processNextRequest();
                // dnsServer.processNextRequest();
        }
#endif
    }

private:
    bool _soft_AP_active = true;

#if ENABLE_DNS_SERVER
    // DNSServer dnsServer;
    DNSServer *pDnsServer = nullptr;
#endif
};