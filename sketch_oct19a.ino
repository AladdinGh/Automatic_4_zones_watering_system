// === Three-Zone Sequential Irrigation System ===
// Each zone: 1 pump relay + 1 moisture sensor

// === Pin Configuration ===
const int number_zones = 4; 
const int moistureThresholds[number_zones] = {1500, 1500, 1500, 1500};
const int pumpPins[number_zones] = {13, 27, 12, 14};
const int moisturePins[number_zones] = {33, 32, 35, 35}; // Zone 3 & 4 share pin 35

// === Threshold and Timing Settings ===
const unsigned long checkInterval = 1000;
const unsigned long maxRunTime = 2000;   // 10 seconds
const unsigned long irrigationInterval = 1000; // to change to every 3/4 days

// === Timing Variables ===
unsigned long lastIrrigationTime = 0;

void setup() {
  Serial.begin(115200);


  // Initialize pumps
  for (int i = 0; i < number_zones; i++) {
    pinMode(pumpPins[i], OUTPUT);
    digitalWrite(pumpPins[i], HIGH);
  }
}

// === Smart Moisture Reading ===
int readMoisture(int zoneIndex) {
  // Zone 4 uses Zone 3's sensor (both on pin 35)
  if (zoneIndex == 3) {
    return analogRead(moisturePins[2]); // Read from Zone 3's pin
  }
  return analogRead(moisturePins[zoneIndex]);
}

// === Print All Moisture Readings ===
void DispalyAllMoisture() {
  Serial.print("Moisture: ");
  for (int i = 0; i < number_zones; i++) {
    int value = readMoisture(i);
    Serial.print("[Zone");
    Serial.print(i + 1);
    Serial.print(":Pin");
    Serial.print(moisturePins[i]);
    Serial.print("=");
    Serial.print(value);
    if (i == 3) Serial.print("(SHARED)"); // Mark Zone 4 as shared
    Serial.print("] ");
  }
  Serial.println();
}

// === Control Pump ===
void controlPump(int pumpPin, bool on) {
  digitalWrite(pumpPin, on ? LOW : HIGH);
  Serial.print("Pump");
  switch(pumpPin) {
    case 13: Serial.print("1"); break;
    case 27: Serial.print("2"); break;
    case 12: Serial.print("3"); break;
    case 14: Serial.print("4"); break;
  }
  Serial.println(on ? " ON" : " OFF");
}

// === Irrigation Logic ===
void irrigateZone(int zoneIndex) {
  unsigned long startTime = millis();
  int moistureValue = readMoisture(zoneIndex);
  int threshold = moistureThresholds[zoneIndex];

  Serial.print("Zone ");
  Serial.print(zoneIndex + 1);
  if (zoneIndex == 3) Serial.print(" (uses Zone3 sensor)");
  Serial.print(" - Value: ");
  Serial.print(moistureValue);
  Serial.print(" vs Threshold: ");
  Serial.print(threshold);
  Serial.print(" - ");

  if (moistureValue > threshold) {
    Serial.println("DRY, starting irrigation");
    
    controlPump(pumpPins[zoneIndex], true);
    
    while (true) {
      moistureValue = readMoisture(zoneIndex);
      
      if (moistureValue < threshold) {
        Serial.print("Zone ");
        Serial.print(zoneIndex + 1);
        Serial.println(" MOIST, stopping irrigation");
        break;
      }
      
      if (millis() - startTime > maxRunTime) {
        Serial.print("Zone ");
        Serial.print(zoneIndex + 1);
        Serial.println(" TIMEOUT, safety stop");
        break;
      }
      
      delay(500);
    }
    
    controlPump(pumpPins[zoneIndex], false);
  } else {
    Serial.println("MOIST, no irrigation needed");
  }
}

// === Run Irrigation Cycle ===
void runIrrigationCycle() {
  Serial.println("=== Starting Irrigation Cycle ===");
  DispalyAllMoisture();
  
  for (int i = 0; i < number_zones; i++) {
    irrigateZone(i);
    delay(1000);
  }
  
  Serial.println("=== Irrigation Complete ===");
  Serial.println();
}

// === Main Loop ===
void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastIrrigationTime >= irrigationInterval) {
    runIrrigationCycle();
    lastIrrigationTime = currentTime;
  }

  delay(1000);
}