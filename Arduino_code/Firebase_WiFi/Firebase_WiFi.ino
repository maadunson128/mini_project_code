#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Ultrasonic.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Provide the required Firebase credentials and API key
#define WIFI_SSID "Pradeep Kumar"
#define WIFI_PASSWORD "26062606"
#define API_KEY "AIzaSyDCwzjvqjrB5DIN8NQvSq_bsZM_FLs_U1c"
#define DATABASE_URL "https://water-monitoring-d4064-default-rtdb.asia-southeast1.firebasedatabase.app/"  // Firebase Realtime Database URL

FirebaseData fbdo;  // Firebase object
FirebaseAuth auth;  // Firebase authentication object
FirebaseConfig config;  // Firebase configuration object

// Ultrasonic sensor pins (change these if necessary)
#define TRIG_PIN 5
#define ECHO_PIN 18

Ultrasonic ultrasonic(TRIG_PIN, ECHO_PIN);  // Initialize ultrasonic sensor

// Define tank depth (in cm) and volume constant
const float TANK_DEPTH = 140.0;  // Tank depth in cm
const float VOLUME_CONSTANT = 91628.57;  // Constant for volume calculation

// NTP server and timezone offset
const long utcOffsetInSeconds = 19800; // 5 hours and 30 minutes (IST)

// Wi-Fi connection function
void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected to Wi-Fi");
}

// Firebase connection function
void connectToFirebase() {
  config.api_key = API_KEY;  // Set Firebase API key
  config.database_url = DATABASE_URL;  // Set Firebase database URL

  // Enable anonymous authentication
  Firebase.signUp(&config, &auth, "", "");

  // Assign the configuration and authentication to Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);  // Reconnect Wi-Fi if connection is lost
  Serial.println("Connected to Firebase");
}

// Function to setup NTP and synchronize time
void setupNTP() {
  configTime(utcOffsetInSeconds, 0, "time.google.com");
  Serial.println("NTP Server time synchronized");
}

// Function to calculate water level and volume
void calculateAndLogData() {
  // Get sensor reading (distance to water surface)
  float sensorReading = ultrasonic.read();
  
  // Calculate water level (cm)
  float waterLevel = TANK_DEPTH - sensorReading;
  
  // Calculate water volume (liters)
  float waterVolume = (VOLUME_CONSTANT * waterLevel) / 1000;

  // Get current time
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  // Create JSON object to store data
  FirebaseJson json;
  json.set("waterLevel", waterLevel);
  json.set("waterVolume", waterVolume);
  
  // Create a timestamp
  String timestamp = String(timeinfo.tm_year + 1900) + "-" + 
                     String(timeinfo.tm_mon + 1) + "-" + 
                     String(timeinfo.tm_mday) + " " + 
                     String(timeinfo.tm_hour) + ":" + 
                     String(timeinfo.tm_min) + ":" + 
                     String(timeinfo.tm_sec);
  json.set("timestamp", timestamp); // Add timestamp to JSON

  // Define Firebase path
  String path = "/waterData/"; // Base path for storing data

  // Push JSON object to Firebase
  if (Firebase.RTDB.pushJSON(&fbdo, path.c_str(), &json)) {
    Serial.println("Data logged successfully.");
  } else {
    Serial.println("Failed to log data: " + fbdo.errorReason());
  }
}

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  connectToWiFi();  // Connect to Wi-Fi
  connectToFirebase();  // Connect to Firebase
  setupNTP();  // Setup NTP for time synchronization
  
}

void loop() {
  calculateAndLogData();  // Calculate and log data to Firebase
  delay(5000);  // Wait for 15 minutes (900000 ms) before the next reading
}
