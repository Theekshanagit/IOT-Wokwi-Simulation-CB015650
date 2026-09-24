#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ESP32Servo.h>

// --- Hardware Pin Definitions ---
#define DHTPIN        4
#define DHTTYPE       DHT22
#define LDR_PIN       34
#define SERVO_PIN     13
#define BUTTON_PIN    26

#define LED1_PIN      27
#define LED2_PIN      2
#define LED3_PIN      5

// --- Display Parameters ---
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT dht(DHTPIN, DHTTYPE);
Servo ventServo;

// --- Thresholds & Settings ---
const float TEMP_THRESHOLD_HIGH = 30.0; // Open vent above 30 C
const int LDR_THRESHOLD_LOW    = 1500;  // Turn grow lights on below this light level (0-4095)
const int SERVO_CLOSED_POS    = 0;     // Vent closed angle
const int SERVO_OPEN_POS      = 90;    // Vent open angle

// --- System States ---
enum SystemMode {
  MODE_AUTONOMOUS,
  MODE_MANUAL,
  MODE_SAFETY
};

SystemMode currentMode = MODE_AUTONOMOUS;

// --- Timing Variables (Non-blocking timer logic) ---
unsigned long lastSensorReadTime = 0;
const unsigned long SENSOR_READ_INTERVAL = 2000; // Read sensors every 2 seconds

// Button Debounce Variables
volatile bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 250;

// System Variables
float temperature = 0.0;
float humidity = 0.0;
int ldrValue = 0;
bool ventIsOpen = false;
String faultReason = "";

// --- Interrupt Service Routine (ISR) for Button ---
void IRAM_ATTR handleButtonInterrupt() {
  unsigned long currentTime = millis();
  if (currentTime - lastDebounceTime > DEBOUNCE_DELAY) {
    buttonPressed = true;
    lastDebounceTime = currentTime;
  }
}

void setup() {
  // Initialize Serial Terminal
  Serial.begin(115200);
  Serial.println(F("\n--- Automated Micro-Climate Nursery System ---"));

  // Pin Configurations
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);

  // Set initial LED state to OFF
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);

  // Attach Hardware Interrupt to Pushbutton
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonInterrupt, FALLING);

  // Initialize Servo Motor
  ventServo.attach(SERVO_PIN);
  ventServo.write(SERVO_CLOSED_POS);

  // Initialize DHT Sensor
  dht.begin();

  // Initialize I2C OLED Display
  Wire.begin(21, 22); // SDA = GPIO21, SCL = GPIO22
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("OLED SSD1306 allocation failed!"));
    for (;;); // Stop execution
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println(F("  Nursery System  "));
  display.println(F("   Initializing... "));
  display.display();
  delay(1500);
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. Handle Mode Switch Request via Pushbutton Interrupt
  if (buttonPressed) {
    buttonPressed = false;
    if (currentMode == MODE_AUTONOMOUS) {
      currentMode = MODE_MANUAL;
      Serial.println(F("[MODE CHANGE] Switched to MANUAL OVERRIDE mode."));
    } else if (currentMode == MODE_MANUAL) {
      currentMode = MODE_AUTONOMOUS;
      Serial.println(F("[MODE CHANGE] Switched to AUTONOMOUS mode."));
    }
  }

  // 2. Non-blocking Sensor Update & State Logic
  if (currentMillis - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
    lastSensorReadTime = currentMillis;

    // Read Sensors
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    ldrValue = analogRead(LDR_PIN); // Returns 0 - 4095

    // Safety Fault Detection
    if (isnan(temperature) || isnan(humidity)) {
      currentMode = MODE_SAFETY;
      faultReason = "DHT22 Sensor Error";
    } else if (ldrValue < 0 || ldrValue > 4095) {
      currentMode = MODE_SAFETY;
      faultReason = "LDR Sensor Error";
    }

    // 3. Execute Mode-Specific State Machine Logic
    switch (currentMode) {
      case MODE_AUTONOMOUS:
        runAutonomousMode();
        break;

      case MODE_MANUAL:
        runManualMode();
        break;

      case MODE_SAFETY:
        runSafetyMode();
        break;
    }

    // 4. Output Status to Serial & Update OLED
    logToSerial();
    updateDisplay();
  }
}

// --- Mode Logic Implementations ---

void runAutonomousMode() {
  // Sunlight / Grow Light Logic
  if (ldrValue < LDR_THRESHOLD_LOW) {
    digitalWrite(LED1_PIN, HIGH);
    digitalWrite(LED2_PIN, HIGH);
    digitalWrite(LED3_PIN, HIGH);
  } else {
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    digitalWrite(LED3_PIN, LOW);
  }

  // Vent Control Logic based on Temperature
  if (temperature > TEMP_THRESHOLD_HIGH) {
    ventServo.write(SERVO_OPEN_POS);
    ventIsOpen = true;
  } else {
    ventServo.write(SERVO_CLOSED_POS);
    ventIsOpen = false;
  }
}

void runManualMode() {
  // Bypass automation and force vent fully open for maintenance/sanitation
  ventServo.write(SERVO_OPEN_POS);
  ventIsOpen = true;

  // Turn off supplemental grow lights in manual override
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
}

void runSafetyMode() {
  // Revert to predefined safe posture: Open vents fully to protect crops from heat
  ventServo.write(SERVO_OPEN_POS);
  ventIsOpen = true;

  // Flash warning LEDs
  digitalWrite(LED1_PIN, !digitalRead(LED1_PIN));
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
}

// --- Serial Logging ---

void logToSerial() {
  Serial.print(F("Mode: "));
  switch (currentMode) {
    case MODE_AUTONOMOUS: Serial.print(F("AUTONOMOUS")); break;
    case MODE_MANUAL:     Serial.print(F("MANUAL OVERRIDE")); break;
    case MODE_SAFETY:     Serial.print(F("SAFETY/FAULT")); break;
  }
  
  Serial.print(F(" | Temp: ")); Serial.print(temperature, 1); Serial.print(F(" C"));
  Serial.print(F(" | Hum: ")); Serial.print(humidity, 1); Serial.print(F(" %"));
  Serial.print(F(" | Light: ")); Serial.print(ldrValue);
  Serial.print(F(" | Vent: ")); Serial.println(ventIsOpen ? F("OPEN") : F("CLOSED"));
}

// --- OLED Update ---

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);

  if (currentMode == MODE_SAFETY) {
    display.setCursor(0, 0);
    display.println(F("!!! SAFETY FAULT !!!"));
    display.println(F("--------------------"));
    display.println(faultReason);
    display.println(F("Posture: Vent OPEN"));
    display.println(F("Check hardware!"));
  } else {
    // Mode Header
    display.setCursor(0, 0);
    if (currentMode == MODE_MANUAL) {
      display.println(F("MODE: MANUAL OVERRIDE"));
    } else {
      display.println(F("MODE: AUTONOMOUS"));
    }
    display.println(F("--------------------"));

    // Sensor Readings
    display.print(F("Temp: ")); display.print(temperature, 1); display.println(F(" C"));
    display.print(F("Hum : ")); display.print(humidity, 1); display.println(F(" %"));
    display.print(F("LDR : ")); display.println(ldrValue);

    // Vent Status
    display.print(F("Vent: "));
    display.println(ventIsOpen ? F("OPEN") : F("CLOSED"));
  }

  display.display();
}