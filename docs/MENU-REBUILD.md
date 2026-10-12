# Menu Wrocław — wykonanie

## Audyt przed przebudową

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

## Moduły i obsługa

`UI/Menu/MenuFlowSubsystem` zarządza boot/main/pause/playing, kamerą, input,
statystykami, zapisem i ładowaniem. `FrontEndMenuWidget` opakowuje Slate w UMG.
`MenuShell` składa wspólne komponenty `MenuStyle`; `MenuSettings` obsługuje
konfigurację, a `MenuMap` projekcję ulic i world-space waypoint pod przyszły GPS.
`MenuLoadingScreen` jest autorską ilustracją wektorową, używaną także przez
MoviePlayer podczas rzeczywistego ładowania mapy. Nie przedstawia procentów.

Menu skaluje projektowy obszar 1920×1080 do viewportu, zachowując szerokość
treści na ultrawide. Długie formularze przewijają się po zmianie focusu.
TAB/strzałki lub D-pad wybierają kontrolkę, ENTER/A zatwierdzają, ESC/B/START
wracają. Na mapie: przeciąganie, kółko/+/−, strzałki/D-pad, LB/RB zoom;
kliknięcie stawia znacznik. Podczas przechwytywania przypisania ESC/START
anuluje. Skróty ekranów kampanii i F5/F9 są zarezerwowane.

Grafika używa UGameUserSettings; audio ma master/music/effects/ambient/UI.
Profil wejścia zapisuje 32 akcje oraz czułości/invert/deadzone w GameUserSettings.
Konflikty uwzględniają pieszo/pojazd/globalne skróty. Ustawienia jakości/audio/
wejścia zapisują się od razu; video ma osobny snapshot i 15 s na potwierdzenie.
Snapshot jest potrzebny, bo GameViewportClient sam aktualizuje UE LastConfirmed
po resize. Blur jest wyłączany przed wymianą backbufferu i chroniony przed
starą geometrią. CommonUI virtual-mouse accept jest tymczasowo wyłączany dla
menu Slate i przywracany dla gry/kreatora; pad A trafia do wybranej kontrolki.

Scena dzielnicy jest zatrzymana w menu. Kamera porusza się niezależnie;
wcześniej widoczne postaci są ukrywane i przywracane po wznowieniu.
Menu ma limit 30 FPS, przywraca limit gracza; audio refresh jest ograniczony do 5 Hz.
Utrata focusu otwiera pauzę. Zamknięcie menu oddaje kamerę i wejście postaci.

UI nie dodaje zewnętrznych płatnych zasobów ani plików GTA. Typografia/brushes
pochodzą z runtime CoreStyle UE; miasto i dźwięki są istniejącymi zasobami
projektu, a ilustracja loading jest własnym kodem. Tło zachowuje ograniczenia
istniejącej sceny prototypowej — przebudowa menu nie jest przebudową artu miasta.

## Powtarzanie testów

Po kompilacji uruchomić `Scripts/Test-Menu.ps1 -Layouts`. Test backendu ustawień
ma replay w osobnym procesie. MenuUISmoke używa rzeczywistych kontrolek Slate,
routowanych zdarzeń klawiszy oraz syntetycznej ścieżki wskaźnika; nie jest to
ręczny test fizycznej klawiatury/myszy/pada. Testy video korzystają z prawdziwego
okna i renderera. Własne INI są w Saved/MenuQA; dwa sloty MenuQA są chronione
przed nadpisaniem i usuwane po teście. Test nie dotyka slotu gracza.
Obrazy oraz budżet klatek trafiają do Saved/Tests i Saved/Logs.
Regresja rozgrywki: `Scripts/Test-PlayableDistrict.ps1`. Żaden skrypt nie używa CI.

## Poprawki wynikające z testów

Test rzeczywistego okna wykrył crash D3D12 przy zmianie rozdzielczości z aktywnym
blur. Menu opróżnia wcześniejsze klatki przed resize i sprawdza geometrię blur
względem backbufferu. Osobny snapshot trybu ekranu chroni cofnięcie przed
automatyczną zmianą `LastConfirmed` przez silnik. Test sprawdza anulowanie,
15-sekundowy timeout oraz zatwierdzenie i zapis nowego trybu.

Nawigacja kontrolerem wymagała zawieszenia symulacji myszy CommonUI, aby A
aktywowało fokusowaną kontrolkę. Powrót do gry i kreatora przywraca ustawienie.
Niepoprawny nagłówek pliku save jest sprawdzany przed deserializacją UE;
`Kontynuuj` pomija uszkodzony najnowszy plik i wybiera najnowszy poprawny zapis.

Przegląd renderów wykrył pasy kamery na ultrawide. Kamera frontend nie wymusza
16:9; tylko szerokość interfejsu jest ograniczona. Ukrywane są również postacie
misji typu `ADistrictDeliveryPoint`, a ich widoczność wraca po wznowieniu gry.

Regresja fizycznej jazdy wykryła rekurencyjne czyszczenie pościgu przy `Destroy`
radiowozu: odziedziczony `RaceSession::EndPlay` ponownie wywoływał `StopPursuit`.
Lista jest teraz odłączana przed niszczeniem aktorów, a pojazdy AI nie resetują
globalnego pościgu przy końcu życia. Nie zmieniono zasad jazdy, misji ani nagród.
Skrypt regresji odrzuca błędy konsoli; testy reguł otwierają pustą mapę silnika,
aby nie ładować niezwiązanych starych importów z domyślnej mapy edytora.
UE 5.8 nadal wypisuje 17 ogólnych `LogAutomationTest: Condition failed` ze swoich
testów startowych przed kolejką WTG.District, również na pustej mapie. Skrypt
jawnie raportuje te wpisy, odrzuca inne błędy startowe oraz wszystkie błędy
podczas wskazanych testów projektu i sond działającej gry. Nie poprawiano silnika.

## Końcowe wyniki — 12 października 2026

- Kompilacja `WroclawTheGameEditor Win64 Development`: PASS, 119,18 s.
- `Test-Menu.ps1 -Layouts`: PASS. Ustawienia grafiki/audio/wejścia odtworzone
  w osobnym procesie. 33 etapy UI zakończone w 41,96 s: start, nowa gra,
  anulowanie, pauza/input lock, mapa, 18 przełączeń zakładek, zapis/odczyt
  statystyk, trzy ścieżki potwierdzania video, konflikt/remap/cancel, focus i A
  kontrolera, utrata focusu, powrót do main, pominięcie uszkodzonego save, quit.
  Sprawdzono też ukrycie i przywrócenie obu postaci misji. Log UI bez Error/Fatal;
  aplikacja rzeczywiście zamknęła się po potwierdzeniu wyjścia.
- 20 natywnych PNG (main/pauza/ustawienia/sterowanie/loading) ma prawidłowe
  wymiary czterech viewportów. Przegląd obrazów potwierdził czytelną typografię,
  brak kolizji etykiet mapy, przewijanie formularzy i pełne tło na ultrawide.
- `Test-PlayableDistrict.ps1`: PASS. Cztery testy reguł/geometrii/save oraz
  zapis pieszo/w aucie, rzeczywista fizyczna sesja i replay w nowym procesie.
  Sesja: 147,29 s, 459,60 m jazdy; chodzenie, ściana/podłoga, skok/kamera,
  failure/restart misji, wejście/wyjście z auta, dostawa, zgubienie pościgu,
  wygaszenie wanted i jednorazowa nagroda. Replay PASS po 5,02 s.
  Logi działającej gry bez Error/Fatal. Cztery testy projektu nie mają błędów;
  opisane wyżej 17 wpisów startup UE pozostaje ograniczeniem instalacji silnika.
- PowerShell parser i `git diff --check`: PASS. Nie uruchamiano CI.
- Końcowy start widocznej gry przez `Scripts/Launch-District.ps1`: PASS.
  Świeży log potwierdza ładowanie Nadodrza, gotowość postaci i menu główne
  o 18:36:40 UTC. Okno pozostawiono otwarte do gry.

| Viewport | Klatki próbki | Średnia ms | p95 ms |
|---|---:|---:|---:|
| 1920×1080 | 600 | 33,34 | 33,33 |
| 2560×1440 | 600 | 33,33 | 33,33 |
| 3840×2160 | 600 | 33,33 | 33,33 |
| 3440×1440 | 599 | 33,39 | 33,36 |

Pomiar dotyczy menu na tej maszynie przy limicie 30 FPS, po rozgrzaniu sceny;
nie jest porównawczym benchmarkiem całej gry. Powrót do limitu rozgrywki jest
osobno sprawdzony. Wejście testowano automatycznie przez natywne Slate, bez
ręcznego testu fizycznego kontrolera. Nie deklarujemy poprawy artu miasta:
filmowe tło nadal wykorzystuje istniejące modele prototypowej dzielnicy.

Dowody lokalne: `Saved/Logs/MenuFullQA.stdout`, `MenuUIQA.log`,
`MenuSettingsQA{,Replay}.log`, `MenuLayout{1080,1440,4K,Ultrawide}.log`,
`MenuGameplayRegression.stdout`, `District{RulesTest,SaveSmoke,SessionSmoke,SessionReplay}.log`.
Obrazy: `Saved/Tests/Menu{Main,Pause,Settings,Controls,Loading}_{1080,1440,4K,Ultrawide}.png`.
