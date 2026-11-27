#include <LittleFS.h>

void setup() {

    Serial.begin(115200);

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed!");
        return;
    }

    Serial.println("LittleFS Mounted Successfully!");

    // Get Total Size
    size_t totalSize = LittleFS.totalBytes()/1024;

    // Get Used Size
    size_t usedSize = LittleFS.usedBytes()/1024;

    // Calculate Free Size
    size_t freeSize = totalSize - usedSize;

    Serial.println("\n===== LittleFS Storage Information =====");

    Serial.printf("Total Size: %u kiloBytes\n", (unsigned int)totalSize);
    Serial.printf("Used Size: %u kiloBytes\n", (unsigned int)usedSize);
    Serial.printf("Free Size: %u kiloBytes\n", (unsigned int)freeSize);
}

void loop() {

}
