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
#include <ThingSpeak.h>      // Include Thingspeak library

/*********************************************
  SECTION: Set APN based on your SIM card
  AIRTEL: "airtelgprs.com"
  BSNL: "bsnlnet"
  VI: "portalnmms"
  JIO: "jionet"
*********************************************/
const char apn[] = "bsnlnet";  // Change this as per your SIM card operator

/*********************************************
  SECTION: Set Thingspeak API key and channel details
*********************************************/
unsigned long myChannelNumber = 2711202;       // Replace with your Thingspeak channel ID
const char* myWriteAPIKey = "7902YSZ23JA2GUW0";  // Replace with your Thingspeak API key

TinyGsm modem(SerialAT);           // TinyGSM instance for the modem
TinyGsmClient client(modem);       // TinyGSM client for Thingspeak        // ThingSpeak instance

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
  Serial.println("  Bharat Pi 4G/LTE Board Network Connection Test with Thingspeak Cloud");
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

  Serial.println("Initializing Thingspeak client...");

  while(!ThingSpeak.begin(client)) {  // Connect Thingspeak to the TinyGSM client
    Serial.println("."); 
  }

  Serial.println("Thingspeak client initialized successfully.");
}

/******************************************************************
SECTION: Loop call to send data to Thingspeak
*******************************************************************/  

void loop() {
  // Blink LED to show activity
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  delay(1000);  // Blink every second

  // Example sensor data (replace with actual sensor readings)
  float temperature = 23.5;   // Dummy temperature value
  float humidity = 60.5;      // Dummy humidity value

  // Set field values for Thingspeak
  ThingSpeak.setField(1, temperature);  // Set Field 1 to temperature
  ThingSpeak.setField(2, humidity);     // Set Field 2 to humidity

  // Send data to Thingspeak
  int responseCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  
  if(responseCode == 200) {
    Serial.println("Data sent to Thingspeak successfully!");
  } else {
    Serial.println("Failed to send data to Thingspeak. Response code: " + String(responseCode));
  }
  
  delay(20000);  // Wait 20 seconds before sending data again (Thingspeak free-tier limit)
}
