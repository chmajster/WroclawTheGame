# Oficjalne modele 3D budynków Wrocławia

## Źródło

Integracja korzysta z państwowego zbioru **GUGiK / Geoportal — Modele 3D budynków**:

- informacja o danych: https://www.geoportal.gov.pl/pl/dane/inne-dane/modele-3d-budynkow/
- aktualna usługa pobierania WMS: https://mapy.geoportal.gov.pl/wss/service/PZGIK/FOTO/WMS/ModeleBudynkow3D
- format źródłowy: CityGML;
- dane są według Geoportalu dostępne bezpłatnie i do dowolnego wykorzystania.

Miejski model 3D Wrocławia ma LoD2 dla ścisłego centrum i wybranych charakterystycznych obiektów:
https://geoportal.wroclaw.pl/mapy/3d/

Miejski model służy tu jako źródło odniesienia wizualnego. Pipeline nie kopiuje danych z przeglądarki miejskiej.

## Zakres importu

`Scripts/gis/fetch_official_city_buildings.py` importuje automatycznie wszystkie modele z oficjalnej paczki, których centroid znajduje się w aktywnych sektorach `Data/city.json` i które można bezpiecznie powiązać z istniejącą bryłą budynku OSM. Dopasowanie preferuje pokrycie footprintów; sam dystans centroidu jest fallbackiem. Jeden obrys OSM może zostać zastąpiony tylko raz.

Wynik jest grupowany przestrzennie w komórki 128 m i zapisywany w `Saved/OfficialBuildings3D/Meshes`. `catalog.json` zachowuje osobny rekord każdego budynku: identyfikator GUGiK, zastąpiony feature OSM, sektor, odległość dopasowania i współczynnik pokrycia footprintu.

`Data/wroclaw_landmarks_3d.json` nadal definiuje sześć obiektów priorytetowych do ręcznego hero-QA:

- Stary Ratusz;
- Bazylikę św. Elżbiety Węgierskiej;
- Archikatedrę św. Jana Chrzciciela;
- gmach główny Uniwersytetu Wrocławskiego;
- Halę Targową;
- Muzeum Narodowe we Wrocławiu.

## Pobranie i konwersja

```bash
python3 -m pip install -r Scripts/gis/requirements.txt
python3 Scripts/gis/build_city.py --output Saved/CityData
python3 Scripts/gis/fetch_official_city_buildings.py \
  --city-data Saved/CityData \
  --output Saved/OfficialBuildings3D
```

Importer:

1. odczytuje `GetCapabilities`;
2. wybiera najnowszą warstwę LoD1 z aktualnej usługi GUGiK (obecnie LoD1-2024); jeśli usługa nie zwróci paczki, może użyć zapisanej konfiguracji fallback TERYT `0264`;
3. zapisuje pobraną paczkę źródłową w cache builda;
4. zapisuje URL i SHA-256 pobranej paczki;
5. odczytuje CityGML bez ładowania całego miasta do pamięci;
6. filtruje budynki do aktywnych sektorów gry i wiąże je z OSM na podstawie nakładania footprintów, z dystansem jako fallbackiem;
7. trianguluje powierzchnie;
8. przelicza autorytatywne współrzędne poziome GUGiK do EPSG:32633 i układu gry X=wschód, Y=południe, Z=góra; obrys OSM nie przesuwa modelu, służy tylko do wyłączenia zastępowanej bryły;
9. wyrównuje wysokość względną CityGML do poziomu gruntu istniejącego GIS, aby nie mieszać bezpośrednio różnych pionowych układów odniesienia;
10. zapisuje odtwarzalne siatki w `Saved/OfficialBuildings3D/Meshes`.

Można też użyć wcześniej pobranej paczki:

```bash
python3 Scripts/gis/fetch_official_city_buildings.py \
  --archive D:/GIS/wroclaw_3d.zip \
  --city-data Saved/CityData \
  --output Saved/OfficialBuildings3D
```

## Integracja z Unreal

Pełny build włącza modele automatycznie:

```powershell
.\Scripts\Build-FullGame.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'
```

`Build-Geography.ps1 -City -OfficialBuildings` wykonuje ten sam tor bez wymuszania pełnej kampanii.

Dla każdego bezpiecznie dopasowanego budynku prosta bryła OSM jest wykluczana z generacji `Saved/CityData/Meshes`. Modele GUGiK są łączone w komórki 128 m; `prepare_geography.py` wypieka każdą komórkę jako `/Game/Generated/OfficialBuildings/<cell>` i tworzy aktora z kolizją oraz HLOD.

## Granice jakości

To nie jest deklaracja finalnego assetu artystycznego. Krajowy model GUGiK dla Wrocławia jest LoD1: ma rzeczywisty obrys i wysokość, lecz uproszczony płaski dach. Miejski model 3D Wrocławia ma LoD2 dla ścisłego centrum i wybranych obiektów, ale nie jest w tej implementacji kopiowany ani traktowany jako paczka pobierana. Materiał w grze jest obecnie materiałem projektu, nie fototeksturą elewacji.

Geoportal udostępnia również **modele siatkowe 3D mesh** w OBJ z teksturą ze zdjęć ukośnych. To lepsze źródło dla przyszłego hero-passu, lecz jest znacznie cięższe i obejmuje także otoczenie, drzewa i teren. Nie zastępuje ono obecnej integracji per-budynek bez osobnego crop/LOD/UV/QA.

Bramka produkcyjna nadal wymaga kompilacji UE 5.8, wygenerowania mapy, kontroli skali/rotacji/kolizji, HLOD, screenshotów i wizualnego porównania z rzeczywistym obiektem.
