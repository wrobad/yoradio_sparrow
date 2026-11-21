// WZÓR listy stacji.
// Swoją listę trzymaj w pliku "stacje.h" (skopiuj ten plik jako stacje.h).
// Jeśli stacje.h nie istnieje, radio użyje tej przykładowej listy.
// stacje.h jest w .gitignore, więc aktualizacje z GitHuba go nie nadpiszą.
//
//   {"Nazwa na ekranie", "http://adres.strumienia"},   <- stacja (przecinek na końcu!)
//   GRUPA("Nazwa grupy"),                               <- nagłówek grupy na liście
#pragma once

const Stacja STACJE[] = {
  GRUPA("Jazz i klasyka"),
  {"Radio Swiss Jazz",    "http://stream.srg-ssr.ch/m/rsj/mp3_128"},
  {"Radio Swiss Classic", "http://stream.srg-ssr.ch/m/rsc_de/mp3_128"},
};

// Ulubione: numery stacji (jak na liście, od 1) dla zakładki "Ulubione", maks. 6.
#define ULUBIONE_ZDEFINIOWANE
const int ULUBIONE[] = {1, 2};
