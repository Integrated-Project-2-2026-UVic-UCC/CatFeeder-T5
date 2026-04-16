#define BTN_FEED 13
#define BTN_TARE 14
#define BTN_MENU 34
#define SQUISHY_PIN 35

void setup() {
  Serial.begin(115200);
  
  // Aquests depenen de PullUp Intern:
  pinMode(BTN_FEED, INPUT_PULLUP);
  pinMode(BTN_TARE, INPUT_PULLUP);
  
  // Els pins 34 i 35 NO tenen resistència PullUp a l'ESP32.
  // Demanen les resistències de 10k connectades de forma física.
  pinMode(BTN_MENU, INPUT);
  pinMode(SQUISHY_PIN, INPUT);

  Serial.println("Iniciant test de la Botonera... Prem algun botó.");
}

void loop() {
  if (digitalRead(BTN_FEED) == LOW) {
    Serial.println("Botó FEED premut!");
    delay(200); // Antirebots temporal
  }
  
  if (digitalRead(BTN_TARE) == LOW) {
    Serial.println("Botó TARE premut!");
    delay(200);
  }
  
  // Assumint contacte cap a GND i resistència externa cap a 3V3.
  if (digitalRead(BTN_MENU) == LOW) {
    Serial.println("Botó MENU premut!");
    delay(200);
  }
  
  if (digitalRead(SQUISHY_PIN) == LOW) {
    Serial.println("Contacte / SQUISHY_PIN detectat!");
    delay(200);
  }
}
