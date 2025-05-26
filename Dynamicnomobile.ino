#include <BlynkSimpleEsp32.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include "BlynkEdgent.h"

// Firebase credentials
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "YOUR_FIREBASE_DATABASE_URL"
#define USER_EMAIL "YOUR_USER_EMAIL"
#define USER_PASSWORD "YOUR_USER_PASSWORD"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool firebaseReady = false;

// Blynk settings
#define BLYNK_TEMPLATE_ID "TMPL2lGVEsMe2"
#define BLYNK_TEMPLATE_NAME "Weather Station"
#define BLYNK_FIRMWARE_VERSION "0.1.0"
#define BLYNK_PRINT Serial
#define APP_DEBUG

// DHT Sensor
#define DHT_BLYNK_VPIN_TEMPERATURE V0
#define DHT_BLYNK_VPIN_HUMIDITY V1
#define DHTPIN 25
#define DHTTYPE DHT21 
DHT dht(DHTPIN, DHTTYPE);

// BMP Sensor
#define BMP_BLYNK_VPIN_PRESSURE V4
#define BMP_BLYNK_VPIN_ALTITUDE V3
#define BMP_SCK 18
#define BMP_MISO 19
#define BMP_MOSI 23
#define BMP_CS 5
#define ALTITUDE_0 1013.25
Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO, BMP_SCK);

// Forecast pins
const int tempForecastPins[7]  = {V6, V7, V8, V9, V10, V11, V12};
const int humidForecastPins[7] = {V13, V15, V16, V17, V18, V19, V2};

// Forecast data cache
float tempForecasts[7] = {0};
float humidForecasts[7] = {0};

BlynkTimer timer;

// Sensor data cache
float DHT_TEMPERATURE = 0;
float DHT_HUMIDITY = 0;
float BMP_PRESSURE = 0;
float BMP_ALTITUDE = 0;

// Ignore small changes
float DHT_TEMPERATURE_IGNORED_DELTA = 0.5;
float DHT_HUMIDITY_IGNORED_DELTA = 0.5;
float BMP_PRESSURE_IGNORED_DELTA = 0.5;
float BMP_ALTITUDE_IGNORED_DELTA = 1.0;

void setupDht() {
  Serial.println("Initializing DHT sensor...");
  dht.begin();
}

void setupBMP() {
  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not found!");
  } else {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
    Serial.println("BMP280 initialized");
  }
}

void sendDhtData() {
  Blynk.virtualWrite(DHT_BLYNK_VPIN_TEMPERATURE, DHT_TEMPERATURE);
  Blynk.virtualWrite(DHT_BLYNK_VPIN_HUMIDITY, DHT_HUMIDITY);
}

void sendBMPData() {
  Blynk.virtualWrite(BMP_BLYNK_VPIN_PRESSURE, BMP_PRESSURE);
  Blynk.virtualWrite(BMP_BLYNK_VPIN_ALTITUDE, BMP_ALTITUDE);
}

void readAndSendDhtData() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT read error");
    return;
  }

  if (abs(humidity - DHT_HUMIDITY) > DHT_HUMIDITY_IGNORED_DELTA || abs(temperature - DHT_TEMPERATURE) > DHT_TEMPERATURE_IGNORED_DELTA) {
    DHT_HUMIDITY = humidity;
    DHT_TEMPERATURE = temperature;
    Serial.printf("Humidity: %.1f%%, Temp: %.1f°C\n", humidity, temperature);
    sendDhtData();
  }
}

void readAndSendBMPData() {
  float pressure = bmp.readPressure() / 100.0;
  float altitude = bmp.readAltitude(ALTITUDE_0);

  if (abs(pressure - BMP_PRESSURE) > BMP_PRESSURE_IGNORED_DELTA || abs(altitude - BMP_ALTITUDE) > BMP_ALTITUDE_IGNORED_DELTA) {
    BMP_PRESSURE = pressure;
    BMP_ALTITUDE = altitude;
    Serial.printf("Pressure: %.1f hPa, Altitude: %.1f m\n", pressure, altitude);
    sendBMPData();
  }
}

void readAndSendAllSensorData() {
  readAndSendBMPData();
  readAndSendDhtData();
}

void fetchAndSendForecastFromFirebase() {
  if (!firebaseReady) return;

  if (Firebase.RTDB.getJSON(&fbdo, "/prediction")) {  // Changed from "/predictions" to "/prediction"
    Serial.println("Fetched forecast from Firebase");
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, fbdo.to<FirebaseJson>().raw());
    if (error) {
      Serial.println("JSON parse failed");
      return;
    }

    JsonArray tempArr = doc["temperature"];
    JsonArray humidArr = doc["humidity"];

    for (int i = 0; i < 7; i++) {
      float t = tempArr[i] | 0.0;
      float h = humidArr[i] | 0.0;

      // Avoid unnecessary writes
      if (t != tempForecasts[i]) {
        tempForecasts[i] = t;
        Blynk.virtualWrite(tempForecastPins[i], t);
      }
      if (h != humidForecasts[i]) {
        humidForecasts[i] = h;
        Blynk.virtualWrite(humidForecastPins[i], h);
      }
    }
  } else {
    Serial.print("Firebase error: ");
    Serial.println(fbdo.errorReason());
  }
}

void setupFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Connecting to Firebase...");
  unsigned long timeout = millis();
  while (!Firebase.ready() && millis() - timeout < 10000) {
    delay(100);
  }
  firebaseReady = Firebase.ready();
  Serial.println(firebaseReady ? "Firebase ready!" : "Firebase not ready.");
}

void setup() {
  Serial.begin(115200);
  BlynkEdgent.begin();
  setupDht();
  setupBMP();
  setupFirebase();

  // Every 30 seconds: read sensors
  timer.setInterval(30000L, readAndSendAllSensorData);

  // Every 4 minutes: fetch forecast
  timer.setInterval(240000L, fetchAndSendForecastFromFirebase);
}

void loop() {
  BlynkEdgent.run();
  timer.run();
}
