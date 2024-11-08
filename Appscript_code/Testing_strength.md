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

// APN settings for BSNL (or any other provider)
const char apn[] = "bsnlnet";    // Replace with your SIM card's APN
const char gprsUser[] = "";      // Username for APN (if needed, otherwise leave empty)
const char gprsPass[] = "";      // Password for APN (if needed, otherwise leave empty)

// Modem and client objects
TinyGsm modem(SerialAT);  // Initialize the TinyGSM modem instance

// Function to power on the modem
void modemPowerOn() {
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);  // Power on sequence (specific to A7672S)
  delay(1000);
  digitalWrite(PWR_PIN, HIGH);
}

// Function to ping a server (e.g., 8.8.8.8)
void pingServer(const char* address) {
  String response;

  // Send the AT+PING command to ping the specified address
  SerialMon.print("Pinging ");
  SerialMon.println(address);
  modem.sendAT("+PING=\"" + String(address) + "\"");
  
  // Wait for the response from the modem
  if (modem.waitResponse(10000L, response) == 1) {
    // If successful, print the response (usually includes RTT)
    SerialMon.println("Ping response: " + response);
  } else {
    // If ping fails, handle the failure
    SerialMon.println("Ping failed or no response.");
  }
}

// Function to check signal strength (RSSI) and network type (2G, 3G, 4G)
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
    // Ping the server to test connectivity
    pingServer("8.8.8.8");  // You can replace "8.8.8.8" with any IP or domain you want to ping
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
}

