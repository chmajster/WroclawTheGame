# WroclawTheGame — Przebudzenie

Projekt Unreal Engine **5.6 / C++20**, docelowo **Windows x64**. Wersja 0.2 rozszerza pierwszy rozdział zgodnie z [nowym zakresem](docs/SCOPE-v2.md): celem jest 45–90 minut pierwszego przejścia, zamiast wcześniejszych 15–30 minut.

**Status: implementacja źródłowa do kompilacji i odbioru w Unreal Engine. Etap nie jest ukończony.** Nie wykonano UHT/UBT, cookingu, uruchomienia EXE ani przejścia gry na Windows — środowisko wykonania nie zawiera Unreal Engine. Czas 45–90 minut, pościg 3–5 minut oraz wydajność w 1080p pozostają niezmierzonymi celami. Testy logiki nie dowodzą grywalności.

## Build Windows

Stanowisko: Unreal Engine 5.6, Visual Studio 2022 z narzędziami C++ do gier i zgodny Windows SDK. Python pochodzi z instalacji Unreal. Wszystkie grafiki i dźwięki robocze są generowane lokalnie, bez płatnych assetów.

```powershell
.\Scripts\Build-Windows.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.6'
```

Skrypt waliduje dane i generuje katalog C++, kompiluje target edytora, tworzy/importuje content oraz mapę, a następnie wykonuje BuildCookRun. Wynik: `Builds/Development/`. Uruchom `WroclawTheGame.exe` z całego wynikowego pakietu Windows; sam EXE nie wystarcza.

```powershell
.\Scripts\Build-Windows.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.6' -Configuration Shipping
```

Opcja `-PrepareOnly` przygotowuje projekt i mapę do otwarcia w edytorze. Geometria i interakcje powstają w runtime z kodu i katalogu danych. Mapa edytora zawiera punkt startowy i bounds nawigacji; nie jest ręcznie umeblowanym poziomem. Binarne assety są odtwarzane podczas przygotowania contentu.

## Zawartość implementacji 0.2

- **17 etapów głównych**, **4 questy poboczne**, **5 sekretów**, **14 paneli/zagadek z wpisywanym rozwiązaniem**, **40 wpisów śledztwa** i **83 akcje/zdarzenia** katalogu. Część paneli jest alternatywna lub opcjonalna; nie trzeba rozwiązywać wszystkich w jednym przejściu.
- Mieszkanie → klatka → **piwnica albo pomieszczenie techniczne** → podwórko → ulica/sklep/parking → opuszczony lokal → zaułek → garaż → warsztat.
- Nowy łańcuch zasilania: ładowarka + kabel → sprawdzenie gniazdka → schowek z bezpiecznikiem → prąd → ładowanie telefonu → PIN → SMS → komputer → TARGET. Telefon nie jest potrzebny do uruchomienia prądu.
- Kody wymagają znalezienia wskazówek. Panele blokują się na 8 sekund po trzech błędach. Panel świateł pokazuje diody i wybiera jeden z trzech trwałych wariantów nowej gry.
- Telefon: SMS, kontakty, zdjęcia/opisy fotografii, notatki i historia połączeń. Śledztwo: Ludzie, Miejsca, Dowody, Wiadomości. Questy i sekrety mają osobny dziennik.
- Wybór pomocy sąsiadowi, opcjonalny sejf, samochód i porzucony telefon. Pominięcie pobocznych nie blokuje obu głównych dróg.
- Jeden archetyp przeciwnika, trzy spotkania: klatka, podwórko, zasadzka. AI Perception Sight/Hearing, nawigacja, ostatnia znana pozycja, poszukiwanie i powrót do patrolu. Rzucony przedmiot generuje dźwięk przy zderzeniu. AI korzysta z natywnej maszyny stanów; nie dodano Behavior Tree/Blackboard.
- Lekki atak ogłusza, blok zużywa staminę, unik ma krótki czas ochrony. Możliwa ucieczka przez tył garażu. Brak wymogu zabijania; nie dodano broni palnej.
- Latarka wymaga baterii, opatrunki są zużywalne, przedmioty do rzutu mają ograniczoną liczbę. Zapis odtwarza neutralizację strażników i użyte przedmioty.
- Trzy poziomy podpowiedzi na żądanie po 90/180/300 sekundach bez ukończenia etapu. Podpowiedzi nie podają odpowiedzi.
- Sześć lokalnych osiągnięć zapisanych niezależnie od nowej gry. „Detektyw” wymaga wszystkich wpisów śledztwa; wybór odejścia od sąsiada wyklucza komplet dowodów w tym przejściu.
- Finał odblokowuje rozdział 2 w stanie kampanii. Nie uruchamia nieistniejącego drugiego poziomu i nie oznacza ukończenia całej gry.

## Sterowanie

| Klawisz | Działanie |
|---|---|
| WASD / mysz | Ruch / kamera third-person |
| Shift / Ctrl / Spacja | Sprint / kucanie / skok |
| E / T / I | Interakcja / telefon / ekwipunek |
| J / B / H / K | Śledztwo / questy / podpowiedzi / osiągnięcia |
| F / G / V | Latarka / rzut przedmiotu / opatrunek |
| LPM / PPM / lewy Alt | Lekki atak / blok / unik |
| 0–9, A–Z, Backspace, Enter | Rozwiązanie panelu, poprawka, zatwierdzenie |
| 1–5 w telefonie / 1–4 w śledztwie | Wybór zakładki |
| Strzałki góra/dół | Przewijanie długich wpisów |
| Esc | Zamknięcie panelu lub pauza |
| N / L / Q w menu | Nowa gra / checkpoint / wyjście |

**Panele zagadek działają bez zatrzymywania świata.** W garażu można zostać zaatakowanym podczas wpisywania przewodów. Esc zamyka panel i przywraca sterowanie. Menu, ekwipunek i czytanie dokumentów zatrzymują rozgrywkę.

## Zapis i rozbudowa

Zapis rozdziału: standardowy katalog gry `Saved/SaveGames/Przebudzenie_v2.sav`. Starszy slot v1 pozostaje nienaruszony, ale nie jest odczytywany jako v2: zmieniły się zależności zagadek. Zapis obejmuje historię stabilnych ID, wariant, ekwipunek odtworzony ze zdarzeń, zużycie przedmiotów, dowody, wybory, strażników, statystyki, podpowiedzi i blokady paneli. Zapis nie przechowuje dowolnego niezaufanego indeksu questu; odtwarza i waliduje zależności. Checkpoint przywraca pełne zdrowie i pozycję stojącego gracza.

Autozapis występuje przy kluczowych etapach. W danych jest 11 znaczników checkpointu, w tym dwa alternatywne wyjścia; każde przejście używa jednego z nich. Profil osiągnięć: `LocalAchievements.sav`.

Treść i powiązania: `Data/chapter1.json`. Generator: `Scripts/compile_chapter.py`. Produkcyjny stan i testy korzystają z tego samego katalogu. Zasady rozszerzania: [docs/AUTHORING.md](docs/AUTHORING.md).

## Weryfikacja

```bash
bash Scripts/test.sh
```

Testy obejmują 12 kombinacji głównych dróg, opuszczenie wszystkich pobocznych, nagrody, wszystkie dowody, wybory, odtworzenie stanu, blokady kodów, wskazówki i 50 000 prób zdarzeń w różnej kolejności. Walidator sprawdza aktualność katalogu, nieznane zależności i cykle. Generatory tworzą 15 tekstur TGA i 10 plików PCM WAV.

Workflow Linux działa na GitHub-hosted runnerze. Ręczny workflow Windows wymaga własnego runnera z etykietami `self-hosted`, `Windows`, `X64`, `unreal-5.6` i zmienną `UE_ROOT`.

[Odbiór](docs/ACCEPTANCE.md) · [Solucja testowa](docs/WALKTHROUGH.md) · [Wyniki i ograniczenia](docs/VALIDATION.md)
