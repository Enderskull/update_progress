#include <WiFi.h>
#include "BluetoothSerial.h"

// Constants for LED pin and PWM range  
const int LEDBuiltin = 2;
const int pwmRange = 255;

// Bluetooth Serial object
BluetoothSerial SerialBT;

bool wifiConnected = false;
bool wifiConfigured = false;
String ssid = "";
String password = "";

enum ConfigState {
  WAIT_SSID,
  WAIT_PASSWORD,
  CONFIG_DONE,
  RETRY_CONFIG
};

ConfigState configState = WAIT_SSID;

bool isConnecting = false;
unsigned long lastFlashTime = 0;
const int flashInterval = 500;  // Flash interval in milliseconds

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);

  // Initialize LED pin
  pinMode(LEDBuiltin, OUTPUT);
  digitalWrite(LEDBuiltin, LOW); // Turn off LED

  // Start Bluetooth
  SerialBT.begin("ESP32-Bluetooth");  // Set your device name here
  Serial.println("Bluetooth started");

  Serial.println("Bluetooth connected. Please send SSID and password.");
  wifiConfigured = true;
}

void loop() {
  if (wifiConfigured && !wifiConnected) {
    while (SerialBT.available()) {
      char incomingChar = SerialBT.read();
      if (incomingChar == '\n') {
        processConfig();
        break;
      }
      if (configState == WAIT_SSID) {
        ssid += incomingChar;
      } else if (configState == WAIT_PASSWORD) {
        password += incomingChar;
      }
    }
  }

  if (configState == RETRY_CONFIG) {
    // Reset variables and prompt user to send SSID and password again
    SerialBT.flush();
    Serial.println("WiFi connection failed. Please send SSID and password again.");
    wifiConnected = false;
    wifiConfigured = true;
    configState = WAIT_SSID;
    ssid = "";
    password = "";
  }

  if (isConnecting) {
    // Continue flashing the LED while connecting
    if (millis() - lastFlashTime >= flashInterval) {
      lastFlashTime = millis();
      digitalWrite(LEDBuiltin, !digitalRead(LEDBuiltin));
    }
  } else if (wifiConnected) {
    // Your main loop code here
    // For example, you can check for specific conditions or perform actions
    
    // Example: Toggle LED state every 1 second
    static unsigned long previousMillis = 0;
    const long interval = 1000;
    
    unsigned long currentMillis = millis();
    
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      digitalWrite(LEDBuiltin, !digitalRead(LEDBuiltin)); // Toggle LED state
    }
  }
}

void processConfig() {
  if (configState == WAIT_SSID) {
    configState = WAIT_PASSWORD;
    Serial.println("Received SSID: " + ssid);
    Serial.println("Please send the password.");
  } else if (configState == WAIT_PASSWORD) {
    wifiConfigured = false;
    SerialBT.flush();
    delay(500);
    if (connectToWiFi()) {
      wifiConnected = true;
      configState = CONFIG_DONE;
      ssid = "";
      password = "";
    } else {
      Serial.println("Failed to connect to WiFi. Retrying...");
      configState = RETRY_CONFIG;
    }
  }
}

bool connectToWiFi() {
  if (!wifiConfigured) {
    Serial.println("WiFi not configured.");
    return false;
  }
  // Attempt to connect to WiFi
  WiFi.begin(ssid, password);
  //Serial.printf(ssid.c_str());
  //Serial.printf(password.c_str());
  Serial.print("Setting WiFi credentials. SSID: ");
  Serial.println(ssid.c_str());
  Serial.println("Connecting...");

  isConnecting = true;  // Indicate that the ESP32 is connecting

  // Wait for WiFi connection or timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    // Flash the LED while connecting
    if (millis() - lastFlashTime >= flashInterval) {
      lastFlashTime = millis();
      digitalWrite(LEDBuiltin, !digitalRead(LEDBuiltin));
    }
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  isConnecting = false;  // Connection attempt is done

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi. LED is on.");
    digitalWrite(LEDBuiltin, HIGH); // Turn on LED
    return true;
  } else {
    Serial.println("Failed to connect to WiFi.");
    digitalWrite(LEDBuiltin, LOW); // Turn off LED
    return false;
  }
}

