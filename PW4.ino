// 3-Phase SPWM Inverter — Arduino Nano 33 IoT
// 50Hz output, 6 MOSFETs, fixed speed
// High-side: pins 9, 10, 11
// Low-side:  pins 3,  5,  6

const int HIGH_U = 9;   // TCC0
const int HIGH_V = 4;   // TCC1 — independent timer
const int HIGH_W = 11;  // TCC0 (different channel from D9)
const int LOW_U  = 3;   // TCC0
const int LOW_V  = 6;   // TCC0
const int LOW_W  = 5;   // TCC0

const float FREQUENCY   = 50.0;
const int   DEADTIME_US = 2;
const float MODULATION  = 0.9;
const int   SINE_STEPS  = 48;

const unsigned long REPORT_INTERVAL_MS = 2000;

int dutyTable[SINE_STEPS];
float prevDuty[3] = {0.5, 0.5, 0.5};

unsigned long cycleCount     = 0;
unsigned long lastReportTime = 0;

// --- Pre-compute duty cycle integers at startup ---;
void buildSineTable() {
  Serial.println("[INIT] Building sine lookup table...");
  for (int i = 0; i < SINE_STEPS; i++) {
    float s = 0.5 + 0.5 * sin(2.0 * PI * i / SINE_STEPS);
    dutyTable[i] = (int)(s * MODULATION * 255);
  }
  Serial.print("[INIT] Sine table built. Sample — step 0: ");
  Serial.print(dutyTable[0]);
  Serial.print("  step 12: ");
  Serial.print(dutyTable[12]);
  Serial.print("  step 24: ");
  Serial.print(dutyTable[24]);
  Serial.print("  step 36: ");
  Serial.println(dutyTable[36]);
}

// --- Phase PWM with deadtime ---
void setPhasePWM(int highPin, int lowPin, int highDuty, int phaseIdx) {
  int lowDuty = 255 - highDuty;
  float rawDuty = highDuty / 255.0;

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

// --- Status report ---
void printStatus() {
  float actualFreq = (float)cycleCount / (REPORT_INTERVAL_MS / 1000.0);
  Serial.println("-------- STATUS --------");
  Serial.print("[RUN] Cycles in last 2s:        "); Serial.println(cycleCount);
  Serial.print("[RUN] Approx output frequency:  "); Serial.print(actualFreq, 1); Serial.println(" Hz (target: 50.0 Hz)");
  Serial.print("[CFG] Modulation index:         "); Serial.println(MODULATION);
  Serial.print("[CFG] Deadtime (us):            "); Serial.println(DEADTIME_US);
  Serial.print("[CFG] Sine steps:               "); Serial.println(SINE_STEPS);
  Serial.println("------------------------");
  cycleCount = 0;
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("========================================");
  Serial.println(" 3-Phase SPWM Inverter — Debug Mode");
  Serial.println("========================================");
  Serial.print("[CFG] Output frequency:  "); Serial.print(FREQUENCY);   Serial.println(" Hz");
  Serial.print("[CFG] Deadtime:          "); Serial.print(DEADTIME_US); Serial.println(" us");
  Serial.print("[CFG] Modulation index:  "); Serial.println(MODULATION);

  pinMode(HIGH_U, OUTPUT); pinMode(LOW_U, OUTPUT);
  pinMode(HIGH_V, OUTPUT); pinMode(LOW_V, OUTPUT);
  pinMode(HIGH_W, OUTPUT); pinMode(LOW_W, OUTPUT);

  analogWrite(HIGH_U, 0); analogWrite(LOW_U, 0);
  analogWrite(HIGH_V, 0); analogWrite(LOW_V, 0);
  analogWrite(HIGH_W, 0); analogWrite(LOW_W, 0);
  Serial.println("[INIT] All outputs LOW — safe state confirmed.");

  buildSineTable();

  Serial.println("[INIT] Ready. Starting inverter loop...");
  lastReportTime = millis();
}

// --- Main loop ---
void loop() {
  const int STEP_DELAY_US = (1000000 / (int)FREQUENCY / SINE_STEPS) - (6 * DEADTIME_US);

  for (int i = 0; i < SINE_STEPS; i++) {
    int iU = i;
    int iV = (i + SINE_STEPS / 3) % SINE_STEPS;
    int iW = (i + 2 * SINE_STEPS / 3) % SINE_STEPS;

    setPhasePWM(HIGH_U, LOW_U, dutyTable[iU], 0);
    setPhasePWM(HIGH_V, LOW_V, dutyTable[iV], 1);
    setPhasePWM(HIGH_W, LOW_W, dutyTable[iW], 2);

    if (STEP_DELAY_US > 0)
      delayMicroseconds(STEP_DELAY_US);
  }

  cycleCount++;

  if (millis() - lastReportTime >= REPORT_INTERVAL_MS) {
    printStatus();
    lastReportTime = millis();
  }
}