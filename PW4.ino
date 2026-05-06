#include <Arduino.h>

// ---- Configuration ----
const float FREQUENCY   = 50.0f;
const float MODULATION  = 0.9f;
const int   SINE_STEPS  = 48;
const int   DEADTIME_US = 2;
const int   TOP         = 1000;

const unsigned long REPORT_INTERVAL_MS = 2000;

// ---- Sine table ----
int dutyTable[SINE_STEPS];

void buildSineTable() {
  Serial.println("[INIT] Building sine table...");
  for (int i = 0; i < SINE_STEPS; i++) {
    float s = 0.5f + 0.5f * sinf(2.0f * PI * i / SINE_STEPS);
    dutyTable[i] = (int)(s * MODULATION * TOP);
  }
  Serial.print("[INIT] Done. Step 0=");  Serial.print(dutyTable[0]);
  Serial.print(" Step 12=");             Serial.print(dutyTable[12]);
  Serial.print(" Step 24=");             Serial.print(dutyTable[24]);
  Serial.print(" Step 36=");             Serial.println(dutyTable[36]);
}

// ---- TCC0: Phase U — D2 (PB10) high-side, D3 (PB11) low-side ----
void setupTCC0(uint32_t top) {
  GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TCC0_TCC1));
  while (GCLK->STATUS.bit.SYNCBUSY);

  PORT->Group[1].DIRSET.reg     = PORT_PB10;
  PORT->Group[1].PINCFG[10].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[1].PMUX[5].reg    = PORT_PMUX_PMUXE(4);

  PORT->Group[1].DIRSET.reg     = PORT_PB11;
  PORT->Group[1].PINCFG[11].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[1].PMUX[5].reg   |= PORT_PMUX_PMUXO(4);

  TCC0->CTRLA.reg &= ~TCC_CTRLA_ENABLE;
  while (TCC0->SYNCBUSY.bit.ENABLE);
  TCC0->CTRLA.reg  = TCC_CTRLA_PRESCALER_DIV1;
  TCC0->WAVE.reg   = TCC_WAVE_WAVEGEN_NPWM;
  while (TCC0->SYNCBUSY.bit.WAVE);
  TCC0->PER.reg    = top;
  while (TCC0->SYNCBUSY.bit.PER);
  TCC0->CC[0].reg  = 0;
  while (TCC0->SYNCBUSY.bit.CC0);
  TCC0->CC[1].reg  = 0;
  while (TCC0->SYNCBUSY.bit.CC1);
  TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
  while (TCC0->SYNCBUSY.bit.ENABLE);
}

// ---- TCC1: Phase V — D4 (PA07) high-side ----
void setupTCC1(uint32_t top) {
  GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TCC0_TCC1));
  while (GCLK->STATUS.bit.SYNCBUSY);

  PORT->Group[0].DIRSET.reg    = PORT_PA07;
  PORT->Group[0].PINCFG[7].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[0].PMUX[3].reg    = PORT_PMUX_PMUXO(4);

  TCC1->CTRLA.reg &= ~TCC_CTRLA_ENABLE;
  while (TCC1->SYNCBUSY.bit.ENABLE);
  TCC1->CTRLA.reg  = TCC_CTRLA_PRESCALER_DIV1;
  TCC1->WAVE.reg   = TCC_WAVE_WAVEGEN_NPWM;
  while (TCC1->SYNCBUSY.bit.WAVE);
  TCC1->PER.reg    = top;
  while (TCC1->SYNCBUSY.bit.PER);
  TCC1->CC[1].reg  = 0;
  while (TCC1->SYNCBUSY.bit.CC1);
  TCC1->CTRLA.reg |= TCC_CTRLA_ENABLE;
  while (TCC1->SYNCBUSY.bit.ENABLE);
}

// ---- TC3: Phase W — D11 (PA16) high-side, D12 (PA19) low-side ----
void setupTC3(uint32_t top) {
  GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TCC2_TC3));
  while (GCLK->STATUS.bit.SYNCBUSY);

  PORT->Group[0].DIRSET.reg     = PORT_PA16;
  PORT->Group[0].PINCFG[16].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[0].PMUX[8].reg     = PORT_PMUX_PMUXE(4);

  PORT->Group[0].DIRSET.reg     = PORT_PA19;
  PORT->Group[0].PINCFG[19].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[0].PMUX[9].reg     = PORT_PMUX_PMUXO(4);

  TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TC3->COUNT16.STATUS.bit.SYNCBUSY);
  TC3->COUNT16.CTRLA.reg  = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MPWM | TC_CTRLA_PRESCALER_DIV1;
  TC3->COUNT16.CC[0].reg  = top;
  TC3->COUNT16.CC[1].reg  = 0;
  while (TC3->COUNT16.STATUS.bit.SYNCBUSY);
  TC3->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC3->COUNT16.STATUS.bit.SYNCBUSY);
}

// ---- Set duty cycles ----
inline void setDutyU(int highDuty) {
  TCC0->CCB[0].reg = highDuty;
  while (TCC0->SYNCBUSY.bit.CCB0);
  TCC0->CCB[1].reg = TOP - highDuty;
  while (TCC0->SYNCBUSY.bit.CCB1);
}

inline void setDutyV(int highDuty) {
  TCC1->CCB[1].reg = highDuty;
  while (TCC1->SYNCBUSY.bit.CCB1);
}

inline void setDutyW(int highDuty) {
  TC3->COUNT16.CC[1].reg = highDuty;
  while (TC3->COUNT16.STATUS.bit.SYNCBUSY);
}

// ---- Debug counters ----
unsigned long cycleCount     = 0;
unsigned long lastReportTime = 0;

void printStatus() {
  float actualFreq = (float)cycleCount / (REPORT_INTERVAL_MS / 1000.0f);
  Serial.println("-------- STATUS --------");
  Serial.print("[RUN] Cycles in last 2s:       "); Serial.println(cycleCount);
  Serial.print("[RUN] Approx output frequency: "); Serial.print(actualFreq, 1); Serial.println(" Hz (target: 50.0)");
  Serial.print("[CFG] Modulation:              "); Serial.println(MODULATION);
  Serial.print("[CFG] Deadtime (us):           "); Serial.println(DEADTIME_US);
  Serial.print("[CFG] Sine steps:              "); Serial.println(SINE_STEPS);
  Serial.println("------------------------");
  cycleCount = 0;
}

// ---- Setup ----
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("========================================");
  Serial.println(" 3-Phase SPWM Inverter — Register Mode");
  Serial.println("========================================");
  Serial.print("[CFG] Frequency:  "); Serial.print(FREQUENCY);   Serial.println(" Hz");
  Serial.print("[CFG] Modulation: "); Serial.println(MODULATION);
  Serial.print("[CFG] Deadtime:   "); Serial.print(DEADTIME_US); Serial.println(" us");
  Serial.println();

  buildSineTable();

  Serial.println("[INIT] Configuring TCC0 — Phase U (D2 high, D3 low)...");
  setupTCC0(TOP);
  Serial.println("[INIT] Configuring TCC1 — Phase V (D4 high)...");
  setupTCC1(TOP);
  Serial.println("[INIT] Configuring TC3  — Phase W (D11 high, D12 low)...");
  setupTC3(TOP);

  Serial.println("[INIT] All timers running.");
  Serial.println("[INIT] Starting inverter loop...");
  Serial.println("[INFO] Status report every 2 seconds.");
  Serial.println();

  lastReportTime = millis();
}

// ---- Main loop ----
void loop() {
  const int STEP_DELAY_US = (1000000 / (int)FREQUENCY / SINE_STEPS) - (DEADTIME_US * 2);

  for (int i = 0; i < SINE_STEPS; i++) {
    int iU = i;
    int iV = (i + SINE_STEPS / 3) % SINE_STEPS;
    int iW = (i + 2 * SINE_STEPS / 3) % SINE_STEPS;

    setDutyU(dutyTable[iU]);
    delayMicroseconds(DEADTIME_US);
    setDutyV(dutyTable[iV]);
    delayMicroseconds(DEADTIME_US);
    setDutyW(dutyTable[iW]);

    if (STEP_DELAY_US > 0)
      delayMicroseconds(STEP_DELAY_US);
  }

  cycleCount++;

  if (millis() - lastReportTime >= REPORT_INTERVAL_MS) {
    printStatus();
    lastReportTime = millis();
  }
}