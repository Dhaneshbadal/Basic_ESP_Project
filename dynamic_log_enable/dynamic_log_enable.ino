
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include "esp_log.h"

// ---------------- CONFIGURATION ----------------

#define LED_PIN 2

WebServer server(80);

// Logging tags
static const char *WIFI_TAG = "WIFI";
static const char *HTTP_TAG = "HTTP";
static const char *LED_TAG  = "LED";

// Application variables
bool ledState = false;
bool loggingEnabled = false;

esp_log_level_t currentLogLevel = ESP_LOG_INFO;

String uartCommand = "";


// ---------------- DYNAMIC LOGGING MACROS ----------------

#define APP_LOGE(tag, format, ...) \
  do { if (loggingEnabled && currentLogLevel >= ESP_LOG_ERROR) ESP_LOGE(tag, format, ##__VA_ARGS__); } while (0)

#define APP_LOGW(tag, format, ...) \
  do { if (loggingEnabled && currentLogLevel >= ESP_LOG_WARN) ESP_LOGW(tag, format, ##__VA_ARGS__); } while (0)

#define APP_LOGI(tag, format, ...) \
  do { if (loggingEnabled && currentLogLevel >= ESP_LOG_INFO) ESP_LOGI(tag, format, ##__VA_ARGS__); } while (0)

#define APP_LOGD(tag, format, ...) \
  do { if (loggingEnabled && currentLogLevel >= ESP_LOG_DEBUG) ESP_LOGD(tag, format, ##__VA_ARGS__); } while (0)

#define APP_LOGV(tag, format, ...) \
  do { if (loggingEnabled && currentLogLevel >= ESP_LOG_VERBOSE) ESP_LOGV(tag, format, ##__VA_ARGS__); } while (0)

// ---------------- DYNAMIC LOGGING CONTROL ----------------

void setLoggingLevel(String level) {

  level.trim();
  level.toUpperCase();

  // Enable logging
  if (level == "LOG ON") {

    loggingEnabled = true;

    Serial.println("Logging Enabled!");

    // Restore previously selected logging level
    esp_log_level_set("*", currentLogLevel);

    Serial.print("Current Logging Level: ");

    if (currentLogLevel == ESP_LOG_VERBOSE)
      Serial.println("VERBOSE");

    else if (currentLogLevel == ESP_LOG_DEBUG)
      Serial.println("DEBUG");

    else if (currentLogLevel == ESP_LOG_INFO)
      Serial.println("INFO");

    else if (currentLogLevel == ESP_LOG_WARN)
      Serial.println("WARNING");

    else
      Serial.println("ERROR");

    return;
  }

  // Disable logging
  if (level == "LOG OFF") {

    loggingEnabled = false;

    // Disable ESP-IDF logging
    esp_log_level_set("*", ESP_LOG_NONE);

    Serial.println("Logging Disabled!");

    return;
  }

  // Level changes are not allowed when logging is OFF
  if (!loggingEnabled) {

    Serial.println("Logging is OFF!");
    Serial.println("Send LOG ON to enable logging first.");

    return;
  }

  // Change logging level
  if (level == "VERBOSE") {

    currentLogLevel = ESP_LOG_VERBOSE;

  }

  else if (level == "DEBUG") {

    currentLogLevel = ESP_LOG_DEBUG;

  }

  else if (level == "INFO") {

    currentLogLevel = ESP_LOG_INFO;

  }

  else if (level == "WARNING") {

    currentLogLevel = ESP_LOG_WARN;

  }

  else if (level == "ERROR") {

    currentLogLevel = ESP_LOG_ERROR;

  }

  else {

    Serial.println("Invalid Command!");

    return;
  }

  // Apply new logging level to ESP-IDF
  esp_log_level_set("*", currentLogLevel);

  Serial.println("Logging Level Changed: " + level);
}


// ---------------- UART COMMAND HANDLER ----------------

void processUART() {

  while (Serial.available()) {

    char c = Serial.read();

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {

      uartCommand.trim();

      if (uartCommand.length() == 0) {
        continue;
      }

      setLoggingLevel(uartCommand);

      uartCommand = "";

    }

    else {

      uartCommand += c;

    }
  }
}


// ---------------- WEB PAGE ----------------

String webpage() {

  String html = "<!DOCTYPE html><html>";

  html += "<head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 LED Control</title>";
  html += "</head><body>";

  html += "<h1>ESP32 LED Control</h1>";

  html += "<h2>LED Status: ";
  html += (ledState ? "ON" : "OFF");
  html += "</h2>";

  html += "<a href='/on'><button>LED ON</button></a>";
  html += "<a href='/off'><button>LED OFF</button></a>";

  html += "</body></html>";

  return html;
}


// ---------------- HTTP ROUTE HANDLERS ----------------

void handleRoot() {

  APP_LOGI(HTTP_TAG, "HTTP GET / received");

  server.send(200, "text/html", webpage());
}


void handleLEDOn() {

  digitalWrite(LED_PIN, LOW);
  ledState = true;

  APP_LOGE(LED_TAG, "LED ON event - ERROR level test");
  APP_LOGW(LED_TAG, "LED ON event - WARNING level test");
  APP_LOGI(LED_TAG, "LED turned ON successfully");
  APP_LOGD(LED_TAG, "HTTP GET /on request processed");
  APP_LOGV(LED_TAG, "LED ON handler execution completed");

  server.send(200, "text/html", webpage());
}

void handleLEDOff() {

  digitalWrite(LED_PIN, HIGH);
  ledState = false;

  APP_LOGE(LED_TAG, "LED OFF event - ERROR level test");
  APP_LOGW(LED_TAG, "LED OFF event - WARNING level test");
  APP_LOGI(LED_TAG, "LED turned OFF successfully");
  APP_LOGD(LED_TAG, "HTTP GET /off request processed");
  APP_LOGV(LED_TAG, "LED OFF handler execution completed");

  server.send(200, "text/html", webpage());
}

// ---------------- SETUP ----------------

void setup() {

  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("Starting ESP32 Application...");

  // Initially disable ESP-IDF logging
  esp_log_level_set("*", ESP_LOG_NONE);


  // LED configuration (Active LOW)

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);


  // WiFi connection

  WiFiManager wifiManager;

  bool connected = wifiManager.autoConnect(
    "ESP32_Setup",
    "12345678"
  );

  if (!connected) {

    Serial.println("WiFi connection failed!");

    ESP.restart();

  }

  Serial.println("WiFi Connected!");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  APP_LOGI(WIFI_TAG, "WiFi Connected");


  // HTTP server configuration

  server.on("/", HTTP_GET, handleRoot);
  server.on("/on", HTTP_GET, handleLEDOn);
  server.on("/off", HTTP_GET, handleLEDOff);
  server.on("/log", HTTP_GET, []() {
    if (server.hasArg("cmd")) {
      String cmd = server.arg("cmd");
      setLoggingLevel(cmd); // तपाईंको पहिलेकै function चलाउँछ
      server.send(200, "text/plain", "Executed: " + cmd);
    } else {
      server.send(400, "text/plain", "Missing 'cmd' parameter");
    }
  });

  server.begin();

  Serial.println("HTTP Server Started!");


  // UART commands

  Serial.println();
  Serial.println("========== UART COMMANDS ==========");

  Serial.println("LOG ON       - Enable Logging");
  Serial.println("LOG OFF      - Disable Logging");

  Serial.println("VERBOSE  - Set VERBOSE Level");
  Serial.println("DEBUG    - Set DEBUG Level");
  Serial.println("INFO     - Set INFO Level");
  Serial.println("WARNING  - Set WARNING Level");
  Serial.println("ERROR    - Set ERROR Level");

  Serial.println("===================================");

}


// ---------------- MAIN LOOP ----------------

void loop() {

  processUART();

  server.handleClient();

}
