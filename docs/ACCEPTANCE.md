# Bramki odbioru pierwszego etapu

**Status: OTWARTE. Nie oznaczać etapu jako ukończonego na podstawie samych klas lub testów logiki.**

| Bramka | Dowód wymagany do zamknięcia | Stan |
|---|---|---|
| Kompilacja UE5.6 Editor | Log UHT/UBT bez błędów | Niewykonane |
| Generowanie contentu | Mapa .umap, materiały, dźwięki, poprawne bounds navmesh | Niewykonane w UE |
| Windows x64 Development/Shipping | Kompletny pakiet uruchamiany bez edytora | Niewykonane |
| Menu → bezpieczny lokal → koniec | Nagranie lub raport pełnego przejścia na Windows | Niewykonane |
| 15–30 minut | Czas pierwszego przejścia co najmniej 3 osób bez solucji | Nie zmierzono |
| Ruch i interakcje | Brak blokad w schodach, drzwiach, narożnikach; czytelny celownik | Niewykonane |
| AI | Widzenie przez przeszkody wykluczone; słuch, nawigacja, pościg, poszukiwanie, powrót | Niewykonane w UE |
| Zapis | Wczytanie każdego z 4 checkpointów po restarcie EXE | Niewykonane na Windows |
| Śmierć i restart | Poprawny powrót z ekranów śmierci i pauzy | Niewykonane na Windows |
| Oprawa | Czytelne wnętrze przed prądem, spójne materiały i słyszalne cue | Niewykonane |
| 1080p | GPU/CPU/RAM komputera, ustawienia, frame-time, FPS, liczba draw calli | Nie zmierzono |
| Stabilność pamięci | Powtórzenia nowej gry i checkpointów w Unreal Insights bez trendu wzrostu pamięci | Nie zmierzono |

Pełna pętla do odebrania:

MENU → MIESZKANIE → ZAGADKI → KLUCZ → WYJŚCIE Z BUDYNKU → NAPASTNIK → POŚCIG → UCIECZKA → BEZPIECZNY PUNKT → KONIEC ROZDZIAŁU.

## Profilowanie

W Development zbierz `stat unit`, `stat gpu`, `stat rhi`, `stat memory` i sesję Unreal Insights, obejmując pierwsze wejście na ulicę, pościg, śmierć oraz trzy wczytania. Cel roboczy: 1920×1080 i 60 FPS przy ustawieniach High na nazwanym komputerze gamingowym; to założenie, nie wynik. Sprawdź szczególnie początkową generację navmesh i rozgrzanie shaderów. Pierwsze generowanie/cooking shaderów w edytorze nie jest miarą płynności gotowej paczki.

## Świadome ograniczenia obecnej implementacji

Bryłowe postacie, prosta animacja proceduralna, syntezowane audio i materiały robocze. Potrzebny jest przegląd wizualny. Natywna maszyna stanów obsługuje pojedynczego przeciwnika; integracja Behavior Tree/Blackboard z pierwotnej wizji nie została dodana. Nie ma rozbudowanego CampaignManagera, ponieważ dostępny jest jeden rozdział. Mapa nie używa World Partition ani streamingu: jest jednym zamkniętym obszarem. Rozbudowa miasta, pełna kampania, pojazdy, tłumy, multiplayer i pogoda nie wchodzą do tego etapu.

Nie dodawać sztucznych czasów oczekiwania, żeby deklarować 15–30 minut. Po pomiarach dopracować czytelność wskazówek i eksplorację w tej samej małej przestrzeni.
