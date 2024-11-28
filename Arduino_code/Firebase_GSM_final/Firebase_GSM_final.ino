#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>
#define TINY_GSM_RX_BUFFER 1024
#include <Firebase_ESP_Client.h>
#include <Ultrasonic.h>

// SIMCom and modem details
 // A7672S is compatible with SIM7600


#define SerialAT Serial1
#define SerialMon Serial

#define UART_BAUD   115200
#define PIN_DTR     25
#define PIN_TX      17
#define PIN_RX      16
#define PWR_PIN     32
#define LED_PIN     2

// APN details (BSNL or Airtel)
const char apn[] = "airtelgprs.com"; // Change to 'airtelgprs.com' if using Airtel SIM

// Firebase credentials
#define API_KEY ""
#define DATABASE_URL "" 

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Ultrasonic sensor pins and tank details
#define TRIG_PIN 5
#define ECHO_PIN 18
Ultrasonic ultrasonic(TRIG_PIN, ECHO_PIN);

const float TANK_DEPTH = 140.0;
const float VOLUME_CONSTANT = 91628.57;

// TinyGSM client
TinyGsm modem(SerialAT);
TinyGsmClient gsmClient(modem);

// Function to power on the modem
void modemPowerOn() {
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1000);
  digitalWrite(PWR_PIN, HIGH);
}

// Function to connect to GPRS/LTE
void connectToNetwork() {
  modemPowerOn();
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  
  if (!modem.init()) {
    SerialMon.println("Failed to initialize modem. Restarting...");
    modem.restart();
    return;
  }

  // Check network registration and attach GPRS
  if (modem.waitForNetwork(30000)) {
    if (!modem.isGprsConnected()) {
      modem.gprsConnect(apn, "", "");
      SerialMon.println("Connected to GPRS/LTE.");
    } else {
      SerialMon.println("GPRS/LTE already connected.");
    }
  } else {
    SerialMon.println("Failed to register on network.");
  }
}

// Firebase connection function
void connectToFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.signUp(&config, &auth, "", "");
  Firebase.begin(&config, &auth);
  SerialMon.println("Connected to Firebase");
}

// Function to calculate and send water data to Firebase
void sendDataToFirebase() {
  float sensorReading = ultrasonic.read();
  float waterLevel = TANK_DEPTH - sensorReading;
  float waterVolume = (VOLUME_CONSTANT * waterLevel) / 1000;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    SerialMon.println("Failed to obtain time.");
    return;
  }

  FirebaseJson json;
  json.set("waterLevel", waterLevel);
  json.set("waterVolume", waterVolume);
  String timestamp = String(timeinfo.tm_year + 1900) + "-" + 
                     String(timeinfo.tm_mon + 1) + "-" + 
                     String(timeinfo.tm_mday) + " " + 
                     String(timeinfo.tm_hour) + ":" + 
                     String(timeinfo.tm_min) + ":" + 
                     String(timeinfo.tm_sec);
  json.set("timestamp", timestamp);

  String path = "/waterData/";

  if (Firebase.RTDB.pushJSON(&fbdo, path.c_str(), &json)) {
    SerialMon.println("Data logged successfully.");
  } else {
    SerialMon.println("Failed to log data: " + fbdo.errorReason());
  }
}

void setup() {
  SerialMon.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  connectToNetwork();  // Connect to the network using the SIM card
  connectToFirebase(); // Connect to Firebase
}

void loop() {
  sendDataToFirebase();
  delay(5000);  // Wait for 15 minutes before sending the next data
}
