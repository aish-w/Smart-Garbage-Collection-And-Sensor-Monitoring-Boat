#include "DHT.h"             // Include the DHT library
#include <WiFi.h>            // For WiFi connectivity
#include <FirebaseESP32.h>   // Firebase library for ESP32

// WiFi Configuration
const char* ssid = "Devanshi";           // Replace with your WiFi SSID
const char* password = "devanshi@09";    // Replace with your WiFi Password

// Firebase Configuration
#define FIREBASE_HOST "https://smart-monitoring-boat-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "AIzaSyDaGElvDPrmD3sfkd67EL71T6ptv85msb8"

// Firebase objects
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

// Turbidity Sensor Setup
const int turbidityPin = 36; // GPIO pin connected to turbidity sensor
int turbidityValue = 0;

// DHT22 Sensor Setup
#define DHTPIN 4           // GPIO pin connected to DHT22
#define DHTTYPE DHT22      // DHT22 sensor type
DHT dht(DHTPIN, DHTTYPE);

// HC-SR04 Ultrasonic Sensor Setup
const int trigPin = 5;     // GPIO pin connected to TRIG
const int echoPin = 18;    // GPIO pin connected to ECHO
long duration;             // Variable for echo time
float distanceCm;          // Calculated distance in cm

// Function to map floats manually
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup() {
  Serial.begin(115200);          // Start serial communication
  pinMode(turbidityPin, INPUT);  // Set turbidity pin mode to input
  pinMode(trigPin, OUTPUT);      // Set TRIG pin as output
  pinMode(echoPin, INPUT);       // Set ECHO pin as input
  dht.begin();                   // Initialize DHT22 sensor
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  // Initialize Firebase Config
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase connected!");
}

void loop() {
  // ------------------ Turbidity Sensor Reading ------------------
  turbidityValue = analogRead(turbidityPin);          // Read analog value from turbidity sensor
  float voltage = turbidityValue * (3.3 / 4095.0);    // Convert to voltage
  float NTU = mapFloat(voltage, 0.0, 3.3, 3000.0, 0.0); // Manual mapping for NTU

  // ------------------ DHT22 Sensor Reading ------------------
  float temperature = dht.readTemperature(); // Read temperature in Celsius
  float humidity = dht.readHumidity();       // Read humidity in percentage

  // ------------------ HC-SR04 Ultrasonic Sensor Reading ------------------
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); // Send a 10µs pulse to trigger pin
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH); // Measure the time for the echo
  distanceCm = duration * 0.034 / 2; // Convert time to distance in cm

  // ------------------ Print Sensor Data ------------------
  Serial.println("Sending Sensor Data to Firebase...");

  // ------------------ Send Data to Firebase ------------------
  if (Firebase.ready()) {
    // Turbidity Sensor Data
    Firebase.setFloat(firebaseData, "/Sensors/Turbidity/Voltage", voltage);
    Firebase.setFloat(firebaseData, "/Sensors/Turbidity/NTU", NTU);
    
    // DHT22 Sensor Data (check for valid readings)
    if (!isnan(temperature) && !isnan(humidity)) {
      Firebase.setFloat(firebaseData, "/Sensors/DHT22/Temperature", temperature);
      Firebase.setFloat(firebaseData, "/Sensors/DHT22/Humidity", humidity);
    } else {
      Serial.println("Failed to read DHT22 data!");
    }

    // Ultrasonic Sensor Data
    Firebase.setFloat(firebaseData, "/Sensors/Ultrasonic/Distance", distanceCm);
    
    Serial.println("Data sent successfully!");
  } else {
    Serial.println("Firebase connection failed!");
  }

  // ------------------ Delay for Readability ------------------
  delay(2000); // Increased to 2 seconds for better Firebase stability
}