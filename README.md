# nats
Arduino header only library for nats.io

very much so beta

## Example ESP code
```
#include "Arduino.h"
#include "../lib/Nats/Nats.h"
#include <ESP8266WiFi.h>

extern "C" {
#include "user_interface.h"
}

const char *ssid = "...";
const char *password = "...";
const int led = 2;

WiFiClient client;
NATS nats("esp.1", &client, IPAddress(192, 168, 1, 3));

void handler1(const char *msg) {
    Serial.print("FOO1: ");
    Serial.print(strlen(msg));
    Serial.print(" ");
    Serial.println(msg);
}

void handler2(const char *msg) {
    Serial.print("FOO2: ");
    Serial.print(strlen(msg));
    Serial.print(" ");
    Serial.println(msg);
}

void freeRam() {
    uint32_t free = system_get_free_heap_size();
    nats.publish("esp.1.free", (int)free);

}

void setup() {
    pinMode(led, OUTPUT);
    digitalWrite(led, HIGH);
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    Serial.println("startup");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    nats.subscribe("foo.1", handler1);
    nats.subscribe("foo.2", handler2);
}

void loop() {
    // turn the LED on (HIGH is the voltage level)
    digitalWrite(2, HIGH);
    nats.loop();
    // wait for a second
    delay(1000);

    freeRam();

    // turn the LED off by making the voltage LOW
    digitalWrite(2, LOW);
    // wait for a second
    delay(1000);
}
```