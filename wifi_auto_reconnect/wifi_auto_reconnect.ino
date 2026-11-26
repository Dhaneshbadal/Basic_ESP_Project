#include <WiFi.h>
#include <WiFiManager.h>

WiFiManager wifiManager;

// Reconnection timing
unsigned long lastReconnectAttempt = 0;

const unsigned long reconnectInterval = 10000;

bool wasConnected = false;


void setup() {

  Serial.begin(115200);

  Serial.println("\nESP32 Starting...");

  // First-time WiFi configuration
  bool connected = wifiManager.autoConnect(
    "ESP32_Setup",
    "12345678"
  );

  if (!connected) {

    Serial.println("Initial WiFi connection failed!");

    ESP.restart();
  }

  // Enable automatic reconnection
  WiFi.setAutoReconnect(true);

  wasConnected = true;

  Serial.println("\nWiFi Connected!");

  Serial.print("Network: ");
  Serial.println(WiFi.SSID());

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}


void loop() {

  // Check WiFi status
  if (WiFi.status() == WL_CONNECTED) {

    if (!wasConnected) {

      Serial.println("\nWiFi Reconnected!");

      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());

      wasConnected = true;
    }

  }

  else {

    if (wasConnected) {

      Serial.println("\nWiFi Disconnected!");

      wasConnected = false;
    }

    // Retry connection every 10 seconds
    if (millis() - lastReconnectAttempt >= reconnectInterval) {

      lastReconnectAttempt = millis();

      Serial.println("Attempting WiFi reconnection..."); 

      WiFi.reconnect();
    }
  }

  delay(100);
}
