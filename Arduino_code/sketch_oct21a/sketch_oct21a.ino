#define TINY_GSM_MODEM_SIM7600  // A7672S is compatible with SIM7600
#define TINY_GSM_RX_BUFFER 1024  // Buffer size for incoming data

#include <TinyGsmClient.h>       // TinyGSM library for SIMCom A7672S

// UART Pins for SIMCom A7672S (adjust according to your setup)
#define SerialMon Serial  // For debugging via Serial Monitor
#define SerialAT Serial1  // For communication with the SIM module

// Pin configurations
#define PWR_PIN 32  // Pin to control modem power
#define PIN_TX 17   // TX pin for SIMCom A7672S
#define PIN_RX 16   // RX pin for SIMCom A7672S
#define LED_PIN 2   // LED to indicate status

// APN settings for your network provider
const char apn[] = "airtelgprs.com";    // Replace with your SIM card's APN
const char gprsUser[] = "";      // Username for APN (if needed, otherwise leave empty)
const char gprsPass[] = "";      // Password for APN (if needed, otherwise leave empty)

TinyGsm modem(SerialAT);        // Initialize the TinyGSM modem instance
TinyGsmClient client(modem);    // TinyGSM client for HTTP communication

// Function to power on the modem
void modemPowerOn() {
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);  // Power on sequence (specific to A7672S)
  delay(1000);
  digitalWrite(PWR_PIN, HIGH);
}

// Function to check signal strength (RSSI) and network type
void checkNetworkDetails() {
  String response;

  // Check if the modem is registered on the network
  SerialMon.println("Checking network registration...");
  if (!modem.waitForNetwork(30000)) {
    SerialMon.println("Network registration failed.");
    return;
  }
  SerialMon.println("Modem registered on the network.");

  // Check signal strength (RSSI)
  int signal = modem.getSignalQuality();
  SerialMon.print("Signal quality (RSSI): ");
  SerialMon.println(signal);

  // Send the AT+CPSI? command to check the network type
  SerialMon.println("Checking network type...");
  modem.sendAT("+CPSI?");
  
  // Wait for the response from the modem and store it in the response string
  modem.waitResponse(10000L, response);
  
  // Print the network type information to the Serial Monitor
  SerialMon.println("Network type information:");
  SerialMon.println(response);
}

// Function to make an HTTP request
void makeHttpRequest() {
  if (!client.connect("httpbin.org", 80)) {  // Connect to httpbin.org
    SerialMon.println("Connection to server failed.");
    return;
  }
  
  // Send a simple HTTP GET request
  client.println("GET /get HTTP/1.1");
  client.println("Host: httpbin.org");
  client.println("Connection: close");
  client.println();
  
  // Wait for the server response
  while (client.connected()) {
    String response = client.readString();
    SerialMon.println(response);  // Print the response to the serial monitor
  }
  
  // Disconnect from the server
  client.stop();
  SerialMon.println("HTTP request sent.");
}

void setup() {
  // Initialize serial monitor for debugging
  SerialMon.begin(115200);
  delay(10);

  // Power on the modem
  modemPowerOn();
  SerialAT.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // LED OFF at start

  SerialMon.println("Initializing modem...");
  modem.restart();  // Restart the modem

  // Wait for modem initialization
  delay(3000);

  // Attach to LTE (Internet)
  SerialMon.println("Attaching to LTE...");
  modem.gprsConnect(apn, gprsUser, gprsPass);

  // Check if GPRS/LTE connection was successful
  if (modem.isGprsConnected()) {
    SerialMon.println("Connected to LTE/Internet.");
    // Make an HTTP request to test data
    makeHttpRequest();
  } else {
    SerialMon.println("Failed to connect to LTE/Internet.");
    return;
  }
}

void loop() {
  // Blink LED to show activity
  checkNetworkDetails();
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  delay(1000);  // Blink every second
  makeHttpRequest();
}
