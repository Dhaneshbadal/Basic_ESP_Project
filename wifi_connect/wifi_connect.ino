
#include <WiFi.h>

// WiFi credentials
// const char* ssid = "amar1249_2.4";
// const char* password = "ONETWONINE129@";

const char* ssid = "REPL2Gk";
const char* password = "educ8@repl";

// Connection timeout: 15 seconds//
//const unsigned long timeout = 15000;

void setup() {

  Serial.begin(115200);

  // Start WiFi connection
  WiFi.begin(ssid, password);

  Serial.println("Connecting to WiFi...");

  // Record starting time
  //unsigned long startTime = millis();

  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {

    // Check timeout
    // if (millis() - startTime >= timeout) {

    //   Serial.println("\nConnection Timeout!");
    //   Serial.println("WiFi connection failed.");

    //   break;
    // }

    delay(500);
    Serial.print(".");
  }

  // Check final connection status
  // if (WiFi.status() == WL_CONNECTED) {

  //   Serial.println("\nWiFi Connected!");

  //   Serial.print("IP Address: ");
  //   Serial.println(WiFi.localIP());

  // } else {

  //   Serial.println("Please check WiFi credentials.");
  // }
  Serial.println();
  Serial.println("WiFi Connected!");

  // Display IP address
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

}
