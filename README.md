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
| Enkoder CLK / DT / SW (np. KY-040, zasilanie 3V3) | 15 / 16 / 17 |
| IR (planowane) | 18 |
| Ekran 2 (2.25" ST7789 76x284, SPI3): SCL / SDA / RST / DC / CS / BL | 41 / 42 / 39 / 40 / 38 / 47 |

PCM5102 zasilany z 5V, wyświetlacz z 3V3.

## Szkice
- **`radio_etap2/` – aktualna wersja.** Ekran z 6 zakładkami: Radio, Lista stacji (z grupami, przewijana palcem), Ulubione (6 kafelków), Wskaźnik VU (analogowa tarcza z dwiema igłami L/P), Zegar (duża godzina i data), Ustawienia (informacje, suwak jasności, wyłącznik czasowy 15/30/60 min). Zakładki zmienia się ikonami na dole ekranu albo podwójnym kliknięciem enkodera.
- `test_ekranu_2/` – test drugiego, podłużnego ekranu 2.25" 76x284 (ST7789) na SPI3: ramka na krawędziach, korekta przesunięcia z Serial Monitora
- `test_wyswietlacza/` – test wyświetlacza i kalibracja dotyku
- `radio_etap1/` – radio: Wi-Fi, odtwarzanie strumienia, ekran z paskiem stanu (zegar NTP, format/bitrate, numer stacji, sygnał Wi-Fi), nazwą stacji, przewijanym tytułem, analizatorem widma (15 pasm), paskiem głośności/jasności i przyciskami dotykowymi (`ekran.h`); podświetlenie PWM na GPIO 8 (`podswietlenie.h`); enkoder (`enkoder.h`): obrót = wybór stacji, klik = tryb głośności, długie naciśnięcie = tryb jasności

## Pierwsze uruchomienie – dane Wi-Fi
W folderze `radio_etap1` skopiuj `sekrety_przyklad.h` jako **`sekrety.h`** i wpisz w nim nazwę i hasło sieci.
Plik `sekrety.h` jest w `.gitignore`, więc hasło nigdy nie trafi do repozytorium.

## Lista stacji
Stacje dopisujesz w pliku **`stacje.h`** w folderze `radio_etap1` (na start skopiuj `stacje_przyklad.h` jako `stacje.h`).
Jedna linia = jedna stacja: `{"Nazwa", "http://adres.strumienia"},`
W `radio_etap2` dodatkowo: `GRUPA("Nazwa grupy"),` tworzy nagłówek na liście, a `ULUBIONE` (numery stacji) wybiera 6 kafelków w zakładce Ulubione.
Plik `stacje.h` jest w `.gitignore`, więc aktualizacje z GitHuba go nie nadpiszą. Bez niego radio użyje listy przykładowej.

## Kalibracja dotyku
Przy pierwszym starcie (bez pliku `kalibracja.h`) radio poprosi o dotknięcie strzałek w rogach i wypisze w Serial Monitorze linię `Kalibracja: {...}`.
Skopiuj `kalibracja_przyklad.h` jako **`kalibracja.h`** i wpisz tam te liczby. Plik jest w `.gitignore`.

## Biblioteki
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) (najnowsza wersja)

## Ustawienia Arduino IDE
Płytka: ESP32S3 Dev Module · PSRAM: QSPI PSRAM · Flash: 8MB · Partition Scheme: 8M with spiffs
