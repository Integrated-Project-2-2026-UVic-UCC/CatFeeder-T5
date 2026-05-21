# Pla d'Integració de Hardware i Firmware — CatFeeder-T5 (ESP32)
**Integrated Project II — GR15 [GEMEC-09UV] · Universitat de Vic — UCC**
**Document destinat a: Google Antigravity (Implementador)**
**Versió:** 2.0 · Maig 2026

---

## 0. Introducció i Abast

Aquest document és una **guia d'integració pas a pas** pensada per a l'implementador. L'objectiu és verificar cada perifèric actiu de forma aïllada abans d'unificar-los en el firmware complet del **CatFeeder-T5**.

En aquesta versió del projecte:
- **S'elimina qualsevol tipus de botó físic frontal**.
- **La pantalla tàctil actua com a botonera virtual** (utilitzant el xip controlador tàctil XPT2046 integrat a la pantalla).
- **Es prescindeix del sensor ambiental DHT22**.
- **S'utilitzen pins I2C personalitzats** per al rellotge DS3231 per tal de deixar lliure el pin de selecció tàctil.

Seguint aquest ordre de proves s'evita el problema clàssic d'integrar tot de cop i no saber quin component falla. Cada fase té:
- **Objectiu** clar i mesurable.
- **Cablejat** necessari (amb referència al mapa de pins mestre).
- **Sketch de prova aïllat** a carregar (ubicats a `Software/arxiusINOproba/TestesIsolats/`).
- **Criteri d'èxit** que cal verificar per poder avançar.

> ⚠️ **Prerequisit absolut**: No passar a la fase següent fins que la fase actual superi el seu criteri d'èxit. El Monitor Sèrie s'ha de tenir obert sempre a **115200 baud**.

---

## 1. Mapa de Pins Mestre (Sense Conflictes)

Configurar **exactament** aquest mapa a `config.h` i al cablejat físic. Aquesta distribució resol el conflicte del pin de selecció tàctil i reassigna el bus I2C per a un disseny 100% net:

| Perifèric | Senyal | GPIO ESP32 | Notes |
| :--- | :--- | :---: | :--- |
| **TFT ILI9488 SPI** | MOSI / SDI | 23 | Bus SPI compartit |
| | MISO / SDO | 19 | Bus SPI compartit |
| | SCLK / SCK | 18 | Bus SPI compartit |
| | CS (TFT CS) | 15 | Chip Select pantalla |
| | DC | 2 | Data/Command |
| | RESET | 4 | Reset físic |
| | LED (backlight) | 32 | Sempre actiu (3V3 o GPIO 32) |
| **Tàctil XPT2046** | T_CS | **21** | Chip Select del controlador tàctil (XPT2046) |
| | T_CLK / T_DIN / T_DO| Compartit | Connectats en paral·lel al bus SPI (18, 23, 19) |
| **Motor NEMA 17 + DRV8825** | EN (Enable) | 25 | Actiu a nivell BAIX |
| | STEP | 26 | Polsos de moviment |
| | DIR | 27 | Sentit de gir |
| **Bàscula HX711** | DT (Dades) | 16 | Lectura amplificada de pes |
| | SCK (Rellotge) | 17 | Rellotge de la bàscula |
| **DS3231 RTC (I2C)** | SDA | **22** | **Bus I2C personalitzat (Dades)** |
| | SCL | **33** | **Bus I2C personalitzat (Rellotge)** |

---

## 2. Entorn de Desenvolupament — Configuració Prèvia

Abans de començar les fases de prova, assegurar-se que l'entorn Arduino IDE està correctament configurat.

### 2.1 Instal·lació del Board Package ESP32

A *Preferences → Additional boards manager URLs* afegir:
```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```
Instal·lar **ESP32 by Espressif** des del Boards Manager. Seleccionar **ESP32 Dev Module** a *Tools → Board*.

Configuració de flash:
- Flash Size: `4MB`
- Partition Scheme: `Default 4MB with spiffs`
- Upload Speed: `115200`

### 2.2 Llibreries necessàries (Library Manager)

| Llibreria | Autor | Per a |
| :--- | :--- | :--- |
| TFT_eSPI | Bodmer | Pantalla ILI9488 i control tàctil XPT2046 |
| HX711 | Bogdan Necula (bogde) | Bàscula gravimètrica |
| RTClib | Adafruit | DS3231 RTC |
| AccelStepper | Mike McCauley | Motor pas a pas |
| ArduinoJson (v7.x) | Benoît Blanchon | Comunicació Supabase |

### 2.3 Configuració TFT_eSPI per ILI9488 i Tàctil

Obrir el fitxer `User_Setup.h` de la llibreria TFT_eSPI (o crear un de propi a `User_Setup_Select.h`) i establir:

```cpp
#define ILI9488_DRIVER

#define TFT_MISO  19
#define TFT_MOSI  23
#define TFT_SCLK  18
#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST    4

// Habilitar el xip tàctil XPT2046 compartint el bus SPI
#define TOUCH_CS  21

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000  // Freqüència segura per estabilitat tàctil

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define SMOOTH_FONT
```

---

## 3. Fases de Prova Aïllada

---

### FASE 1 — Bàscula gravimètrica (HX711) + Calibració ⚖️

**Objectiu**: Obtenir lectures de pes estables i calibrar el factor de conversió per a la dispensació gravimètrica.

#### Cablejat

| HX711 | ESP32 |
| :--- | :--- |
| VCC | 5V |
| GND | GND |
| DT | GPIO 16 |
| SCK | GPIO 17 |

La cèl·lula de càrrega es connecta als terminals `E+`, `E-`, `A+`, `A-` de la placa HX711.

#### Sketch de prova

Carregar `TestesIsolats/Test_HX711.ino`. El sketch ha de mostrar lectures contínues pel Monitor Sèrie.

#### Procediment de calibració (IMPORTANT)

La calibració és **obligatòria** abans d'integrar el motor. Sense un factor correcte, el bucle de dispensació gravimètrica no s'aturarà a la dosi correcta.

**Pas 1 — Tara en buit:**
```
- Assegurar que el plat de pesatge està buit.
- Enviar 't' pel Monitor Sèrie.
- El sketch registra el valor raw del zero (offset).
```

**Pas 2 — Aplicar pes de referència:**
```
- Col·locar un pes conegut i precís al plat (recomanat: 100g o 200g).
- Anotar el valor raw que mostra el Monitor Sèrie.
```

**Pas 3 — Calcular el factor:**
```
Factor = Valor_RAW_llegit / Pes_de_referència_en_grams

Exemple:
  - Pes de referència: 100 g
  - Valor RAW llegit: 42350
  - Factor = 42350 / 100 = 423.50
```

**Pas 4 — Actualitzar `config.h`:**
```cpp
// A config.h:
#define HX711_CALIBRATION_FACTOR  423.50   // ← substituir pel valor calculat
```

**Pas 5 — Verificació:**
```
- Retirar el pes, tornar a tarar.
- Col·locar pesos de 50g, 100g i 150g successivament.
- Cada lectura hauria de tenir un error < 2g (< 2%).
```

#### Criteri d'èxit ✅
- Lectures estables (variació < 1g en repòs).
- Error < 2% amb pesos de referència independents.
- `HX711_CALIBRATION_FACTOR` actualitzat i confirmat a `config.h`.

---

### FASE 2 — Pantalla TFT ILI9488 SPI 🖥️

**Objectiu**: Inicialitzar la pantalla gràfica i verificar la correcta representació visual.

#### Cablejat

Connectar la pantalla TFT a l'ESP32 segons el mapa de la Secció 1 (SPI compartit, CS a GPIO 15, DC a GPIO 2, RST a GPIO 4, LED a GPIO 32 o 3V3).

#### Sketch de prova

Compilar i carregar `Display.ino` de forma aïllada (o el sketch TFT_eSPI `graphicstest`).

#### Verificació

- La pantalla s'encén i renderitza formes i text sense cap tipus d'artifacte gràfic.
- Es comprova la brillantor del backlight.

#### Criteri d'èxit ✅
- Text legible i imatges renderitzades correctament.
- Pantalla inicialitzada completament sense errors al Monitor Sèrie.

---

### FASE 3 — Pantalla Tàctil (XPT2046) com a Botonera 🔘

**Objectiu**: Detectar correctament els tocs a la pantalla i mapejar-los a botons virtuals frontals (com ara "Tara" i "Dispensa Manual") sense polsadors físics.

#### Cablejat

| Pin Tàctil | ESP32 / Bus SPI |
| :--- | :--- |
| T_CS | GPIO 21 |
| T_CLK | GPIO 18 (SCLK compartit) |
| T_DIN | GPIO 23 (MOSI compartit) |
| T_DO | GPIO 19 (MISO compartit) |

#### Sketch de prova

Carregar l'esbós `TestesIsolats/TouchTest/TouchTest.ino` o `probaMenuTouch.ino`. El programa demana tocar la pantalla per registrar les coordenades.

#### Mapeig de botons i cal·libració
Mitjançant la llibreria `TFT_eSPI`, s'ha de realitzar una calibració per corregir la rotació i desplaçament dels eixos.
1. Carregar l'esbós de calibració.
2. Tocar els 4 cantons de la pantalla quan es demani.
3. Emmagatzemar o introduir el bloc `touchCalibrationData` obtingut al fitxer de configuració per a un funcionament precís.

#### Verificació

El Monitor Sèrie ha d'imprimir les coordenades (X, Y) reals del toc i identificar el botó virtual premut:
```
[TÀCTIL] Toc detectat a X: 120, Y: 240
[TÀCTIL] Acció detectada: BOTO_TARE
```

#### Criteri d'èxit ✅
- Lectura de coordenades correcta i repetible.
- Mapeig exacte de les zones de la pantalla tàctil que actuen com a botons de l'alimentador (FEED i TARE).
- Absència de polsadors físics externs en el disseny.

---

### FASE 4 — Rellotge en temps real DS3231 (I2C Personalitzat) 🕐

**Objectiu**: Establir l'hora correcta i verificar que el DS3231 la reté utilitzant pins I2C personalitzats (SDA=22, SCL=33).

#### Cablejat

| DS3231 | ESP32 |
| :--- | :--- |
| VCC | 3V3 |
| GND | GND |
| SDA | **GPIO 22** |
| SCL | **GPIO 33** |

#### Inicialització per Codi (CRÍTIC)
Atès que no s'utilitzen els pins I2C per defecte de l'ESP32, cal reconfigurar el bus `Wire` en fer la inicialització. A l'inici del programa (`setup()`):

```cpp
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  
  // Inicialitza l'I2C amb els pins personalitzats
  Wire.begin(22, 33);
  
  if (!rtc.begin()) {
    Serial.println("No s'ha trobat el mòdul RTC!");
    while (1);
  }
}
```

#### Sketch de prova

Carregar `TestesIsolats/Test_RTC.ino` modificat amb els nous pins. El sketch ha d'ajustar l'hora i mantenir-la després d'apagar i tornar a alimentar el sistema.

#### Criteri d'èxit ✅
- L'hora es sincronitza correctament i no es perd en treure el corrent.
- Cap error d'inicialització I2C.

---

### FASE 5 — Motor pas a pas NEMA 17 + Driver DRV8825 ⚙️

**Objectiu**: Controlar el moviment de pas a pas de forma segura i assegurar que el driver s'apaga en repòs.

#### Cablejat

| DRV8825 | ESP32 / Alimentació |
| :--- | :--- |
| EN | GPIO 25 |
| STEP | GPIO 26 |
| DIR | GPIO 27 |
| VMOT | 12V (font externa amb condensador 100µF) |
| GND | GND compartit amb ESP32 |

#### Sketch de prova

Carregar `TestesIsolats/Test_Motor.ino`. El motor ha de fer girs cap a una direcció i posteriorment cap a l'altra, alliberant el parell (desactivant EN=HIGH) en repòs.

#### Criteri d'èxit ✅
- Gir fluid sense pèrdua de passos.
- La línia EN desactiva correctament el consum i el corrent del driver en repòs.

---

### FASE 6 — Connexió WiFi i API Supabase (Network) 🌐

**Objectiu**: Connectar-se a la xarxa local i comunicar la telemetria i estat amb la base de dades al núvol.

#### Sketch de prova

Carregar `TestesIsolats/Test_Network.ino` introduint les credencials del WiFi i les claus de Supabase a `config.h`.

#### Criteri d'èxit ✅
- Connexió reeixida i transmissió d'esdeveniments cap al núvol.

---

## 4. FASE 7 — Integració Final del Sistema Complet 🔗

Una vegada totes les fases han estat superades, es procedeix a la unificació total a la carpeta `Software/arxiusINOproba/programesProba/CatFeeder/`.

### 4.1 Estructura Modular Final del Projecte

El projecte queda reduït a 7 fitxers, eliminant components innecessaris per a una major eficiència i robustesa:

```
CatFeeder/
├── CatFeeder.ino       ← Orquestrador:setup(), loop(), màquina d'estats
├── config.h            ← Pins (SDA=22, SCL=33, T_CS=21), calibració, credencials
├── Display.ino         ← Renderitzat gràfic + Dibuix dels botons virtuals
├── TouchManager.ino    ← Captura de tocs SPI, filtratge i gestió de botons de pantalla
├── Motor.ino           ← Control AccelStepper (startDispense / motorStop)
├── Scale.ino           ← Gestió de pes del sensor HX711
├── RTCManager.ino      ← Manteniment horari DS3231 (I2C SDA=22, SCL=33)
└── Network.ino         ← WiFi + Enllaç API REST a Supabase
```

### 4.2 Lògica de la Màquina d'Estats i Dispensació Gravimètrica

En el loop central no-bloquejant:
1. El sistema roman en `STATE_IDLE`. La pantalla mostra la interfície del disseny.
2. Si l'usuari **toca la zona virtual de la pantalla tàctil** configurada com a "FEED", o arriba una ordre des de Supabase, o el RTC activa un horari programat:
   - S'executa `scale.tare()` per fixar el punt zero.
   - S'activa el motor (`EN = LOW`).
   - S'entra a `STATE_DISPENSING`.
3. Durant la dispensació, el loop crida constantment a `stepper.run()` de forma no-bloquejant. 
4. Quan la bàscula llegeix que el pes és igual o superior al programat (menys la tolerància), el motor es frena a l'acte i `EN` passa a `HIGH` per seguretat.
5. Es registra l'esdeveniment correctament enviant-lo via HTTPS a Supabase.

---

## 5. Problemes Coneguts i Solucions

| Problema | Causa probable | Solució |
| :--- | :--- | :--- |
| **La pantalla no respon als tocs** | Conflicte de bus o pin `TOUCH_CS` mal definit | Comprovar que `TOUCH_CS` és el pin 21 a `User_Setup.h`. |
| **Lectures errònies del tàctil** | Pantalla no calibrada o eix invertit | Calibrar de nou i guardar el array de dades exactes a `config.h`. |
| **El RTC no es troba (RTC Init Error)**| Pins I2C per defecte de l'ESP32 no reassignats | Assegurar que es fa `Wire.begin(22, 33)` abans de fer `rtc.begin()`. |
| **El motor fa soroll però no gira** | Vref insuficient | Ajustar finament el cargol del potenciòmetre del driver DRV8825. |

---

## 6. Checklist de Lliurament

- [ ] **Fase 1 (Bàscula)**: Calibració realitzada i `HX711_CALIBRATION_FACTOR` actualitzat.
- [ ] **Fase 2 (Pantalla)**: Text i imatges mostrats a la ILI9488 sense artifacts.
- [ ] **Fase 3 (Tàctil)**: Botons virtuals actuen com a botons de servei (FEED i TARE) correctament i el tàctil està calibrat.
- [ ] **Fase 4 (RTC)**: Inicialització amb `Wire.begin(22, 33)` completada i manteniment horari vàlid.
- [ ] **Fase 5 (Motor)**: NEMA 17 funciona i el driver es desactiva en repòs.
- [ ] **Fase 6 (Network)**: Telemetries i polling amb el backend de Supabase funcionals.
- [ ] **Fase 7 (Integració)**: Màquina d'estats del CatFeeder lliure de polsadors físics.
- [ ] **GitHub**: Codi desat a la branca d'integració llest per a validació final.

---
*Integrated Project II — GR15 [GEMEC-09UV] · Universitat de Vic — UCC · 2025–2026*
