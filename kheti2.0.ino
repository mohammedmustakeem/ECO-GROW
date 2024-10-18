#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>
#include <Firebase_ESP_Client.h>

#define soil_moisture_pin 32
#define relay 23

#define API_KEY "AIzaSyCTG6zsr5jh7NNETtUEhl5Jf6AYVWNmwXw"
#define DATABASE_URL "https://plantirrigation-e3561-default-rtdb.asia-southeast1.firebasedatabase.app/" 
#define USER_EMAIL "bhardwajkeshav5173@gmail.com"
#define USER_PASSWORD "keshav@5173"
const char *ssid = "Redmi 12 5G";
const char *password = "12345677";

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

WebServer server(80);
DHT dht(15, DHT11);


void handleRoot() {
  char msg[1500];

  snprintf(msg, 1500,
           "<html>\
  <head>\
    <meta http-equiv='refresh' content='2'/>\
    <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <link rel='stylesheet' href='https://use.fontawesome.com/releases/v5.7.2/css/all.css' integrity='sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr' crossorigin='anonymous'>\
    <title>ESP32 DHT Server</title>\
    <style>\
    html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center;}\
    h2 { font-size: 3.0rem; }\
    p { font-size: 3.0rem; }\
    .units { font-size: 1.2rem; }\
    .dht-labels{ font-size: 1.5rem; vertical-align:middle; padding-bottom: 15px;}\
    </style>\
  </head>\
  <body>\
      <h2>Plant Monetering syster</h2>\
      <p>\
        <i class='fas fa-thermometer-half' style='color:#ca3517;'></i>\
        <span class='dht-labels'>Temperature</span>\
        <span>%.2f</span>\
        <sup class='units'>&deg;C</sup>\
      </p>\
      <p>\
        <i class='fas fa-tint' style='color:#00add6;'></i>\
        <span class='dht-labels'>Humidity</span>\
        <span>%.2f</span>\
        <sup class='units'>&percnt;</sup>\
      </p>\
      <p>\
        <i class='fas fa-tint' style='color:#00add6;'></i>\
        <span class='dht-labels'>Soil Moisture</span>\
        <span>%.2f</span>\
        <sup class='units'>&percnt;</sup>\
      </p>\
  </body>\
</html>",
           readDHTTemperature(), readDHTHumidity(), readSoilMoisture()
          );
  server.send(200, "text/html", msg);
}

void setup(void) {

  Serial.begin(115200);
  dht.begin();

  pinMode(relay, OUTPUT);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Firebase configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Authentication
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Initialize Firebase
  Firebase.begin(&config, &auth);

  Firebase.reconnectWiFi(true);
  Serial.println("Firebase initialized");

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);

  server.begin();
  Serial.println("HTTP server started");

}

void loop() {
  server.handleClient();
  float temp = readDHTTemperature();
  float hum = readDHTHumidity();
  float soil_per = readSoilMoisture();

  // Send data to Firebase
  if (Firebase.ready()) {
    Firebase.RTDB.setFloat(&fbdo, "/eco-grow/Temperature", temp);
    Firebase.RTDB.setFloat(&fbdo, "/eco-grow/Humidity", hum);
    Firebase.RTDB.setFloat(&fbdo, "/eco-grow/Soil_moisture", soil_per);
    Serial.println("Data sent to Firebase");
  }

  working(temp, hum, soil_per);
  delay(1000);
}


float readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  if (isnan(t)) {    
    Serial.println("Failed to read from DHT sensor!");
    return -1;
  }
  else {
    // Serial.println(t);
    return t;
  }
}

float readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return -1;
  }
  else {
    // Serial.println(h);
    return h;
  }
}
float readSoilMoisture(){
  float soil_val = analogRead(soil_moisture_pin);
  soil_val -= 4095;
  soil_val -= 2*soil_val;

  float soil_per = (soil_val / 4095.0) * 100.0; 
  // Serial.println(soil_per);
  return soil_per;
}


void working(float temp, float hum, float soil_per){
  int tempUpperLimit = 35; 
  int tempLowerLimit = 18; 
  int humidUpperLimit = 70; 
  int humidLowerLimit = 40; 
  int soilMoistUpperLimit = 80;
  int soilMoistLowerLimit = 55; 

  if (soil_per > 50) {
    digitalWrite(relay, LOW); 
    Serial.println("Water pump OFF: Soil moisture above 50 %");
  } 
  else if(soil_per > 25 && soil_per <= 50) {
    if(temp > 28 && hum < humidLowerLimit) {
        digitalWrite(relay, HIGH);
        Serial.println("Water Pump ON: Soil moisture between 10%-30%, temp > 28°C and humidity < lower limit");
    }
  }
  else if(soil_per<20){
    digitalWrite(relay, HIGH);
    Serial.println("Water pump ON: Soil moisture less than 10%");
  }
  else {
    if(temp > tempUpperLimit) {  
        digitalWrite(relay, HIGH);
        Serial.println("Water pump ON: Soil moisture <= 10% and temp > upper limit");
    }
    else if(temp < tempLowerLimit) {
        digitalWrite(relay, LOW); 
        Serial.println("Water pump OFF: Soil moisture <= 10% but temp < lower limit");
    }
}

  delay(10);
  Serial.print("Soil Percentage: ");
  Serial.println(soil_per);
  Serial.print("Temperature: ");
  Serial.println(temp);
  Serial.print("Humidity: ");
  Serial.println(hum);
}