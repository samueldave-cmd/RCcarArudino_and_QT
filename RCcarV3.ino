#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <VL53L0X.h>

// ===============================
// WIFI SETTINGS
// ===============================
const char* ssid = "ESPWIFIController";
const char* password = "552888000";

WiFiServer server(8888);
WiFiClient client;

// ===============================
// MOTOR PINS
// ===============================
#define IN1 27 // Drive Forward
#define IN2 26 // Drive Backward
#define IN3 12 // Steer Left
#define IN4 14 // Steer Right

// ===============================
// ULTRASONIC PINS
// ===============================
const int PIN_TRIG_FA = 18;
const int PIN_ECHO_FA = 19;
const int PIN_TRIG_FB = 17;
const int PIN_ECHO_FB = 16;

// ===============================
// TOF & STATE SETTINGS
// ===============================
const int TOF_SDA = 21;
const int TOF_SCL = 22;
VL53L0X tofSensor;

unsigned long lastSensorReadTime = 0;
bool autoMode = false; // Tracks if the car is driving itself

// ===============================
// MOVEMENT FUNCTIONS
// ===============================
void driveForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
}

void driveBackward() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
}

void stopDrive() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
}

void steerLeft() {
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void steerRight() {
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void centerSteering() {
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

// ===============================
// SENSOR READING (Optimized for Low Lag)
// ===============================
int readUltrasonic(int pinTrig, int pinEcho) {
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);

  // Timeout set to 5000us to prevent "High Ping" feeling
  long duration = pulseIn(pinEcho, HIGH, 3000);
  if (duration == 0) return 999;
  return duration * 0.034 / 2.0;
}

// ===============================
// AUTOMATIC MODE LOGIC
// ===============================
void runAutomaticMode() {
  int frontA = readUltrasonic(PIN_TRIG_FA, PIN_ECHO_FA);
  int frontB = readUltrasonic(PIN_TRIG_FB, PIN_ECHO_FB);
  uint16_t tofRawDist1 = tofSensor.readRangeSingleMillimeters();


  // 1. Emergency Stop if too close to a wall
  if (frontA < 15 || frontB < 15 || (tofRawDist1/10.0) < 20) {
    stopDrive();
    centerSteering();
    return;
  }

  // 2. Alignment Logic
  if ((frontB - frontA) > 0) {
    steerLeft();
    driveForward();
  } else if ((frontA - frontB) > 0) {
    steerRight();
    driveForward();
  } else {
    centerSteering();
    driveForward();
  }
}

// ===============================
// COMMAND HANDLER
// ===============================
void handleCommand(char command) {
  command = toupper(command);
 
  // Mode Switching
  if (command == 'G') { autoMode = true; Serial.println("MODE: AUTOMATIC"); return; }
  if (command == 'M') { autoMode = false; stopDrive(); centerSteering(); Serial.println("MODE: MANUAL"); return; }

  // Manual Controls (Only work if autoMode is OFF)
  if (!autoMode) {
    switch (command) {
      case 'W': driveForward(); break;
      case 'S': driveBackward(); break;
      case 'A': steerLeft(); break;
      case 'D': steerRight(); break;
      case 'X': stopDrive(); centerSteering(); break;
      case 'C': centerSteering(); break;
    }
  }
}

void sendSensorData() {
  int frontA = readUltrasonic(PIN_TRIG_FA, PIN_ECHO_FA);
  int frontB = readUltrasonic(PIN_TRIG_FB, PIN_ECHO_FB);
  uint16_t tofRawDist = tofSensor.readRangeSingleMillimeters();
  uint16_t tofRawDist1 = tofSensor.readRangeSingleMillimeters();

  String sensorLine = "FA:" + String(frontA) + "|FB:" + String(frontB);
  if (!tofSensor.timeoutOccurred() && tofRawDist < 8190) {
    sensorLine += "|ToF:" + String(tofRawDist / 10.0);
  }
 
  if (client && client.connected()) client.println(sensorLine);
  Serial.println(sensorLine);
}

// ===============================
// SETUP & LOOP
// ===============================
void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(PIN_TRIG_FA, OUTPUT); pinMode(PIN_ECHO_FA, INPUT);
  pinMode(PIN_TRIG_FB, OUTPUT); pinMode(PIN_ECHO_FB, INPUT);

  Wire.begin(TOF_SDA, TOF_SCL);
  tofSensor.init();
  tofSensor.setTimeout(500);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
 
  server.begin();
  Serial.println("\nReady. IP: " + WiFi.localIP().toString());
}

void loop() {
  // 1. Check for WiFi Commands first (Highest Priority)
  if (!client || !client.connected()) {
    client = server.available();
  } else if (client.available() > 0) {
    handleCommand(client.read());
  }

  // 2. If in Auto Mode, run the logic
  if (autoMode) {
    runAutomaticMode();
  }

  // 3. Send data to Qt every 2 seconds (Reduced frequency to save bandwidth)
  if (millis() - lastSensorReadTime > 500) {
    lastSensorReadTime = millis();
    sendSensorData();
  }
}