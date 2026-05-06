// Explicitly using the Dx macros to avoid IDE pin numbering conflicts
const int pinA_High = D9; 
const int pinA_Low  = D8; 

const int pinB_High = D7; 
const int pinB_Low  = D6; 

const int pinC_High = D5; 
const int pinC_Low  = D4; 

int stepDelay = 5000; 
int dutyCycle = 50;  

void setup() {
  Serial.begin(115200);

  pinMode(pinA_High, OUTPUT); pinMode(pinA_Low, OUTPUT);
  pinMode(pinB_High, OUTPUT); pinMode(pinB_Low, OUTPUT);
  pinMode(pinC_High, OUTPUT); pinMode(pinC_Low, OUTPUT);

  turnOffAll();
  delay(2000);
}

void loop() {
  // Step 1: Phase A High, Phase B Low
  turnOffAll(); 
  delayMicroseconds(20); 
  analogWrite(pinA_High, dutyCycle);
  digitalWrite(pinB_Low, HIGH);
  delayMicroseconds(stepDelay);

  // Step 2: Phase A High, Phase C Low
  turnOffAll(); 
  delayMicroseconds(20);
  analogWrite(pinA_High, dutyCycle);
  digitalWrite(pinC_Low, HIGH);
  delayMicroseconds(stepDelay);

  // Step 3: Phase B High, Phase C Low
  turnOffAll(); 
  delayMicroseconds(20);
  analogWrite(pinB_High, dutyCycle);
  digitalWrite(pinC_Low, HIGH);
  delayMicroseconds(stepDelay);

  // Step 4: Phase B High, Phase A Low
  turnOffAll(); 
  delayMicroseconds(20);
  analogWrite(pinB_High, dutyCycle);
  digitalWrite(pinA_Low, HIGH);
  delayMicroseconds(stepDelay);

  // Step 5: Phase C High, Phase A Low
  turnOffAll(); 
  delayMicroseconds(20);
  analogWrite(pinC_High, dutyCycle);
  digitalWrite(pinA_Low, HIGH);
  delayMicroseconds(stepDelay);

  // Step 6: Phase C High, Phase B Low
  turnOffAll(); 
  delayMicroseconds(20);
  analogWrite(pinC_High, dutyCycle);
  digitalWrite(pinB_Low, HIGH);
  delayMicroseconds(stepDelay);
}

void turnOffAll() {
  analogWrite(pinA_High, 0); digitalWrite(pinA_Low, LOW);
  analogWrite(pinB_High, 0); digitalWrite(pinB_Low, LOW);
  analogWrite(pinC_High, 0); digitalWrite(pinC_Low, LOW);
}