# Rzeczywisty sektor Nadodrza

Obszar: długość 17.025–17.038° E, szerokość 51.118–51.126° N. Obejmuje m.in. Pomorską, Rydygiera, Paulińską, Jagiellończyka i Trzebnicką. To źródłowy wycinek miasta, nie siatka ulic narysowana na potrzeby gry.

## Dane i licencje

- `Data/source/wroclaw/osm/nadodrze.osm.gz`: pobrany 17.09.2026 wycinek [API OpenStreetMap](https://www.openstreetmap.org/api/0.6/map?bbox=17.025,51.118,17.038,51.126). Usunięto dane kont współtwórców; zachowano ID, geometrię i tagi. [© OpenStreetMap contributors, ODbL 1.0](https://www.openstreetmap.org/copyright). Przetworzona baza OSM podlega ODbL; licencja kodu projektu nie zastępuje licencji danych.
- `Data/source/wroclaw/terrain/nadodrze.csv`: wycinek oryginalnej siatki wysokości z [N51E017.hgt.gz](https://s3.amazonaws.com/elevation-tiles-prod/skadi/N51/N51E017.hgt.gz), udostępnionej w [Terrain Tiles](https://registry.opendata.aws/terrain-tiles/). Źródła: Mapzen, Copernicus EU-DEM oraz USGS SRTM/GMTED2010; [warunki i atrybucja dostawców](https://github.com/tilezen/joerd/blob/master/docs/attribution.md). EU-DEM: dane opracowane z wykorzystaniem informacji Copernicus finansowanych przez Unię Europejską. Dane SRTM/GMTED2010 dzięki USGS. CSV jest przycięciem źródła; nie zatwierdza go ani nie gwarantuje dostawca. To raster rzędu dziesiątek metrów, **nie LiDAR ani dokładny model krawężników**.
- Metadane obok źródeł zapisują URL, datę pobrania, licencję/warunki, CRS, datę importu, wersję przetwarzania i SHA256. `SourceDate` oznacza snapshot pobrania, nie datę pomiaru każdego obiektu.

Nie użyto modeli ani zrzutów Google Maps. Dodano odtwarzalny tor pobierania oficjalnych modeli budynków GUGiK przez usługę `ModeleBudynkow3D`; paczka, URL i SHA-256 są zapisywane w `Saved/OfficialBuildings3D`, a wybrane landmarki zastępują proste bryły OSM. Miejski model 3D Wrocławia jest referencją wizualną, nie kopiowanym źródłem danych. Szczegóły: [oficjalne budynki 3D](OFFICIAL_BUILDINGS_3D.md). Ogólna etykieta `Terrain-Tiles-provider-terms` odsyła do warunków konkretnych źródeł, nie oznacza public domain całego rastra.

## Konwersja i powtarzalność

Wymagany Python 3.12+ dla przypiętej wersji pyproj 3.8.

```bash
python3 -m pip install -r Scripts/gis/requirements.txt
python3 Scripts/gis/import_sector.py
python3 Scripts/gis/build_meshes.py
```

Importer sprawdza SHA256, przycina geometrię, obsługuje otwory multipolygonów i przelicza EPSG:4326 → EPSG:32633. Początek jest zapisany jako całkowite współrzędne UTM; UE: X wschód, Y południe, Z góra, 100 cm/m. Wysokości i początek Z pozostają ortometryczne EGM96. Wrapper konwertuje współrzędne poziome przez GeoReferencing, a wysokość osobno; nie deklaruje wykonania transformacji geoidy do wysokości elipsoidalnej.

Źródła i manifest są w git. Duży `sector.json` oraz siatki w `Saved/GISMeshes` są odtwarzane. Rozdzielono `Data/source` i `Data/processed` przy zachowaniu jednej wielkości liter katalogu Data, aby uniknąć rozbieżności Windows/Linux.

Graf łączy tylko wspólne ID węzłów OSM, nie każde geometryczne skrzyżowanie linii. Uwzględnia oneway i podstawowe ograniczenia access/vehicle/foot. Nie obsługuje jeszcze relacji zakazów skrętu ani kompletnych reguł polskiego ruchu. Drogi na innych warstwach, mosty i tunele nie trafiają do prób jazdy przed odbiorem wysokości. Szerokości dróg i część wysokości budynków są przybliżeniem, obrysy pozostają źródłowe. Nie jest to mapa nawigacji do rzeczywistej jazdy.

Siatki grupowane są przestrzennie w komórki 128 m, a następnie zapisywane jako Static Mesh. Teren korzysta z interpolacji rastra, nie płaskiej płyty. Budynki są bryłami z prawdziwym obrysem i otworami dziedzińców; elewacje, dachy, wejścia i wnętrza nie zostały zinwentaryzowane. Powierzchnie drogowe nadal wymagają dopracowania skrzyżowań, mostów, krawężników i chodników. Landscape, HLOD w praktyce i ostateczny budżet pamięci pozostają do sprawdzenia w UE.

## Otwarte zadania

Importer wykonuje obecnie OSM XML + wysokości CSV oraz opcjonalny import wybranych oficjalnych budynków CityGML GUGiK. Hero-pass fototeksturowanych landmarków, obsługa PBF/GeoJSON/SHP, kafli Landscape, pełnych relacji turn-restriction, grafu pasów, walidacja nakładania/ciągłości całej sieci oraz ręczne punkty kontroli geodezyjnej nie są ukończone. Graf pieszy współdzieli krawędzie z grafem dróg; nie jest osobnym kompletnym modelem chodników i przejść. Kampania wymaga osobnego umieszczenia w rzeczywistych budynkach i migracji zapisów.
