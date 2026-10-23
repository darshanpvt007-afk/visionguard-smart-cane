// VisionGuard Smart Cane - obstacle + wet-surface warning
// Board: Arduino Nano (ATmega328P)
//
// PROVISIONAL: based on the original sketch in
// firmware/original/visionguard_original. Same pins, same thresholds,
// same alert priority. Pin numbers are taken from that sketch and still
// have to be confirmed on the real wiring (see docs/REVIEW1_PREP.md).
//
// Changes from the original:
//   1. pulseIn() has a 30 ms timeout, so a missed echo no longer freezes
//      the loop for 1 second (the default timeout).
//   2. Distance is clamped before map(), so the beep gap always stays
//      between 50 ms and 400 ms.
//   3. Serial Monitor prints the alert state, which helps during the demo.

// Pin Configurations
#define TRIG_PIN 5
#define ECHO_PIN 6
#define SOIL_PIN A0
#define BUZZER_PIN 2   // Active buzzer (beeps when the pin is HIGH)
#define VIB_PIN 3      // Vibration output - no motor confirmed in the hardware yet.
                       // Never connect a motor straight to this pin; use the
                       // IRLZ44N MOSFET + flyback diode as a driver.

// Calibration Thresholds
const int OBSTACLE_DISTANCE_THRESHOLD = 50; // Distance in cm
const int MIN_DISTANCE = 2;                 // HC-SR04 cannot measure closer than ~2 cm
const int WET_SOIL_THRESHOLD = 500;         // Adjust based on sensor readings (Lower = Wetter)
const unsigned long ECHO_TIMEOUT_US = 30000; // ~5 m round trip; longer means "nothing in range"

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(VIB_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(VIB_PIN, LOW);
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

void alertsOff() {
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(VIB_PIN, LOW);
}

void loop() {
  // 1. Read Distance from Ultrasonic Sensor
  int distance = readDistanceCm();

  // 2. Read Soil Moisture Level (dry ~1023, wet = lower)
  int soilMoisture = analogRead(SOIL_PIN);

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm | Soil Value: ");
  Serial.print(soilMoisture);

  // 3. Alert Logic
  if (soilMoisture < WET_SOIL_THRESHOLD) {
    // Priority 1: Water/Wet Mud Detected - Continuous Warning
    Serial.println(" | ALERT: WATER");
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(VIB_PIN, HIGH);
  }
  else if (distance > 0 && distance < OBSTACLE_DISTANCE_THRESHOLD) {
    // Priority 2: Obstacle Detected - closer obstacle = faster beeping
    Serial.println(" | ALERT: OBSTACLE");
    int clamped = constrain(distance, MIN_DISTANCE, OBSTACLE_DISTANCE_THRESHOLD);
    int delayTime = map(clamped, MIN_DISTANCE, OBSTACLE_DISTANCE_THRESHOLD, 50, 400);

    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(VIB_PIN, HIGH);
    delay(50); // Sharp beep
    alertsOff();
    delay(delayTime); // Dynamic wait interval
  }
  else {
    // Path Clear
    Serial.println(" | CLEAR");
    alertsOff();
  }

  delay(30);
}
