#define TINY_GSM_MODEM_SIM7600 // SIM7600 compatible with SIMCom A7672S
#define TINY_GSM_RX_BUFFER 1024

#define SerialAT Serial1
#define SerialMon Serial

#define UART_BAUD   115200
#define PIN_DTR     25
#define PIN_TX      17
#define PIN_RX      16
#define PWR_PIN     32

#define LED_PIN 2

#include <TinyGsmClient.h>

/*********************************************
  SECTION: Set APN based on your SIM card
  AIRTEL: "airtelgprs.com"
  BSNL: "bsnlnet"
  VI: "portalnmms"
  JIO: "jionet"
*********************************************/
const char apn[] = "airtelgprs.com";  // Change this as per your SIM card operator

/*********************************************
  SECTION: Firebase Settings
*********************************************/
const char* FIREBASE_HOST = "https://water-monitoring-d4064-default-rtdb.asia-southeast1.firebasedatabase.app/";  // Your Firebase Realtime Database URL (without 'https://')
const char* FIREBASE_API_KEY = "AIzaSyDCwzjvqjrB5DIN8NQvSq_bsZM_FLs_U1c";   // Your Firebase API key for anonymous authentication

TinyGsm modem(SerialAT);           // TinyGSM instance for the modem
TinyGsmClient client(modem);       // TinyGSM client

/*********************************************
  SECTION: Modem operation functions
*********************************************/
void modemPowerOn() {
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1000);
  digitalWrite(PWR_PIN, HIGH);
}

void modemPowerOff() {
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1500);
  digitalWrite(PWR_PIN, HIGH);
}

void modemRestart() {
  modemPowerOff();
  delay(1000);
  modemPowerOn();
}

/*********************************************
  SECTION: Main setup
*********************************************/
void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  delay(100);

  modemPowerOn();
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  Serial.clearWriteError();
  Serial.println();
  Serial.println("/**********************************************************/");
  Serial.println("  Bharat Pi 4G/LTE Board Network Connection Test with Firebase");
  Serial.println("  Ensure antenna is connected and SIM card is inserted.");
  Serial.println("/**********************************************************/\n\n");

  delay(5000);

  String res;
  Serial.println("Initializing Modem...");

  if (!modem.init()) {
    digitalWrite(LED_PIN, HIGH);
    modemRestart();
    delay(2000);
    Serial.println("Failed to restart modem, continue without restarting");
    digitalWrite(LED_PIN, LOW);
    return;
  }

  // Display Modem Information
  String modemName = modem.getModemName();
  Serial.println("Modem Name: " + modemName);
  delay(1000);

  String modemInfo = modem.getModemInfo();
  Serial.println("Modem Info: " + modemInfo);
  delay(1000);

  /*********************************************
    SECTION: Connect to Sim network 
  *********************************************/
  Serial.println("Testing network mode connectivity (GSM, LTE, or GSM/LTE)...");

  for (int i = 0; i <= 4; i++) {
    uint8_t network[] = {
        2,  /*Automatic*/
        13, /*GSM only*/
        38, /*LTE only*/
        51  /*GSM and LTE only*/
    };
    Serial.printf("Trying network mode: %d\n", network[i]);
    modem.setNetworkMode(network[i]);
    delay(3000);

    bool isConnected = false;
    int tryCount = 60;
    while (tryCount--) {
      String networkOperator = modem.getOperator();
      Serial.print("Operator: ");
      Serial.println(networkOperator);

      int16_t signal = modem.getSignalQuality();
      Serial.print("Signal: ");
      Serial.println(signal);

      Serial.print("Network connected: ");
      isConnected = modem.isNetworkConnected();
      Serial.println(isConnected ? "CONNECTED" : "NOT CONNECTED YET");

      if (isConnected) {
        break;
      }

      delay(1000);
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));  // Blink LED during network search
    }

    if (isConnected) {
      break;
    }
  }

  digitalWrite(LED_PIN, HIGH); // Modem connected to network

  Serial.println("\nDevice is connected to the SIM network.\n");

  Serial.println("Checking User Equipment (UE) system information...");
  modem.sendAT("+CPSI?");
  modem.waitResponse(1000L, res);
  res.replace(AT_NL "OK" AT_NL, "");
  Serial.println(res);

  delay(1000);
  Serial.println("Modem setup complete.");
}

/******************************************************************
SECTION: Loop call to send data to Firebase
*******************************************************************/  

void loop() {
  // Blink LED to show activity
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  delay(1000);  // Blink every second

  // Example sensor data (replace with actual sensor readings)
  float temperature = 23.5;   // Dummy temperature value
  float humidity = 60.5;      // Dummy humidity value

  // Send data to Firebase
  sendDataToFirebase(temperature, humidity);

  delay(20000);  // Wait 20 seconds before sending data again
}

/******************************************************************
SECTION: Function to Send Data to Firebase
*******************************************************************/  

void sendDataToFirebase(float temperature, float humidity) {
  // Construct the URL with the Firebase API key for authentication
  String url = "https://" + String(FIREBASE_HOST) + "/sensor_data.json?auth=" + FIREBASE_API_KEY;

  // Build JSON data
  String postData = "{";
  postData += "\"temperature\": " + String(temperature) + ",";
  postData += "\"humidity\": " + String(humidity);
  postData += "}";

  // Initialize HTTP service
  Serial.println("Initializing HTTP service...");
  modem.sendAT("+HTTPINIT");
  if (modem.waitResponse(10000L) != 1) {
    Serial.println("ERROR: Failed to initialize HTTP service!");
    modem.sendAT("+HTTPTERM");  // Terminate HTTP service if initialization fails
    return;
  }

  // Set SSL (HTTPS)
  Serial.println("Enabling HTTPS...");
  modem.sendAT("+HTTPSSL=1");
  if (modem.waitResponse(10000L) != 1) {
    Serial.println("ERROR: Failed to enable HTTPS!");
    modem.sendAT("+HTTPTERM");
    return;
  }

  // Set HTTP parameters (CID and URL)
  Serial.println("Setting HTTP parameters...");
  modem.sendAT("+HTTPPARA=\"CID\",1");
  modem.waitResponse(10000L);

  modem.sendAT("+HTTPPARA=\"URL\",\"" + url + "\"");
  if (modem.waitResponse(10000L) != 1) {
    Serial.println("ERROR: Failed to set URL!");
    modem.sendAT("+HTTPTERM");
    return;
  }

  // Set content type to JSON
  modem.sendAT("+HTTPPARA=\"CONTENT\",\"application/json\"");
  if (modem.waitResponse(10000L) != 1) {
    Serial.println("ERROR: Failed to set content type!");
    modem.sendAT("+HTTPTERM");
    return;
  }

  // Send HTTP POST data
  Serial.println("Sending data to Firebase...");
  modem.sendAT("+HTTPDATA=" + String(postData.length()) + ",10000");
  if (modem.waitResponse("DOWNLOAD") != 1) {
    Serial.println("ERROR: Failed to initiate data download!");
    modem.sendAT("+HTTPTERM");
    return;
  }
  modem.waitResponse(10000L);  // Wait for the timeout

  // Write JSON data
  modem.streamWrite(postData);
  modem.waitResponse(10000L);

  // Perform HTTP POST
  modem.sendAT("+HTTPACTION=1");
  if (modem.waitResponse(10000L) != 1) {
    Serial.println("ERROR: Failed to perform HTTP POST!");
    modem.sendAT("+HTTPTERM");
    return;
  }

  // Read HTTP response
  Serial.println("Reading response...");
  modem.sendAT("+HTTPREAD");
  if (modem.waitResponse(10000L) == 1) {
    String response = modem.stream.readStringUntil('\n');
    Serial.println("Firebase response: " + response);
  } else {
    Serial.println("ERROR: Failed to read HTTP response!");
  }

  // Terminate HTTP service
  modem.sendAT("+HTTPTERM");
}
