// WZÓR listy stacji.
// Swoją listę trzymaj w pliku "stacje.h" (skopiuj ten plik jako stacje.h).
// Jeśli stacje.h nie istnieje, radio użyje tej przykładowej listy.
// stacje.h jest w .gitignore, więc pobranie nowej wersji z GitHuba
// nie nadpisze Twoich stacji.
//
// Format jednej stacji (przecinek na końcu linii):
//   {"Nazwa na ekranie", "http://adres.strumienia"},
#pragma once

const Stacja STACJE[] = {
  {"Radio Swiss Jazz",    "http://stream.srg-ssr.ch/m/rsj/mp3_128"},
  {"Radio Swiss Classic", "http://stream.srg-ssr.ch/m/rsc_de/mp3_128"},
};
