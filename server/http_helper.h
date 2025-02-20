#pragma once

#include <HTTPClient.h>

#if 0
/**
 * Dispatches an HTTP GET request to the specified URL and returns the response.
 * Assumes that the device is already connected to Wi-Fi.
 *
 * @param url The URL to send the GET request to.
 * @return The response from the server, or an empty string in case of an error.
 */
String dispatchHttpRequestGet(const char* url) {
    HTTPClient http;
    http.begin(url);
    
    int httpCode = http.GET();
    
    String response = "";
    if (httpCode > 0) {
        response = http.getString();
    } else {
        Serial.printf("HTTP GET request failed. Error code: %d\n", httpCode);
    }

    http.end();
    Serial.println("Response: " + response);

    return response;
}
#else

/**
 * Dispatches an HTTP GET request to the specified URL and does not wait for the response.
 * Assumes that the device is already connected to Wi-Fi.
 *
 * @param url The URL to send the GET request to.
 * @return empty string.
 */
String dispatchHttpRequestGet(const char *url)
{
    HTTPClient http;
    Serial.println("dispatchHttpRequestGet: " + String(url));
    http.begin(url);
    http.GET();
    http.end();
    return "";
}

#endif

