// =====================================================================
//   WSZYSTKIE PINY RADIA W JEDNYM MIEJSCU  (ESP32-S3-N8R2)
//   Zmieniasz podłączenie -> zmieniasz tylko ten plik.
// =====================================================================
#pragma once

// ---------- Ekran główny 2.8" ILI9341 z dotykiem - TAŚMA 11 żył ----------
// Taśma wpięta w 11 kolejnych pinów prawej listwy: 16 17 18 8 3 46 9 10 11 12 13
// (VCC -> 3.3, GND -> GND osobnymi przewodami, T_IRQ niepodłączony)
//   pin ekranu:  CS  RESET DC  SDI(MOSI) SCK  LED  SDO(MISO) T_CLK T_CS T_DIN T_DO
//   GPIO:        16  17    18  8         3    46   9         10    11   12    13
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

// ---------- Ekran 2: podłużny 2.25" 76x284 ST7789 ----------
// 6 kolejnych pinów lewej listwy: 41 40 39 38 37 36  (BL CS DC RST SDA SCL)
#define P2_BL         41     // świeci stanem NISKIM (tranzystor PNP na module)
#define P2_CS         40
#define P2_DC         39
#define P2_RST        38
#define P2_MOSI       37     // SDA
#define P2_SCK        36     // SCL

// ---------- Dźwięk: PCM5102 (I2S) ----------
#define I2S_BCLK       7     // BCK
#define I2S_DOUT      15     // DATA
#define I2S_LRC       14     // LRCK

// ---------- Enkoder 1 - strojenie ----------
// 3 kolejne piny lewej listwy: 42 2 1  (KEY S2 S1)
#define ENC1_A         1     // S1 / CLK
#define ENC1_B         2     // S2 / DT
#define ENC1_SW       42     // KEY / SW

// ---------- Enkoder 2 - głośność ----------
// 3 kolejne piny prawej listwy: 4 5 6  (KEY S2 S1)
#define ENC2_A         6     // S1 / CLK
#define ENC2_B         5     // S2 / DT
#define ENC2_SW        4     // KEY / SW

// ---------- Wolne ----------
// 35 (np. CS drugiego podłużnego ekranu), 21, 47
//
// Nie używać: 0, 45 (piny startowe), 19, 20 (USB), 43, 44 (Serial), 48 (dioda RGB na płytce)
