# WroclawTheGame — Przebudzenie

Unreal Engine **5.8 / C++20**, docelowo **Windows x64**. Wersja źródłowa 0.3 obejmuje rozdział, systemy otwartego świata oraz laboratorium rzeczywistej geografii i pojazdu.

**Etap nie jest ukończony. Nie ma zweryfikowanego pakietu Windows.** Nie wykonano UHT/UBT, cookingu ani przejścia gry w Unreal. Czas rozgrywki i wydajność pozostają niezmierzone. Testy logiki nie stanowią dowodu grywalności.

## Co zawiera projekt

- Rozdział: **98 akcji**, **17 celów głównych**, **8 questów pobocznych**, **52 definicje dowodów**, **15 przedmiotów**. Mieszkanie, dwie drogi przez budynek, podwórko, ulica, lokal, garaż i warsztat; rozszerzenie o skwer, kryjówki i opcjonalne zagadki.
- Wspólny `IInteractable`, komponenty zdrowia, staminy, walki, inventory, drzwi, zasilania, zagadek, frakcji i ukrywania. Rejestr typów celów, Gameplay Tags, event bus oraz edytowalny `ChapterDefinition`.
- Profile 4 strażników jednego archetypu, wzrok/słuch, ostatnia znana pozycja, lokalne alarmowanie z opóźnieniem, ukrywanie, Heat, dyrektor 5 zdarzeń, harmonogram NPC, pogoda i odkrywanie miejsc. CCTV ma kamerę sceny i podgląd; wyłączenie zasilania wyłącza obserwację.
- Telefon z 9 stronami, rejestr prezentacji, śledztwo i łączenie dowodów. Aparat rozpoznaje fotografowany obiekt i zapisuje raster PNG 1280×720 w `Saved/Photos`; galeria tekstowo sygnalizuje obecność pliku. Mapa telefonu korzysta z katalogu GIS, pokazuje aktualny sektor, status, waypoint i dystans w linii prostej. Runtime routing uliczny i wizualna mapa 2D nadal pozostają do wykonania.
- Zapis v3: uporządkowane zdarzenia i zużycia, świat/Heat/pogoda, odkrycia, cooldowny, NPC, neutralizacja, wariant zagadek. Odczyt wcześniejszego v2 bez nadpisywania jego pliku. Debug blokuje zapis kampanii i osiągnięć.
- Geometria kampanii jest wypiekana do aktorów edytora; skrypt konwertuje mapę do World Partition, sprawdza streaming i opcjonalnie buduje HLOD. **Są to nieuruchomione jeszcze procedury UE.**
- Rzeczywisty sektor Nadodrza: OSM z 17.09.2026, EPSG:32633, skala 100 cm/m, źródłowe footprinty z dziedzińcami, ulice, tory, tereny zielone i wysokości. Import tworzy 3019 obiektów/linii, 5706 węzłów grafu i 281 grup siatek. Zachowuje kierunki jazdy i źródłową topologię. Mosty/tunele oczekują ręcznego opracowania wysokości.
- Osobne laboratorium `Nadodrze_GIS`: samochód z napędem fizycznym, wsiadaniem/wysiadaniem, hamowaniem, biegiem wstecznym, światłami, klaksonem i uszkodzeniami. Trzy próby po prawdziwych drogach: sprint, czas, dostawa; kolejność checkpointów, restart bez przeładowania poziomu i osobny zapis rekordów. **Nie przetestowano fizyki w silniku.**

## Wave 1 — południowe sektory GIS

Dodano rzeczywiste dane OSM i terenu dla obszaru Hub/Gaju/Borka/Krzyków oraz łącznika z Nadodrzem: **82 823 obiekty i linie, 141 175 węzłów, 162 324 krawędzie**. Komórki produkcyjne nie są administracyjnymi granicami dzielnic.

`Build-Geography.ps1 -City` rozszerza istniejący świat GIS w tym samym układzie współrzędnych. Dodano katalog sektorów, profile materiałów, adresy powiązane z budynkami, routing portalowy offline, raport gotowości oraz integrację katalogu z telefonem i developerskim overlayem `CityCoverage`.

**Status odbioru: GISOnly, 0 sektorów potwierdzonych jako Playable.** Sześć pomostów prototypowych przywraca połączenia wszystkich sektorów w grafie. Dodano 32 aktywności (4 zadania, 4 sekrety, 12 zdarzeń i 12 tropów), cztery wnętrza powiązane z budynkami, podstawową populację, zapis miasta/pojazdu i predykcję streamingu. Kod UE i wygenerowane assety nadal wymagają kompilacji oraz playtestu. Kampania pozostaje na swojej dotychczasowej mapie. Szczegóły i polecenia: [Wave 1](docs/CITY-WAVE1.md).

## Uruchomienie i build kampanii

Wymagane lokalnie: UE 5.8, Visual Studio z narzędziami C++, zgodny Windows SDK oraz .NET Framework SDK. Brakujący .NET Framework Developer Pack można zainstalować przez skrypt za pomocą `-InstallPrerequisites` (wymaga `winget` i może wyświetlić monit administratora).

```powershell
.\Scripts\Build-Windows.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' -InstallPrerequisites
# Alternatywnie:
.\Scripts\Build-Windows.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' -Configuration Shipping -BuildHLOD -InstallPrerequisites
```

Skrypt waliduje katalogi i tagi, kompiluje target edytora, generuje materiały/audio/mapę, konwertuje World Partition i pakuje. Zatrzymuje się po błędzie lub braku znaczników generacji. `-PrepareOnly` przygotowuje content do edytora. Wynik pakowania: `Builds/Development` lub `Builds/Shipping`; uruchamiać cały pakiet, nie sam EXE.

Mapa domyślna `/Game/Maps/Przebudzenie_Source` otwiera menu. `N` rozpoczyna grę, `L` odczytuje checkpoint. Nie dodano drugiego rozdziału ani możliwości zakończenia całej kampanii.

## Laboratorium Nadodrza

Import GIS wymaga osobnego Pythona 3.12 lub nowszego (pyproj 3.8). Python wbudowany w UE służy wyłącznie do wypieku gotowych danych.

```powershell
python -m pip install -r Scripts/gis/requirements.txt
.\Scripts\Build-Geography.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' -Package
```

Otwórz `/Game/Maps/Nadodrze_GIS` w edytorze. W pakiecie Development z powyższego skryptu użyj konsoli `open /Game/Maps/Nadodrze_GIS`. Tryb nie zapisuje postępu kampanii. Ulice i trasy prób korzystają z tej samej importowanej sieci. **Kampania nie została jeszcze przeniesiona na prawdziwe budynki Nadodrza**; dotychczasowa mapa pozostaje fikcyjnym blockoutem gameplayu.

| Sterowanie | Działanie |
|---|---|
| WASD / mysz | Ruch / kamera |
| Shift / Ctrl / Spacja | Sprint / kucanie / skok |
| E / T / I | Interakcja / telefon / ekwipunek |
| J / B / H / K | Śledztwo / questy / podpowiedzi / osiągnięcia |
| LPM / środkowy / PPM / Alt | Lekki atak / ciężki / blok / unik |
| F / G / V / Esc | Latarka / rzut / opatrunek / pauza |
| Samochód: W/S, A/D, Spacja | Gaz/hamulec/wstecz, kierownica, ręczny |
| Samochód: Num 0 / Num + / Num * / E | Silnik / światła / klakson / wysiadanie |
| Samochód: F5 / F6 | Start lub restart próby / wybór próby |

## Weryfikacja i rozszerzanie

```bash
python3 -m pip install -r Scripts/gis/requirements.txt
bash Scripts/test.sh
```

Testy C++ z ASan/UBSan sprawdzają 12 ścieżek rozdziału, 50 000 mieszanych interakcji, zapis, systemy świata oraz checkpointy wyścigu. Python sprawdza katalogi, assety i import GIS. `WTG.Save.Version3MemoryRoundTrip` jest dodatkowym testem silnikowym, jeszcze nieuruchomionym.

Instrukcje: [architektura](docs/ARCHITECTURE.md), [dodawanie zawartości](docs/ADDING_CONTENT.md), [GIS i licencje](docs/GEOGRAPHY.md), [walidacja](docs/VALIDATION.md), [odbiór](docs/ACCEPTANCE.md).

Nadal brakuje m.in. połączenia kampanii z GIS, pełnej migracji checkpointów na nowe współrzędne, odbioru nowego fizycznego ruchu AI w silniku, pełnych skrzyżowań i sygnalizacji, pościgów samochodowych, blokad drogowych, misji śledzenia pojazdu oraz trzech pozostałych scenariuszy samochodowych. Pełną listę braków zawiera dokument odbioru. Nie należy przedstawiać tego PR jako gotowej gry.
