// ==========================================================================
// Test_Motor.ino — Programa de Prova del Motor Pas a Pas NEMA 17 i DRV8825
// Projecte Integrat II — GR15 [GEMEC-09UV] · Universitat de Vic — UCC
// ==========================================================================
// FASE 5 (Interactiva & Jamming):
//   - Escriu "start" per posar el motor en marxa contínua (Sentit Invertit: LOW).
//   - Escriu "stop" per aturar el motor immediatament i alliberar el parell.
//   - Escriu "jam" o prem el botó BTN_FEED (GPIO 13) per simular un enclavament:
//     El motor s'aturarà, farà un retrocés ràpid en sentit contrari (HIGH)
//     per alliberar el pinço i després reprendrà la marxa normal (LOW).
// ==========================================================================

#include <Arduino.h>

// Definició de Pins (Mapa de Pins Mestre)
#define EN_PIN    25   // Actiu a nivell BAIX (LOW) per habilitar el driver
#define STEP_PIN  26   // Genera els polsos per fer cada pas
#define DIR_PIN   27   // Controla el sentit de gir (LOW = Sentit Normal, HIGH = Retrocés)
#define BTN_FEED  13   // Botó físic de pinçament / FEED per simular JAMMING

// Direccions de gir (Invertit una altra vegada segons petició)
#define DIRECCIO_NORMAL   LOW   // Direcció de marxa establerta per defecte
#define DIRECCIO_RETROCES  HIGH  // Direcció de desblocatge de seguretat

// Velocitat del Motor (Retard de Pas en microsegons)
#define STEP_DELAY_US     1000  
#define STEP_JAM_DELAY_US  600   // Velocitat més ràpida per a la maniobra de desblocatge

// Variable d'estat
bool motorActiu = false;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10); // Resposta sèrie ràpida
  delay(500);
  
  Serial.println(F("\n========================================================"));
  Serial.println(F("  CATFEEDER-T5: Control Interactiu + Protecció JAMMING"));
  Serial.println(F("========================================================"));
  Serial.println(F(" [PINS CONFIGURATS]:"));
  Serial.printf("  - ENABLE (EN):    GPIO %d (actiu en LOW)\n", EN_PIN);
  Serial.printf("  - STEP:          GPIO %d\n", STEP_PIN);
  Serial.printf("  - DIRECTION:     GPIO %d (normal en %s)\n", DIR_PIN, DIRECCIO_NORMAL == LOW ? "LOW" : "HIGH");
  Serial.printf("  - SENSOR JAMMING: Botó BTN_FEED (GPIO %d, amb PULLUP)\n", BTN_FEED);
  Serial.println(F(" ------------------------------------------------------"));
  Serial.println(F("  >>> COMANDES DISPONIBLES AL MONITOR SÈRIE: <<<"));
  Serial.println(F("  1) Escriu 'start' per iniciar el moviment."));
  Serial.println(F("  2) Escriu 'stop'  per aturar i alliberar el motor."));
  Serial.println(F("  3) Escriu 'jam'   per simular un enclavament de menjar."));
  Serial.println(F("  * NOTA: També pots prémer el botó físic (GPIO 13) per simular el Jamming."));
  Serial.println(F("========================================================\n"));

  // Configurar pins
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  pinMode(BTN_FEED, INPUT_PULLUP); // Activat amb resistència interna PULLUP
  
  // Motor deshabilitat d'inici per seguretat
  digitalWrite(EN_PIN, HIGH); 
  Serial.println(F("[INICI] Motor en repòs (Desactivat / Lliure). Esperant comanda..."));
}

/**
 * Executa una maniobra ràpida de desobstrucció (Unjamming).
 * Atura el motor, fa un retrocés ràpid en sentit oposat i reprèn la marxa.
 */
void executarDesblocatge() {
  Serial.println(F("\n[ALERTA] !!! DETECTAT ENCLAVAMENT DEL PINÇO (JAMMING) !!!"));
  Serial.println(F("[ALERTA] Iniciant maniobra ràpida de desobstrucció..."));
  
  // 1. Aturar el motor immediatament
  digitalWrite(STEP_PIN, LOW);
  delay(150); // Breu pausa per evitar canvis bruscs d'inèrcia mecànica
  
  // 2. Canviar a sentit de retrocés ràpid (DIRECCIO_RETROCES = HIGH)
  digitalWrite(DIR_PIN, DIRECCIO_RETROCES);
  Serial.println(F("[DESBLOCATGE] Girant en sentit contrari (HIGH) ràpidament durant 500 passos..."));
  
  for (int i = 0; i < 500; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_JAM_DELAY_US);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(STEP_JAM_DELAY_US);
  }
  
  // 3. Aturar i reposar un moment
  delay(200); 
  
  // 4. Tornar al sentit normal (DIRECCIO_NORMAL = LOW)
  digitalWrite(DIR_PIN, DIRECCIO_NORMAL);
  Serial.println(F("[DESBLOCATGE] Bloqueig alliberat correctament. Reprenent marxa normal...\n"));
}

void loop() {
  // 1. Llegir ordres des del port sèrie
  if (Serial.available() > 0) {
    String comanda = Serial.readStringUntil('\n');
    comanda.trim();
    comanda.toLowerCase();

    if (comanda == "start") {
      if (!motorActiu) {
        motorActiu = true;
        digitalWrite(EN_PIN, LOW); // Habilita el pas de corrent
        delay(5); // Càrrega de bobines
        digitalWrite(DIR_PIN, DIRECCIO_NORMAL); // Sentit normal (LOW)
        Serial.println(F("[COMANDA] 'start' rebuda. Motor en marxa (Direcció: LOW)..."));
      } else {
        Serial.println(F("[INFO] El motor ja està en funcionament."));
      }
    } 
    else if (comanda == "stop") {
      if (motorActiu) {
        motorActiu = false;
        digitalWrite(EN_PIN, HIGH); // Allibera el parell (sense corrent)
        Serial.println(F("[COMANDA] 'stop' rebuda. Motor aturat i lliure."));
      } else {
        Serial.println(F("[INFO] El motor ja està aturat."));
      }
    }
    else if (comanda == "jam" || comanda == "jamming") {
      if (motorActiu) {
        executarDesblocatge();
      } else {
        Serial.println(F("[ERR] El motor està parat. Només pots simular Jamming amb el motor actiu ('start')."));
      }
    }
    else if (comanda.length() > 0) {
      Serial.printf("[ERROR] Comanda '%s' no vàlida. Utilitza 'start', 'stop' o 'jam'.\n", comanda.c_str());
    }
  }

  // 2. Si el motor està en marxa, fer un pas
  if (motorActiu) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_DELAY_US);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(STEP_DELAY_US);
    
    // 3. Comprovació física en temps real: si es prem el botó físic BTN_FEED (GPIO 13)
    if (digitalRead(BTN_FEED) == LOW) {
      executarDesblocatge();
      // Debounce simple per evitar múltiples deteccions mecàniques seguides
      delay(150); 
    }
  }
}

