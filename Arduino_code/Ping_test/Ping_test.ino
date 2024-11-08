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

TinyGsm modem(SerialAT);           // TinyGSM instance for the modem
TinyGsmClient client(modem);       // TinyGSM client for HTTP requests

/*********************************************
  SECTION: Set APN based on your SIM card
  AIRTEL: "airtelgprs.com"
  BSNL: "bsnlnet"
  VI: "portalnmms"
  JIO: "jionet"
*********************************************/
const char apn[] = "bsnlnet";  // Change this as per your SIM card operator

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
  SECTION: Network Connectivity Check
*********************************************/
void checkNetworkConnectivity() {
  SerialMon.println("Checking network registration...");
  
  if (!modem.waitForNetwork(30000)) {
    SerialMon.println("Network registration failed! Check SIM card or signal strength.");
    return;
  }
  
  SerialMon.println("Modem is registered on the network.");
  
  // Check if modem is attached to data (GPRS/LTE)
  SerialMon.println("Checking GPRS/LTE attachment...");
  if (!modem.isGprsConnected()) {
    SerialMon.println("Modem is not attached to GPRS/LTE. Attempting to attach...");
    modem.gprsConnect(apn, "", "");
  }
  
  if (modem.isGprsConnected()) {
    SerialMon.println("Modem successfully attached to GPRS/LTE network.");
  } else {
    SerialMon.println("Failed to attach to GPRS/LTE network. Check APN or data plan.");
  }
}

/*********************************************
  SECTION: Send Simple HTTP Request to Test Data Connection
*********************************************/
void sendHttpRequest() {
  // Make a simple HTTP GET request to http://example.com
  SerialMon.println("Making HTTP GET request to http://example.com...");
  
  if (!client.connect("example.com", 80)) {
    SerialMon.println("Failed to connect to example.com. Check network or data connection.");
    return;
  }

  // Send HTTP GET request
  client.println("GET / HTTP/1.1");
  client.println("Host: example.com");
  client.println("Connection: close");
  client.println();

  // Wait for server response
  while (client.connected() || client.available()) {
    String line = client.readStringUntil('\n');
    SerialMon.println(line);
  }
  
  client.stop();
  SerialMon.println("HTTP request complete.");
}

/*********************************************
  SECTION: Main setup
*********************************************/
void setup() {
  // Initialize serial monitor for debugging
  SerialMon.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  delay(100);

  modemPowerOn();
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  Serial.clearWriteError();
  Serial.println();
  Serial.println("/**********************************************************/");
  Serial.println("  Bharat Pi 4G/LTE Board Data Connection Test with HTTP GET");
  Serial.println("  Ensure antenna is connected and SIM card is inserted.");
  Serial.println("/**********************************************************/\n\n");

  delay(5000);

  String res;
  Serial.println("Initializing Modem...");

  if (!modem.init()) {
    digitalWrite(LED_PIN, HIGH);
    modemRestart();
    delay(2000);
    Serial.println("Failed to restart modem, continuing without restarting...");
    digitalWrite(LED_PIN, LOW);
  }

  // Display Modem Information
  String modemName = modem.getModemName();
  Serial.println("Modem Name: " + modemName);
  delay(1000);

  String modemInfo = modem.getModemInfo();
  Serial.println("Modem Info: " + modemInfo);
  delay(1000);

  /*********************************************
    SECTION: Connect to Network 
  *********************************************/
  checkNetworkConnectivity();

  // If successfully connected to the network, send the HTTP request
  if (modem.isGprsConnected()) {
    sendHttpRequest();
  }
}

/******************************************************************
SECTION: Loop call for modem status updates
*******************************************************************/  
void loop() {
  // Print data to Arduino serial monitor
  while (1) {
    while (SerialAT.available()) {
      SerialMon.write(SerialAT.read());
    }
    while (SerialMon.available()) {
      SerialAT.write(SerialMon.read());
    }
  }
}
