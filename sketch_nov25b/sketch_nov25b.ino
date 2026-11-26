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

// ---------------- DYNAMIC LOGGING MACROS (SERIAL + WEB) ----------------
#define APP_LOGE(tag, format, ...) \
  do { if (loggingEnabled && currentLogLevel >= ESP_LOG_ERROR) { \
    ESP_LOGE(tag, format, ##__VA_ARGS__); \
    sendLogToServer("ERROR", tag, format, ##__VA_ARGS__); \
  } } while (0)

#define APP_LOGW(tag, format, ...) \
  do { if (loggingEnabled && currentLogLevel >= ESP_LOG_WARN) { \
    ESP_LOGW(tag, format, ##__VA_ARGS__); \
    sendLogToServer("WARN", tag, format, ##__VA_ARGS__); \
  } } while (0)

#define APP_LOGI(tag, format, ...) \
  do { if (loggingEnabled && currentLogLevel >= ESP_LOG_INFO) { \
    ESP_LOGI(tag, format, ##__VA_ARGS__); \
    sendLogToServer("INFO", tag, format, ##__VA_ARGS__); \
  } } while (0)

#define APP_LOGD(tag, format, ...) \
  do { if (loggingEnabled && currentLogLevel >= ESP_LOG_DEBUG) { \
    ESP_LOGD(tag, format, ##__VA_ARGS__); \
    sendLogToServer("DEBUG", tag, format, ##__VA_ARGS__); \
  } } while (0)

#define APP_LOGV(tag, format, ...) \
  do { if (loggingEnabled && currentLogLevel >= ESP_LOG_VERBOSE) { \
    ESP_LOGV(tag, format, ##__VA_ARGS__); \
    sendLogToServer("VERBOSE", tag, format, ##__VA_ARGS__); \
  } } while (0)

// ---------------- DYNAMIC LOGGING CONTROL ----------------
void setLoggingLevel(String level) {
  level.trim();
  level.toUpperCase();

  if (level == "LOG ON") {
    loggingEnabled = true;
    Serial.println("Logging Enabled!");
    esp_log_level_set("*", currentLogLevel);
    APP_LOGI(HTTP_TAG, "Logging enabled via Web Dashboard");
    return;
  }

  if (level == "LOG OFF") {
    APP_LOGI(HTTP_TAG, "Logging disabled via Web Dashboard");
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
        APP_LOGE(LED_TAG, "LED ON - ERROR level test");
        APP_LOGW(LED_TAG, "LED ON - WARNING level test");
        APP_LOGI(LED_TAG, "LED turned ON successfully");
        APP_LOGD(LED_TAG, "HTTP Poll LED ON processed");
        APP_LOGV(LED_TAG, "LED ON handler completed");
      } else if (newLed == "off") {
        digitalWrite(LED_PIN, HIGH);
        ledState = false;
        APP_LOGE(LED_TAG, "LED OFF - ERROR level test");
        APP_LOGW(LED_TAG, "LED OFF - WARNING level test");
        APP_LOGI(LED_TAG, "LED turned OFF successfully");
        APP_LOGD(LED_TAG, "HTTP Poll LED OFF processed");
        APP_LOGV(LED_TAG, "LED OFF handler completed");
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
