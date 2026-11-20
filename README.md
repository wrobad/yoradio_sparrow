# yoradio_sparrow

Autorskie radio internetowe na ESP32-S3 (w stylu yoRadio).

## Sprzęt
- ESP32-S3-N8R2 (DevKitC-1, 8 MB flash, 2 MB PSRAM)
- PCM5102 (moduł PCM5102MK 2.0) – wyjście audio I2S
- Wyświetlacz 2.8" TFT 240x320 ILI9341 z dotykiem XPT2046 i slotem SD

## Połączenia
| Funkcja | GPIO |
|---|---|
| I2S BCK / LRCK / DATA | 5 / 6 / 7 |
| SPI MOSI / SCK / MISO (TFT, dotyk, SD) | 11 / 12 / 13 |
| TFT CS / DC / RESET / LED | 10 / 9 / 14 / 8 |
| Dotyk T_CS | 4 |
| SD_CS (planowane) | 21 |
| Enkoder A / B / SW (planowane) | 15 / 16 / 17 |
| IR (planowane) | 18 |

PCM5102 zasilany z 5V, wyświetlacz z 3V3.

## Szkice
- `test_wyswietlacza/` – test wyświetlacza i kalibracja dotyku
- `radio_etap1/` – etap 1: Wi-Fi, odtwarzanie strumienia, ekran ze stacją, tytułem i głośnością, przyciski dotykowe

## Biblioteki
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) (najnowsza wersja)

## Ustawienia Arduino IDE
Płytka: ESP32S3 Dev Module · PSRAM: QSPI PSRAM · Flash: 8MB · Partition Scheme: 8M with spiffs
