#include <LittleFS.h>

void setup() {

    Serial.begin(115200);

    Serial.println("LittleFS Initialization");

    // Initialize/Mount LittleFS=========================================================================
    if (!LittleFS.begin(true)) {

        Serial.println("LittleFS Mount Failed!");

        return;
    }

    Serial.println("LittleFS Mounted Successfully!");

    // Create and open file=================================================================================
    File file = LittleFS.open("/test.txt", "w");

    // Check whether file opened successfully
    if (!file) {

        Serial.println("File Creation Failed!");

        return;
    }

    // Write data on file ======================================================================================
    file.println("Hello ESP32!");
    file.println("Learning LittleFS.");
    file.println("This is my first file.");

    Serial.println("Data Written Successfully!");

    // Open file in Read Mode====================================================================================
    File file = LittleFS.open("/test.txt", "r");

    // Check file
    if (!file) {

        Serial.println("File Open Failed!");

        return;
    }

    Serial.println("Reading File...");
    Serial.println("----------------");

    // Read data character by character
    while (file.available()) {

        Serial.write(file.read());

    }

    // Open file in Append Mode===============================================================================
    File file = LittleFS.open("/test.txt", "a");

    // Check file
    if (!file) {

        Serial.println("File Open Failed!");

        return;
    }

    // Append new data
    file.println("I am learnig abount how Little FS create, open, write and read on/from file. ");


    // Close file
    file.close();


}

void loop() {

}