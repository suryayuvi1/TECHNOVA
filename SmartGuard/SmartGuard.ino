#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <SPI.h>
#include <DHT.h>
#include <MFRC522.h>
#include <LiquidCrystal_PCF8574.h>

// ==========================================
// SMARTGUARD - INTELLIGENT SAFETY SYSTEM
// BOARD: PCB CUPID GLYPH-C3 ESP32-C3
// ==========================================

// -------- WIFI ACCESS POINT --------

const char* wifiName = "SMARTGUARD";
const char* wifiPassword = "CHANGE_THIS_PASSWORD";

WebServer server(80);

// -------- DHT11 --------

#define DHT_PIN 0
#define DHT_TYPE DHT11

// -------- BUZZER --------

#define BUZZER_PIN 1

// -------- RGB LED --------

#define RED_PIN 3
#define GREEN_PIN 6
#define BLUE_PIN 7

// -------- PHYSICAL BUTTON --------

#define BUTTON_PIN 10

// -------- FLAME SENSOR --------

#define FLAME_PIN 2
#define FLAME_DETECTED LOW

// -------- LCD --------

#define SDA_PIN 4
#define SCL_PIN 5
#define LCD_ADDRESS 0x27

// -------- RFID RC522 --------

#define RFID_SS 8
#define RFID_RST 20

#define RFID_SCK 9
#define RFID_MISO 19
#define RFID_MOSI 18

// -------- SETTINGS --------

#define TEMP_LIMIT 35.0

const unsigned long SENSOR_INTERVAL = 2000;
const unsigned long AUTH_TIME = 5000;
const unsigned long DENIED_TIME = 3000;

// ==========================================
// OBJECTS
// ==========================================

DHT dht(DHT_PIN, DHT_TYPE);

LiquidCrystal_PCF8574 lcd(LCD_ADDRESS);

MFRC522 rfid(RFID_SS, RFID_RST);

// ==========================================
// VARIABLES
// ==========================================

float temperature = 0;
float humidity = 0;

bool sensorError = true;
bool flameDetected = false;

bool emergencyMode = false;

bool workerAuthenticated = false;
bool accessDenied = false;

String lastUID = "NONE";
String lastRFIDStatus = "WAITING FOR CARD";

unsigned long authTime = 0;
unsigned long deniedTime = 0;

unsigned long lastSensorRead = 0;
unsigned long lastLCDUpdate = 0;
unsigned long lastPrint = 0;

bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;

unsigned long lastDebounceTime = 0;

const unsigned long debounceDelay = 50;

int previousMode = -1;

// ==========================================
// AUTHORIZED CARD UID
// CHANGE THIS TO YOUR ACTUAL CARD UID
// ==========================================

byte authorizedUID[] = {
  0xA9, 0xDB, 0xAB, 0x29
};

byte authorizedUIDLength = 4;

// ==========================================
// MOBILE DASHBOARD HTML
// ==========================================

const char webpage[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>
<head>

<meta name="viewport" content="width=device-width, initial-scale=1">

<title>SmartGuard Dashboard</title>

<style>

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  font-family: Arial, sans-serif;
  background: #101827;
  color: white;
}

header {
  background: #17243a;
  padding: 20px;
  text-align: center;
}

header h2 {
  margin: 0;
  color: #42e89b;
}

header p {
  color: #b8c4d6;
  font-size: 13px;
}

.container {
  padding: 15px;
  max-width: 650px;
  margin: auto;
}

.status {
  padding: 16px;
  border-radius: 12px;
  text-align: center;
  font-size: 20px;
  font-weight: bold;
  margin-bottom: 15px;
  background: #26364e;
}

.grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}

.card {
  background: #1c2b41;
  padding: 18px;
  border-radius: 12px;
  text-align: center;
}

.card h4 {
  color: #b8c4d6;
  margin: 0 0 12px;
}

.value {
  font-size: 27px;
  font-weight: bold;
}

.green {
  color: #39ed91;
}

.red {
  color: #ff5353;
}

.blue {
  color: #54aaff;
}

.yellow {
  color: #ffd65c;
}

.full {
  grid-column: span 2;
}

button {
  width: 100%;
  border: none;
  padding: 18px;
  border-radius: 12px;
  font-size: 17px;
  font-weight: bold;
  color: white;
  margin-top: 12px;
}

.emergency {
  background: #df3030;
}

.reset {
  background: #218c59;
}

.info {
  background: #26364e;
  padding: 15px;
  border-radius: 12px;
  margin-top: 12px;
  word-break: break-word;
}

footer {
  text-align: center;
  color: #8999af;
  padding: 20px;
  font-size: 12px;
}

</style>

</head>

<body>

<header>

<h2>SMARTGUARD</h2>

<p>Intelligent Safety Monitoring System</p>

<p>ESP32-C3 Offline Wi-Fi Dashboard</p>

</header>

<div class="container">

<div class="status" id="mainStatus">
CONNECTING...
</div>

<div class="grid">

<div class="card">

<h4>TEMPERATURE</h4>

<div class="value yellow" id="temp">
-- °C
</div>

</div>

<div class="card">

<h4>HUMIDITY</h4>

<div class="value blue" id="humidity">
-- %
</div>

</div>

<div class="card">

<h4>FLAME SENSOR</h4>

<div class="value green" id="flame">
SAFE
</div>

</div>

<div class="card">

<h4>WORKER RFID</h4>

<div class="value yellow" id="rfid">
WAITING
</div>

</div>

<div class="card full">

<h4>RFID CARD UID</h4>

<div class="value" id="uid">
NONE
</div>

<p id="rfidMessage">Scan your card</p>

</div>

<div class="card full">

<h4>EMERGENCY STATUS</h4>

<div class="value green" id="emergency">
OFF
</div>

</div>

<div class="card full">

<h4>SYSTEM SENSOR STATUS</h4>

<div class="value green" id="sensor">
READY
</div>

</div>

</div>

<button class="emergency" id="emergencyButton"
onclick="toggleEmergency()">

ACTIVATE EMERGENCY

</button>

<button class="reset" id="resetButton"
onclick="resetEmergency()">

RESET EMERGENCY

</button>

<div class="info">

<b>Wi-Fi:</b> SMARTGUARD<br><br>

<b>IP Address:</b> 192.168.4.1<br><br>

<b>Connection:</b> Offline Local Network<br><br>

<b>Update:</b> <span id="connection">Connecting...</span>

</div>

</div>

<footer>

SMARTGUARD | ESP32-C3 SAFETY SYSTEM

</footer>

<script>

let emergencyState = false;

async function updateDashboard() {

  try {

    const response = await fetch('/status');

    const data = await response.json();

    document.getElementById('temp').innerHTML =
      data.temperature + " °C";

    document.getElementById('humidity').innerHTML =
      data.humidity + " %";

    // FLAME STATUS

    const flame = document.getElementById('flame');

    if(data.flame) {

      flame.innerHTML = "FIRE DETECTED";
      flame.className = "value red";

    } else {

      flame.innerHTML = "SAFE";
      flame.className = "value green";

    }

    // RFID STATUS

    const rfid = document.getElementById('rfid');

    rfid.innerHTML = data.rfidStatus;

    rfid.className = data.rfidStatus === "AUTHORIZED"
      ? "value green"
      : data.rfidStatus === "DENIED"
      ? "value red"
      : "value yellow";

    document.getElementById('uid').innerHTML =
      data.uid;

    document.getElementById('rfidMessage').innerHTML =
      data.rfidMessage;

    // EMERGENCY

    emergencyState = data.emergency;

    const emergency =
      document.getElementById('emergency');

    emergency.innerHTML =
      emergencyState ? "ACTIVE" : "OFF";

    emergency.className =
      emergencyState ? "value red" : "value green";

    // SENSOR

    const sensor = document.getElementById('sensor');

    sensor.innerHTML = data.sensorError
      ? "DHT11 ERROR"
      : "READY";

    sensor.className = data.sensorError
      ? "value red"
      : "value green";

    // MAIN STATUS

    document.getElementById('mainStatus').innerHTML =
      data.systemStatus;

    // EMERGENCY BUTTON

    document.getElementById('emergencyButton').style.display =
      emergencyState ? "none" : "block";

    document.getElementById('resetButton').style.display =
      emergencyState ? "block" : "none";

    document.getElementById('connection').innerHTML =
      "CONNECTED";

  }

  catch(error) {

    document.getElementById('connection').innerHTML =
      "CONNECTION LOST";

  }

}

// MOBILE EMERGENCY ON

async function toggleEmergency() {

  await fetch('/emergency?set=1');

  updateDashboard();

}

// MOBILE EMERGENCY OFF

async function resetEmergency() {

  await fetch('/emergency?set=0');

  updateDashboard();

}

// UPDATE EVERY SECOND

setInterval(updateDashboard, 1000);

updateDashboard();

</script>

</body>
</html>

)rawliteral";

// ==========================================
// RGB LED
// ==========================================

void setRGB(bool red, bool green, bool blue) {

  digitalWrite(RED_PIN, red);
  digitalWrite(GREEN_PIN, green);
  digitalWrite(BLUE_PIN, blue);

}

// ==========================================
// BUZZER
// ==========================================

void buzzerOn() {

  digitalWrite(BUZZER_PIN, HIGH);

}

void buzzerOff() {

  digitalWrite(BUZZER_PIN, LOW);

}

// ==========================================
// BUTTON
// ==========================================

void checkButton() {

  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {

    lastDebounceTime = millis();

  }

  if (millis() - lastDebounceTime > debounceDelay) {

    if (reading != stableButtonState) {

      stableButtonState = reading;

      if (stableButtonState == LOW) {

        emergencyMode = !emergencyMode;

        Serial.println(
          emergencyMode
          ? "EMERGENCY ACTIVATED"
          : "EMERGENCY RESET"
        );

      }

    }

  }

  lastButtonReading = reading;

}

// ==========================================
// DHT11 SENSOR
// ==========================================

void readSensor() {

  if (millis() - lastSensorRead >= SENSOR_INTERVAL) {

    lastSensorRead = millis();

    float newTemp = dht.readTemperature();

    float newHumidity = dht.readHumidity();

    if (!isnan(newTemp) && !isnan(newHumidity)) {

      temperature = newTemp;
      humidity = newHumidity;

      sensorError = false;

    }

    else {

      sensorError = true;

      Serial.println("DHT11 ERROR");

    }

  }

}

// ==========================================
// FLAME SENSOR
// ==========================================

void readFlameSensor() {

  flameDetected =
    (digitalRead(FLAME_PIN) == FLAME_DETECTED);

}

// ==========================================
// RFID UID CHECK
// ==========================================

bool checkAuthorizedCard() {

  if (rfid.uid.size != authorizedUIDLength) {

    return false;

  }

  for (byte i = 0; i < authorizedUIDLength; i++) {

    if (rfid.uid.uidByte[i] != authorizedUID[i]) {

      return false;

    }

  }

  return true;

}

// ==========================================
// RFID CARD READING
// ==========================================

void readRFID() {

  if (!rfid.PICC_IsNewCardPresent()) {

    return;

  }

  if (!rfid.PICC_ReadCardSerial()) {

    return;

  }

  // GET UID

  lastUID = "";

  Serial.print("RFID UID: ");

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {

      lastUID += "0";

      Serial.print("0");

    }

    lastUID += String(rfid.uid.uidByte[i], HEX);

    Serial.print(rfid.uid.uidByte[i], HEX);

    if (i < rfid.uid.size - 1) {

      lastUID += " ";

      Serial.print(" ");

    }

  }

  lastUID.toUpperCase();

  Serial.println();

  // CHECK AUTHORIZATION

  if (checkAuthorizedCard()) {

    workerAuthenticated = true;

    accessDenied = false;

    authTime = millis();

    lastRFIDStatus = "AUTHORIZED";

    Serial.println("WORKER AUTHENTICATED");

  }

  else {

    accessDenied = true;

    workerAuthenticated = false;

    deniedTime = millis();

    lastRFIDStatus = "DENIED";

    Serial.println("INVALID RFID");

  }

  rfid.PICC_HaltA();

  rfid.PCD_StopCrypto1();

}

// ==========================================
// RFID TIMER
// ==========================================

void updateRFIDTimer() {

  if (workerAuthenticated &&
      millis() - authTime >= AUTH_TIME) {

    workerAuthenticated = false;

  }

  if (accessDenied &&
      millis() - deniedTime >= DENIED_TIME) {

    accessDenied = false;

  }

}

// ==========================================
// LCD DISPLAY
// ==========================================

void showNormal() {

  lcd.setCursor(0, 0);
  lcd.print("SmartGuard SAFE ");

  lcd.setCursor(0, 1);

  lcd.print("T:");
  lcd.print(temperature, 1);

  lcd.print("C H:");

  lcd.print(humidity, 0);

  lcd.print("%   ");

}

void showFireAlert() {

  lcd.setCursor(0, 0);
  lcd.print("!!! FIRE ALERT !!!");

  lcd.setCursor(0, 1);
  lcd.print("FIRE DETECTED!   ");

}

void showEmergency() {

  lcd.setCursor(0, 0);
  lcd.print("!!! EMERGENCY !!!");

  lcd.setCursor(0, 1);
  lcd.print("ALERT ACTIVATED  ");

}

void showTemperatureAlert() {

  lcd.setCursor(0, 0);
  lcd.print("HIGH TEMPERATURE");

  lcd.setCursor(0, 1);

  lcd.print("Temp: ");
  lcd.print(temperature, 1);
  lcd.print(" C   ");

}

void showSensorError() {

  lcd.setCursor(0, 0);
  lcd.print("DHT11 SENSOR ERR");

  lcd.setCursor(0, 1);
  lcd.print("CHECK CONNECTION");

}

void showWorkerAuthenticated() {

  lcd.setCursor(0, 0);
  lcd.print("WORKER AUTH OK  ");

  lcd.setCursor(0, 1);
  lcd.print("ACCESS GRANTED  ");

}

void showAccessDenied() {

  lcd.setCursor(0, 0);
  lcd.print("ACCESS DENIED   ");

  lcd.setCursor(0, 1);
  lcd.print("INVALID RFID    ");

}

// ==========================================
// SYSTEM STATUS
// ==========================================

String getSystemStatus() {

  if (flameDetected) {

    return "FIRE ALERT";

  }

  if (emergencyMode) {

    return "EMERGENCY ACTIVE";

  }

  if (!sensorError && temperature >= TEMP_LIMIT) {

    return "HIGH TEMPERATURE";

  }

  if (sensorError) {

    return "SENSOR ERROR";

  }

  if (accessDenied) {

    return "ACCESS DENIED";

  }

  if (workerAuthenticated) {

    return "WORKER AUTHENTICATED";

  }

  return "SYSTEM SAFE";

}

// ==========================================
// MOBILE WEB STATUS
// ==========================================

void handleStatus() {

  String json = "{";

  json += "\"temperature\":";
  json += String(temperature, 1);

  json += ",\"humidity\":";
  json += String(humidity, 0);

  json += ",\"flame\":";
  json += flameDetected ? "true" : "false";

  json += ",\"emergency\":";
  json += emergencyMode ? "true" : "false";

  json += ",\"sensorError\":";
  json += sensorError ? "true" : "false";

  json += ",\"uid\":\"";
  json += lastUID;
  json += "\"";

  json += ",\"rfidStatus\":\"";

  if (workerAuthenticated) {

    json += "AUTHORIZED";

  }

  else if (accessDenied) {

    json += "DENIED";

  }

  else {

    json += "WAITING";

  }

  json += "\"";

  json += ",\"rfidMessage\":\"";
  json += lastRFIDStatus;
  json += "\"";

  json += ",\"systemStatus\":\"";
  json += getSystemStatus();
  json += "\"";

  json += "}";

  server.send(200, "application/json", json);

}

// ==========================================
// MOBILE EMERGENCY BUTTON
// ==========================================

void handleEmergency() {

  if (!server.hasArg("set")) {

    server.send(400, "text/plain", "Missing set value");

    return;

  }

  String value = server.arg("set");

  if (value == "1") {

    emergencyMode = true;

    Serial.println("MOBILE EMERGENCY ACTIVATED");

  }

  else if (value == "0") {

    emergencyMode = false;

    Serial.println("MOBILE EMERGENCY RESET");

  }

  else {

    server.send(400, "text/plain", "Invalid value");

    return;

  }

  server.send(200, "text/plain", "OK");

}

// ==========================================
// MOBILE HOME PAGE
// ==========================================

void handleRoot() {

  server.send_P(
    200,
    "text/html",
    webpage
  );

}

// ==========================================
// SETUP
// ==========================================

void setup() {

  Serial.begin(115200);

  // OUTPUT PINS

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // INPUT PINS

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(FLAME_PIN, INPUT);

  // INITIAL OUTPUT

  buzzerOff();

  setRGB(false, false, false);

  // I2C

  Wire.begin(SDA_PIN, SCL_PIN);

  // LCD

  lcd.begin(16, 2);

  lcd.setBacklight(255);

  lcd.clear();

  // DHT

  dht.begin();

  // RFID SPI

  SPI.begin(
    RFID_SCK,
    RFID_MISO,
    RFID_MOSI,
    RFID_SS
  );

  rfid.PCD_Init();

  // WIFI ACCESS POINT

  WiFi.mode(WIFI_AP);

  WiFi.softAP(
    wifiName,
    wifiPassword
  );

  delay(500);

  Serial.println();
  Serial.println("==========================");
  Serial.println("SMARTGUARD STARTED");
  Serial.println("==========================");

  Serial.print("WiFi Name: ");
  Serial.println(wifiName);

  Serial.print("WiFi Password: ");
  Serial.println(wifiPassword);

  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // WEB SERVER ROUTES

  server.on("/", HTTP_GET, handleRoot);

  server.on("/status", HTTP_GET, handleStatus);

  server.on("/emergency", HTTP_GET, handleEmergency);

  server.begin();

  Serial.println("WEB SERVER STARTED");

  // STARTUP LCD

  lcd.setCursor(0, 0);
  lcd.print("SMARTGUARD");

  lcd.setCursor(0, 1);
  lcd.print("WiFi Starting");

  delay(2000);

  lcd.clear();

  lastSensorRead = millis() - SENSOR_INTERVAL;

}

// ==========================================
// MAIN LOOP
// ==========================================

void loop() {

  // HANDLE MOBILE REQUESTS

  server.handleClient();

  // PHYSICAL EMERGENCY BUTTON

  checkButton();

  // READ SENSORS

  readSensor();

  readFlameSensor();

  // RFID CARD

  readRFID();

  // RFID TIMERS

  updateRFIDTimer();

  // SERIAL MONITOR

  if (millis() - lastPrint >= 2000) {

    lastPrint = millis();

    Serial.print("Temperature: ");
    Serial.print(temperature);

    Serial.print(" C | Humidity: ");
    Serial.print(humidity);

    Serial.print(" % | Flame: ");

    Serial.print(flameDetected ? "FIRE" : "SAFE");

    Serial.print(" | Status: ");

    Serial.println(getSystemStatus());

  }

  // SYSTEM CONTROL

  int currentMode;

  // PRIORITY 1: FIRE

  if (flameDetected) {

    currentMode = 5;

    setRGB(true, false, false);

    buzzerOn();

  }

  // PRIORITY 2: EMERGENCY

  else if (emergencyMode) {

    currentMode = 4;

    setRGB(true, false, false);

    buzzerOn();

  }

  // PRIORITY 3: HIGH TEMPERATURE

  else if (!sensorError && temperature >= TEMP_LIMIT) {

    currentMode = 3;

    setRGB(true, false, false);

    buzzerOn();

  }

  // PRIORITY 4: SENSOR ERROR

  else if (sensorError) {

    currentMode = 2;

    setRGB(false, false, true);

    buzzerOff();

  }

  // PRIORITY 5: INVALID RFID

  else if (accessDenied) {

    currentMode = 1;

    setRGB(true, false, false);

    buzzerOn();

  }

  // PRIORITY 6: AUTHENTICATED WORKER

  else if (workerAuthenticated) {

    currentMode = 6;

    setRGB(false, true, false);

    buzzerOff();

  }

  // NORMAL CONDITION

  else {

    currentMode = 0;

    setRGB(false, true, false);

    buzzerOff();

  }

  // LCD UPDATE

  if (currentMode != previousMode ||
      millis() - lastLCDUpdate >= 1000) {

    lcd.clear();

    switch (currentMode) {

      case 5:
        showFireAlert();
        break;

      case 4:
        showEmergency();
        break;

      case 3:
        showTemperatureAlert();
        break;

      case 2:
        showSensorError();
        break;

      case 1:
        showAccessDenied();
        break;

      case 6:
        showWorkerAuthenticated();
        break;

      default:
        showNormal();
        break;

    }

    previousMode = currentMode;

    lastLCDUpdate = millis();

  }

}
