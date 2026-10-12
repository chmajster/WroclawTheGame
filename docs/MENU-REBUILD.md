# Menu Wrocław — wykonanie

## Audyt

UE 5.8 / C++20, UMG, Slate, Enhanced Input i CommonUI są już w projekcie.
`PlayerMenuWidget.cpp` buduje duży interfejs programowo: siedem zakładek,
studio postaci, mapę sektorów i podstawowe ustawienia. Start Nadodrza pomija
menu przez `GeoPreviewGameMode::BeginPlay`. `SliceController` steruje pauzą,
modalami kampanii i kreatorem. `DistrictSaveSubsystem` ma walidowany,
atomowo zastępowany slot dzielnicy; `SliceMission` przechowuje kampanię.
Istnieją `UGameUserSettings`, ustawienia audio i profil wejścia. Overrides
klawiszy profilu nie są stosowane do mapowań i nie są trwale zapisywane.
Mapa sektorów nie odpowiada ulicom `DistrictRegistry`. HUD jest osobny.

## Kierunek i podział wskazany w zleceniu

1. Audyt i kontrakty integracji — ten dokument.
2. Wspólne komponenty Slate: typografia, przyciski, panele, suwaki, dialogi,
   focus, nawigacja; nowy widget UMG stanowi cienką warstwę viewportu.
3. Menu główne i stan frontend: rzeczywista scena Nadodrza, siedem opcji,
   sprawdzanie zapisu, bezpieczne rozpoczęcie/odczyt, powrót bez przeładowania.
4. Pauza: sześć zakładek, mapa ulic z pozycją/POI/pan/zoom, prawdziwe dane
   postaci i sesji, zapis/odczyt i potwierdzenia.
5. Grafika/audio/sterowanie: działające kontrolki, trwałość, remapping,
   kontroler, bezpieczne zatwierdzanie rozdzielczości.
6. Filmowe ujęcia, animacje, dźwięki, ekran ładowania bez fikcyjnego procentu,
   ograniczenie pracy w menu i utrata focusu.
7. Natywne testy menu, regresja gry, render i ocena układu przy różnych
   proporcjach/rozdzielczościach, końcowy start.

Każdy etap ma osobną gałąź/PR z main. PR jest głównym dziennikiem wykonania.
Nie wykonujemy CI. Mechaniki kampanii, kreator i zapis pozostają dostępne.

## Kontrakty

- Frontend/pauza/loading to jawne stany; menu przejmuje wejście w trybie
  UIOnly. Zamknięcie przywraca kamerę, pauzę i input; modale mają pierwszeństwo.
- Autosave/autoload nie mogą zmieniać postępu podczas ekranu startowego.
- Domyślne tło wykorzystuje lokalną scenę 3D, bez kopiowania GTA i bez nowych
  zewnętrznych zasobów. Jakość miasta pozostaje ograniczeniem istniejącej sceny.
- Statystyki pokazują pomiary sesji i zapisany postęp, z jawnymi jednostkami;
  nie wymyślamy historii ani parametrów postaci.
- Lista zapisów pokazuje rzeczywiste pliki, datę i stan walidacji; uszkodzony
  zapis nie jest nadpisywany automatycznie. Operacje utraty postępu wymagają dialogu.
- Ustawienia są oparte o publiczne API silnika i istniejący config.
  Zmiana trybu ekranu ma zatwierdzenie i czasowe cofnięcie.
- Skalowanie zachowuje maksymalną szerokość treści na ultrawide i czytelność
  na 1080p/1440p/4K; przewijanie zastępuje obcinanie długiej zawartości.
- Testy nie nadpisują slotu gracza ani trwałej konfiguracji użytkownika.

## Odbiór

Sprawdzić startup/main/new/load, pause/resume, zakładki, dialogi, input lock,
focus, klawiaturę/mysz/pad, persistence i cofnięcie ustawień, return main/quit,
mapę, loading, rozdzielczości i realny render. Wyniki zapisywać tutaj oraz w PR.
