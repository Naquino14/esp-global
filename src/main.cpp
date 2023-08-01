#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <base64.h>

#include "../lib/secrets.h"

#define LED_BUILTIN 2
#define string String
#define byte uint8_t

WebServer webServer(80);

StaticJsonDocument<512> json;

string header;

File indexHtml;
string indexHtmlString;

bool state = false;

void serve404() {
    Serial.println("Sending 404.");
    webServer.send(404, "text/plain", "404: Not found");
}

void serveIndex() {
    Serial.println("Sending index.");
    webServer.send(200, "text/html", indexHtmlString);
}

void serveFavicon() {
    Serial.println("Sending favicon.");
    File favicon = SPIFFS.open("/favicon.ico", "rb");
    if (!favicon) {
        Serial.println("Failed to open favicon.ico file");
        serve404();
        return;
    }

    // send favicon
    webServer.streamFile(favicon, "image/x-icon");
    favicon.close();
}

void togglePinEP() {
    Serial.println("Toggle pin endpoint called.");
    Serial.println("Request body:");
    Serial.println(webServer.arg("plain"));
    deserializeJson(json, webServer.arg("plain"));
    if (!json.containsKey("pin") || !json.containsKey("state"))
        return webServer.send(400, "text/plain", "Bad request: missing pin or state");
    int pin = json["pin"];
    state = json["state"];
    digitalWrite(pin, state);
    Serial.printf("Pin: %d, State: %d\n", pin, state ? "true" : "false");
    // send 200
    webServer.send(200, "text/plain", "bazinga");
}

void getPinEP() {
    Serial.println("Get pin endpoint called.");
    Serial.println("Request body:");
    Serial.println(webServer.arg("plain"));
    deserializeJson(json, webServer.arg("plain"));
    if (!json.containsKey("pin"))
        return webServer.send(400, "text/plain", "Bad request: missing pin");
    int pin = json["pin"];
    // get pin state
    byte mask = digitalPinToBitMask(pin);
    byte port = digitalPinToPort(pin);
    if (port == NOT_A_PIN) {
        return webServer.send(400, "text/plain", "Bad request: invalid pin");
    }
    bool state = (*portOutputRegister(port) & mask) ? true : false;
    Serial.printf("Pin: %d, State: %d\n", pin, state ? "true" : "false");
    // send 200
    webServer.send(200, "text/plain", state ? "{\"state\":true}" : "{\"state\":false}");
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    delay(3000);

    // setup fs
    Serial.println("Mounting FS...");
    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // read static html
    Serial.println("Reading index.html file");
    indexHtml = SPIFFS.open("/index.html");
    if (!indexHtml) {
        Serial.println("Failed to open index.html file");
        return;
    }
    indexHtmlString = indexHtml.readString();
    Serial.println("BEGIN INDEX.HTML");
    Serial.println(indexHtmlString);
    Serial.println("END INDEX.HTML");

    // setup wifi
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);
    Serial.print("Connecting to wifi.");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    IPAddress ip = WiFi.localIP();
    Serial.printf("\nAP IP address: %s\n", ip.toString());

    // setup server
    Serial.println("Starting server...");

    webServer.on("/", HTTP_GET, serveIndex);
    webServer.onNotFound(serve404);
    webServer.on("/favicon.ico", HTTP_GET, serveFavicon);

    // this is the endpoint that toggles a pin
    // the json would look like this:
    // {
    //    "pin": 2,
    //    "state": true
    // }
    webServer.on("/api/set", HTTP_POST, togglePinEP);
    webServer.on("/api/get", HTTP_POST, getPinEP);

    webServer.begin();
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Server started.");
}

void loop() {
    webServer.handleClient();
    digitalWrite(LED_BUILTIN, state ? HIGH : LOW);
}