
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>

#define LED_PIN 1

WebServer server(80);

bool ledState = false;

// Create HTML webpage
String webpage() {

  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>ESP32 LED Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">

    <style>
      body {
        font-family: Arial;
        text-align: center;
        background: #f2f2f2;
        padding: 30px;
      }

      .container {
        background: white;
        padding: 30px;
        border-radius: 15px;
        max-width: 400px;
        margin: auto;
      }

      button {
        padding: 15px 30px;
        font-size: 18px;
        border: none;
        border-radius: 8px;
        color: white;
        margin: 10px;
        cursor: pointer;
      }

      .on { background: green; }
      .off { background: red; }
    </style>
  </head>

  <body>
    <div class="container">
      <h1>ESP32 LED Control</h1>

      <h2>LED Status: LED_STATUS</h2>

      <a href="/on">
        <button class="on">LED ON</button>
      </a>

      <a href="/off">
        <button class="off">LED OFF</button>
      </a>
    </div>
  </body>
  </html>
  )rawliteral";

  html.replace("LED_STATUS", ledState ? "ON" : "OFF");

  return html;
}

// Homepage
void handleRoot() {
  server.send(200, "text/html", webpage());
}

// Turn LED ON
void handleLEDOn() {

  ledState = true;
  digitalWrite(LED_PIN,LOW);

  Serial.println("LED turned ON");

  server.send(200, "text/html", webpage());
}

// Turn LED OFF
void handleLEDOff() {

  ledState = false;
  digitalWrite(LED_PIN, HIGH);

  Serial.println("LED turned OFF");

  server.send(200, "text/html", webpage());
}

void setup() {

  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("ESP32 Starting...");

  // Wi-Fi connection
  WiFiManager wm;

  if (!wm.autoConnect("ESP32_Setup", "12345678")) {
    Serial.println("WiFi connection failed!");
    ESP.restart();
  }

  Serial.println("WiFi Connected!");

  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // Register routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/on", HTTP_GET, handleLEDOn);
  server.on("/off", HTTP_GET, handleLEDOff);

  // Start web server
  server.begin();

  Serial.println("HTTP Server Started!");
}

void loop() {
  server.handleClient();
}
