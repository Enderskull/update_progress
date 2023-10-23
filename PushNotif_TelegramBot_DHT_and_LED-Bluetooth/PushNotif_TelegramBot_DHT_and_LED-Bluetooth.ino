#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <DHT.h> // DHT22 sensor library
#include <ArduinoJson.h>

// Parameters for temperature and humidity
const float minTemperature = 19.0;
const float maxTemperature = 25.0;
const float minHumidity = 40.0;
const float maxHumidity = 60.0;

// Variable to store LED state (ON/OFF)
bool ledState = false; 

// Replace with your network credentials
const char* ssid = "IOT - Tsat";
const char* password = "SemangatP4G!";

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "-990222954"
String warning = "";
// Initialize Telegram BOT
#define BOTtoken "6018857596:AAE3H_38k6F4Pv8r4lI8-pvAC2FUUzy20R0"  // your Bot Token (Get from Botfather)

#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Interval to update sensor data in milliseconds (2 minutes)
const unsigned long updateInterval = 2 * 60 * 1000;

// Variable to store the last time sensor data was updated
unsigned long lastUpdateTime = 0;


// Checks for new messages.
int botRequestDelay = 1000;;
unsigned long lastTimeBotRan;

// DHT22 sensor connect to ESP32/ESP8266 GPIO pin (any digital pin)
#define DHTPIN 27 // Change this to the appropriate GPIO pin
#define DHTTYPE DHT11   // DHT 22 
#define LED_BUILTIN  2

DHT dht(DHTPIN, DHTTYPE);

// Get DHT22 sensor readings and return them as a String variable
String getReadings(){
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Compute heat index in Celcius (the default)
  float hic = dht.computeHeatIndex(t, h, false);
 
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(hic)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  }

  bool isTemperatureWarning = (t < minTemperature || t > maxTemperature);
  bool isHumidityWarning = (h < minHumidity || h > maxHumidity);

  if (isTemperatureWarning && isHumidityWarning) {
     String warning = "PERINGATAN!!! Suhu dan Kelembapan melebihi parameter standarisasi.";
     bot.sendMessage(CHAT_ID, warning, "");
  } else if (isTemperatureWarning) {
     String warning = "PERINGATAN!!! Suhu melebihi parameter standarisasi.";
     bot.sendMessage(CHAT_ID, warning, "");
  } else if (isHumidityWarning) {
     String warning = "PERINGATAN!!! Kelembapan melebihi parameter standarisasi.";
     bot.sendMessage(CHAT_ID, warning, "");
  }
  
   
  // For string format
  // String message = "Temperature: " + String(t) + " ºC \n";
  // message += "Humidity: " + String (h) + " % \n";message += "Heat Index: " + String (hic) + " ºC \n";

  // Membuat objek JSON dan mengisi data
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["sensor"] = "DHT11";
  jsonDoc["temperature"] = String(t);
  jsonDoc["humidity"] = String(h, 2);
  jsonDoc["heatIndex"] = String(hic, 2);

  // Konversi objek JSON ke string
  String message;
  serializeJson(jsonDoc, message);

  return message;
}

// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Selamat Datang, " + from_name + ".\n";
      welcome += "Tekan Tanda Dibawah untuk kirim perintah (command for):  \n\n";
      welcome += "Baca sensor DHT: /readings \n";
      welcome += "For turn ON LED /LED_ON\nturn OFF LED /LED_OFF\n";
      welcome += "Get status LED: /status\n";
      welcome += "Threshold DC: /threshold";
      bot.sendMessage(chat_id, welcome, "");
    } else if (text == "/readings") {
      String readings = getReadings();
      bot.sendMessage(chat_id, "Pembacaan manual: "+ readings, "");
    } else if (text == "/LED_ON"){
      controlLED(true);
      bot.sendMessage(chat_id,"LED telah diaktifkan.", "");
    } else if (text == "/LED_OFF"){
      controlLED(false);
      bot.sendMessage(chat_id, "LED telah dinonaktifkan.", "");
    } else if (text == "/status"){
      String statusMessage = "Status LED: " + String(ledState ? "ON" : "OFF") + "\n";
      bot.sendMessage(chat_id, statusMessage, "");
    } else if (text == "/threshold"){
      String statusMessage = "Range threshold data center:\n";
      statusMessage +="Suhu: " + String(minTemperature) + "-"+ String(maxTemperature) +"\n";
      statusMessage +="Kelembapan: " + String(minHumidity) + "-"+ String(maxHumidity) +"\n";
      bot.sendMessage(chat_id, statusMessage, "");
    } else {
      String statusMessage = "Command Invalid!!! for begin task please use /start";
      bot.sendMessage(chat_id, statusMessage, "");
    }
  }
}


void setup() {
  Serial.begin(115200);

  #ifdef ESP8266
    configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
    client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  #endif

  // Initialize built-in LED pin as OUTPUT
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // Turn OFF LED saat setup

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
  dht.begin();
}

void controlLED(bool state) {
  digitalWrite(LED_BUILTIN, state ? HIGH : LOW);
  ledState = state;
}

void loop() {

  unsigned long currentMillis = millis();

  // Check if it's time to update sensor data
  if (currentMillis - lastUpdateTime >= updateInterval) {
    // Read sensor data
    String readings = getReadings();
    bot.sendMessage(CHAT_ID, readings, "");

    // Update last update time
    lastUpdateTime = currentMillis;
  }
  //check for new messages
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}