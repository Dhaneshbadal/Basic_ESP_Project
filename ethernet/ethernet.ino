#include <SPI.h>
#include <Ethernet.h>

// Custom MAC address for your W5500
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize W5500 with SCS connected to G5
  Ethernet.init(5);

  Serial.println("\nConnecting W5500 to network...");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("DHCP failed!");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("W5500 not detected. Check MISO, MOSI, SCLK, SCS, and GND wires.");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is unplugged.");
    }
  } else {
    Serial.print("Success! Assigned IP Address: ");
    Serial.println(Ethernet.localIP());
  }
}

void loop() {
  // Keep DHCP lease active
  Ethernet.maintain();
}
