#pragma once
/*********
 * INSPIRED by:
 * 
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp32-wi-fi-manager-asyncwebserver/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "spiffs_helper.h"

#define USE_IP_GATEWAY 0

// Variables to save values from HTML form
String ssid, pass;
#if USE_IP_GATEWAY
String ip;
String gateway;
#endif

// File paths to save input values permanently
const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";
#if USE_IP_GATEWAY
const char *ipPath = "/ip.txt";
const char *gatewayPath = "/gateway.txt";
#endif

IPAddress localIP;
// IPAddress localIP(192, 168, 1, 200); // hardcoded

IPAddress localGateway;
// IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 0, 0);

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000; // interval to wait for Wi-Fi connection (milliseconds)

void loadValues()
{
  ssid = readFile(ssidPath);
  pass = readFile(passPath);

#if USE_IP_GATEWAY
  ip = readFile(ipPath);
  gateway = readFile(gatewayPath);
#endif
}

bool initWiFi()
{
    printf("Initializing WiFi...\n");
    
    loadValues();

    if (ssid == "") // || ip == "")
    {
        Serial.println("Undefined SSID or IP address.");
        return false;
    }

    WiFi.mode(WIFI_STA);

#if USE_IP_GATEWAY
    localIP.fromString(ip.c_str());
    localGateway.fromString(gateway.c_str());
#endif

#if 0 // dont use local ip etc...
    if (!WiFi.config(localIP, localGateway, subnet))
    {
        Serial.println("STA Failed to configure");
        return false;
    }
#endif

    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.println("Connecting to WiFi...");

    unsigned long currentMillis = millis();
    previousMillis = currentMillis;

    while (WiFi.status() != WL_CONNECTED)
    {
        currentMillis = millis();
        if (currentMillis - previousMillis >= interval)
        {
            Serial.println("Failed to connect.");
            return false;
        }
    }

    printf("Connected with IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
}
