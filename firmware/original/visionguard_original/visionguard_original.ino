// Pin Configurations
#define TRIG_PIN 5
#define ECHO_PIN 6
#define SOIL_PIN A0
#define BUZZER_PIN 2
#define VIB_PIN 3

// Calibration Thresholds
const int OBSTACLE_DISTANCE_THRESHOLD = 50; // Distance in cm
const int WET_SOIL_THRESHOLD = 500;         // Adjust based on sensor readings (Lower = Wetter)

void setup() {
  Serial.begin(9600);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(VIB_PIN, OUTPUT);
}

void loop() {
  // 1. Read Distance from Ultrasonic Sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = duration * 0.034 / 2; // Calculate distance in cm
  
  // 2. Read Soil Moisture Level
  int soilMoisture = analogRead(SOIL_PIN);
  
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm | Soil Value: ");
  Serial.println(soilMoisture);

  // 3. Alert Logic
  if (soilMoisture < WET_SOIL_THRESHOLD) {
    // Priority 1: Water/Wet Mud Detected - Continuous Warning
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(VIB_PIN, HIGH);
  } 
  else if (distance > 0 && distance < OBSTACLE_DISTANCE_THRESHOLD) {
    // Priority 2: Obstacle Detected - Graded Beeping Frequency
    int delayTime = map(distance, 2, OBSTACLE_DISTANCE_THRESHOLD, 50, 400); 
    
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(VIB_PIN, HIGH);
    delay(50); // Sharp beep
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(VIB_PIN, LOW);
    delay(delayTime); // Dynamic wait interval
  } 
  else {
    // Path Clear
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(VIB_PIN, LOW);
  }
  
  delay(30); 
}
