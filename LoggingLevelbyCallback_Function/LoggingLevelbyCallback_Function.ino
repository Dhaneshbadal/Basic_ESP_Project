#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include "esp_log.h"


// ---------------- CONFIGURATION ----------------
#define LED_PIN 2

const char* SERVER_BASE_URL = "http://192.168.1.139:8000";

static const char *WIFI_TAG = "WIFI";
static const char *HTTP_TAG = "HTTP";
static const char *LED_TAG  = "LED";

bool ledState = false;
bool loggingEnabled = false;
esp_log_level_t currentLogLevel = ESP_LOG_INFO;

String lastLogCmd = "";
String lastLedCmd = "";
unsigned long lastPollTime = 0;
const unsigned long pollInterval = 2000;

// ---------------- SEND LOG TO PYTHON SERVER ----------------
void sendLogToServer(const char* level, const char* tag, const char* format, ...) {
  char msgBuffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(msgBuffer, sizeof(msgBuffer), format, args);
  va_end(args);

  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setReuse(false);
  http.setConnectTimeout(3000);
  http.begin(String(SERVER_BASE_URL) + "/api/esp32/logs");
  http.addHeader("Content-Type", "application/json");

  String jsonPayload = "{\"level\":\"" + String(level) + "\",\"tag\":\"" + String(tag) + "\",\"message\":\"" + String(msgBuffer) + "\"}";
  http.POST(jsonPayload);
  http.end();
}

// ---------------- Callback Function ----------------
void logCallback(
  const char* level,
  const char* tag,
  const char* message
) {
  Serial.printf("[%s] [%s] %s\n", level, tag, message);

  sendLogToServer(level, tag, "%s", message);

}

//-------------------Logging function-----------------
void appLog(
  esp_log_level_t level,
  const char* levelName,
  const char* tag,
  const char* format,
  ...
) {
  if (!loggingEnabled || level > currentLogLevel) {
    return;
  }

  char message[256];

  va_list args;
  va_start(args, format);
  vsnprintf(message, sizeof(message), format, args);
  va_end(args);

  logCallback(levelName, tag, message);
}


// ---------------- DYNAMIC LOGGING CONTROL ----------------
void setLoggingLevel(String level) {
  level.trim();
  level.toUpperCase();

  if (level == "LOG ON") {
    loggingEnabled = true;
    Serial.println("Logging Enabled!");
    esp_log_level_set("*", currentLogLevel);
    appLog(ESP_LOG_INFO, "INFO", HTTP_TAG,
       "Logging enabled via Web Dashboard");
    return;
  }

  if (level == "LOG OFF") {
    appLog(ESP_LOG_INFO, "INFO", HTTP_TAG,
       "Logging disabled via Web Dashboard");
    loggingEnabled = false;
    esp_log_level_set("*", ESP_LOG_NONE);
    Serial.println("Logging Disabled!");
    return;
  } 

  if (!loggingEnabled) {
    Serial.println("Logging is OFF! Send LOG ON first.");
    return;
  }

  if (level == "VERBOSE") currentLogLevel = ESP_LOG_VERBOSE;
  else if (level == "DEBUG") currentLogLevel = ESP_LOG_DEBUG;
  else if (level == "INFO") currentLogLevel = ESP_LOG_INFO;
  else if (level == "WARNING") currentLogLevel = ESP_LOG_WARN;
  else if (level == "ERROR") currentLogLevel = ESP_LOG_ERROR;
  else return;

  esp_log_level_set("*", currentLogLevel);
  Serial.println("Logging Level Changed: " + level);
  sendLogToServer("INFO", HTTP_TAG, "Log level changed to %s", level.c_str());
}

// ---------------- SIMPLE JSON VALUE EXTRACTOR ----------------
String extractJsonValue(String json, String key) {
  String searchKey = "\"" + key + "\":\"";
  int startIdx = json.indexOf(searchKey);
  if (startIdx == -1) return "";
  startIdx += searchKey.length();
  int endIdx = json.indexOf("\"", startIdx);
  if (endIdx == -1) return "";
  return json.substring(startIdx, endIdx);
}

// ---------------- POLL PYTHON SERVER ----------------
void pollServer() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setReuse(false);
  http.setConnectTimeout(5000);
  http.begin(String(SERVER_BASE_URL) + "/api/esp32/poll"); //endpoint set(means  set path for sending HTTP request) 

  int httpCode = http.GET();// start GET request
  if (httpCode == 200) {                // HTTP status code 200 means request successful
    String payload = http.getString();  // Read Response body
    String newLed = extractJsonValue(payload, "led");     // extract command fron JSON 
    String newLog = extractJsonValue(payload, "log_command");

    if (newLog != "" && newLog != lastLogCmd) {
      lastLogCmd = newLog;
      setLoggingLevel(newLog);
    }

    if (newLed != "" && newLed != lastLedCmd) {
      lastLedCmd = newLed;
      if (newLed == "on") {
        digitalWrite(LED_PIN, LOW);
        ledState = true;
        appLog(ESP_LOG_ERROR, "ERROR", LED_TAG,
       "LED ON - ERROR level test");

        appLog(ESP_LOG_WARN, "WARNING", LED_TAG,
              "LED ON - WARNING level test");

        appLog(ESP_LOG_INFO, "INFO", LED_TAG,
              "LED turned ON successfully");

        appLog(ESP_LOG_DEBUG, "DEBUG", LED_TAG,
              "HTTP Poll LED ON processed");

        appLog(ESP_LOG_VERBOSE, "VERBOSE", LED_TAG,
              "LED ON handler completed");   
      } else if (newLed == "off") {
        digitalWrite(LED_PIN, HIGH);
        ledState = false;
        appLog(ESP_LOG_ERROR, "ERROR", LED_TAG,
              "LED OFF - ERROR level test");

        appLog(ESP_LOG_WARN, "WARNING", LED_TAG,
              "LED OFF - WARNING level test");

        appLog(ESP_LOG_INFO, "INFO", LED_TAG,
              "LED turned off successfully");

        appLog(ESP_LOG_DEBUG, "DEBUG", LED_TAG,
              "HTTP Poll LED off processed");

        appLog(ESP_LOG_VERBOSE, "VERBOSE", LED_TAG,
              "LED ON handler completed");
      }
    }
  } else {
    Serial.printf("Poll failed, HTTP error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  delay(500);

  esp_log_level_set("*", ESP_LOG_NONE);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  WiFiManager wifiManager;
  if (!wifiManager.autoConnect("ESP32_Setup", "12345678")) {
    ESP.restart();
  }

  Serial.print("WiFi Connected! ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

// ---------------- MAIN LOOP ----------------
void loop() {
  if (millis() - lastPollTime >= pollInterval) {
    lastPollTime = millis();
    pollServer();
  }
}
