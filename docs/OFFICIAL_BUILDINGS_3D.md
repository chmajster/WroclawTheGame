# Oficjalne modele 3D budynków Wrocławia

## Źródło

Integracja korzysta z państwowego zbioru **GUGiK / Geoportal — Modele 3D budynków**:

- informacja o danych: https://www.geoportal.gov.pl/pl/dane/inne-dane/modele-3d-budynkow/
- usługa pobierania WMS: https://integracja.gugik.gov.pl/cgi-bin/ModeleBudynkow3D
- format źródłowy: CityGML;
- dane są według Geoportalu dostępne bezpłatnie i do dowolnego wykorzystania.

Miejski model 3D Wrocławia ma LoD2 dla ścisłego centrum i wybranych charakterystycznych obiektów:
https://geoportal.wroclaw.pl/mapy/3d/

Miejski model służy tu jako źródło odniesienia wizualnego. Pipeline nie kopiuje danych z przeglądarki miejskiej.

## Obiekty pierwszego zestawu

`Data/wroclaw_landmarks_3d.json` definiuje:

- Stary Ratusz;
- Bazylikę św. Elżbiety Węgierskiej;
- Archikatedrę św. Jana Chrzciciela;
- gmach główny Uniwersytetu Wrocławskiego;
- Halę Targową;
- Muzeum Narodowe we Wrocławiu.

Każdy obiekt jest kotwiczony do istniejącego budynku OSM w `Saved/CityData/sector.json`. Jeśli tag nazwy nie pasuje, importer używa najbliższego obrysu w zadanym promieniu i zapisuje sposób dopasowania w katalogu wynikowym.

## Pobranie i konwersja

```bash
python3 -m pip install -r Scripts/gis/requirements.txt
python3 Scripts/gis/build_city.py --output Saved/CityData
python3 Scripts/gis/fetch_official_buildings_3d.py \
  --city-data Saved/CityData \
  --output Saved/OfficialBuildings3D \
  --require-all
```

Importer:

1. odczytuje `GetCapabilities`;
2. preferuje LoD2, lecz dla obszaru bez LoD2 przechodzi na LoD1;
3. pobiera paczkę powiatu wskazaną przez `GetFeatureInfo`;
4. zapisuje URL i SHA-256 pobranej paczki;
5. odczytuje CityGML bez ładowania całego miasta do pamięci;
6. wybiera budynek najbliższy odpowiadającemu mu obrysowi OSM;
7. trianguluje powierzchnie;
8. przelicza poziomo do EPSG:32633 i układu gry X=wschód, Y=południe, Z=góra;
9. wyrównuje wysokość względną CityGML do poziomu gruntu istniejącego GIS, aby nie mieszać bezpośrednio różnych pionowych układów odniesienia;
10. zapisuje odtwarzalne siatki w `Saved/OfficialBuildings3D/Meshes`.

Można też użyć wcześniej pobranej paczki:

```bash
python3 Scripts/gis/fetch_official_buildings_3d.py \
  --archive D:/GIS/wroclaw_3d.zip \
  --city-data Saved/CityData \
  --output Saved/OfficialBuildings3D \
  --require-all
```

## Integracja z Unreal

Pełny build włącza modele automatycznie:

```powershell
.\Scripts\Build-FullGame.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'
```

`Build-Geography.ps1 -City -OfficialBuildings` wykonuje ten sam tor bez wymuszania pełnej kampanii.

Dla znalezionego landmarku prosta bryła OSM jest wykluczana z generacji `Saved/CityData/Meshes`. `prepare_geography.py` wypieka siatkę CityGML jako `/Game/Generated/OfficialBuildings/<id>` i tworzy aktora `OfficialBuilding_<id>` z kolizją oraz HLOD.

## Granice jakości

To nie jest deklaracja finalnego assetu artystycznego. LoD1 ma rzeczywisty obrys i wysokość, lecz uproszczony dach. LoD2 zachowuje bryłę dachu tam, gdzie zbiór ją udostępnia. Materiał w grze jest obecnie materiałem projektu, nie fototeksturą elewacji.

Geoportal udostępnia również **modele siatkowe 3D mesh** w OBJ z teksturą ze zdjęć ukośnych. To lepsze źródło dla przyszłego hero-passu, lecz jest znacznie cięższe i obejmuje także otoczenie, drzewa i teren. Nie zastępuje ono obecnej integracji per-budynek bez osobnego crop/LOD/UV/QA.

Bramka produkcyjna nadal wymaga kompilacji UE 5.8, wygenerowania mapy, kontroli skali/rotacji/kolizji, HLOD, screenshotów i wizualnego porównania z rzeczywistym obiektem.
