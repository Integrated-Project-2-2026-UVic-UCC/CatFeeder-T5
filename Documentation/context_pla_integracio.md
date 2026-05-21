# Context de Projecte i Pla d'Integració: CatFeeder-T5 (ESP32)

Aquest document ha estat dissenyat per servir de **context complet per a Claude** per tal que pugui dissenyar, implementar i guiar-te pas a pas en les fases de prova individual de perifèrics i la seva posterior integració en el sistema complet del **CatFeeder-T5**.

---

## 1. Objectiu del Projecte

L'objectiu és construir un **alimentador automàtic per a gats** gravimètric controlat per un **ESP32 (WROOM-32)**. El sistema no només dispensa menjar basant-se en hores programades (RTC local i configuració del núvol), sinó que calcula la quantitat de pinso exacta en temps real utilitzant una cèl·lula de càrrega. També envia telemetria de pes, i rep ordres en temps real des d'una aplicació web mitjançant una base de dades **Supabase (via WiFi)**.

En aquest disseny de maquinari de darrera generació:
- **No s'utilitzen botons físics frontals**. La pròpia **pantalla tàctil** actua com a botonera interactiva de control local.
- **S'ha eliminat el sensor DHT22** per a simplificar el cablejat.
- **Els pins I2C per al DS3231 RTC han estat moguts** a pins lliures no estàndards per evitar qualsevol interferència física.

---

## 2. Disseny de Programari i Arquitectura del Firmware

El firmware principal està ubicat a la carpeta `Software/arxiusINOproba/programesProba/` i està modularitzat en diverses pestanyes d'Arduino per separar la lògica de cada perifèric:

- **`CatFeeder.ino`**: L'orquestrador principal. Conté el `setup()` i el `loop()`. El loop és **estrictament no-bloquejant** (utilitza temporitzadors basats en `millis()`) per garantir que el motor es mogui de manera fluida i la pantalla respongui de manera immediata als tocs de l'usuari.
- **`config.h`**: Conté totes les definicions globals, credencials de WiFi, claus de Supabase, calibres de la bàscula, llindars de velocitat del motor i, el més important, el **mapa de pins de la placa**.
- **`Display.ino`**: Gestiona la pantalla TFT (ILI9488 SPI). A part de mostrar els menús i informació, defineix visualment les àrees dels botons tàctils frontals.
- **`TouchManager.ino`**: Gestiona la inicialització del xip XPT2046 i descodifica les coordenades de pressió reals en botons lògics (FEED, TARA).
- **`Motor.ino`**: Controla el motor pas a pas NEMA 17 a través del controlador DRV8825 utilitzant la llibreria `AccelStepper`. El control és gravimètric (el motor gira fins que la cèl·lula llegeix el pes desitjat).
- **`Scale.ino`**: Gestiona la bàscula utilitzant el xip convertidor ADC HX711 de 24 bits. Inclou rutines de calibració i tara automàtica.
- **`RTCManager.ino`**: Interactua amb el xip DS3231 via I2C personalitzat per mantenir l'hora exacta i avaluar els horaris de menjada.
- **`Network.ino`**: Administra la connexió WiFi i realitza les peticions REST a Supabase en format JSON per fer el seguiment en viu de les ordres, telemetries i configuracions.

---

## 3. Mapa de Pins Mestre (Auditat i Sense Conflictes)

Per tal d'evitar qualsevol col·lisió del SPI tàctil i alliberar correctament el canal de la pantalla, s'ha de configurar **exactament** el següent mapa de pins al maquinari i a `config.h`:

| Perifèric / Component | Senyal Físic | ESP32 GPIO | Estat / Cablejat |
| :--- | :--- | :--- | :--- |
| **Pantalla TFT (ILI9488 SPI)** | `MOSI` / `SDI` | **GPIO 23** | Compartit (Bus SPI) |
| | `MISO` / `SDO` | **GPIO 19** | Compartit (Bus SPI) |
| | `SCLK` / `SCK` | **GPIO 18** | Compartit (Bus SPI) |
| | `CS` (TFT CS) | **GPIO 15** | Control de Pantalla |
| | `DC` | **GPIO 2** | Selecció de dades |
| | `RESET` | **GPIO 4** | Reinici físic |
| | `LED` (Retroil·luminació) | **GPIO 32** | Control lluminós permanent |
| **Tàctil (XPT2046 SPI)** | `T_CS` (Touch CS) | **GPIO 21** | Control de panell tàctil |
| | `T_CLK` / `T_DIN` / `T_DO`| Compartits | Paral·lel als busos SPI 18, 23 i 19 |
| **Motor Pas a Pas (DRV8825)** | `EN` (Enable) | **GPIO 25** | Actiu a nivell BAIX |
| | `STEP` | **GPIO 26** | Polsos de moviment |
| | `DIR` | **GPIO 27** | Control de sentit de gir |
| **Bàscula (Amplificador HX711)**| `DT` (Dades) | **GPIO 16** | Dades de pes |
| | `SCK` (Rellotge) | **GPIO 17** | Rellotge de dades |
| **Rellotge I2C (DS3231)** | `SDA` | **GPIO 22** | **Bus I2C personalitzat (Dades)** |
| | `SCL` | **GPIO 33** | **Bus I2C personalitzat (Rellotge)** |

---

## 4. Full de Ruta per a Claude: Pla de Proves i Integració

> [!IMPORTANT]
> **Instruccions per a Claude**: Guia l'usuari seguint aquest ordre d'execució. Cada fase ha d'incloure el cablejat específic, les llibreries necessàries, un petit sketch de prova aïllat (molts d'ells es troben a `arxiusINOproba/TestesIsolats/`) i la confirmació de funcionament mitjançant el **Monitor Sèrie (115200 baud)**.

### FASE 1: Alimentació i Bàscula (HX711)
- **Objectiu**: Comprovar que l'amplificador HX711 respon i s'obtenen lectures estables de pes.
- **Passos**:
  1. Cablejar VCC (5V), GND, DT (16) i SCK (17).
  2. Carregar l'esbós `Test_HX711.ino` (o `Scale.ino`).
  3. Fer la tara en buit i calibrar amb un pes conegut.

### FASE 2: Pantalla Local (ILI9488 SPI)
- **Objectiu**: Inicialitzar la pantalla TFT de 3.5" i comprovar el rendiment visual.
- **Passos**:
  1. Configurar la llibreria `TFT_eSPI` a `User_Setup.h` amb la configuració de pins de la taula (MOSI=23, MISO=19, SCLK=18, CS=15, DC=2, RST=4, i molt important: `TOUCH_CS=21`).
  2. Compilar i carregar `Display.ino` per provar els gràfics.

### FASE 3: Controlador Tàctil XPT2046 (Botonera Virtual)
- **Objectiu**: Detectar correctament els tocs de l'usuari i calibrar les coordenades de la pantalla.
- **Passos**:
  1. Cablejar el pin `T_CS` al GPIO 21. Compartir els pins CLK, DIN i DO amb els de la pantalla.
  2. Carregar un sketch de proves tàctil (`probaMenuTouch` o `TouchTest.ino`).
  3. Calibrar els punts de toc de la pantalla i anotar el bloc de configuració resultant a `config.h`.

### FASE 4: Rellotge DS3231 (I2C Personalitzat)
- **Objectiu**: Configurar el RTC amb el bus I2C mogut a pins lliures (SDA=22, SCL=33).
- **Passos**:
  1. Cablejar SDA al pin 22 de l'ESP32 i SCL al pin 33.
  2. A l'inici del programa (`setup()`), inicialitzar el bus `Wire` forçant els pins personalitzats: `Wire.begin(22, 33)` abans d'invocar `rtc.begin()`.
  3. Validar que l'hora es manté després de desconnectar l'ESP32.

### FASE 5: Actuador de Dispensació (Motor NEMA 17 + DRV8825)
- **Objectiu**: Controlar el moviment de pas a pas de forma segura.
- **Passos**:
  1. Cablejar EN (25), STEP (26) i DIR (27). VMOT a 12V amb condensador de seguretat de 100µF.
  2. Utilitzar `Test_Motor.ino` per fer girar el motor de forma controlada cap a un sentit i l'altre, verificant que el senyal `EN` bloquegi el pas de corrent quan el motor està aturat per evitar sobreescalfament.

### FASE 6: Connexió WiFi i API Supabase (Network)
- **Objectiu**: Establir comunicació amb el núvol.
- **Passos**:
  1. Introduir les credencials del WiFi i les claus de Supabase a `config.h`.
  2. Provar que `Network.ino` es connecta a la xarxa, envia un *heartbeat* de prova i fa *polling* de les comandes de la base de dades.

---

## 5. FASE 7: Integració Final del Sistema Sencer

Una vegada s'hagi verificat cada component per separat, demana a Claude que t'ajudi a:

1. **Unificar tot el codi**: Ajuntar els 7 mòduls modulars a `CatFeeder.ino`.
2. **Validar la Màquina d'Estats**: Comprovar que el cicle de canvis d'estat funciona de forma robusta (`STATE_BOOT` -> `STATE_IDLE` -> `STATE_DISPENSING` -> `STATE_IDLE`/`STATE_ERROR`).
3. **Control Gravimètric de Menjar (Bucle Tancat)**:
   - Al prémer el botó virtual **FEED** a la pantalla tàctil o al rebre una ordre de Supabase:
     1. El sistema llegeix el pes inicial (tara).
     2. El motor comença a girar (`STEPPER_DISPENSE_SPEED`).
     3. A cada volta del `loop()`, es llegeix el sensor de pes (`telemetry.weightG`).
     4. En el moment que el pes dispensat arriba al pes desitjat, el motor s'atura immediatament, es desactiva el driver per a no consumir energia, i s'envia el log d'esdeveniment a Supabase.
     5. Si el motor triga més de 30 segons en arribar al pes objectiu (p. ex. per un embossament de pinso), el motor es frena immediatament per seguretat i es passa a `STATE_ERROR` amb l'avís "Motor timeout" a la pantalla i al núvol.
