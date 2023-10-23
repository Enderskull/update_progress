// Import required libraries
#include "ESPAsyncWebServer.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "ESP32_MailClient.h"
#include <AsyncTCP.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "IOT - Tsat";
const char* password = "SemangatP4G!";

// Buat object http
 HTTPClient http;

//sesuaikan dgn ip dan direktori penyimpanan file php anda
String url = "http://10.99.226.239/getdata.php?";
String payload;

// // Set your Static IP address
// IPAddress local_IP(192, 168, 219, 3);
// // Set your Gateway IP address
// IPAddress gateway(192, 168, 219, 116);

// IPAddress subnet(255, 255, 255, 0);
// IPAddress primaryDNS(8, 8, 8, 8);   //optional
// IPAddress secondaryDNS(8, 8, 4, 4); //optional

#define emailSenderAccount    "projectesp3@gmail.com"
#define emailSenderPassword   "agidnrfqogjibqwf"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define emailSubject          "[ALERT] ESP32 Status Report"

int query = 0;
const int pinDoor = 18;
String doorState;
const int buzzer = 2;

#define DHTPIN 15     // Digital pin connected to the DHT sensor
// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

//-----------------------------------------------Lanjut dari sini--------

// Default Recipient Email Address
String inputMessage = "roysta881@gmail.com";
String enableEmailChecked = "checked";
String ceklisEmail = "true";
// Default Threshold Temperature Value
String tempMin = "19.0";
String tempMax = "25.0";
String humidMin = "40.0";
String humidMax = "60.0";
String lastTemperature;
String lastHumidity;

DHT dht(DHTPIN, DHTTYPE);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

SMTPData smtpData;

void alarmOn(){
  for(int i = 0; i<3; i++){
    digitalWrite(buzzer, HIGH);
    delay(300);
    digitalWrite(buzzer, LOW);
    delay(300);
  }
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

bool sendEmailNotification(String emailMessage){
  // Set the SMTP Server Email host, port, account and password
  smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);

  // For library version 1.2.0 and later which STARTTLS protocol was supported,the STARTTLS will be 
  // enabled automatically when port 587 was used, or enable it manually using setSTARTTLS function.
  //smtpData.setSTARTTLS(true);

  // Set the sender name and Email
  smtpData.setSender("ESP32", emailSenderAccount);

  // Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
  smtpData.setPriority("High");

  // Set the subject
  smtpData.setSubject(emailSubject);

  // Set the message with HTML format
  smtpData.setMessage(emailMessage, true);

  // Add recipients
  smtpData.addRecipient(inputMessage);

  smtpData.setSendCallback(sendCallback);

  // Start sending Email, can be set callback function to track the status
  if (!MailClient.sendMail(smtpData)) {
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
    return false;
  }
  // Clear all data from Email object to free memory
  smtpData.empty();
  return true;
}

// Callback function to get the Email sending status
void sendCallback(SendStatus msg) {
  // Print the current status
  Serial.println(msg.info());

  // Do something when complete
  if (msg.success()) {
    Serial.println("----------------");
  }
}

String readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float t = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(t)) {    
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else {
    Serial.println(t);
    return String(t);
  }
}

String readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else {
    Serial.println(h);
    return String(h);
  }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 2.5rem; }
    p { font-size: 2.5rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
    label {
      font-weight: bold;
    }
    form {
      background-color: #fff;
      padding: 20px;
      border-radius: 5px;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.2);
      width: 300px;
      margin: 0 auto;
    }
    input[type="checkbox"] {
      margin-right: 5px;
    }
    input[type="submit"] {
      background-color: #007BFF;
      color: #fff;
      padding: 10px 20px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
    }
    
  </style>
</head>
<body>
  <h2>ESP32 DHT Server</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">&percnt;</sup>
  </p>
  <h2>ESP Email Notification</h2>
  <form action="/get">
    <label for="email_input">Email Address:</label>
    <input type="email" name="email_input" id="email_input" value="%EMAIL_INPUT%" required><br>
    
    <label for="enable_email_input">Enable Email Notification:</label>
    <input type="checkbox" name="enable_email_input" id="enable_email_input" value="true" %ENABLE_EMAIL%><br>

    <label for="threshold_input_min">Temperature Threshold min:</label>
    <input type="number" step="0.1" name="threshold_input_min" id="threshold_input_min" value="%THRESHOLD1%" required><br>

    <label for="threshold_input_max">Temperature Threshold max:</label>
    <input type="number" step="0.1" name="threshold_input_max" id="threshold_input_max" value="%THRESHOLD2%" required><br>

    <label for="threshold_input_min_h">Humidity Threshold min:</label>
    <input type="number" step="0.1" name="threshold_input_min_h" id="threshold_input_min_h" value="%HUMIDMIN%" required><br>

    <label for="threshold_input_max_h">Humidity Threshold max:</label>
    <input type="number" step="0.1" name="threshold_input_max_h" id="threshold_input_max_h" value="%HUMIDMAX%" required><br>

    <input type="submit" value="Submit">
  </form>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return readDHTTemperature();
  }
  else if(var == "HUMIDITY"){
    return readDHTHumidity();
  }
  else if(var == "EMAIL_INPUT"){
    return inputMessage;
  }
  else if(var == "ENABLE_EMAIL"){
    return enableEmailChecked;
  }
  else if(var == "THRESHOLD1"){
    return tempMin;
  }
  else if(var == "THRESHOLD2"){
    return tempMax;
  }
  else if(var == "HUMIDMIN"){
    return humidMin;
  }
  else if(var == "HUMIDMAX"){
    return humidMax;
  }
  return String();
}

// Flag variable to keep track if email notification was sent or not
bool emailSent = false;

const char* PARAM_EMAIL = "email_input";
const char* PARAM_CEKLIS_EMAIL = "enable_email_input";
const char* PARAM_TEMP_MIN = "threshold_input_min";
const char* PARAM_TEMP_MAX = "threshold_input_max";
const char* PARAM_HUMID_MIN = "threshold_input_min_h";
const char* PARAM_HUMID_MAX = "threshold_input_max_h";


// Interval between sensor readings. Learn more about timers: https://RandomNerdTutorials.com/esp32-pir-motion-sensor-interrupts-timers/
unsigned long previousMillis = 0;     
const long interval = 5000;
// The Email Sending data object contains config and data to send


void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  pinMode(pinDoor, INPUT);
  pinMode(buzzer, OUTPUT);
  dht.begin();
  
 // Configures static IP address
  // if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
  //   Serial.println("STA Failed to configure");
  // }
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDHTTemperature().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDHTHumidity().c_str());
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // GET email_input value on <ESP_IP>/get?email_input=<inputMessage>
    if (request->hasParam(PARAM_EMAIL)) {
      inputMessage = request->getParam(PARAM_EMAIL)->value();
      // GET enable_email_input value on <ESP_IP>/get?enable_email_input=<ceklisEmail>
      if (request->hasParam(PARAM_CEKLIS_EMAIL)) {
        ceklisEmail = request->getParam(PARAM_CEKLIS_EMAIL)->value();
        enableEmailChecked = "checked";
      }
      else {
        ceklisEmail = "false";
        enableEmailChecked = "";
      }
      // GET threshold_input value on <ESP_IP>/get?threshold_input=<tempMin>
      if (request->hasParam(PARAM_TEMP_MIN)) {
        tempMin = request->getParam(PARAM_TEMP_MIN)->value();
      }
      if (request->hasParam(PARAM_TEMP_MAX)) {
        tempMax = request->getParam(PARAM_TEMP_MAX)->value();
      }
      if (request->hasParam(PARAM_HUMID_MIN)) {
        humidMin = request->getParam(PARAM_HUMID_MIN)->value();
      }
      if (request->hasParam(PARAM_HUMID_MAX)) {
        humidMax = request->getParam(PARAM_HUMID_MAX)->value();
      }
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    Serial.println(ceklisEmail);
    Serial.println(tempMin);
    Serial.println(tempMax);
    request->send(200, "text/html", "HTTP GET request sent to your ESP.<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  // Start server
  server.begin();
}
 
void loop(){
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    //sensors.requestTemperatures();

    int door = digitalRead(pinDoor);
  
    if(door == 0){
      doorState = "Pintu Terbuka";
    }
    else{
      doorState = "Pintu Tertutup";
    }


    // Read temperature as Celsius (the default)
    float temperature = dht.readTemperature();
    Serial.print(temperature);
    Serial.println(" *C");
    float h = dht.readHumidity();
    Serial.print(h);
    Serial.println(" %");
    if (isnan(h) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
    }

    if (WiFi.status() == WL_CONNECTED) {
      
      http.begin(url + "door=" + String(door)); 
         
      int httpCode = http.GET();

      if (httpCode > 0){
        char json[500];
        String payload = http.getString();
        payload.toCharArray(json, 500);

        DynamicJsonDocument doc(JSON_OBJECT_SIZE(2));
        // Deserialize the JSON document
        deserializeJson(doc, json);
          String alarm  = doc["alarm"];
        
       
        Serial.print("HTTP Response= ");
        Serial.println(httpCode);
        Serial.println(doorState);
        Serial.println("---------------------------");

        if(alarm == "1" && door == 0){
         alarmOn();
         String emailMessage = String("Pintu terbuka");
          if(sendEmailNotification(emailMessage)) {
            Serial.println(emailMessage);
            emailSent = true;
          }
          else {
            Serial.println("Email failed to send");
          }
        }
        else{
          digitalWrite(buzzer, LOW);
        }
      }
      else{
        Serial.println("Failed Connect to Server");
      }
     
    }  
    else {
      Serial.println("Disconnected!!!");
    }

    // Temperature in Fahrenheit degrees
    /*float temperature = sensors.getTempFByIndex(0);
    SerialMon.print(temperature);
    SerialMon.println(" *F");*/
    
    lastTemperature = String(temperature);
    lastHumidity = String(h);
    
    // Check if temperature is above threshold and if it needs to send the Email alert
    if(temperature < tempMin.toFloat() && ceklisEmail == "true" && !emailSent){
      String emailMessage = String("Temperature below threshold. Current temperature: ") + 
                            String(temperature) + String("C");
      if(sendEmailNotification(emailMessage)) {
        Serial.println(emailMessage);
        emailSent = true;
      }
      else {
        Serial.println("Email failed to send");
      }    
    }
    // Check if temperature is below threshold and if it needs to send the Email alert
    else if((temperature > tempMax.toFloat()) && ceklisEmail == "true" && !emailSent) {
      String emailMessage = String("Temperature above threshold. Current temperature: ") + 
                            String(temperature) + String(" C");
      if(sendEmailNotification(emailMessage)) {
        Serial.println(emailMessage);
        emailSent = true;
      }
      else {
        Serial.println("Email failed to send");
      }
    }
    if(h < humidMin.toFloat() && ceklisEmail == "true" && !emailSent){
      String emailMessage = String("Humidity below threshold. Current humidity: ") + 
                            String(h) + String("%");
      if(sendEmailNotification(emailMessage)) {
        Serial.println(emailMessage);
        emailSent = true;
      }
      else {
        Serial.println("Email failed to send");
      }    
    }
    // Check if temperature is below threshold and if it needs to send the Email alert
    else if((h > humidMax.toFloat()) && ceklisEmail == "true" && !emailSent) {
      String emailMessage = String("humidity above threshold. Current Humidity: ") + 
                            String(h) + String(" %");
      if(sendEmailNotification(emailMessage)) {
        Serial.println(emailMessage);
        emailSent = true;
      }
      else {
        Serial.println("Email failed to send");
      }
    }
    else {
      emailSent = false;
    }
  }
}

