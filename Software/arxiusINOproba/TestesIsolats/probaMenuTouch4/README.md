# 🐱 Guia de Resolució i Calibració del Tàctil (320x240 px)

Aquest document detalla el diagnòstic complet del comportament erràtic del tàctil XPT2046 a la pantalla ILI9341 de 2,8 polzades, explicant el motiu elèctric dels errors i oferint la guia de connexió i programari definitiva per fer-lo funcionar perfectament.

---

## 🔍 1. El Diagnòstic (Què està passant?)

Durant les proves s'han observat els següents símptomes al Serial Monitor:
```text
[touch] px=(302,226) rawZ=61447
[check] toc px=(302,226)  esperat=TL zona x=0..70 y=0..70
[unlock] fora de zona
```

Aquestes dades ens donen dues proves clares de problemes de maquinari:

1. **Desbordament elèctric (`rawZ = 61447`):**
   El controlador tàctil XPT2046 llegeix la pressió amb un convertidor analògic-digital de **12 bits**, de manera que el valor màxim real és **`4095`**. Una lectura de `61447` indica que la línia SPI està rebent brossa o un senyal continu de nivell alt (tot uns lògics), causat per un pin flotant o un curtcircuit elèctric d'un altre bus.
   
2. **Coordenades bloquejades a la cantonada inferior dreta (`px = (302, 226)`):**
   Tot i tocar diferents punts de la pantalla, les coordenades rebudes no canvien gaire (sempre al voltant de `x ≈ 302, y ≈ 226`). Això passa perquè l'ESP32 no s'està comunicant realment amb el tàctil, sinó que interpreta el soroll elèctric del bus de forma constant.

---

## ⚡ 2. La Causa: Conflicte amb el bus I2C (RTC DS3231)

A la configuració inicial de pins tenies definit:
```cpp
#define TFT_DC 22
#define TOUCH_CS 21
```

* **GPIO 21** és la línia física **SDA** (I²C Data) del rellotge RTC DS3231.
* **GPIO 22** és la línia física **SCL** (I²C Clock) del mateix RTC.

El xip del tàctil és actiu per nivell baix (**Active-LOW**). Cada vegada que el bus I²C de l'RTC transmet o es manté actiu, força aquestes línies a `0` lògic, activant involuntàriament el xip del tàctil en moments aleatoris i inundant el bus SPI de soroll i dades corruptes.

---

## 🛠️ 3. La Solució: Reassignació i Cablejat Correcte

Per evitar interferències amb el bus de l'RTC, hem d'alliberar completament els pins **GPIO 21 i 22** i connectar la pantalla i el tàctil a pins totalment segurs i lliures.

### 📋 Taula de Connexions Definitiva (ESP32 ↔ Pantalla)

| Pad de la Pantalla | ESP32 GPIO | Funció configurada | Estat / Compartit |
| :--- | :--- | :--- | :--- |
| **`T_CS`** (Touch CS) | **GPIO 5** | Selecció de xip del Tàctil | **Lliure d'I2C** |
| **`CS`** (Display CS) | **GPIO 15** | Selecció de xip del Display | Lliure de conflictes |
| **`DC/RS`** (Data/Command)| **GPIO 2** | Control de registre display | Lliure de conflictes |
| **`RESET`** | **GPIO 4** | Reinici físic del display | Lliure de conflictes |
| **`MISO` / `T_DO`** | **GPIO 19** | Dades SPI (Sortida) | Compartit SPI |
| **`MOSI` / `T_DIN`** | **GPIO 23** | Dades SPI (Entrada) | Compartit SPI |
| **`SCK` / `T_CLK`** | **GPIO 18** | Rellotge SPI | Compartit SPI |

---

## 💻 4. Configuració del Programari

### A. Actualitzar `User_Setup.h` (TFT_eSPI)
Obre el fitxer de configuració de la llibreria a `/Users/butifarrii/Documents/Arduino/libraries/TFT_eSPI/User_Setup.h` i configura exactament aquests pins:

```cpp
#define ILI9341_DRIVER  // Assegura't que el controlador ILI9341 està actiu

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST   4

#define TOUCH_CS  5  // Assignat al pin 5 lliure de conflictes!
```

---

## 🎯 5. Mètode de Calibratge Pas a Pas

Un cop fets els canvis de maquinari i programari explicats a sobre, el tàctil començarà a mesurar correctament. Segueix aquests passos per deixar-lo perfectament calibrat:

### Pas 1: Executar el calibratge natiu de `TFT_eSPI`
1. Obre el projecte de calibratge que es troba a: [Calibrate_Touch.ino](file:///Users/butifarrii/Desktop/CatFeeder-T5/Software/arxiusINOproba/TestesIsolats/Calibrate_Touch/Calibrate_Touch.ino).
2. Puja'l a l'ESP32 i **obre el Serial Monitor a 115200 baud**.
3. A la pantalla et sortirà una mira de color vermell. Toca amb cura el centre exacte de la mira a les 4 cantonades successives per calibrar el rang real.
4. El programa comprovarà el tàctil i imprimirà una línia com aquesta pel Serial Monitor:
   ```cpp
   #define TOUCH_CAL_DATA          { 281, 3535, 347, 3583, 7 }
   ```
5. Copia aquests 5 números generats per a la teva pantalla física.

### Pas 2: Desar la calibració a `probaMenuTouch4.ino`
1. Obre el fitxer principal del menú: [probaMenuTouch4.ino](file:///Users/butifarrii/Desktop/CatFeeder-T5/Software/arxiusINOproba/TestesIsolats/probaMenuTouch4/probaMenuTouch4.ino).
2. Substitueix els valors de la línia **58** pels valors obtinguts al Pas 1:
   ```cpp
   #define TOUCH_CAL_DATA         { 281, 3535, 347, 3583, 7 } // Substitueix amb els teus valors!
   ```
3. Comprova que `#define USE_NATIVE_CALIBRATION` està configurat a `true` (línia 57). Això fa que el codi utilitzi els controladors natius de la llibreria `TFT_eSPI`, evitant qualsevol mena d'error amb rotacions o eixos invertits.
4. Puja el programa i ja podràs interactuar fluidament amb la interfície tàctil i el patró de desbloqueig!
