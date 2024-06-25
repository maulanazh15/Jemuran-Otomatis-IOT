#define BLYNK_TEMPLATE_ID "TMPL6YxwG5TWw"
#define BLYNK_TEMPLATE_NAME "Jemuran Otomatis"
#define BLYNK_AUTH_TOKEN "pexckRwoI6VmlumMMgKzx5Z5ZieXjaia"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#define SERVO_PIN 26
#define RAINDROP_SENSOR_PIN 34
#define DHT_PIN 25
#define DHT_TYPE DHT11

#define I2C_SDA 21
#define I2C_SCL 22

const char* ssid = "APDC";           // Ganti dengan SSID WiFi Anda
const char* password = "";           // Kosongkan jika WiFi tidak memiliki password
const char* auth = "pexckRwoI6VmlumMMgKzx5Z5ZieXjaia";

uint32_t delayMS;
float humidity;
float temperature;

bool isRaining = false;
bool previousIsRaining = false;

bool manualControl = false;  // Flag to check if manual control is activated
bool roofOpen = false;       // Track the state of the roof
bool autoMode = true;        // Flag to check if the system is in auto mode

DHT_Unified dht(DHT_PIN, DHT_TYPE);
Servo myservo;
BlynkTimer timer;
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27, 16 column and 2 rows

void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(humidity);
  lcd.print("%");
  
  lcd.setCursor(10, 0);
  if (autoMode) {
    lcd.print("Auto");
  } else {
    lcd.print("Manual");
  }

  lcd.setCursor(10, 1);
  if (roofOpen) {
    lcd.print("Open");
  } else {
    lcd.print("Closed");
  }
}

void myTimer() {
  // This function describes what will happen with each timer tick
  // e.g. writing sensor value to datastream V5
  Blynk.virtualWrite(V0, humidity);
  Blynk.virtualWrite(V1, temperature);
  updateLCD();  // Update LCD display with latest values
}


BLYNK_WRITE(V2) {
  // This function will be called every time the value of V2 changes (roof control)
  int pinValue = param.asInt(); // Get value as integer
  if (autoMode) {
    return; // Ignore manual control if in auto mode
  }
  if (pinValue == 1) {
    manualControl = true;
    roofOpen = true;
    myservo.write(90); // Move servo to open the roof
    Blynk.virtualWrite(V3, "Terbuka");
  } else {
    manualControl = true;
    roofOpen = false;
    myservo.write(0); // Move servo to close the roof
    Blynk.virtualWrite(V3, "Tertutup");
  }
  updateLCD();  // Update LCD display with new roof status
}

BLYNK_WRITE(V4) {
  // This function will be called every time the value of V4 changes (mode control)
  int pinValue = param.asInt(); // Get value as integer
  if (pinValue == 1) {
    autoMode = false;
    Serial.println("Manual mode activated");
  } else {
    autoMode = true;
    manualControl = false; // Reset manual control flag
    Serial.println("Auto mode activated");
  }
  updateLCD();  // Update LCD display with new mode
}

void setup() {
  Serial.begin(9600);

  // Menghubungkan ke WiFi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Jika tidak ada password, panggil WiFi.begin hanya dengan ssid
  if (strlen(password) == 0) {
    WiFi.begin(ssid);
  } else {
    WiFi.begin(ssid, password);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize servo
  myservo.attach(SERVO_PIN);
  myservo.write(90); // Initial position
  roofOpen = true;

  // Initialize DHT sensor
  dht.begin();

  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print(F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print(F("Driver Ver: ")); Serial.println(sensor.version);
  Serial.print(F("Unique ID: ")); Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value: ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print(F("Min Value: ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print(F("Resolution: ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print(F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print(F("Driver Ver: ")); Serial.println(sensor.version);
  Serial.print(F("Unique ID: ")); Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value: ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print(F("Min Value: ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print(F("Resolution: ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;

  // Initialize raindrop sensor pin
  pinMode(RAINDROP_SENSOR_PIN, INPUT);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  timer.setInterval(1000L, myTimer);
  
  // Initialize LCD
  lcd.init(I2C_SDA, I2C_SCL);
  lcd.backlight();
  updateLCD();  // Initial LCD update
}

void loop() {
  // Delay between measurements.
  Blynk.run();
  delay(delayMS);

  if (autoMode) {  // If auto mode is active, use automatic control
    // Get temperature event and print its value.
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
    } else {
      Serial.print(F("Temperature: "));
      Serial.print(event.temperature);
      Serial.println(F("°C"));
      temperature = event.temperature;
    }

    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
    } else {
      Serial.print(F("Humidity: "));
      Serial.print(event.relative_humidity);
      Serial.println(F("%"));
      humidity = event.relative_humidity;
    }

    // Read raindrop sensor value
    int raindropValue = analogRead(RAINDROP_SENSOR_PIN);
    Serial.print("Raindrop Sensor Value: ");
    Serial.println(raindropValue);

    // Determine if it is raining
    isRaining = (raindropValue < 2500 || humidity >= 80);

    // If the rain status has changed, send a log event
    if (isRaining != previousIsRaining) {
      if (isRaining) {
        Blynk.logEvent("hujan", "Terjadi Hujan, atap ditutup.");
        Serial.println("Hujan");
        myservo.write(0); // Move servo to close the roof
        Blynk.virtualWrite(V3, "Tertutup");
        roofOpen = false;
      } else {
        Serial.println("Cerah");
        Blynk.logEvent("cerah", "Cerah, atap dibuka.");
        myservo.write(90); // Move servo to open the roof
        Blynk.virtualWrite(V3, "Terbuka");
        roofOpen = true;
      }
      previousIsRaining = isRaining; // Update the previous state
      updateLCD();  // Update LCD display with new roof status
    }
  }

  timer.run();
}
