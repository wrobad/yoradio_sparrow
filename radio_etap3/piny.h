// =====================================================================
//   WSZYSTKIE PINY RADIA W JEDNYM MIEJSCU  (ESP32-S3-N8R2)
//   Zmieniasz podłączenie -> zmieniasz tylko ten plik.
// =====================================================================
#pragma once

// ---------- Ekran główny 2.8" ILI9341 z dotykiem - TAŚMA 12 żył ----------
// Taśma wpięta w 12 kolejnych pinów prawej listwy: 16 17 18 8 3 46 9 10 11 12 13 14
// (VCC -> 3.3, GND -> GND osobnymi przewodami)
//   pin ekranu:  CS  RESET DC  SDI(MOSI) SCK  LED  SDO(MISO) T_CLK T_CS T_DIN T_DO T_IRQ
//   GPIO:        16  17    18  8         3    46   9         10    11   12    13   14
#define PIN_TFT_CS    16
#define PIN_TFT_RST   17
#define PIN_TFT_DC    18
#define PIN_MOSI       8
#define PIN_SCK        3
#define PIN_TFT_BL    46     // podświetlenie (LED), świeci stanem wysokim
#define PIN_MISO       9
// Dotyk XPT2046 - własne piny (programowe SPI)
#define PIN_T_CLK     10
#define PIN_T_CS      11
#define PIN_T_DIN     12
#define PIN_T_DO      13
#define PIN_T_IRQ     14     // nieużywany

// ---------- Ekran 2: podłużny 2.25" 76x284 ST7789 ----------
#define P2_SCK        41     // SCL
#define P2_MOSI       42     // SDA
#define P2_RST        39
#define P2_DC         40
#define P2_CS         38
#define P2_BL         47     // świeci stanem NISKIM (tranzystor PNP na module)

// ---------- Dźwięk: PCM5102 (I2S) ----------
#define I2S_BCLK       5     // BCK
#define I2S_LRC        6     // LRCK
#define I2S_DOUT       7     // DATA

// ---------- Enkoder 1 - strojenie ----------
#define ENC1_A         2     // S1 / CLK
#define ENC1_B         1     // S2 / DT
#define ENC1_SW       21     // KEY / SW

// ---------- Enkoder 2 - głośność ----------
#define ENC2_A        35     // S1 / CLK
#define ENC2_B        36     // S2 / DT
#define ENC2_SW       37     // KEY / SW

// ---------- Wolne ----------
// 4, 15  (np. odbiornik IR, czujnik światła)
//
// Nie używać: 0, 45 (piny startowe), 19, 20 (USB), 43, 44 (Serial), 48 (dioda RGB na płytce)
