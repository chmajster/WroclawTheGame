# Weryfikacja implementacji v2 — 2026-09-17

Lokalnie wykonano testy niezależne od Unreal Engine:

- 12 pełnych głównych sekwencji: 2 wyjścia × 2 wejścia do sklepu × 3 warianty panelu.
- Pominięcie wszystkich pobocznych w tych sekwencjach; osobna odmowa pomocy sąsiadowi nie blokuje zakończenia.
- Zebranie wszystkich dowodów i osiągnięcie Detektyw przed końcem rozdziału.
- Wzajemne wykluczenie wyborów, unikalne klucze i dokładne zużywanie przedmiotów.
- Niepoprawne rozwiązania, brak wskazówek, blokada po trzech błędach, odblokowanie po 8 sekundach i zachowanie blokady w kopii stanu.
- Odmowa zakończenia pościgu, otwarcia warsztatu i finału podczas zagrożenia.
- Warianty i niezmienniki odtworzenia stanu, odrzucenie niespójnej historii/ekwipunku.
- Progi podpowiedzi 90/180/300 sekund i reset po postępie.
- 50 000 prób interakcji w losowych kolejnościach, sprawdzających rzeczywistą klasę produkcyjną.
- Walidacja źródła treści i wygenerowanego katalogu, błędnych referencji, cykli i wariantów.
- Generowanie/sprawdzenie 15 tekstur TGA i 10 WAV oraz składni generatorów.

Test C++ jest kompilowany jako **C++20**, z ostrzeżeniami jako błędami, AddressSanitizerem i UndefinedBehaviorSanitizerem. Lokalnie LeakSanitizer wymaga wyłączenia z powodu środowiska ptrace:

```bash
ASAN_OPTIONS=detect_leaks=0 bash Scripts/test.sh
```

CI pozostawia domyślne ustawienia sanitizerów. Wyniki modelu nie są dowodem braku wycieków pamięci gry.

**Niewykonane:** kompilacja UE/UHT/UBT, tworzenie mapy i import przez editor API, serializacja SaveGame na dysk, cooking i Windows EXE, sprawdzenie kolizji/AI/UI, pomiar długości rozgrywki, FPS, frame-time i pamięci. Tych bramek nie zastępują powyższe testy.
