// WZÓR pliku z kalibracją dotyku.
// Skopiuj jako "kalibracja.h" i wpisz liczby z Serial Monitora
// (linia "Kalibracja: {...}" pojawia się po kalibracji przy starcie).
// Same zera = kalibracja przy każdym starcie.
// kalibracja.h jest w .gitignore - aktualizacje go nie nadpiszą.
#pragma once

uint16_t KALIBRACJA[8] = {0, 0, 0, 0, 0, 0, 0, 0};
