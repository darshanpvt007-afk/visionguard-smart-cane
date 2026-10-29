// VisionGuard Smart Cane - obstacle + wet-surface warning
// Board: Arduino Nano (ATmega328P), new bootloader -> Processor "ATmega328P"
// Wiring: Trig A0, Echo A1, moisture A2, buzzer D13
// Team: S Darshan (25BCE2883), Tanay Agarwal (25BCE2475)
//
// What it does:
//   1. Water at the tip   -> continuous beep   (highest priority)
//   2. Obstacle < 50 cm   -> short beeps, faster when closer
//   3. Otherwise          -> silent (tiny "I'm on" chirp every 30 s)
//
// Extra features:
//   - Start-up self-test with a "ready" chime and a report on the Serial Monitor
//   - Low-battery warning: the chip measures its own supply voltage using the
//     internal 1.1 V reference (the battery feeds the 5V pin directly)
//   - Median filter on distance to remove single wrong echoes
//   - Non-blocking timing (millis) so water is detected even during a beep
//
// Calibration: open Serial Monitor at 9600, note "Soil Value" dry and wet,
// then set WET_ON_THRESHOLD between them.

// Pin Configurations (matches the actual wiring on the cane)
#define TRIG_PIN A0    // HC-SR04 Trig  (orange wire) - analog pins work as digital pins too
#define ECHO_PIN A1    // HC-SR04 Echo  (yellow wire)
#define SOIL_PIN A2    // LM393 module A0 output (grey wire)
#define BUZZER_PIN 13  // Active buzzer, soldered on D13 (on-board "L" LED blinks with it)
#define VIB_PIN 3      // Reserved for a vibration motor - connect it through
                       // the IRLZ44N MOSFET with a flyback diode, never directly.

// Obstacle settings
const int OBSTACLE_DISTANCE_THRESHOLD = 50;   // cm
const int MIN_DISTANCE = 2;                   // HC-SR04 cannot measure closer than ~2 cm
const unsigned long ECHO_TIMEOUT_US = 30000;  // ~5 m round trip; longer means "nothing in range"
const unsigned long PING_INTERVAL_MS = 60;    // datasheet: at least 60 ms between pings
const int NO_ECHO_CM = 999;                   // used when no echo comes back

// Water settings (dry ~1023, wet = lower).
// Set WET_ON_THRESHOLD about halfway between your dry and wet readings.
// WET_OFF_THRESHOLD is a little higher so the alarm does not flicker (hysteresis).
const int WET_ON_THRESHOLD = 500;
const int WET_OFF_THRESHOLD = 550;
const int MOISTURE_SAMPLES = 4;               // readings averaged to reduce noise

// Battery settings (millivolts). One 18650 cell: 4.2 V full, ~3.0 V empty.
const long LOW_BATTERY_MV = 3400;             // start warning below this
const long BATTERY_OK_MV = 3600;              // stop warning above this
const unsigned long BATTERY_CHECK_MS = 10000; // check every 10 s
const unsigned long BATTERY_WARN_MS = 20000;  // warning chirps every 20 s

// Heartbeat: short chirp so a blind user knows the cane is still on
const bool HEARTBEAT_ENABLED = true;
const unsigned long HEARTBEAT_MS = 30000;

// Beep timing
const unsigned long BEEP_ON_MS = 50;          // obstacle beep length
const unsigned long CHIRP_ON_MS = 30;         // heartbeat / battery chirp length
const unsigned long CHIRP_OFF_MS = 120;       // gap between chirps

enum State { CLEAR, OBSTACLE, WATER };

int distanceHistory[3] = {NO_ECHO_CM, NO_ECHO_CM, NO_ECHO_CM};
int historyIndex = 0;
int distance = NO_ECHO_CM;
int soilMoisture = 1023;
bool isWet = false;
bool lowBattery = false;
long batteryMv = 0;

bool buzzerOn = false;
unsigned long lastBuzzerChange = 0;
int chirpsLeft = 0;

unsigned long lastPing = 0;
unsigned long lastBatteryCheck = 0;
unsigned long lastBatteryWarn = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastPrint = 0;

// ---------------- Sensors ----------------

// Returns distance in cm, or NO_ECHO_CM if no echo came back in time
int readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return NO_ECHO_CM;
  // Sound travels 0.034 cm/us; divide by 2 because the pulse goes out and back
  return duration * 0.034 / 2;
}

// Middle value of the last 3 readings: one wrong echo cannot change the result
int medianOf3(int a, int b, int c) {
  if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
  if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
  return c;
}

// Average a few ADC readings so one noisy value cannot trigger the alarm
int readMoisture() {
  long total = 0;
  for (int i = 0; i < MOISTURE_SAMPLES; i++) {
    total += analogRead(SOIL_PIN);
  }
  return total / MOISTURE_SAMPLES;
}

// Measures the chip's own supply voltage (= battery voltage, since the cell
// feeds the 5V pin). Trick: use the supply as the ADC reference and measure
// the internal 1.1 V bandgap. Vcc = 1.1 V x 1023 / reading.
long readSupplyMv() {
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);  // ref = AVcc, input = 1.1 V bandgap
  delay(2);                                               // let the reference settle
  ADCSRA |= _BV(ADSC);                                    // start conversion
  while (bit_is_set(ADCSRA, ADSC)) {}                     // wait until done
  int low = ADCL;                                         // read ADCL first, then ADCH
  int high = ADCH;
  long reading = (high << 8) | low;
  if (reading == 0) return 0;
  return 1125300L / reading;                              // 1.1 x 1023 x 1000
}

// ---------------- Buzzer ----------------

void setBuzzer(bool on, unsigned long now) {
  buzzerOn = on;
  lastBuzzerChange = now;
  digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
  digitalWrite(VIB_PIN, on ? HIGH : LOW);
}

// Blocking beep, used only during start-up
void beep(int onMs, int offMs) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(onMs);
  digitalWrite(BUZZER_PIN, LOW);
  delay(offMs);
}

// ---------------- Start-up self-test ----------------

void selfTest() {
  Serial.println("=== VisionGuard self-test ===");

  int echoes = 0;
  for (int i = 0; i < 5; i++) {
    if (readDistanceCm() != NO_ECHO_CM) echoes++;
    delay(PING_INTERVAL_MS);
  }
  Serial.print("Ultrasonic: ");
  Serial.println(echoes > 0 ? "OK" : "no echo - check wiring or point at a wall");

  Serial.print("Moisture reading: ");
  Serial.println(readMoisture());

  batteryMv = readSupplyMv();
  Serial.print("Supply voltage: ");
  Serial.print(batteryMv);
  Serial.println(" mV");

  if (echoes == 0) {
    beep(500, 200);  // one long beep = sensor problem
  } else {
    beep(60, 80);    // "ready" chime: three rising-speed beeps
    beep(60, 60);
    beep(120, 0);
  }
  Serial.println("=== Ready ===");
}

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(VIB_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(VIB_PIN, LOW);

  selfTest();

  unsigned long now = millis();
  lastHeartbeat = now;
  lastBatteryCheck = now;
  lastBatteryWarn = now;
}

// ---------------- Main loop (non-blocking) ----------------

void loop() {
  unsigned long now = millis();

  // 1. Distance: one ping every 60 ms, median of the last 3
  if (now - lastPing >= PING_INTERVAL_MS) {
    lastPing = now;
    distanceHistory[historyIndex] = readDistanceCm();
    historyIndex = (historyIndex + 1) % 3;
    distance = medianOf3(distanceHistory[0], distanceHistory[1], distanceHistory[2]);
  }

  // 2. Water, with hysteresis - read every loop so it reacts immediately
  soilMoisture = readMoisture();
  if (soilMoisture < WET_ON_THRESHOLD) {
    isWet = true;
  } else if (soilMoisture > WET_OFF_THRESHOLD) {
    isWet = false;
  }

  // 3. Battery, every 10 s, with hysteresis
  if (now - lastBatteryCheck >= BATTERY_CHECK_MS) {
    lastBatteryCheck = now;
    batteryMv = readSupplyMv();
    if (batteryMv < LOW_BATTERY_MV) lowBattery = true;
    else if (batteryMv > BATTERY_OK_MV) lowBattery = false;
  }

  // 4. Decide the state - water first, so it always has priority
  State state = CLEAR;
  if (isWet) {
    state = WATER;
  } else if (distance >= MIN_DISTANCE && distance < OBSTACLE_DISTANCE_THRESHOLD) {
    state = OBSTACLE;
  }

  // 5. Drive the buzzer without delay()
  if (state == WATER) {
    chirpsLeft = 0;
    if (!buzzerOn) setBuzzer(true, now);              // continuous tone
  }
  else if (state == OBSTACLE) {
    chirpsLeft = 0;
    int clamped = constrain(distance, MIN_DISTANCE, OBSTACLE_DISTANCE_THRESHOLD);
    unsigned long gap = map(clamped, MIN_DISTANCE, OBSTACLE_DISTANCE_THRESHOLD, 50, 400);
    if (buzzerOn && now - lastBuzzerChange >= BEEP_ON_MS) {
      setBuzzer(false, now);                           // end of beep
    } else if (!buzzerOn && now - lastBuzzerChange >= gap) {
      setBuzzer(true, now);                            // start next beep
    }
  }
  else {
    // Path clear: queue chirps for low battery (3) or heartbeat (1)
    if (lowBattery && chirpsLeft == 0 && now - lastBatteryWarn >= BATTERY_WARN_MS) {
      lastBatteryWarn = now;
      chirpsLeft = 3;
    } else if (HEARTBEAT_ENABLED && chirpsLeft == 0 && now - lastHeartbeat >= HEARTBEAT_MS) {
      lastHeartbeat = now;
      chirpsLeft = 1;
    }

    if (buzzerOn && now - lastBuzzerChange >= CHIRP_ON_MS) {
      setBuzzer(false, now);
      if (chirpsLeft > 0) chirpsLeft--;
    } else if (!buzzerOn && chirpsLeft > 0 && now - lastBuzzerChange >= CHIRP_OFF_MS) {
      setBuzzer(true, now);
    }
  }

  // 6. Status on the Serial Monitor, 5 times a second
  if (now - lastPrint >= 200) {
    lastPrint = now;
    Serial.print("Distance: ");
    if (distance == NO_ECHO_CM) Serial.print("--");
    else Serial.print(distance);
    Serial.print(" cm | Soil Value: ");
    Serial.print(soilMoisture);
    Serial.print(" | Battery: ");
    Serial.print(batteryMv);
    Serial.print(" mV | ");
    if (state == WATER) Serial.print("ALERT: WATER");
    else if (state == OBSTACLE) Serial.print("ALERT: OBSTACLE");
    else Serial.print("CLEAR");
    if (lowBattery) Serial.print(" | LOW BATTERY");
    Serial.println();
  }
}
