> Aktualizacja 0.3: bieżąca architektura i procedury są w [ARCHITECTURE.md](ARCHITECTURE.md), [ADDING_CONTENT.md](ADDING_CONTENT.md) i [GEOGRAPHY.md](GEOGRAPHY.md). Opis poniżej dotyczy wcześniejszego blockoutu 0.2; geometrię zastąpił wypiek aktorów do mapy edytora.

# Dodawanie questów, zagadek i dowodów

`Data/chapter1.json` jest źródłem treści. `Source/WroclawTheGame/Content/ChapterCatalog.h` jest generowanym katalogiem C++ używanym zarówno przez Unreal, jak i testy. Nie edytuj katalogu ręcznie.

```bash
python3 Scripts/compile_chapter.py
bash Scripts/test.sh
```

## Pola akcji

| Pole | Znaczenie |
|---|---|
| `id` | Stabilny identyfikator używany przez stan i historię zapisu |
| `label`, `body` | Komunikat interakcji i treść dostępna po odczycie |
| `position` | Położenie interaktywnego obiektu w centymetrach UE |
| `kind` | read, pickup, use, door, code, password, symbols, sequence, lights, order, wires, choice, zone, virtual |
| `requires` | Wszystkie wymagane zdarzenia |
| `any_of` | Co najmniej jedna dostępna gałąź; pusta lista nie nakłada warunku |
| `items` | Wymagane przedmioty; zużycie bezpiecznika jest jawnie obsługiwane przez stan |
| `reward` | Dodawane przedmioty; klucze są unikalne, medkit/distraction/cash mogą się sumować |
| `clues` | Zdarzenia wskazówek wymagane przed zaakceptowaniem rozwiązania |
| `answer` | Rozwiązanie 1–8 znaków lub `@variant` dla wariantu sekwencji |
| `evidence` | ID wpisu śledztwa, dodawanego po akcji |
| `choice` | Wzajemnie wykluczająca się grupa wyborów |
| `alternate` | Drugi wybór panelu choice |
| `checkpoint` | Zapis po skutecznym zdarzeniu |

Panel i treść nie kończą się automatycznie po znalezieniu kodu. Gracz musi rzeczywiście wpisać rozwiązanie. Błędne rozwiązanie nie dodaje dowodu, nagrody ani ukończenia.

`quests` określa 17 etapów: `all` wymaga wszystkich zdarzeń, `any` opisuje alternatywne drogi. Opcjonalne questy nie mogą trafić do wymaganych zależności kampanii. Zmiana ID lub znaczenia zależności wymaga decyzji o migracji i wersji zapisu.

## Warstwa Unreal

- `USliceMission` tłumaczy zdarzenia gry na przenośny stan, zapis i komunikaty.
- `ASliceProp` implementuje `IInteractable`, a katalog określa rodzaj interakcji.
- `ASliceGameMode` obsługuje strefy, aktywację spotkań i zakończenie pościgu.
- `ASliceWorld` buduje geometrię modułami: mieszkanie, trasy, otoczenie. Sam wpis `position` nie tworzy nowego pomieszczenia — potrzebna jest fizyczna geometria i nawigacja.
- `ASliceController` obsługuje zakładki, przewijanie, wizjer oraz panele, które nie zatrzymują pościgu.
- `ANoiseThrowable` emituje hałas przy kolizji; AI bada pozycję uderzenia.

Nowe typy wejścia i nowe bramy wymagają jawnej obsługi w `ASliceProp`/UI; samo dopisanie nieznanego `kind` zostanie odrzucone. Przy nowych dowodach należy dobrać kategorię w `USliceMission`. Nie zakładaj, że poszerzenie JSON automatycznie daje poprawną mapę albo nową mechanikę.

Do każdej nowej alternatywy dodaj przejście testowe z pominięciem pozostałych alternatyw. Testy logiczne uzupełnij odbiorem fizycznej drogi w Unreal. Nie zwiększaj czasu gry sztucznym oczekiwaniem.
