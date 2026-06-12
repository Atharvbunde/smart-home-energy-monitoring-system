#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ===================== CONFIG =====================
// WiFi credentials (Wokwi simulated network)
const char* ssid     = "Wokwi-GUEST";
const char* password = "";

// ThingSpeak settings
const char* thingSpeakApiKey = "UGOB6NJFY4AD83OZ"; // Write API Key
const char* thingSpeakServer = "http://api.thingspeak.com/update";
const unsigned long thingSpeakInterval = 16000; // ThingSpeak free tier needs >=15s between updates

// Pins
const int potPin    = 34; // Slide potentiometer (simulated power sensor)
const int buzzerPin = 12;
const int ledPin    = 13;

// LCD (I2C address 0x27, 20x4)
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Thresholds
const float powerThreshold = 100.0; // Watts - alert level

// Timing
unsigned long lastUpload = 0;
unsigned long lastDisplay = 0;
const unsigned long displayInterval = 1000;

// Function declarations
int myFunction(int, int);
float readPower();
void updateDisplay(float voltage, float power, float energyWh);
void checkAlert(float power);
void sendToThingSpeak(float voltage, float power, float energyWh);
void connectWiFi();

// Energy accumulation
float totalEnergyWh = 0;
unsigned long lastEnergyUpdate = 0;

void setup() {
  Serial.begin(115200);

  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, LOW);

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Energy Monitoring");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  int result = myFunction(2, 3);
  Serial.print("myFunction(2,3) = ");
  Serial.println(result);

  connectWiFi();

  lastEnergyUpdate = millis();
  lastDisplay = millis();
  lastUpload = millis();

  delay(1000);
  lcd.clear();
}

void loop() {
  unsigned long now = millis();

  // Read sensor and compute power
  float voltage = (analogRead(potPin) / 4095.0) * 30.0; // 0-30V range
  float current  = 5.0; // simulated constant load current (A)
  float power = voltage * current; // Watts

  // Update accumulated energy (Wh)
  unsigned long elapsed = now - lastEnergyUpdate;
  totalEnergyWh += power * (elapsed / 3600000.0);
  lastEnergyUpdate = now;

  // Update LCD periodically
  if (now - lastDisplay >= displayInterval) {
    updateDisplay(voltage, power, totalEnergyWh);
    lastDisplay = now;
  }

  // Check buzzer/LED alert
  checkAlert(power);

  // Send to ThingSpeak periodically
  if (now - lastUpload >= thingSpeakInterval) {
    sendToThingSpeak(voltage, power, totalEnergyWh);
    lastUpload = now;
  }
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi...");
  Serial.print("Connecting to WiFi");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    lcd.setCursor(0, 1);
    lcd.print("WiFi Connected     ");
  } else {
    Serial.println("\nWiFi connection failed");
    lcd.setCursor(0, 1);
    lcd.print("WiFi Failed        ");
  }
}

void updateDisplay(float voltage, float power, float energyWh) {
  lcd.setCursor(0, 0);
  lcd.print("Energy Monitoring   ");

  lcd.setCursor(0, 1);
  lcd.print("Voltage: ");
  lcd.print(voltage, 2);
  lcd.print(" V    ");

  lcd.setCursor(0, 2);
  lcd.print("Power:   ");
  lcd.print(power, 2);
  lcd.print(" W    ");

  lcd.setCursor(0, 3);
  lcd.print("Energy: ");
  lcd.print(energyWh, 3);
  lcd.print(" Wh   ");
}

void checkAlert(float power) {
  if (power > powerThreshold) {
    digitalWrite(ledPin, HIGH);
    digitalWrite(buzzerPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
    digitalWrite(buzzerPin, LOW);
  }
}

void sendToThingSpeak(float voltage, float power, float energyWh) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping upload");
    connectWiFi();
    return;
  }

  HTTPClient http;
  String url = String(thingSpeakServer) + "?api_key=" + thingSpeakApiKey +
                "&field1=" + String(voltage, 2) +
                "&field2=" + String(power, 2) +
                "&field3=" + String(energyWh, 3);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.print("ThingSpeak response: ");
    Serial.println(httpCode);
  } else {
    Serial.print("ThingSpeak upload failed, error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}