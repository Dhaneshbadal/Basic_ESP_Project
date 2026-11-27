
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>

#include <SPI.h>
#include <LittleFS.h>
#include <Adafruit_PN532.h>
#include <ArduinoJson.h>

#include <time.h>
#include <sys/time.h>
#include "esp_system.h"

// ==================================================
// 1. CONFIGURATION
// ==================================================

const char* DEVICE_ID = "demo";

// Change this to your computer's actual LAN IP
const char* API_URL =
  "http://192.168.1.139:8000/api/attendance";

// Fixed location: replace with actual device coordinates
const double LATITUDE  = 27.66;
const double LONGITUDE = 85.36;

// PN532 software SPI pins
#define PN532_SCK   18
#define PN532_MISO  19
#define PN532_MOSI  23
#define PN532_SS    5

// LittleFS files
const char* FILE_PATH = "/attendance.json";
const char* TEMP_PATH = "/attendance.tmp";

// Timing
const unsigned long SYNC_INTERVAL = 15000;
const unsigned long WIFI_RETRY_INTERVAL = 30000;
const unsigned long CARD_REMOVAL_TIME = 1000;

// ==================================================
// 2. OBJECTS
// ==================================================

Adafruit_PN532 nfc(
  PN532_SCK,
  PN532_MISO,
  PN532_MOSI,
  PN532_SS
);

// ==================================================
// 3. GLOBAL VARIABLES
// ==================================================

String lastUID = "";

bool timeReady = false;
bool absenceTiming = false;

unsigned long absenceStart = 0;
unsigned long lastSyncTime = 0;
unsigned long lastWiFiRetry = 0;

// Forward declaration
bool migrateRecordIDs();

// ==================================================
// 4. GENERATE UNIQUE RECORD ID
// ==================================================

String generateRecordID() {

  char recordID[37];

  snprintf(
    recordID,
    sizeof(recordID),
    "%08X-%08X-%08X-%08X",
    (unsigned int)esp_random(),
    (unsigned int)esp_random(),
    (unsigned int)esp_random(),
    (unsigned int)esp_random()
  );

  return String(recordID);
}

// ==================================================
// 5. INITIALIZE INTERNAL CLOCK
// ==================================================

void initializeClock() {

  setenv("TZ", "UTC0", 1);
  tzset();

  const char* date = __DATE__;
  const char* buildTime = __TIME__;

  char month[4];

  int day, year;
  int hour, minute, second;

  sscanf(date, "%3s %d %d",
         month, &day, &year);

  sscanf(buildTime, "%d:%d:%d",
         &hour, &minute, &second);

  const char* months[] = {
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec"
  };

  int monthNumber = -1;

  for (int i = 0; i < 12; i++) {

    if (strcmp(month, months[i]) == 0) {
      monthNumber = i;
      break;
    }
  }

  if (monthNumber < 0) {
    Serial.println("Invalid build date!");
    return;
  }

  struct tm timeInfo = {};

  timeInfo.tm_year = year - 1900;
  timeInfo.tm_mon = monthNumber;
  timeInfo.tm_mday = day;

  timeInfo.tm_hour = hour;
  timeInfo.tm_min = minute;
  timeInfo.tm_sec = second;

  timeInfo.tm_isdst = 0;

  time_t epoch = mktime(&timeInfo);

  struct timeval tv = {};

  tv.tv_sec = epoch;
  tv.tv_usec = 0;

  if (settimeofday(&tv, nullptr) == 0) {

    timeReady = true;
    Serial.println("Internal Clock Initialized!");

  } else {

    Serial.println("Clock Initialization Failed!");
  }
}

// ==================================================
// 6. GET CURRENT UTC TIME
// ==================================================

bool getCurrentTime(String &timestamp) {

  if (!timeReady) {
    return false;
  }

  time_t now = time(nullptr);

  if (now < 1704067200) {
    return false;
  }

  struct tm timeInfo;

  gmtime_r(&now, &timeInfo);

  char buffer[25];

  strftime(
    buffer,
    sizeof(buffer),
    "%Y-%m-%dT%H:%M:%SZ",
    &timeInfo
  );

  timestamp = String(buffer);

  return true;
}

// ==================================================
// 7. WRITE JSON TO TEMPORARY FILE
// ==================================================

bool writeJSONToTemp(JsonDocument &doc) {

  File file = LittleFS.open(TEMP_PATH, "w");

  if (!file) {
    Serial.println("Temporary File Open Failed!");
    return false;
  }

  size_t written = serializeJson(doc, file);

  file.close();

  if (written == 0) {

    Serial.println("JSON Write Failed!");

    LittleFS.remove(TEMP_PATH);

    return false;
  }

  return true;
}

// ==================================================
// 8. REPLACE ATTENDANCE FILE
// ==================================================

bool replaceAttendanceFile() {

  if (!LittleFS.exists(TEMP_PATH)) {
    Serial.println("Temporary File Not Found!");
    return false;
  }

  if (LittleFS.exists(FILE_PATH)) {
    LittleFS.remove(FILE_PATH);
  }

  if (!LittleFS.rename(TEMP_PATH, FILE_PATH)) {

    Serial.println("File Rename Failed!");

    return false;
  }

  return true;
}

// ==================================================
// 9. MIGRATE OLD RECORDS
// Add recordId to records that do not have one
// ==================================================

bool migrateRecordIDs() {

  File file = LittleFS.open(FILE_PATH, "r");

  if (!file) {
    Serial.println("Migration: File Open Failed!");
    return false;
  }

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, file);

  file.close();

  if (error) {

    Serial.println("Migration: Invalid JSON!");

    return false;
  }

  JsonArray punchCards =
    doc["punchCards"].as<JsonArray>();

  if (punchCards.isNull()) {
    Serial.println("Migration: Invalid Structure!");
    return false;
  }

  bool changed = false;

  for (JsonObject punch : punchCards) {

    if (!punch["recordId"].is<const char*>()) {

      punch["recordId"] = generateRecordID();

      changed = true;
    }
  }

  if (!changed) {

    Serial.println("All Records Already Have IDs!");

    return true;
  }

  if (!writeJSONToTemp(doc)) {
    return false;
  }

  if (!replaceAttendanceFile()) {
    return false;
  }

  Serial.println("Old Records Migrated Successfully!");

  return true;
}

// ==================================================
// 10. INITIALIZE LITTLEFS
// ==================================================

void initializeLittleFS() {

  // false prevents automatic formatting
  if (!LittleFS.begin(false)) {

    Serial.println("LittleFS Mount Failed!");

    while (true) {
      delay(1000);
    }
  }

  Serial.println("LittleFS Mounted Successfully!");

  if (!LittleFS.exists(FILE_PATH)) {

    JsonDocument doc;

    doc["deviceIdentifier"] = DEVICE_ID;
    doc["punchCards"].to<JsonArray>();

    if (!writeJSONToTemp(doc)) {
      Serial.println("Initial JSON Creation Failed!");
      return;
    }

    if (!replaceAttendanceFile()) {
      Serial.println("Initial File Creation Failed!");
      return;
    }

    Serial.println("Attendance File Created!");
  }

  // Add IDs to older records if needed
  migrateRecordIDs();
}

// ==================================================
// 11. READ ATTENDANCE FILE
// ==================================================

void readAttendanceFile() {

  Serial.println("\n----- LITTLEFS DATA -----");

  File file = LittleFS.open(FILE_PATH, "r");

  if (!file) {

    Serial.println("Attendance File Open Failed!");

    return;
  }

  Serial.println("File Content:");

  while (file.available()) {
    Serial.write(file.read());
  }

  file.close();

  Serial.println("\n-------------------------");
}

// ==================================================
// 12. READ NFC CARD UID
// ==================================================

String readCardUID() {

  uint8_t uid[7];
  uint8_t uidLength = 0;

  bool success = nfc.readPassiveTargetID(
    PN532_MIFARE_ISO14443A,
    uid,
    &uidLength,
    100
  );

  if (!success) {
    return "";
  }

  String cardUID = "";

  for (uint8_t i = 0; i < uidLength; i++) {

    if (uid[i] < 0x10) {
      cardUID += "0";
    }

    cardUID += String(uid[i], HEX);
  }

  cardUID.toUpperCase();

  return cardUID;
}

// ==================================================
// 13. SAVE ATTENDANCE TO LITTLEFS
// ==================================================

bool saveAttendance(String cardUID) {

  String punchTime;

  if (!getCurrentTime(punchTime)) {

    Serial.println("Clock Not Ready!");

    return false;
  }

  // Read existing JSON
  File file = LittleFS.open(FILE_PATH, "r");

  if (!file) {

    Serial.println("File Read Failed!");

    return false;
  }

  JsonDocument doc;

  DeserializationError error =
    deserializeJson(doc, file);

  file.close();

  if (error) {

    Serial.print("JSON Error: ");
    Serial.println(error.c_str());

    return false;
  }

  JsonArray punchCards =
    doc["punchCards"].as<JsonArray>();

  if (punchCards.isNull()) {

    Serial.println("Invalid JSON Structure!");

    return false;
  }

  // Generate a new ID for this attendance event
  String recordID = generateRecordID();

  // Add new attendance record
  JsonObject punch = punchCards.add<JsonObject>();

  punch["recordId"] = recordID;
  punch["cardIdentifier"] = cardUID;
  punch["punchOn"] = punchTime;
  punch["latitude"] = LATITUDE;
  punch["longitude"] = LONGITUDE;

  // Save to temporary file
  if (!writeJSONToTemp(doc)) {
    return false;
  }

  // Replace old file
  if (!replaceAttendanceFile()) {
    return false;
  }

  Serial.println("\nAttendance Saved Successfully!");

  Serial.print("Record ID: ");
  Serial.println(recordID);

  Serial.print("Card UID: ");
  Serial.println(cardUID);

  Serial.print("Punch Time: ");
  Serial.println(punchTime);

  return true;
}

// ==================================================
// 14. SEND ATTENDANCE TO FASTAPI
// ==================================================

bool syncAttendance() {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi Not Connected!");

    return false;
  }

  // Read stored JSON
  File file = LittleFS.open(FILE_PATH, "r");

  if (!file) {

    Serial.println("Attendance File Not Found!");

    return false;
  }

  String payload = file.readString();

  file.close();

  // Validate JSON
  JsonDocument doc;

  DeserializationError error =
    deserializeJson(doc, payload);

  if (error) {

    Serial.println("Stored JSON Invalid!");

    return false;
  }

  JsonArray punchCards =
    doc["punchCards"].as<JsonArray>();

  if (punchCards.isNull() || punchCards.size() == 0) {

    Serial.println("No Attendance Records to Upload!");

    return true;
  }

  // HTTP client
  HTTPClient http;

  http.setConnectTimeout(3000);
  http.setTimeout(5000);

  if (!http.begin(API_URL)) {

    Serial.println("HTTP Connection Initialization Failed!");

    return false;
  }

  http.addHeader(
    "Content-Type",
    "application/json"
  );

  Serial.println("\nSending Attendance to FastAPI...");

  Serial.println(payload);

  // POST request
  int httpResponseCode = http.POST(payload);

  Serial.print("HTTP Response: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode >= 200 &&
      httpResponseCode < 300) {

    Serial.println("Server Accepted Attendance!");

    Serial.println("Local Records Preserved!");

    http.end();

    return true;
  }

  Serial.println("Attendance Upload Failed!");

  if (httpResponseCode > 0) {

    Serial.println(http.getString());

  } else {

    Serial.println("Connection Error!");
  }

  http.end();

  // Keep all records for next attempt
  return false;
}

// ==================================================
// 15. WIFI RECONNECTION
// ==================================================

void reconnectWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (millis() - lastWiFiRetry >= WIFI_RETRY_INTERVAL) {

    lastWiFiRetry = millis();

    Serial.println("Trying WiFi Reconnection...");

    WiFi.reconnect();
  }
}

// ==================================================
// 16. SETUP
// ==================================================

void setup() {

  Serial.begin(115200);

  delay(500);

  Serial.println("\nESP32 Attendance System Starting...");

  // Initialize LittleFS
  initializeLittleFS();

  // Initialize internal clock
  initializeClock();

  // Initialize WiFi
  WiFiManager wifiManager;

  wifiManager.setConfigPortalTimeout(180);

  Serial.println("Connecting WiFi...");

  if (wifiManager.autoConnect("ESP32_Setup")) {

    Serial.println("WiFi Connected!");

    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

  } else {

    Serial.println("WiFi Unavailable!");

    Serial.println("Offline Attendance Mode!");
  }

  // Initialize PN532
  nfc.begin();

  uint32_t version = nfc.getFirmwareVersion();

  if (!version) {

    Serial.println("PN532 Not Found!");

    while (true) {
      delay(1000);
    }
  }

  Serial.println("PN532 Detected!");

  nfc.SAMConfig();

  Serial.println("NFC Reader Ready!");

  // Display stored data
  readAttendanceFile();

  Serial.println("\nSystem Ready!");
}

// ==================================================
// 17. MAIN LOOP
// ==================================================

void loop() {

  // Reconnect WiFi when disconnected
  reconnectWiFi();

  // Read NFC card
  String currentUID = readCardUID();

  if (currentUID.length() > 0) {

    absenceTiming = false;

    // Prevent repeated scan while card remains present
    if (currentUID != lastUID) {

      Serial.println("\nCard Detected!");

      if (saveAttendance(currentUID)) {

        lastUID = currentUID;
      }
    }
  }

  else {

    // Detect card removal
    if (lastUID.length() > 0) {

      if (!absenceTiming) {

        absenceTiming = true;

        absenceStart = millis();
      }

      if (millis() - absenceStart >= CARD_REMOVAL_TIME) {

        lastUID = "";

        absenceTiming = false;

        Serial.println("Card Removed!");
      }
    }
  }

  // Periodically synchronize records
  if (WiFi.status() == WL_CONNECTED) {

    if (millis() - lastSyncTime >= SYNC_INTERVAL) {

      lastSyncTime = millis();

      syncAttendance();
    }
  }

  delay(50);
}
