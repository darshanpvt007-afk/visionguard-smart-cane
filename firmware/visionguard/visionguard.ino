// VisionGuard Smart Cane - obstacle + wet-surface warning
// Board: Arduino Nano (ATmega328P), new bootloader -> Processor "ATmega328P"
// Wiring: Trig A0, Echo A1, moisture A2, buzzer D13
// Team: S Darshan (25BCE2883), Tanay Agarwal (25BCE2475)
//
// What it does:
//   1. Water at the tip   -> continuous beep   (highest priority)
//   2. Obstacle < 50 cm   -> short beeps, faster when closer
//   3. Otherwise          -> silent
//
// Calibration: open Serial Monitor at 9600, note "Soil Value" dry and wet,
// then set WET_ON_THRESHOLD between them (see comment below).

// Pin Configurations (matches the actual wiring on the cane)
#define TRIG_PIN A0    // HC-SR04 Trig  (orange wire) - analog pins work as digital pins too
#define ECHO_PIN A1    // HC-SR04 Echo  (yellow wire)
#define SOIL_PIN A2    // LM393 module A0 output (grey wire)
#define BUZZER_PIN 13  // Active buzzer, soldered on D13 (minus leg to GND)
#define VIB_PIN 3      // Reserved for a vibration motor - connect it through
                       // the IRLZ44N MOSFET with a flyback diode, never directly.

// Settings
const int OBSTACLE_DISTANCE_THRESHOLD = 50;   // cm
const int MIN_DISTANCE = 2;                   // HC-SR04 cannot measure closer than ~2 cm
const unsigned long ECHO_TIMEOUT_US = 30000;  // ~5 m round trip; longer means "nothing in range"

// Water thresholds (dry ~1023, wet = lower).
// Set WET_ON_THRESHOLD about halfway between your dry and wet readings.
// WET_OFF_THRESHOLD is a little higher so the alarm does not flicker (hysteresis).
const int WET_ON_THRESHOLD = 500;
const int WET_OFF_THRESHOLD = 550;
const int MOISTURE_SAMPLES = 4;               // readings averaged to reduce noise

bool isWet = false;

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(VIB_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(VIB_PIN, LOW);

  Serial.println("VisionGuard ready");
}

// Returns distance in cm, or 0 if no echo came back in time
int readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  // Sound travels 0.034 cm/us; divide by 2 because the pulse goes out and back
  return duration * 0.034 / 2;
}

// Average a few ADC readings so one noisy value cannot trigger the alarm
int readMoisture() {
  long total = 0;
  for (int i = 0; i < MOISTURE_SAMPLES; i++) {
    total += analogRead(SOIL_PIN);
  }
  return total / MOISTURE_SAMPLES;
}

void alertsOn() {
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(VIB_PIN, HIGH);
}

void alertsOff() {
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(VIB_PIN, LOW);
}

void loop() {
  // 1. Read sensors
  int distance = readDistanceCm();
  int soilMoisture = readMoisture();

  // 2. Water detection with hysteresis
  if (soilMoisture < WET_ON_THRESHOLD) {
    isWet = true;
  } else if (soilMoisture > WET_OFF_THRESHOLD) {
    isWet = false;
  }

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm | Soil Value: ");
  Serial.print(soilMoisture);

  // 3. Alert logic - water is checked first, so it always has priority
  if (isWet) {
    // Priority 1: water / wet mud - continuous warning
    Serial.println(" | ALERT: WATER");
    alertsOn();
  }
  else if (distance > 0 && distance < OBSTACLE_DISTANCE_THRESHOLD) {
    // Priority 2: obstacle - closer obstacle = shorter gap = faster beeping
    Serial.println(" | ALERT: OBSTACLE");
    int clamped = constrain(distance, MIN_DISTANCE, OBSTACLE_DISTANCE_THRESHOLD);
    int delayTime = map(clamped, MIN_DISTANCE, OBSTACLE_DISTANCE_THRESHOLD, 50, 400);

    alertsOn();
    delay(50);        // Sharp beep
    alertsOff();
    delay(delayTime); // Gap: 50 ms (very close) to 400 ms (at 50 cm)
  }
  else {
    // Path clear
    Serial.println(" | CLEAR");
    alertsOff();
  }

  delay(30);
}
