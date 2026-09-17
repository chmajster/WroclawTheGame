# Weryfikacja — 2026-09-17

Sprawdzone w Linux, bez Unreal Engine:

- Test produkcyjnego `SliceProgress.h`: pełna sekwencja, kolejność zależności, ponowne interakcje, snapshoty checkpointów i odrzucenie niespójnych stanów.
- Eksploracja **38 osiągalnych stanów**: zachowanie invariantów i osiągalność zakończenia.
- Kompilacja tego testu w GCC, C++17, `-Wall -Wextra -Werror -pedantic`, AddressSanitizer i UndefinedBehaviorSanitizer.
- Wygenerowanie i sprawdzenie **15 tekstur TGA** oraz **9 plików WAV**: wymiary, format, długość, obecność sygnału i brak clippingu.
- Sprawdzenie składni Python dla generatorów.

Lokalnie użyto:

```bash
ASAN_OPTIONS=detect_leaks=0 bash Scripts/test.sh
```

LeakSanitizer zgłosił brak obsługi środowiska działającego pod ptrace. Wyłączono tylko jego część; nie stanowi to dowodu braku wycieków gry. Workflow Linux pozostawia domyślne ustawienia sanitizerów.

**Niesprawdzone:** kompilacja klas UE/UHT, editor API do tworzenia mapy i navmesh, cooking, Windows EXE, kolizje, AI w silniku, UI na ekranie, 15–30 minut rozgrywki, wydajność i pamięć podczas działania gry. Brak narzędzi silnika w środowisku jest blokadą tej weryfikacji. Patrz `ACCEPTANCE.md`.
