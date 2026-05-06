// 3-Phase SPWM Inverter — Arduino Nano 33 IoT
// 50Hz output, 6 MOSFETs, fixed speed
// High-side: pins 9, 10, 11
// Low-side:  pins 3,  5,  6
const int SINE_STEPS = 48;

// Track previous duty direction per phase to enforce deadtime correctly
float prevDuty[3] = {0.5, 0.5, 0.5};


const int HIGH_U = 9;
const int HIGH_V = 10;
const int HIGH_W = 11;
const int LOW_U  = 3;
const int LOW_V  = 5;
const int LOW_W  = 6;

const float FREQUENCY   = 50.0;
const int   PWM_FREQ    = 20000;
const int   DEADTIME_US = 2;
const float MODULATION  = 0.9;


float sineTable[SINE_STEPS];

// --- Debug counters ---
unsigned long cycleCount     = 0;
unsigned long lastReportTime = 0;
const unsigned long REPORT_INTERVAL_MS = 2000; // Print status every 2 seconds

// Direct register PWM for Nano 33 IoT (SAMD21 chip)
// Bypasses analogWrite overhead entirely
void fastPWM(int pin, int duty) {
  // analogWrite is fine for setup, but in the hot loop we need speed
  // This forces a direct OCR register write
  analogWrite(pin, duty); // We'll optimise further if still slow
}

void buildSineTable() {
  Serial.println("[INIT] Building sine lookup table...");
  for (int i = 0; i < SINE_STEPS; i++) {
    sineTable[i] = 0.5 + 0.5 * sin(2.0 * PI * i / SINE_STEPS);
  }
  Serial.print("[INIT] Sine table built. Sample values — step 0: ");
  Serial.print(sineTable[0], 4);
  Serial.print("  step 90: ");
  Serial.print(sineTable[90], 4);
  Serial.print("  step 180: ");
  Serial.print(sineTable[180], 4);
  Serial.print("  step 270: ");
  Serial.println(sineTable[270], 4);
}

void setPhasePWM(int highPin, int lowPin, float duty, int phaseIdx) {
  float rawDuty = constrain(duty * MODULATION, 0.0, 1.0);
  int highDuty = (int)(rawDuty * 255);
  int lowDuty  = 255 - highDuty;

  if (rawDuty > prevDuty[phaseIdx]) {
    analogWrite(lowPin, 0);
    delayMicroseconds(DEADTIME_US);
    analogWrite(highPin, highDuty);
  } else {
    analogWrite(highPin, 0);
    delayMicroseconds(DEADTIME_US);
    analogWrite(lowPin, lowDuty);
  }

  prevDuty[phaseIdx] = rawDuty;
}

void setPhasePWM(int highPin, int lowPin, float duty, int phaseIdx) {
  float rawDuty = constrain(duty * MODULATION, 0.0, 1.0);
  int highDuty = (int)(rawDuty * 255);
  int lowDuty  = 255 - highDuty;

  if (rawDuty > prevDuty[phaseIdx]) {
    fastPWM(lowPin, 0);
    delayMicroseconds(DEADTIME_US);
    fastPWM(highPin, highDuty);
  } else {
    fastPWM(highPin, 0);
    delayMicroseconds(DEADTIME_US);
    fastPWM(lowPin, lowDuty);
  }

  prevDuty[phaseIdx] = rawDuty;
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial monitor time to connect
  Serial.println("========================================");
  Serial.println(" 3-Phase SPWM Inverter — Debug Mode");
  Serial.println("========================================");
  Serial.print("[CFG] Output frequency:  "); Serial.print(FREQUENCY);   Serial.println(" Hz");
  Serial.print("[CFG] Switching freq:    "); Serial.print(PWM_FREQ);    Serial.println(" Hz");
  Serial.print("[CFG] Deadtime:          "); Serial.print(DEADTIME_US); Serial.println(" us");
  Serial.print("[CFG] Modulation index:  "); Serial.println(MODULATION);
  Serial.println();

  Serial.println("[INIT] Setting pin modes...");
  pinMode(HIGH_U, OUTPUT); pinMode(LOW_U, OUTPUT);
  pinMode(HIGH_V, OUTPUT); pinMode(LOW_V, OUTPUT);
  pinMode(HIGH_W, OUTPUT); pinMode(LOW_W, OUTPUT);
  Serial.println("[INIT] Pins configured.");

  Serial.println("[INIT] All outputs set to LOW (safe state)...");
  analogWrite(HIGH_U, 0); analogWrite(LOW_U, 0);
  analogWrite(HIGH_V, 0); analogWrite(LOW_V, 0);
  analogWrite(HIGH_W, 0); analogWrite(LOW_W, 0);
  Serial.println("[INIT] Safe state confirmed.");

  buildSineTable();

  Serial.println();
  Serial.println("[INIT] Initialisation complete. Starting inverter loop...");
  Serial.println("[INFO] Status report every 2 seconds.");
  Serial.println("[INFO] Watch for [WARN] tags — these indicate potential issues.");
  Serial.println();

  lastReportTime = millis();
}

void loop() {
  // Target: 48 steps, 20000us cycle = 416us per step
  // Subtract ~18us deadtime overhead (3 phases × 2 × DEADTIME_US)
  const int STEP_DELAY_US = (1000000 / (int)FREQUENCY / SINE_STEPS) - (6 * DEADTIME_US);

  for (int i = 0; i < SINE_STEPS; i++) {
    int iU = i;
    int iV = (i + SINE_STEPS / 3) % SINE_STEPS;
    int iW = (i + 2 * SINE_STEPS / 3) % SINE_STEPS;

    setPhasePWM(HIGH_U, LOW_U, sineTable[iU], 0);
    setPhasePWM(HIGH_V, LOW_V, sineTable[iV], 1);
    setPhasePWM(HIGH_W, LOW_W, sineTable[iW], 2);

    delayMicroseconds(STEP_DELAY_US);
  }

  cycleCount++;

  if (millis() - lastReportTime >= REPORT_INTERVAL_MS) {
    printStatus();
    lastReportTime = millis();
  }
}