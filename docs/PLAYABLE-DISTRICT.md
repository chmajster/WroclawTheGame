# Grywalny fragment Nadodrze / Odra

Projekt pozostaje grą Unreal Engine 5.8.2 w C++20. Ten etap dodaje osobną
mapę `/Game/Maps/Nadodrze_Playable` opartą na istniejących danych OSM/DEM.
Zakres obejmuje zwarty fragment Paulińskiej/Nadodrza i otoczenia Odry,
nie całe miasto. Źródła: `Data/playable_district.json` i generator GIS.

## Uruchomienie

Na przygotowanym komputerze, w PowerShell z katalogu repozytorium:

```powershell
.\Scripts\Launch-District.ps1
```

Inna instalacja silnika: `-EngineRoot 'ścieżka do UE_5.8'`. Mapę można też
otworzyć w edytorze i uruchomić Play. Domyślna mapa gry to teraz Nadodrze.
Pierwsze uruchomienie przygotowuje tekstury i dane oświetlenia w tle.

Odtworzenie mapy i assetów: `Scripts/Build-PlayableDistrict.ps1` z parametrami
EngineRoot, Blender i Python. Potrzebne są lokalne źródła GIS, zaimportowany
katalog modeli/animacji i Python z pyproj/shapely. Mapy, importowane modele
i Saved są wynikami pipeline; manifesty/źródła pozostają autorytatywne.

## Sterowanie i pierwsza dostawa

| Klawisz | Działanie |
|---|---|
| WASD | Ruch postaci; gaz/hamowanie/cofanie i skręt auta |
| Mysz | Kamera; czułość i odwrócenie Y w ustawieniach |
| Shift / Ctrl / Spacja | Sprint / kucanie / skok |
| Spacja w aucie | Hamulec ręczny |
| E | Rozmowa, wejście do auta, wyjście po zatrzymaniu |
| R | Ponowienie nieudanej dostawy / odzyskanie postaci |
| F5 / F9 | Zapis / odczyt postępu dzielnicy |
| Esc | Pauza, zapis, wczytanie, grafika, audio i sterowanie |
| LPM / PPM | Atak / blok; agresja może uruchomić policję |
| Num 0 / Num + / Num * | Silnik / światła / klakson |

Podejdź do Mai przy pracowni i użyj E. Wsiądź do auta wskazanego przez cel,
przywieź moduł nagłośnienia Olkowi, zatrzymaj auto i doręcz paczkę pieszo.
Limit wynosi 7 minut; nagroda to 180 zł. Wysiadanie zachowuje zadanie.
Utrata auta, śmierć lub timeout umożliwiają restart. Ukończenie wymaga
zgubienia poszukiwania. Rozliczenie jest zapisane razem z jednorazowym ID
transakcji; nieudany zapis cofa rozliczenie i pozwala ponowić próbę.

Slot `Saved/SaveGames/WroclawDistrict_v1.sav` jest oddzielny od kampanii
Przebudzenie i wcześniejszego miasta. Zapis obejmuje postać, auto, pieniądze,
stan misji, wygląd i poziom poszukiwania; odczyt zatrzymuje pojazd. Autosave
co 30 s i autoload przy uruchomieniu. Uszkodzony/nowszy plik jest zachowany,
a zapis blokowany do świadomego rozpoczęcia nowej gry.

## Kontrole i ograniczenia

`Scripts/Test-PlayableDistrict.ps1` wykonuje natywne testy reguł i kontrole
w faktycznym świecie UE na osobnych slotach testowych. Testy nie zastępują
ręcznej rozgrywki i nie nadpisują slotu gracza.

Potwierdzone: build UHT/UBT, cztery testy WTG.District, osiem testów GIS,
pipeline ram/balustrad, przygotowanie mapy/animacji/audio, render HUD i
SaveSmoke pieszo/w aucie/complete/reward once/future version preserved.
Pełny SessionSmoke PASS: 151,88 s, 459,48 m rzeczywistej jazdy, rozpoczęcie,
restart, wejście/wyjście, dostawa, utrata wanted, reakcja policji i zapis.
Replay w nowym procesie PASS: ukończona misja, 430 zł i pojedyncza transakcja.
Pomiar dotyczy automatycznego sterowania przez tę samą fizykę, bez teleportacji
przejazdu; nie jest ręcznym testem odczuć sterowania.

Oprawa jest rozwijanym prototypem: uproszczone bryły elewacji i modele CC0,
bez kompletnej garderoby, autorskiego voice acting i szczegółowych modeli
wszystkich zabytków. Nie jest to oprawa klasy GTA VI ani pełny Wrocław.
Mosty nie mają pomiaru rzeczywistego prześwitu. Populacja jest ograniczona,
policja podstawowa (radiowozy/pościg/szukanie); brak pełnej symulacji miasta.
Lumen/PBR/HISM działają; małe siatki nie uzasadniają Nanite. Zrzuty pochodzą
z renderera UE, bez ręcznej kontroli aplikacji przez Computer Use.

Dotychczasowe pomiary: 1234 klatki, mean17,83ms/p9518,87ms oraz 1100 klatek,
mean20,00ms/p9528,07ms przy limicie60FPS i przygotowaniu Lumen w tle.
Końcowy render: 1310 klatek, średnia16,78ms, p9516,67ms (limit60FPS).
Zrzuty `Saved/Tests/DistrictPlayer.png`, `DistrictCar.png`, `DistrictStreet.png`
i `DistrictRiver.png` pochodzą z faktycznego renderera UE.
To pomiary czasu klatki tej sesji, nie benchmark GPU ani gwarancja FPS.
