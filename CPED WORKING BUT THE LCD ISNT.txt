/*
  Quake Guard: An Automated Arduino-Based Earthquake Detector for School Grounds
  Board: Arduino Uno R3

  Libraries (Library Manager):
    - LiquidCrystal I2C (Frank de Brabander)
    - Adafruit MPU6050
    - Adafruit Unified Sensor
    - Adafruit BusIO

  Wiring:
    Green LED  -> D2     Yellow LED -> D3     Red LED -> D4
    Buzzer (+) -> D8
    Reset button (optional) -> D7 and GND (uses internal pull-up)
    MPU-6050 + LCD I2C -> SDA (A4), SCL (A5), 5V, GND

  Earthquake rules:
    1) STRONG: vibration >= THRESHOLD_RED, confirmed for ~0.5 s -> instant earthquake (red)
    2) SUSTAINED: vibration >= THRESHOLD_YELLOW lasting 3 seconds -> earthquake (red)
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ---------------- Hardware ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);   // try 0x3F if the LCD stays blank
Adafruit_MPU6050 mpu;

const uint8_t PIN_GREEN  = 2;
const uint8_t PIN_YELLOW = 3;
const uint8_t PIN_RED    = 4;
const uint8_t PIN_BUZZER = 8;
const uint8_t PIN_RESET  = 7;         // optional manual reset button

// ---------------- Tunable settings ----------------
const float THRESHOLD_YELLOW = 1.5;   // m/s^2  moderate shaking (starts the 3 s timer)
const float THRESHOLD_RED    = 3.0;   // m/s^2  strong tremor (instant earthquake)

const unsigned long SUSTAINED_SHAKE_MS = 3000;  // yellow-level shaking this long = earthquake
const unsigned long SHAKE_GAP_MS       = 600;   // short pauses in shaking are forgiven
const uint8_t       RED_CONFIRM        = 2;     // consecutive strong windows (~0.5 s)

const unsigned long SAMPLE_MS      = 10;    // 100 samples per second
const unsigned long WINDOW_MS      = 250;   // peak is evaluated every 250 ms
const unsigned long ALARM_HOLD_MS  = 5000;  // alarm stays on this long after shaking stops
const unsigned long YELLOW_HOLD_MS = 1500;  // caution stays on briefly so it is readable
const unsigned long LCD_REFRESH_MS = 250;
const unsigned long SERIAL_MS      = 50;

const int CALIBRATION_SAMPLES = 200;  // ~2 seconds, keep the unit still

// ---------------- State ----------------
enum State : uint8_t { STATE_GREEN, STATE_YELLOW, STATE_RED };
State state = STATE_GREEN;
bool forceRedraw = true;

float baseline = 9.8;                 // measured at startup (gravity as seen by the sensor)
float latestVib = 0;
float windowPeak = 0;
float alarmPeak = 0;                  // strongest reading during the current alarm
uint8_t redStreak = 0;
uint16_t alertCount = 0;

bool shaking = false;                 // currently in a continuous shake
unsigned long shakeStart = 0;
unsigned long lastShakeSeen = 0;

bool alarmActive = false;
unsigned long alarmUntil = 0;
bool yellowActive = false;
unsigned long yellowUntil = 0;

unsigned long lastSample = 0, windowStart = 0, lastLcd = 0, lastSerial = 0;
uint8_t lastPhase = 255;

// ---------------- Helpers ----------------
void setLeds(bool g, bool y, bool r) {
  digitalWrite(PIN_GREEN, g);
  digitalWrite(PIN_YELLOW, y);
  digitalWrite(PIN_RED, r);
}

float readAcceleration() {
  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  return sqrt(sq(a.acceleration.x) + sq(a.acceleration.y) + sq(a.acceleration.z));
}

void lcdLine(uint8_t row, const __FlashStringHelper *text) {
  lcd.setCursor(0, row);
  lcd.print(text);
}

void fatalError(const __FlashStringHelper *line2) {
  lcd.clear();
  lcdLine(0, F("MPU6050 Failed! "));
  lcdLine(1, line2);
  while (true) {                      // blink red so the fault is obvious
    digitalWrite(PIN_RED, !digitalRead(PIN_RED));
    delay(300);
  }
}

void selfTest() {
  lcdLine(0, F("Quake Guard     "));
  lcdLine(1, F("Self-test...    "));
  setLeds(HIGH, HIGH, HIGH);
  tone(PIN_BUZZER, 1200, 200);
  delay(600);
  setLeds(LOW, LOW, LOW);
  noTone(PIN_BUZZER);
}

void calibrate() {
  lcdLine(0, F("Calibrating...  "));
  lcdLine(1, F("Keep unit still "));
  float sum = 0;
  for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
    sum += readAcceleration();
    delay(10);
  }
  baseline = sum / CALIBRATION_SAMPLES;
  Serial.print(F("Baseline (m/s^2): "));
  Serial.println(baseline);
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);

  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_RESET, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();

  if (!mpu.begin()) {
    fatalError(F("Check wiring    "));
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);   // smooths electrical noise

  selfTest();
  calibrate();

  windowStart = millis();
  lcd.clear();
}

// ---------------- Detection ----------------
void sampleSensor(unsigned long now) {
  float total = readAcceleration();
  latestVib = fabs(total - baseline);
  if (latestVib > windowPeak) windowPeak = latestVib;
  if (alarmActive && latestVib > alarmPeak) alarmPeak = latestVib;

  // Slowly follow drift (temperature, tilt) but only while everything is calm
  if (state == STATE_GREEN && latestVib < THRESHOLD_YELLOW * 0.5) {
    baseline = baseline * 0.999 + total * 0.001;
  }
}

void evaluateWindow(unsigned long now) {
  float peak = windowPeak;
  windowPeak = 0;
  windowStart = now;

  // --- Rule 1: STRONG shaking (confirmed over a couple of windows so one knock won't trigger) ---
  if (peak >= THRESHOLD_RED) {
    if (redStreak < 255) redStreak++;
  } else {
    redStreak = 0;
  }

  // --- Rule 2: SUSTAINED shaking above the yellow threshold ---
  if (peak >= THRESHOLD_YELLOW) {
    if (!shaking) {
      shaking = true;
      shakeStart = now;
    }
    lastShakeSeen = now;
    yellowActive = true;
    yellowUntil = now + YELLOW_HOLD_MS;
  } else if (shaking && now - lastShakeSeen > SHAKE_GAP_MS) {
    shaking = false;                      // shaking stopped, reset the 3 s timer
  }

  bool strongQuake    = (redStreak >= RED_CONFIRM);
  bool sustainedQuake = shaking && (now - shakeStart >= SUSTAINED_SHAKE_MS);

  if (strongQuake || sustainedQuake) {
    if (!alarmActive) {
      alarmActive = true;
      alarmPeak = peak;
      alertCount++;
      Serial.print(F("ALERT #"));
      Serial.print(alertCount);
      Serial.println(strongQuake ? F(" (strong tremor)") : F(" (sustained shaking)"));
    }
    alarmUntil = now + ALARM_HOLD_MS;     // stays on while shaking continues
  }
}

State computeState(unsigned long now) {
  if (alarmActive && (long)(alarmUntil - now) <= 0) alarmActive = false;
  if (yellowActive && (long)(yellowUntil - now) <= 0) yellowActive = false;

  if (alarmActive)  return STATE_RED;
  if (yellowActive) return STATE_YELLOW;
  return STATE_GREEN;
}

// ---------------- Outputs ----------------
void updateOutputs(unsigned long now) {
  State newState = computeState(now);
  if (newState != state) {
    state = newState;
    forceRedraw = true;
    lastPhase = 255;
  }

  switch (state) {
    case STATE_GREEN:
      setLeds(HIGH, LOW, LOW);
      noTone(PIN_BUZZER);
      if (forceRedraw) {
        lcdLine(0, F("Status: NORMAL  "));
      }
      break;

    case STATE_YELLOW:
      setLeds(LOW, HIGH, LOW);
      noTone(PIN_BUZZER);
      if (forceRedraw) {
        lcdLine(0, F("Caution: SHAKE  "));
      }
      break;

    case STATE_RED: {
      uint8_t phase = (now / 250) & 1;        // 2 Hz flashing and two-tone siren
      if (phase != lastPhase) {
        lastPhase = phase;
        digitalWrite(PIN_RED, phase);
        digitalWrite(PIN_GREEN, LOW);
        digitalWrite(PIN_YELLOW, LOW);
        tone(PIN_BUZZER, phase ? 1500 : 1000);
      }
      if (forceRedraw) {
        lcdLine(0, F("*EARTHQUAKE!*   "));
      }
      break;
    }
  }

  // LCD second line refreshes on a timer so it stays steady and readable
  if (forceRedraw || now - lastLcd >= LCD_REFRESH_MS) {
    lastLcd = now;
    lcd.setCursor(0, 1);

    if (state == STATE_RED) {
      if ((now / 1000) & 1) {
        lcd.print(F("Peak:"));
        lcd.print(alarmPeak, 2);
        lcd.print(F(" m/s2  "));
      } else {
        lcd.print(F("EVACUATE NOW!   "));
      }
    } else if (state == STATE_YELLOW && shaking) {
      // Live countdown toward the 3-second rule
      lcd.print(F("Shake:"));
      lcd.print((now - shakeStart) / 1000.0, 1);
      lcd.print(F("s/"));
      lcd.print((int)(SUSTAINED_SHAKE_MS / 1000));
      lcd.print(F("s   "));
    } else {
      lcd.print(F("Vib:"));
      lcd.print(latestVib, 2);
      lcd.print(F(" m/s2   "));
    }
    forceRedraw = false;
  }
}

// ---------------- Main loop (no blocking delay) ----------------
void loop() {
  unsigned long now = millis();

  if (now - lastSample >= SAMPLE_MS) {
    lastSample = now;
    sampleSensor(now);
  }

  if (now - windowStart >= WINDOW_MS) {
    evaluateWindow(now);
  }

  // Manual reset: silence and clear a latched alarm
  if (digitalRead(PIN_RESET) == LOW && alarmActive) {
    alarmActive = false;
    redStreak = 0;
    shaking = false;
    forceRedraw = true;
  }

  updateOutputs(now);

  // Serial Plotter friendly output for tuning thresholds
  if (now - lastSerial >= SERIAL_MS) {
    lastSerial = now;
    Serial.print(F("Vib:"));     Serial.print(latestVib, 2);
    Serial.print(F(",Yellow:")); Serial.print(THRESHOLD_YELLOW, 1);
    Serial.print(F(",Red:"));    Serial.println(THRESHOLD_RED, 1);
  }
}