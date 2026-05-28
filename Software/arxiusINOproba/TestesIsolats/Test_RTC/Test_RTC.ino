#include <Wire.h>
#include <RTClib.h>

// Pins I2C personalitzats definits per l'usuari
#define I2C_SDA 32
#define I2C_SCL 33

RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Espera que s'obri el Monitor Sèrie
  
  Serial.println("\n==============================================");
  Serial.println("  TEST AÏLLAT: DS3231 RTC (I2C Personalitzat) ");
  Serial.println("==============================================");
  Serial.printf("Configuració de pins: SDA = GPIO %d, SCL = GPIO %d\n", I2C_SDA, I2C_SCL);

  // Inicialitza l'I2C amb els pins personalitzats
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000); // 400kHz

  // Inicialització del mòdul RTC
  if (!rtc.begin()) {
    Serial.println("\n[ERROR] No s'ha trobat el mòdul RTC DS3231!");
    Serial.println("-> Comprova que els cables VCC (3V3), GND, SDA (32) i SCL (33) estiguin ben connectats.");
    while (1) {
      delay(1000);
    }
  }

  // Comprova si ha perdut l'alimentació (i per tant ha perdut l'hora)
  if (rtc.lostPower()) {
    Serial.println("\n[AVÍS] El RTC ha perdut l'alimentació (pila descarregada o nova).");
    Serial.println("-> Sincronitzant l'hora amb la data i hora de compilació...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  } else {
    Serial.println("\n[OK] El RTC manté l'alimentació i l'hora correctament.");
  }

  Serial.println("\n=== Instruccions pel Monitor Sèrie ===");
  Serial.println("Envia 's' o 'S' per forçar la sincronització amb l'hora de compilació.");
  Serial.println("======================================\n");
}

void loop() {
  // Comprovació de comandes sèrie de l'usuari
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 's' || c == 'S') {
      Serial.println("\n>>> Sincronitzant RTC amb la data i hora de compilació...");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      
      DateTime now = rtc.now();
      Serial.printf(">>> Hora sincronitzada a: %04d-%02d-%02d %02d:%02d:%02d\n\n",
                    now.year(), now.month(), now.day(),
                    now.hour(), now.minute(), now.second());
    }
  }

  // Llegim l'hora actual del DS3231
  DateTime now = rtc.now();
  
  // Llegim la temperatura del sensor integrat al DS3231
  float temp = rtc.getTemperature();

  // Imprimim la data, hora i temperatura
  Serial.printf("[%04d-%02d-%02d %02d:%02d:%02d] Temp: %.2f °C\n",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second(),
                temp);

  delay(1000);
}
