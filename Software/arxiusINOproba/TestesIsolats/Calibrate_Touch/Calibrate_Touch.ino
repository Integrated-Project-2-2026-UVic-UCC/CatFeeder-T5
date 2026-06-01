// ==========================================================================
// Calibrate_Touch.ino — Calibració Manual per a pantalla de 320x280 pxls
// Projecte Alimentador Gatos — CatFeeder-T5
// ==========================================================================
// Dissenyat específicament per a pantalles amb àrea activa de 320x280 pxls.
// En lloc de fer servir la calibració nativa de TFT_eSPI (que pot pintar
// fora de la pantalla si el driver assumeix 480x320), aquest sketch utilitza
// un mètode manual de 2 punts dins dels límits de 320x280 i calcula els valors.
// ==========================================================================

#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// Límits del viewport indicats per l'usuari
#define CAL_WIDTH   320
#define CAL_HEIGHT  280

// Punts de calibració (separats 20 pxls de les cantonades)
#define PT1_X       20
#define PT1_Y       20
#define PT2_X       300 // 320 - 20
#define PT2_Y       260 // 280 - 20

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("  INICIANT CALIBRACIÓ MANUAL (320x280)  "));
  Serial.println(F("========================================"));

  tft.init();
  tft.setRotation(1); // Orientació horitzontal
  
  run_manual_calibration();
}

void loop() {
  uint16_t tx = 0, ty = 0;
  
  // Prova en temps real de la calibració obtinguda
  if (tft.getTouch(&tx, &ty)) {
    Serial.printf("[test] px=(%u,%u)\n", tx, ty);
    
    // Dibuixem només si està dins del rang visible de 320x280
    if (tx < CAL_WIDTH && ty < CAL_HEIGHT) {
      tft.fillCircle(tx, ty, 3, TFT_BLUE);
    }
  }
  delay(10);
}

// Funció que espera a que l'usuari toqui i deixi anar a les coordenades indicades
void wait_for_touch(uint16_t target_x, uint16_t target_y, uint16_t &raw_x, uint16_t &raw_y) {
  // Neteja pantalla i pinta objectiu
  tft.fillScreen(TFT_BLACK);
  
  // Dibuixa retícules de l'objectiu
  tft.drawCircle(target_x, target_y, 10, TFT_RED);
  tft.drawLine(target_x - 15, target_y, target_x + 15, target_y, TFT_WHITE);
  tft.drawLine(target_x, target_y - 15, target_x, target_y + 15, TFT_WHITE);
  
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("Toca el centre de la mira vermella", CAL_WIDTH / 2, CAL_HEIGHT / 2 - 20);
  
  char buf[30];
  snprintf(buf, sizeof(buf), "Objectiu: (%u, %u)", target_x, target_y);
  tft.drawString(buf, CAL_WIDTH / 2, CAL_HEIGHT / 2 + 10);

  // Espera que es premi amb prou pressió
  while (true) {
    if (tft.getTouchRawZ() > 400) {
      tft.getTouchRaw(&raw_x, &raw_y);
      break;
    }
    delay(10);
  }
  
  // Feedback visual de premut
  tft.drawCircle(target_x, target_y, 10, TFT_GREEN);
  tft.drawCircle(target_x, target_y, 11, TFT_GREEN);
  
  // Espera que l'usuari aixequi el dit
  while (tft.getTouchRawZ() > 150) {
    delay(10);
  }
  delay(300); // Debounce
}

void run_manual_calibration() {
  uint16_t rx1 = 0, ry1 = 0;
  uint16_t rx2 = 0, ry2 = 0;

  // Pas 1: Cantonada Superior Esquerra (20, 20)
  wait_for_touch(PT1_X, PT1_Y, rx1, ry1);
  Serial.printf("[cal] Punt 1 obtingut: raw=(%u, %u)\n", rx1, ry1);

  // Pas 2: Cantonada Inferior Dreta (300, 260)
  wait_for_touch(PT2_X, PT2_Y, rx2, ry2);
  Serial.printf("[cal] Punt 2 obtingut: raw=(%u, %u)\n", rx2, ry2);

  // --- Càlculs matemàtics d'extrapolació ---
  // Calculem la relació entre píxels i valors raw (x_scale i y_scale)
  float x_pixel_dist = PT2_X - PT1_X; // 300 - 20 = 280
  float y_pixel_dist = PT2_Y - PT1_Y; // 260 - 20 = 240
  
  float raw_x_diff = (float)rx2 - (float)rx1;
  float raw_y_diff = (float)ry2 - (float)ry1;

  float scale_x = raw_x_diff / x_pixel_dist;
  float scale_y = raw_y_diff / y_pixel_dist;

  // Extrapolem els valors a les coordenades virtuals (0) i (width / height)
  // calData[0] és raw a x=0
  // calData[1] és raw a x=CAL_WIDTH
  // calData[2] és raw a y=0
  // calData[3] és raw a y=CAL_HEIGHT
  int32_t x_0   = (int32_t)rx1 - (int32_t)(PT1_X * scale_x);
  int32_t x_max = (int32_t)rx2 + (int32_t)((CAL_WIDTH - PT2_X) * scale_x);
  int32_t y_0   = (int32_t)ry1 - (int32_t)(PT1_Y * scale_y);
  int32_t y_max = (int32_t)ry2 + (int32_t)((CAL_HEIGHT - PT2_Y) * scale_y);

  // Control de límits per si de cas (valors típics 100..4000)
  if (x_0 < 0) x_0 = 0;
  if (x_max < 0) x_max = 4095;
  if (y_0 < 0) y_0 = 0;
  if (y_max < 0) y_max = 4095;

  uint16_t calData[5] = { (uint16_t)x_0, (uint16_t)x_max, (uint16_t)y_0, (uint16_t)y_max, 7 };

  // Mostrem feedback a la pantalla
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("CALIBRACIÓ COMPLETADA!", CAL_WIDTH / 2, CAL_HEIGHT / 2 - 30);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("Dibuixa a la pantalla per provar-la.", CAL_WIDTH / 2, CAL_HEIGHT / 2 + 10);
  tft.drawString("Revisa la sortida del Serial Monitor per copiar el codi.", CAL_WIDTH / 2, CAL_HEIGHT / 2 + 25);
  
  // Dibuixem el marc límit visible de 320x280
  tft.drawRect(0, 0, CAL_WIDTH, CAL_HEIGHT, TFT_DARKGREY);

  // Imprimim els resultats en format apte per al codi per Serial
  Serial.println();
  Serial.println(F("===================================================="));
  Serial.println(F("✓ CALIBRACIÓ MANUAL (320x280) REEIXIDA!"));
  Serial.println(F("===================================================="));
  Serial.println(F("Copia la línia corresponent a 'probaMenuTouch.ino':"));
  Serial.println();
  Serial.printf("  #define TOUCH_CAL_DATA          { %u, %u, %u, %u, %u }\n", 
                calData[0], calData[1], calData[2], calData[3], calData[4]);
  Serial.println();
  Serial.println(F("===================================================="));
  Serial.println();

  // Mode de prova
  tft.setTouch(calData);
}
