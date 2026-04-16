# TouchTest — sketch autònom per provar el tàctil ILI9488 / XPT2046

Aquest sketch **només** prova el controlador tàctil XPT2046 de la pantalla
3.5" SPI ILI9488 i imprimeix cada pulsació pel port sèrie. La pantalla fa
servir `TFT_eSPI` amb el seu suport integrat del XPT2046 — no cal cap
llibreria addicional.

## 1. Pre-requisits

- Placa ESP32 WROOM-32.
- Llibreria **TFT_eSPI** (Bodmer) instal·lada des del Library Manager.
- La mateixa configuració de display que al firmware principal.

## 2. `User_Setup.h` de TFT_eSPI

Obre el fitxer `User_Setup.h` de la llibreria TFT_eSPI (o el teu
`User_Setup_Select.h` personalitzat). A la secció del driver, assegura't
que hi tens:

```cpp
#define ILI9488_DRIVER
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4

// --- NOU: suport del panell tàctil XPT2046 ------------------------
#define TOUCH_CS            21       // qualsevol GPIO lliure
#define SPI_TOUCH_FREQUENCY 2500000  // 2.5 MHz és prou segur
```

> ⚠️ Si després vols integrar el tàctil al firmware principal (`CatFeeder/`),
> recorda que al `config.h` d'aquell projecte el **GPIO 21** s'està fent
> servir com a `I2C_SDA` del DS3231. Mou el `TOUCH_CS` a un pin lliure
> (per exemple GPIO 0 o 12 amb cura pels *strapping*) o reassigna l'I²C.

## 3. Cablejat del mòdul tàctil

Els mòduls ILI9488 de 3.5" porten 5 pins addicionals per al tàctil. Els
pins de rellotge/MOSI/MISO es **comparteixen** amb els del display; només
cal un CS propi.

| Pad al mòdul | ESP32     | Notes                              |
|--------------|-----------|------------------------------------|
| `T_CS`       | GPIO 21   | ha de coincidir amb `TOUCH_CS`     |
| `T_CLK`      | GPIO 18   | mateix que `TFT_SCLK`              |
| `T_DIN`      | GPIO 23   | mateix que `TFT_MOSI`              |
| `T_DO`       | GPIO 19   | mateix que `TFT_MISO`              |
| `T_IRQ`      | — (lliure) | no s'utilitza en mode polling     |

## 4. Com l'executes

1. Obre `TouchTest.ino` a l'Arduino IDE.
2. Selecciona *ESP32 Dev Module* i el port sèrie correcte.
3. Compila i flasheja.
4. Obre el **Serial Monitor a 115200 baud**.
5. Toca la pantalla.

## 5. Sortida esperada

Amb la pantalla en repòs veuràs una línia cada 2 s:

```
idle   baseline Z=27
idle   baseline Z=31
```

Quan toquis la pantalla:

```
#00001  TOUCH  raw=(1782,2154)  Z=1342  px=(172,128)
#00002  TOUCH  raw=(1784,2156)  Z=1405  px=(172,129)
#00003  TOUCH  raw=(1790,2180)  Z=1510  px=(173,131)
```

- `raw` són les coordenades directes del XPT2046 (rang típic ~200–3900).
- `Z` és la pressió — si és molt baixa, prem una mica més fort.
- `px` són les coordenades mapejades a píxels del display fent servir la
  calibració per defecte del sketch (`calData`). No són precises fins que
  no facis una calibració real.

## 6. Calibració precisa (opcional)

Si vols coordenades en píxels exactes, executa l'exemple
**`Touch_calibrate`** que ve amb la llibreria TFT_eSPI (File → Examples →
TFT_eSPI → Generic → Touch_calibrate). El sketch et demanarà tocar les
quatre cantonades i imprimirà pel sèrie quelcom com:

```
uint16_t calData[5] = { 281, 3535, 347, 3583, 7 };
```

Copia aquests 5 valors a la variable `calData` del `TouchTest.ino` i torna
a compilar.

## 7. Troubleshooting ràpid

| Símptoma                                             | Causa probable                                       |
|------------------------------------------------------|------------------------------------------------------|
| Només surt `idle baseline Z=...`, mai puja en tocar  | `TOUCH_CS` mal definit o no declarat a `User_Setup.h`|
| `Z` sempre dispara a ~4095 (com si estigués premut)  | `T_CS` no connectat o flotant                        |
| La pantalla es queda en blanc                        | Error al User_Setup del display (anterior al tàctil) |
| `px=(no-cal)` o pixels estranys                      | Falta calibració; és normal fins que l'executis      |
| Compila però no arrenca                              | Pins `TOUCH_CS` en conflicte amb altres perifèrics   |
