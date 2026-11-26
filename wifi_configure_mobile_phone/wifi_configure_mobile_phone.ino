
#include <WiFi.h>
#include <WiFiManager.h>

WiFiManager wifiManager;

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println("\nESP32 Starting...");

  // Set timeout for WiFi connection attempt
  wifiManager.setConnectTimeout(15);

  // Set configuration portal timeout
  wifiManager.setConfigPortalTimeout(180);

  Serial.println("Starting WiFiManager...");

  bool connected = wifiManager.autoConnect(
    "ESP32_Setup",
    "12345678"
  );

  if (!connected) {

    Serial.println("WiFi connection failed!");
    Serial.println("Restarting ESP32...");

    delay(1000);
    ESP.restart();
  }

  Serial.println("\nWiFi Connected!");

  Serial.print("Connected Network: ");
  Serial.println(WiFi.SSID());

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

}



// for reset wifi credentials

// #include <WiFi.h>
// #include <WiFiManager.h>

// WiFiManager wifiManager;

// void setup() {

//   Serial.begin(115200);

//   Serial.println("Clearing saved WiFi credentials...");

//   // Clear previously saved WiFi credentials
//   wifiManager.resetSettings();

//   // Disconnect from current WiFi
//   WiFi.disconnect(true, true);

//   Serial.println("Saved WiFi credentials cleared!");

// }

// void loop() {

// }


