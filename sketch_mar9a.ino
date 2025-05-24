#include <Blynk.h>

// *** MAIN SETTINGS ***
#define BLYNK_TEMPLATE_ID "TMPL2lGVEsMe2"
#define BLYNK_TEMPLATE_NAME "Weather Station"
#define BLYNK_FIRMWARE_VERSION "0.1.0"
#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG
#define APP_DEBUG

// Add the required libraries
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include "BlynkEdgent.h"

// DHT sensor settings and configuration
#define DHT_BLYNK_VPIN_TEMPERATURE V0 //Virtual pin on Blynk side
#define DHT_BLYNK_VPIN_HUMIDITY V1 //Virtual pin on Blynk side

#define DHTPIN 25
#define DHTTYPE DHT21 
DHT dht(DHTPIN, DHTTYPE);

// BMP sensor settings and configuration
#define BMP_BLYNK_VPIN_PRESSURE V4 //Virtual pin on Blynk side
#define BMP_BLYNK_VPIN_ALTITUDE V3 //Virtual pin on Blynk side

#define BMP_SCK  (18)
#define BMP_MISO (19)
#define BMP_MOSI (23)
#define BMP_CS   (5)
Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO, BMP_SCK);

#define ALTITUDE_0 (1013.25)

// Forecast Virtual Pins Configuration
const int tempForecastPins[7] = {V6, V7, V8, V9, V10, V11, V12}; // Temperature forecast pins
const int humidForecastPins[7] = {V13, V15, V16, V17, V18, V19, V2}; // Humidity forecast pins

// Forecast data arrays (initial values)
float tempForecasts[7] = {18.0, 22.0, 16.0, 22.0, 16.0, 12.0, 19.0}; // Example temperature forecasts
float humidForecasts[7] = {52.0, 48.0, 45.0, 49.0, 50.0, 60.0, 61.0}; // Example humidity forecasts

BlynkTimer timer; 

// DHT related variables
int DHT_ENABLED = 0;
float DHT_HUMIDITY;
float DHT_HUMIDITY_IGNORED_DELTA = 0.01;
float DHT_TEMPERATURE;
float DHT_TEMPERATURE_IGNORED_DELTA = 0.01;

// BMP related variables
int BMP_ENABLED = 0;
float BMP_ALTITUDE;
float BMP_PRESSURE;
float BMP_PRESSURE_IGNORED_DELTA = 0.01;
float BMP_ALTITUDE_IGNORED_DELTA = 0.01;

int RUN = 0;

// SETUP BLOCK
void setupDht() {
  Serial.println("DHT startup!");
  dht.begin();
  DHT_ENABLED = 1;
}

void setupBMP() {
  Wire.begin();
  unsigned status;
  status = bmp.begin(0x76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                    "try a different address!"));
  } else {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);
    Serial.println(F("Valid BMP280 sensor"));
  }
}

// SENSOR DATA FUNCTIONS
void sendDhtData() {
  Serial.println("Sending DHT data");
  Blynk.virtualWrite(DHT_BLYNK_VPIN_TEMPERATURE, DHT_TEMPERATURE);
  Blynk.virtualWrite(DHT_BLYNK_VPIN_HUMIDITY, DHT_HUMIDITY);
}

void sendBMPData() {
  Serial.println("Sending BMP data");
  Blynk.virtualWrite(BMP_BLYNK_VPIN_PRESSURE, BMP_PRESSURE);
  Blynk.virtualWrite(BMP_BLYNK_VPIN_ALTITUDE, BMP_ALTITUDE);
}

// NEW: Forecast data sending function
void sendForecastData() {
  Serial.println("Sending forecast data");
  
  // Send temperature forecasts
  for(int i = 0; i < 7; i++) {
    Blynk.virtualWrite(tempForecastPins[i], tempForecasts[i]);
  }
  
  // Send humidity forecasts
  for(int i = 0; i < 7; i++) {
    Blynk.virtualWrite(humidForecastPins[i], humidForecasts[i]);
  }
}

// DATA PROCESSING BLOCK
void readAndSendDhtData() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT");
  } else {
    float humidityDelta = abs(humidity - DHT_HUMIDITY) - DHT_HUMIDITY_IGNORED_DELTA;
    float temperatureDelta = abs(temperature - DHT_TEMPERATURE) - DHT_HUMIDITY_IGNORED_DELTA;
    
    if (humidityDelta > 0 || temperatureDelta > 0) {
      DHT_HUMIDITY = humidity;
      DHT_TEMPERATURE = temperature;
      Serial.printf("Humidity: %f%%. Temperature: %f*C.\n", humidity, temperature);
      sendDhtData();
    }
  }
}

void readAndSendBMPData() {
  float pressure = bmp.readPressure() / 100.0F;
  float altitude = bmp.readAltitude(ALTITUDE_0);
  
  float pressureDelta = abs(pressure - BMP_PRESSURE) - BMP_PRESSURE_IGNORED_DELTA;
  float altitudeDelta = abs(altitude - BMP_ALTITUDE) - BMP_ALTITUDE_IGNORED_DELTA;

  if (pressureDelta > 0 || altitudeDelta > 0) {
    BMP_PRESSURE = pressure;
    BMP_ALTITUDE = altitude;
    Serial.printf("Pressure = %fhPa; Approx. Altitude = %fm;\n", pressure, altitude);
    sendBMPData();
  }
}

// UPDATED: Combined sensor and forecast data function
void readAndSendAllData() {
  readAndSendBMPData();
  readAndSendDhtData();
  sendForecastData(); // Added forecast data sending
  Serial.println("Sending all sensor and forecast data");
}

void setup() {
  Serial.begin(115200);
  delay(100);

  BlynkEdgent.begin();
  setupDht();
  setupBMP();

  // Set up timer to run every 5 sec
  timer.setInterval(5000L, readAndSendAllData); // Updated to use the combined function
}

void loop() {
  BlynkEdgent.run();
  timer.run();
  
  // Note: In a real implementation, you would update the forecast arrays
  // periodically from your prediction model instead of using static values
}
