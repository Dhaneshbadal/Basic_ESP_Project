#include <LittleFS.h>

void setup() {

    Serial.begin(115200);

    if (!LittleFS.begin(true)) {

        Serial.println("LittleFS Mount Failed!");
        return;

    }

    Serial.println("LittleFS Mounted!");

    // Check whether file exists==============================================================================
    if (LittleFS.exists("/test.txt")) {

        Serial.println("File Exists!");

    } else {

        Serial.println("File Does Not Exist!");

    }

    // File Deletion=============================================
    if (LittleFS.exists("/test.txt")) {

        if (LittleFS.remove("/test.txt")) {
        Serial.println("File Deleted Successfully!");
        } else {
        Serial.println("File Deletion Failed!");
        }

    } else {
        Serial.println("File Does Not Exist!");
    }

    //File Rename=============================================
    if (LittleFS.exists("/test.txt")) {

        if (LittleFS.rename("/test.txt", "/student.txt")) {
            Serial.println("File Renamed Successfully!");
        } else {
            Serial.println("File Rename Failed!");
        }

    } else {
        Serial.println("Original File Does Not Exist!");
    }

    //See Directory file list===================================

    // Open Root Directory
    File root = LittleFS.open("/");

    // Check Directory
    if (!root || !root.isDirectory()) {
        Serial.println("Failed to Open Directory!");
        return;
    }

    Serial.println("Files in LittleFS:");

    // Open First File
    File file = root.openNextFile();

    // Read All Files
    while (file) {

        Serial.print("File Name: ");
        Serial.println(file.name());

        Serial.print("File Size: ");
        Serial.print(file.size());
        Serial.println(" bytes");

        file.close();

        // Open Next File
        file = root.openNextFile();
    }

    root.close();

    Serial.println("Directory Listing Completed!");

}


void loop() {

}