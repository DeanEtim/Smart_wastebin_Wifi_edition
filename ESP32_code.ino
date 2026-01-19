/*
 * Author: Dean Etim (Radiant Tech Nig. Ltd.)
 * Last Updated: 01/08/2025
 */
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESP32Servo.h>
#include "Webpage.h"

Servo lidServo;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Pin definitions
const int SERVO_PIN = 15;
const int TRIG_PIN = 14;
const int ECHO_PIN = 12;

// Distance measurement variables
const float conversionFactor = 0.0171;
const float proximityThreshold = 50.0;
const unsigned long interval = 300;
unsigned long lastDistanceTime = 0;
int distance = 0;

bool lidOpen = false;

// Function Prototypes
void openLid();
void closeLid();
int measureDistance();
void handleWebSocketEvent(uint8_t, WStype_t, uint8_t*, size_t);

void setup() {
  Serial.begin(115200);

  lidServo.attach(SERVO_PIN);
  lidServo.write(180);  // Closed position

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Setup AP Mode
  WiFi.softAP("SmartBin", "pulseNIEEE");
  Serial.println("AP started: SmartBin_AP");

  // Setup Web Server
  server.on("/", []() {
    server.send(200, "text/html", htmlPage);
  });
  server.begin();

  // Setup WebSocket
  webSocket.begin();
  webSocket.onEvent(handleWebSocketEvent);
}  // end setup()

void loop() {
  server.handleClient();
  webSocket.loop();

  // Periodically measure distance
  if (millis() - lastDistanceTime >= interval) {
    lastDistanceTime = millis();
    distance = measureDistance();
    webSocket.broadcastTXT(String(distance).c_str());
  }
}  // end main loop()

int measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * conversionFactor;
}  // end measureDistance()

void openLid() {
  for (int angle = 180; angle >= 95; angle--) {
    lidServo.write(angle);
    delay(20);
  }
}  // end openLid()

void closeLid() {
  for (int angle = 95; angle <= 180; angle++) {
    lidServo.write(angle);
    delay(20);
  }
}  // end closeLid()

void handleWebSocketEvent(uint8_t clientNum, WStype_t type, uint8_t* payload, size_t length) {
  String command = String((char*)payload);

  if (type == WStype_TEXT) {
    if (command == "open" && !lidOpen) {
      openLid();
      lidOpen = true;
      webSocket.sendTXT(clientNum, "status:open");
    } else if (lidOpen && command == "close") {
      closeLid();
      lidOpen = false;
      webSocket.sendTXT(clientNum, "status:close");
    }
  }
}  // end handleWebSocketEvent()
