# Wrocław: pierwsza kompletna sesja otwartego świata

Zakres użytkownika z 12.10.2026. Rozwijamy istniejący projekt UE 5.8 / C++20.
Pierwszy rezultat to zwarty fragment Nadodrza i nadodrzańskich wysp, a nie cały
Wrocław. Źródła GIS i JSON pozostają autorytatywne; mapy binarne są produktami
generatorów. Własna misja, polskie komunikaty i lokalna architektura budują
tożsamość gry. GTA VI służy jako kierunek jakości, nie jako źródło assetów.

## Stan zastany

| Element | Potwierdzony stan |
| --- | --- |
| Silnik | UE 5.8.2 zainstalowany w `D:/SteamLibrary/UnrealEngine/UE_5.8` |
| Uruchomienie | Moduł C++ kompiluje się; kampania `Przebudzenie_Source` ładuje się w widocznym oknie `-game` |
| Świat | Kampania blockout oraz oddzielny generator GIS; źródła OSM/terenu z metadanymi i stałym origin EPSG:32633 |
| Postać | `ASliceCharacter`, Enhanced Input, spring arm, zdrowie/stamina; animacje ruchu siatki szkieletowej wymagają spięcia w runtime |
| Pojazd | `ADriveableVehicle`: fizyczne podwozie, zawieszenie, napęd, wejście/wyjście, światła, klakson i uszkodzenia |
| Miasto | Pula pieszych `ACityPopulation`, fizyczny traffic i `UVehicleAIDriverComponent`; bazowy pościg wymaga lepszego śledzenia celu i reakcji na wykroczenia |
| Misje | Kampania oraz niezależne aktywności GIS; brak kompletnej dostawy pieszo/samochodem z jednym trwałym rozliczeniem |
| Ekonomia/zapis | `UWTGEconomySubsystem` ma idempotentny ledger; zapis miasta nie obejmuje kompletnej nowej sesji dostawy i ekonomii |
| Oprawa | Istnieją materiały PBR, CC0 modele i 311 zaimportowanych animacji; nie potwierdzono fotorealizmu ani pełnego wizualnego QA |
| Walidacja | Dostępne lokalne UHT/UBT i proces UE. Computer Use nie ma połączenia z native pipe; nie można uczciwie deklarować ręcznego oglądu |

## Plan: osobna gałąź i PR dla każdego punktu

1. **Rozpoznanie** — niniejszy audyt i plan; bez zmiany silnika.
2. **Fragment Wrocławia** — ograniczony generator na rzeczywistych danych OSM:
   ulice, skrzyżowanie, podwórko, bulwar, woda i punkt orientacyjny. Umiejscowienie
   postaci, auta, chodników i misji w jednym układzie centymetrowym. Rozsądny budżet
   geometrii i źródłowy raport, bez oznaczania całego miasta jako ukończonego.
3. **Podstawa rozgrywki** — płynny ruch i animacje, kamera, stabilny samochód,
   bezpieczne wyjście, piesi/traffic ze skrzyżowaniem oraz wykroczenia i pościg.
4. **Pierwsza misja** — autorska dostawa „Przesyłka znad Odry”: rozmowa, auto,
   trasa, doręczenie, jednorazowa nagroda; ponowienie po utracie pojazdu lub śmierci.
5. **Oprawa i atmosfera** — skala materiałów, światło, woda, polskie detale,
   odgłosy miasta i pojazdu; reuse audytowanych źródeł i pipeline dla nowych siatek.
6. **HUD i zapis** — zdrowie/pieniądze/poszukiwanie/cel/kierunek/prędkość,
   interakcje, istniejąca pauza/ustawienia; odrębny zwalidowany zapis sesji z
   pozycją, samochodem, misją i ledgerem nagród.
7. **Pełna sesja** — kompilacja, generacja i faktyczny start; automatyczne kontrole
   kolizji, kamery, wejścia/wyjścia, restartu oraz save/load; pomiary wyłącznie z
   uruchomionego programu. Dokumentacja poleceń, sterowania i ograniczeń.

Nie dodajemy workflow GitHub Actions ani nie uruchamiamy CI. PR-y zawierają żywy
stan pracy. `docs/IMPLEMENTATION-STATE.md` jest dodatkowym punktem wznowienia.

## Odbiór pierwszego rezultatu

Uruchomienie → eksploracja piesza → rozmowa i wejście do auta → przejazd →
doręczenie → jedna nagroda → zapis → odczyt zachowujący postęp i pieniądze.
Wyjście z auta nie gubi misji. Utrata auta/śmierć pozwala ponowić zadanie. Żaden
błąd zapisu nie może po cichu nadpisać uszkodzonego lub nowszego formatu.

## Referencje i źródła

- Umiejscowienie i geometria: wersjonowane `Data/source/wroclaw` (OSM ODbL,
  źródła terenu i ich metadane), a nie arbitralna makieta udająca rzeczywisty GIS.
- [Wyspa Słodowa — Zarząd Zieleni Miejskiej](https://zzm.wroc.pl/obiektyzzmcp/wyspa-slodowa/)
  i [opis miasta](https://www.wroclaw.pl/dla-mieszkanca/wyspa-slodowa-poznaj-wroclaw)
  są referencjami układu wysp, kładek i bulwarów. Zdjęcia nie są dołączane do gry.
- Assety: istniejące audytowane katalogi CC0 w `Data/free_*_catalog.json` i
  `Data/model_bindings.json`; ograniczenia jakości źródłowej dokumentujemy jawnie.
