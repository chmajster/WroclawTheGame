# WroclawTheGame — Przebudzenie

Pierwsza implementacja zamkniętego vertical slice w **Unreal Engine 5.6 / C++**, z docelowym buildem **Windows x64**.

**Status: implementacja do walidacji w Unreal Engine. Etap produkcji nie jest ukończony.**
W środowisku przygotowania kodu nie ma UE ani toolchainu Windows. Nie wykonano kompilacji UHT/UBT, cookingu, uruchomienia EXE ani przejścia misji. Repozytorium nie zawiera gotowego pliku EXE. Testy niezależnej logiki nie zastępują tych bramek. Czas 15–30 minut i wydajność w 1080p pozostają celami do pomiaru.

## Zbuduj na Windows

Wymagania stanowiska: Unreal Engine **5.6**, Visual Studio 2022 z narzędziami C++ do gier i Windows SDK kompatybilnymi z tą instalacją UE. Skrypt korzysta z Pythona dostarczonego z edytorem. Nie wymaga płatnych ani zewnętrznych assetów.

W PowerShell, w katalogu repozytorium:

```powershell
.\Scripts\Build-Windows.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.6'
```

Skrypt kolejno kompiluje target edytora, generuje/importuje materiały, tekstury, WAV i mapę, następnie wykonuje BuildCookRun. Przerwie działanie przy błędzie kompilacji/importu lub braku poprawnej mapy. Wynik trafia do `Builds/Development/`. Uruchom `WroclawTheGame.exe` w wynikowym katalogu Windows. Do dystrybucji potrzebny jest **cały katalog pakietu**, nie sam plik EXE.

Wersja Shipping:

```powershell
.\Scripts\Build-Windows.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.6' -Configuration Shipping
```

Przygotowanie edytora bez pakowania:

```powershell
.\Scripts\Build-Windows.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.6' -PrepareOnly
```

Następnie otwórz `WroclawTheGame.uproject`, mapę `Content/Maps/Przebudzenie` i wybierz Play. Geometria i interakcje są tworzone przez `ASliceWorld` po rozpoczęciu gry; w samym widoku edycji mapa zawiera punkty startu i obszar nawigacji. Nie trzeba ręcznie rozmieszczać gameplayowych aktorów. Wygenerowane binarne assety pozostają lokalne i są odtwarzane podczas budowania.

## Sterowanie

| Klawisz | Działanie |
|---|---|
| WASD / mysz | Ruch / kamera third-person |
| Shift / Ctrl / Spacja | Sprint / kucanie / skok |
| E | Interakcja z obiektem pod celownikiem |
| T / I | Telefon / ekwipunek |
| LPM / PPM | Lekki atak / blok od przodu |
| Esc | Pauza lub zamknięcie wskazówki |
| 0–9, Backspace, Enter | Wpisanie, poprawienie i zatwierdzenie kodu |
| N / L / Q w menu | Nowa gra / wczytanie / wyjście |
| Enter na ekranie śmierci | Wczytanie ostatniego checkpointu |

## Zakres kodu

- Mieszkanie, klatka z 18 stopniami, parter, podwórko, fragment ulicy, zaułek i lokal docelowy. Centymetry UE, wnętrza o wysokości około 3 m. Umowne Nadodrze z polskimi szyldami, kamienicami, znakami i przewodami tramwajowymi; nie jest to odwzorowanie ulicy 1:1.
- Wspólny `IInteractable`: drzwi, szuflada, szafki, podnoszenie przedmiotów, rozdzielnia, przełącznik lampy i kartki. Telefon dostępny z ekwipunku przez T.
- Zależności: telefon + powerbank z kablem + kartka z PIN-em → uruchomienie → wiadomość → bezpiecznik → prąd → lampa → kod szafki → klucz.
- Misja z 13 celami. Jedna implementacja stanu jest używana przez runtime i testy. Zakończenie rozdziału nie oznacza opuszczenia Wrocławia ani ukończenia całej kampanii.
- AI Perception Sight/Hearing, nawigacja, patrol, podejrzliwość, pościg, atak, ostatnia znana pozycja, przeszukiwanie i powrót. AI tego etapu używa natywnej maszyny stanów C++; **nie ma jeszcze assetu Behavior Tree/Blackboard**.
- Zdrowie, stamina, koszt sprintu/ataku/bloku, śmierć, restart. Postacie są bryłowymi proxy; gracz ma prostą proceduralną animację kończyn. Brak finalnych modeli i animacji szkieletowych.
- Zapis początkowy oraz autosave po otwarciu szafki, opuszczeniu mieszkania, wejściu na ulicę i dotarciu do schronienia. Odtworzenie stanu zagadek, przedmiotów i misji. Checkpoint przywraca pełne zdrowie i resetuje spotkanie z napastnikiem.
- Minimalny HUD, menu, pauza, ekwipunek, wskazówki, klawiatury kodów i zakończenie rozdziału.
- Generowane tekstury i parametry PBR, Lumen w konfiguracji, oświetlenie wnętrz/ulicy i syntetyzowane audio prototypowe. Są to assety robocze, nie finalna oprawa realistyczna.

`USliceMission` przechowuje sesję pomiędzy przeładowaniami mapy. `USliceSave` zapisuje wersjonowany snapshot w standardowym katalogu `Saved/SaveGames/Przebudzenie_v1.sav` gry. Uszkodzone lub niezgodne logicznie zapisy są odrzucane. Nowa gra zapisuje początek nowej sesji w tym samym slocie.

## Testy i CI

```bash
bash Scripts/test.sh
```

Testy C++ wykonują rzeczywistą maszynę stanów produkcyjnych: pełna droga, przedwczesne interakcje, powtórzenia, odtworzenie checkpointów, odrzucenie niespójnego stanu oraz eksploracja wszystkich osiągalnych kombinacji zdarzeń. Test assetów sprawdza wygenerowane PCM i tekstury. **Nie testują kolizji UE, UHT, renderingu, percepcji ani grywalności.**

Workflow `Logic and source assets` działa na GitHub-hosted Linux. Workflow `Windows UE5 package` jest uruchamiany ręcznie i wymaga własnego runnera z etykietami `self-hosted`, `Windows`, `X64`, `unreal-5.6` oraz zmienną środowiskową `UE_ROOT`. Nie zakłada, że taki runner już istnieje.

Odbiór: [docs/ACCEPTANCE.md](docs/ACCEPTANCE.md). Solucja do sprawdzania zależności: [docs/WALKTHROUGH.md](docs/WALKTHROUGH.md). Wynik lokalnej weryfikacji: [docs/VALIDATION.md](docs/VALIDATION.md).
