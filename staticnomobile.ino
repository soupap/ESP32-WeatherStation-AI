#define BLYNK_TEMPLATE_NAME "Weather Station"
#define BLYNK_FIRMWARE_VERSION "0.1.0"
#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG
#define APP_DEBUG

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include "BlynkEdgent.h"

// --- Virtual Pins ---
#define DHT_BLYNK_VPIN_TEMPERATURE V0
#define DHT_BLYNK_VPIN_HUMIDITY V1
#define BMP_BLYNK_VPIN_PRESSURE V4
#define BMP_BLYNK_VPIN_ALTITUDE V3

const int tempForecastPins[7] = {V6, V7, V8, V9, V10, V11, V12};
const int humidForecastPins[7] = {V13, V15, V16, V17, V18, V19, V2};

// --- Sensors ---
#define DHTPIN 25
#define DHTTYPE DHT21
DHT dht(DHTPIN, DHTTYPE);

#define BMP_SCK  (18)
#define BMP_MISO (19)
#define BMP_MOSI (23)
#define BMP_CS   (5)
Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO, BMP_SCK);
#define ALTITUDE_0 (1013.25)

// --- Sensor State Variables ---
float DHT_TEMPERATURE = 0;
float DHT_HUMIDITY = 0;
float BMP_PRESSURE = 0;
float BMP_ALTITUDE = 0;

// --- Change Thresholds ---
float DHT_TEMPERATURE_IGNORED_DELTA = 0.5;
float DHT_HUMIDITY_IGNORED_DELTA = 0.5;
float BMP_PRESSURE_IGNORED_DELTA = 0.5;
float BMP_ALTITUDE_IGNORED_DELTA = 1.0;

// --- Forecast Values (replace with model output later) ---
float tempForecasts[7] = {18.0, 22.0, 16.0, 22.0, 16.0, 12.0, 19.0};
float humidForecasts[7] = {52.0, 48.0, 45.0, 49.0, 50.0, 60.0, 61.0};

BlynkTimer timer;

// --- Setup Sensors ---
void setupDht() {
  Serial.println("DHT startup!");
  dht.begin();
}

void setupBMP() {
  Wire.begin();
  if (!bmp.begin(0x76)) {
    Serial.println(F("Could not find a valid BMP280 sensor!"));
  } else {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
    Serial.println(F("BMP280 initialized"));
  }
}

// --- Send Data to Blynk ---
void sendDhtData() {
  Blynk.virtualWrite(DHT_BLYNK_VPIN_TEMPERATURE, DHT_TEMPERATURE);
  Blynk.virtualWrite(DHT_BLYNK_VPIN_HUMIDITY, DHT_HUMIDITY);
  Serial.println("Sent DHT data to Blynk");
}

void sendBMPData() {
  Blynk.virtualWrite(BMP_BLYNK_VPIN_PRESSURE, BMP_PRESSURE);
  Blynk.virtualWrite(BMP_BLYNK_VPIN_ALTITUDE, BMP_ALTITUDE);
  Serial.println("Sent BMP data to Blynk");
}

void sendForecastData() {
  Serial.println("Sending forecast data");
  for (int i = 0; i < 7; i++) {
    Blynk.virtualWrite(tempForecastPins[i], tempForecasts[i]);
    Blynk.virtualWrite(humidForecastPins[i], humidForecasts[i]);
  }
}

// --- Read and Send Sensor Data ---
void readAndSendDhtData() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor");
    return;
  }

  if (abs(humidity - DHT_HUMIDITY) > DHT_HUMIDITY_IGNORED_DELTA ||
      abs(temperature - DHT_TEMPERATURE) > DHT_TEMPERATURE_IGNORED_DELTA) {
    DHT_HUMIDITY = humidity;
    DHT_TEMPERATURE = temperature;
    Serial.printf("Humidity: %.2f%%, Temp: %.2f°C\n", humidity, temperature);
    sendDhtData();
  }
}

void readAndSendBMPData() {
  float pressure = bmp.readPressure() / 100.0F;
  float altitude = bmp.readAltitude(ALTITUDE_0);

  if (abs(pressure - BMP_PRESSURE) > BMP_PRESSURE_IGNORED_DELTA ||
      abs(altitude - BMP_ALTITUDE) > BMP_ALTITUDE_IGNORED_DELTA) {
    BMP_PRESSURE = pressure;
    BMP_ALTITUDE = altitude;
    Serial.printf("Pressure: %.2f hPa, Altitude: %.2f m\n", pressure, altitude);
    sendBMPData();
  }
}

void readAndSendAllSensorData() {
  readAndSendBMPData();
  readAndSendDhtData();
  Serial.println("Completed sensor read cycle");
}

// --- Setup and Loop ---
void setup() {
  Serial.begin(115200);
  delay(1000);
  BlynkEdgent.begin();
  setupDht();
  setupBMP();

  timer.setInterval(15000L, readAndSendAllSensorData);  // every 15 sec
  timer.setInterval(60000L, sendForecastData);          // every 60 sec
}

void loop() {
  BlynkEdgent.run();
  timer.run();
}
