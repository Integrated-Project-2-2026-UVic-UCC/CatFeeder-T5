#define EN_PIN 25
#define STEP_PIN 26
#define DIR_PIN 27

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciant test del Motor Stepper NEMA17...");
  
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  
  // Activa el motor (per defecte els drivers DRV8825 s'activen en LOW)
  digitalWrite(EN_PIN, LOW); 
}

void loop() {
  // Configurar que giri cal al davant
  digitalWrite(DIR_PIN, HIGH);
  Serial.println("Girant en Endavant...");
  
  for(int i = 0; i < 200; i++) { // 200 passos = 1 revolució normal (sense microstepping)
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(2000); // Temps / velocitat d'espera
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(2000);
  }
  
  delay(1000); // Pausa

  // Gira cap a l'altra banda
  digitalWrite(DIR_PIN, LOW);
  Serial.println("Girant Enrere...");
  
  for(int i = 0; i < 200; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(2000);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(2000);
  }

  delay(1000); // Pausa
}
