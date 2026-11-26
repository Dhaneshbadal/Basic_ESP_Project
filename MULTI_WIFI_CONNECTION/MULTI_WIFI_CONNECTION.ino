#include <WiFi.h>

// Store multiple WiFi credentials
const char* ssid[] = {
  "Home_WiFi",
  "Office_WiFi",
  "Mobile_Hotspot"
};

const char* password[] = {
  "home_password",
  "office_password",
  "hotspot_password"
};

const int totalNetworks =
    sizeof(ssid) / sizeof(ssid[0]);

void connectWiFi() {

  Serial.println("Searching for available WiFi...");

  for (int i = 0; i < totalNetworks; i++) {

    Serial.print("Connecting to: ");
    Serial.println(ssid[i]);

    WiFi.begin(ssid[i], password[i]);

    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {

      Serial.println("\nWiFi Connected!");

      Serial.print("Network: ");
      Serial.println(WiFi.SSID());

      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());

      return;
    }

    Serial.println("\nConnection failed!");

    WiFi.disconnect();
    delay(500);
  }

  Serial.println("No WiFi network available!");
}

void setup() {

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  connectWiFi();
}

void loop() {

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi disconnected!");

    connectWiFi();
  }

  delay(5000);
}
