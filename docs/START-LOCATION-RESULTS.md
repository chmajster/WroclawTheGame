# Lokacja początkowa — wynik przebudowy

Zakres: istniejący start Nadodrza przy Pomorskiej/Paulińskiej, około 100 m.
Oryginalne położenie gracza, punkty misji i trasa dostawy są zachowane.
Gra i domyślny podgląd edytora wskazują teraz `Nadodrze_Playable`.
Nie przełączamy ani nie zapisujemy otwartego edytora użytkownika.

## Wykonane zmiany

- Osiem nowych pełnych modeli: Pomorska 43, 45, 47 i 32, Paulińska 2a,
  Nobla 3 oraz dwa obrysy podwórzowe. Otwory okienne, cofnięte szkło, ramy,
  nadokienniki, gzymsy, balkony, partery usługowe, dachy, rynny i kominy.
- Pięć własnych modeli: krawężnik, wpust, stojak rowerowy, młode drzewo,
  samochód dekoracyjny. Reużycie istniejących lamp, ławek i koszy.
- 13 edytowalnych modeli Blender/GLB, 39 eksportów FBX LOD, rzeczywisty
  import i 72 natywne uasset w obszarze StartLocation.
- 2272 instancje w 29 HISM, 50 odcinków szyn i 29 pobliskich komórek PBR.
  Rozmieszczenie odtwarzane ze źródeł GIS/OSM oraz jawnych oszacowań.
  Bez fikcyjnej sygnalizacji; dekoracyjne auta omijają trasy pojazdów.
- Poprawiona ekspozycja światła dziennego i współpraca z istniejącą pogodą;
  poprawione flagi materiałów i nadmierna okluzja cienkiego gruntu w Lumen.
- Bezpieczna wysokość kapsuły, zgodny kierunek startu i odzyskiwania,
  kolizja kamery, 74 hulle ścian i osobna kolizja pni bez blokowania liśćmi.
- Nanite budynków, trzy LOD, pełny fallback, progi zachowujące gałęzie
  blisko gracza, wygaszanie instancji 180–260 m i streaming map do 2k.
- Wersjonowane źródła, manifesty, katalog licencji i wykonywalny punkt
  wejścia `Scripts/Invoke-StartLocationProduction.ps1`.

## Zasoby i referencje

Powered by Poly Haven. Materiały mają licencję CC0-1.0:

| Zestaw | Autorzy | Źródło |
|---|---|---|
| Painted Plaster Wall | Amal Kumar | [Poly Haven](https://polyhaven.com/a/painted_plaster_wall) |
| Asphalt 03 | Dario Barresi, Charlotte Baglioni | [Poly Haven](https://polyhaven.com/a/asphalt_03) |
| Concrete Pavement 03 | Charlotte Baglioni | [Poly Haven](https://polyhaven.com/a/concrete_pavement_03) |
| Cobblestone Floor 08 | Rob Tuytel | [Poly Haven](https://polyhaven.com/a/cobblestone_floor_08) |
| Roof Tiles 14 | Rob Tuytel | [Poly Haven](https://polyhaven.com/a/roof_tiles_14) |
| Grass Ground | Charlotte Baglioni | [Poly Haven](https://polyhaven.com/a/grass_ground) |
| Sandstone Cracks — wcześniejszy zestaw | Rob Tuytel | [Poly Haven](https://polyhaven.com/a/sandstone_cracks) |

Reużyte modele CC0: Street Lamp 01 — Josh Dean; Modular Street Seating —
Stuart Attenborrow; Metal Trash Can — GurJas Studios. Dokładne URL, pliki,
licencje, rozmiary i sumy MD5/SHA256 są w `Data/start_location_asset_catalog.json`
oraz `Data/city_asset_acquisition.json`. 21 map rzeczywiście pobrano/sprawdzono.
Nowe siatki mają własny generator bpy; obrysy budynków pochodzą z OSM ODbL.
Miejskie fotografie remontu Pomorskiej i Pomorskiej 47 posłużyły wyłącznie
jako referencje. Są opisane w `Data/start_location.json`; nie dystrybuujemy ich.

## Faktycznie wykonane sprawdzenia

| Kontrola | Wynik |
|---|---|
| Blender, 39 FBX, import UE 13 siatek | PASS |
| Natywne skale, UV, przypisania materiałów, sekcje LOD, collidery | PASS, 13 siatek |
| Budżet nowych obiektów wraz z instancjami | PASS, 466628 trójkątów LOD0 przy limicie 1600000 |
| Źródła, sumy 21 map, 28 manifestów, katalog ścieżek | PASS |
| Lokalne testy źródeł i parsery | PASS; szczegóły etapów w raporcie produkcji |
| Natywne DeliveryRules, GeneratedMeshCollision, SaveValidation, WantedDecay | 4 × PASS |
| Gracz, osiem ścian, podłoga, kamera, zwykłe chodzenie | PASS, dystans 2,01 m |
| Zapis/odzyskiwanie/pojazd | PASS |
| Pełna misja i fizyczna jazda | PASS, 460,04 m / 147,56 s |
| Odczyt sesji w nowym procesie | PASS |
| Cztery nowe obrazy gry, obejrzane po poprawkach | PASS |
| FPS, RTX 5070 Ti / D3D12 / 1280×720 | 59,65 FPS, średnio 16,764 ms; p95 17,684 ms |

Pomiar obejmuje 1163 klatki po 50 s rozgrzewania. Końcowe `t.MaxFPS=60`:
jest to przebieg z limitem, nie benchmark maksymalnej wydajności GPU.
Wcześniejszy przebieg miał średnio 16,78 ms przy limicie 60 z inną kamerą
i metodą próbkowania. Ten pomiar nie wykazuje dużego spadku w sprawdzonej scenie.

Po ponownym otwarciu głównego edytora linker nie mógł podmienić DLL.
Testy wykonano na rzeczywiście skompilowanej kopii identycznych 171 plików C++,
z tymi samymi Content/Data przez junction i osobnym katalogiem zapisów.
Zmieniono tylko opcję adaptive unity celu kompilacji kopii. Udokumentowany
build kopii zakończył się powodzeniem. Nie twierdzimy, że podmieniono zablokowaną
DLL głównego, nadal otwartego edytora. Zmiana jego mapy startowej dotyczy
następnego uruchomienia; nie zmienia mapy gry ani wyników tych testów.

Dowody: `docs/qa/start-location-2026-10-12/` — obrazy przed/po, raporty
importu/jakości/gry, sumy źródeł i zasobów oraz wyniki testów projektu.
Ogólne błędy startowych testów silnika zostały oddzielone od pozytywnych
wyników projektu; szczegółowe surowe logi pozostają w Saved.

## Ograniczenia i dalsze kierunki

To istotna poprawa istniejącej lokacji, a nie deklaracja ukończonej oprawy AAA.
Wysokości i rytm okien są oszacowane, dachy uproszczone, wnętrza niedostępne.
Pozostałe budynki i dotychczasowe postacie zachowują wcześniejszą oprawę;
test wykorzystuje świeży domyślny profil postaci. Układ ma źródłowe szerokości
z istniejącymi wartościami zastępczymi, bez deklarowania pomiarów geodezyjnych.
Mapa ma komórki GIS i HISM, bez World Partition. FPS sprawdzono na jednym
komputerze i w określonym obszarze; nie jest to benchmark całego miasta.

Nowy punkt wejścia faktycznie sprawdzono w trybie Validate. Etapy źródłowe,
eksport, import i testy natywne wykonano wcześniej oddzielnie; nie powtarzano
całego Full jako jednego polecenia przy otwartym edytorze. Raport nigdy nie
przypisuje powodzenia etapom niewykonanym w wybranym trybie.

## Strumienie realizacji

Każdy numer ma osobną gałąź i PR do main:

1. Audyt — [#152](https://github.com/chmajster/WroclawTheGame/pull/152).
2. Referencje i obrysy — [#153](https://github.com/chmajster/WroclawTheGame/pull/153).
3. Legalne materiały — [#154](https://github.com/chmajster/WroclawTheGame/pull/154).
4. Modele i import — [#155](https://github.com/chmajster/WroclawTheGame/pull/155).
5. Integracja otoczenia — [#156](https://github.com/chmajster/WroclawTheGame/pull/156).
6. Start i kolizje — [#157](https://github.com/chmajster/WroclawTheGame/pull/157).
7. Jakość i optymalizacja — [#158](https://github.com/chmajster/WroclawTheGame/pull/158).
8. Testy gry — [#159](https://github.com/chmajster/WroclawTheGame/pull/159).
9. Organizacja i pipeline — [#160](https://github.com/chmajster/WroclawTheGame/pull/160).
10. Odbiór i raport — końcowy PR bieżącej gałęzi.

Wykonane: powyższy zakres wdrożony i sprawdzony. Do zrobienia: nic w zakresie
tej przebudowy. Następny krok: brak wymaganej kontynuacji; kolejne zmiany mogą
dotyczyć dokładniejszych dachów, fasad tła i oprawy postaci. Stan: zakończone.
Nie uruchamiano, nie ponawiano ani nie oczekiwano na CI.
