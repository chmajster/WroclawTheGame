# Wave 1 — rzeczywiste dane i fundament wspólnego świata

## Stan dostawy

Ta zmiana rozpoczyna Wave 1. **Nie kończy Wave 1 do poziomu Playable.** Unreal Engine nie jest dostępny w środowisku implementacji. Nie potwierdzono kompilacji UHT/UBT, wypieku assetów, jazdy, kolizji, streamingu, HLOD ani wydajności.

Dane obejmują południe Wrocławia, ciągły łącznik przez centrum i Nadodrze. OSM i teren są zapisane w repozytorium z metadanymi źródeł i sumami kontrolnymi. Build nie wykonuje zapytań sieciowych. Duży snapshot OSM jest przechowywany w częściach `.part001`–`.part003`; importer składa je w pamięci, sprawdzając sumę każdej części i całego archiwum. Każdy główny obszar Wave 1 jest reprezentowany przez komórkę produkcyjną. **Prostokąty nie są granicami administracyjnymi osiedli** i nie oznaczają pełnego pokrycia nazwanych dzielnic.

## Jeden świat GIS

`Build-Geography.ps1 -City` rozszerza dotychczasowy `/Game/Maps/Nadodrze_GIS`; nie dodaje map dla poszczególnych dzielnic. Zachowuje origin EPSG:32633 `[641702, 5665787, 115]`, skalę 100 cm/m i osie X wschód, Y południe. Powiększenie bbox nie przesuwa istniejących punktów ani tras Nadodrza. Osobne uruchomienie bez `-City` odtwarza mniejsze laboratorium.

Kampania `Przebudzenie_Source` nadal jest oddzielnym blockoutem. Migracja kampanii do GIS wymaga przeniesienia interakcji, wnętrz, punktów zapisu i nawigacji. Ta zmiana nie przypisuje starego gameplayu do nowych współrzędnych i nie modyfikuje istniejących questów ani formatu zapisu.

## Uruchomienie

```powershell
python -m pip install -r Scripts/gis/requirements.txt
python Scripts/gis/build_city.py
# Przegląd raportu: Saved/CityData/coverage.json
.\Scripts\Build-Geography.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.6' -City -Package
```

W edytorze otwórz `/Game/Maps/Nadodrze_GIS`. W Development komenda konsoli `CityCoverage` przełącza podgląd sektorów, ich statusów i pozycji gracza. Overlay nie jest dostępny w Shipping. Telefon pobiera listę obszarów z tego samego katalogu co overlay. Jest to lista, nie pełny GPS ani mapa drogowa.

`UCityDefinition` jest importowany z wygenerowanego JSON i wskazywany przez stale załadowany `ACityRegistry`; dzięki twardej referencji katalog trafia do cookingu. `UWroclawMapSubsystem` udostępnia dane i wyszukuje sektor po położeniu. `UCityCoverageSubsystem` prezentuje status. Brak katalogu na mapie kampanii nie powoduje podmiany jej dzielnic.

## Dane i rozszerzanie

- `Data/city.json`: stabilne ID dzielnic/sektorów, komórki produkcyjne, ringi, profile architektury i cele contentu. `content_targets` to wymagania do wykonania, **nie gotowe wnętrza lub aktywności**.
- `Saved/CityData/sector.json`: jeden globalny graf oraz geometria w stałym układzie współrzędnych.
- `routing.json`: właściciele węzłów, rzeczywiste punkty połączenia sektorów i dwukierunkowa osiągalność samochodowa/piesza względem Nadodrza.
- `streets.json`: wspólny słownik nazw, stabilne dla danej nazwy identyfikatory i odwołania do OSM ways. Zmiana nazwy ulicy wymaga przyszłego aliasu/migracji ID.
- `addresses.json`: rzeczywiste adresy z tagów OSM z `street_id`, numerem, geolokalizacją i `building_id`. Nie tworzy fikcyjnych adresów. Wnętrza należy wiązać z tym samym `building_id`.
- `courtyards.json`: dziedzińce wykryte jako wewnętrzne pierścienie footprintów. `accessible=false` do czasu ręcznej oceny bram, przejść i nawigacji. Nie wykrywa wszystkich kwartałów złożonych z oddzielnych budynków.
- Profile zmieniają paletę materiałów budynków. Stabilny hash ID wybiera materiał, więc ponowny import nie losuje fasad. To nadal bryły GIS o poziomie `Background`, bez realistycznie opracowanych fasad i mieszania materiałów na granicach.

Aktualizacja źródeł jest osobną, jawną czynnością:

```powershell
python Scripts/gis/fetch_wave1.py --download
```

Skrypt pobiera małe wycinki sekwencyjnie, dzieli zbyt gęste obszary, zachowuje sumy odpowiedzi i scala obiekty po OSM type/ID. Pobieranie nie stanowi transakcyjnego snapshotu OSM; przy kolizji wersji wybiera nowszą. Dzienne pliki tymczasowe w `Saved/SourceDownloads` umożliwiają wznowienie. Nowy teren zawiera także fragment kafla N51E016, ponieważ zachodnia krawędź wykracza poza 17°E.

## Routing

`HierarchicalRouter` jest **implementacją offline w Pythonie**, testowaną względem globalnej najkrótszej ścieżki. Oddziela lokalne przeszukiwania wewnątrz sektora od przejść przez portale. Obsługuje ponowne wejście do tego samego sektora, jednokierunkowość i osobny tryb pieszy. Nie łączy bliskich geometrycznie dróg bez wspólnej topologii OSM. Graf district route można odczytać z sekwencji sektorów i ich przypisania do dzielnic; osobnego trzeciego poziomu optymalizacji jeszcze nie wdrożono.

Mosty, tunele i niezerowe warstwy są wykluczane z tras do czasu opracowania wysokości. Osiągalność w grafie **nie dowodzi fizycznej przejezdności** w Unreal. Brakuje importu routingu do runtime VehicleAI, ograniczeń skrętów OSM, pełnej sieci tramwajów i dynamicznych blokad.

## Statusy i odbiór

`Missing → GISOnly → Blockout → Playable → Detailed → Final`.

Geometria sama daje najwyżej `GISOnly`. `Blockout` wymaga potwierdzonego wypieku w silniku. `Playable` wymaga także: układu, kolizji, nawigacji pieszej, ruchu, populacji, ambientu, miejsca gameplayowego, aktywności, zdarzeń świata, sekretu, save/load, streamingu, wydajności i obustronnej osiągalności pieszo/samochodem. `Detailed` i `Final` wymagają dalszych odbiorów.

Raport `CityCoverageReport` jest zapisywany jako `coverage.json`. `RoadCoverage` i `BuildingCoverage` oznaczają ułamek zarejestrowanych sektorów mających geometrię GIS. `PedestrianCoverage`, `TrafficCoverage` i `QuestCoverage` wymagają dowodów silnikowych. Żadna z tych liczb nie oznacza procentu powierzchni całego Wrocławia.

Dowody są opcjonalnym plikiem odbioru:

```json
{
  "schema_version": 1,
  "fingerprint": "dokładna wartość z aktualnego coverage.json",
  "sectors": {
    "sector.huby": {
      "engine_bake": {"passed": true, "evidence": "ścieżka lub identyfikator faktycznego raportu UE"}
    }
  }
}
```

```powershell
python Scripts/gis/build_city.py --evidence Saved/CityVerification.json --require-playable
```

Brak któregokolwiek wymaganego kryterium blokuje odbiór. Dowody muszą być wprowadzone po rzeczywistych testach; narzędzie waliduje ich strukturę i fingerprint, ale nie uwierzytelnia treści zewnętrznych raportów. Zmiana źródeł, katalogu, kodu GIS/UE lub konfiguracji unieważnia poprzedni fingerprint. Build z `--require-playable` na dostarczonym stanie powinien zakończyć się błędem — to prawidłowa blokada odbioru.

## Następna praca w Unreal

1. UHT/UBT, import mapy, kontrola transformacji GeoReferencing i World Partition, HLOD, pomiar pamięci oraz jazda/pieszy przegląd wszystkich czterech sektorów.
2. Wysokości tuneli i mostów, rzeczywiste skrzyżowania, kolizje i nawigacja, optymalizacja komórek siatek. Obecny generator nadal grupuje trójkąty według środka, więc długie obiekty mogą przekraczać granice komórek.
3. Huby: klatki, mieszkanie, opuszczony lokal, parking i przejścia; Gaj/Borek/Krzyki: miejsca z `content_targets`.
4. Populacja, traffic runtime, ambient, po jednej aktywności, kilku zdarzeniach i sekrecie na sektor; testy save/load i migracji trwałych ID.
5. Połączenie kampanii ze światem GIS. Dopiero po odbiorze Wave 1 — pełny content pass Wave 2.

Pozostałe wymagania załącznika, m.in. predykcja streamingu, pooling, pamięć według kategorii, questy wielodzielnicowe, AlleyNetwork, stany zagrożenia, trasy ucieczki i automatyczny city tour w silniku, pozostają niewdrożone. Nie zastąpiono ich pustymi klasami.

## Weryfikacja tej zmiany

- `ASAN_OPTIONS=detect_leaks=0 bash Scripts/test.sh`: 25 testów Python i trzy zestawy testów C++ przeszły; w tym 12 ścieżek kampanii oraz 50 000 mieszanych interakcji. LeakSanitizer został wyłączony z powodu niezgodności środowiska z ptrace; ASan i UBSan pozostały aktywne.
- Dodatkowy test dzielonego źródła: poprawne składanie, odrzucenie uszkodzonej części i niedozwolonej ścieżki.
- Generacja siatek offline: 10 845 komórek, 1 558 081 trójkątów; sprawdzono zakresy indeksów, kompletność trójek i skończoność współrzędnych. Nie jest to weryfikacja kolizji ani renderowania w UE.
- Import zwraca ostrzeżenia `polygonize` dla części relacji OSM. Dane wymagają ręcznej kontroli geometrii; nie podniesiono ich statusu ponad GISOnly.
- Kontrola `--require-playable` ma odrzucać ten stan. Cztery południowe sektory mają blokady połączeń z Nadodrzem, a wszystkie sektory nie mają odbioru silnikowego.
