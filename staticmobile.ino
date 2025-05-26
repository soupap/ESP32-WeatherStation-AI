#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include "BlynkEdgent.h"

// Blynk settings
#define BLYNK_TEMPLATE_ID "TMPL2lGVEsMe2"
#define BLYNK_TEMPLATE_NAME "Weather Station"
#define BLYNK_AUTH_TOKEN "YOUR_MOBILE_AUTH_TOKEN"
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

// Static forecast data (adjust as you like)
float tempForecasts[7]  = {23.5, 24.1, 22.8, 25.3, 26.0, 24.7, 23.9};
float humidForecasts[7] = {55.0, 57.2, 60.1, 58.5, 54.9, 56.3, 59.0};

BlynkTimer timer;

float DHT_TEMPERATURE = 0;
float DHT_HUMIDITY = 0;
float BMP_PRESSURE = 0;
float BMP_ALTITUDE = 0;

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

void sendStaticForecastToBlynk() {
  for (int i = 0; i < 7; i++) {
    Blynk.virtualWrite(tempForecastPins[i], tempForecasts[i]);
    Blynk.virtualWrite(humidForecastPins[i], humidForecasts[i]);
  }
}

void setup() {
  Serial.begin(115200);
  BlynkEdgent.begin();
  Blynk.config(BLYNK_AUTH_TOKEN);
  setupDht();
  setupBMP();

  timer.setInterval(30000L, readAndSendBMPData);
  timer.setInterval(30000L, readAndSendDhtData);
  timer.setInterval(240000L, sendStaticForecastToBlynk);  // every 4 minutes
}

void loop() {
  BlynkEdgent.run();
  timer.run();
}
