# 122. RZECZYWISTA MAPA WROCŁAWIA

WroclawTheGame nie ma posiadać fikcyjnego miasta jedynie inspirowanego Wrocławiem.

Świat gry ma być zbudowany na podstawie rzeczywistej geografii Wrocławia.

Należy zachować możliwie dokładnie:

- przebieg ulic,
- skrzyżowania,
- place,
- mosty,
- rzeki,
- kanały,
- fosę,
- tory kolejowe,
- linie tramwajowe,
- parki,
- główne ciągi piesze,
- rzeczywiste położenie budynków,
- charakterystyczne obiekty,
- układ dzielnic i osiedli.

Gracz znający Wrocław powinien być w stanie orientować się przede wszystkim na podstawie rzeczywistego układu miasta.

Przykład:

jeżeli gracz znajduje się przy Rynku, powinien móc na podstawie prawdziwego układu ulic przejść w kierunku:

Rynek
→ Oławska
→ okolice pl. Dominikańskiego
→ dalsza część miasta.

Nie twórz losowych ulic między prawdziwymi lokalizacjami.

# 123. GEOREFERENCING

Włącz i wykorzystaj system GeoReferencing Unreal Engine.

Świat powinien posiadać rzeczywiste odniesienie geograficzne.

Unreal Engine posiada system GeoReferencing pozwalający powiązać współrzędne świata UE z rzeczywistymi układami współrzędnych oraz konwertować pozycje geograficzne i projektowane.

Przygotuj:

WroclawGeoReferenceSubsystem

który będzie obsługiwać:

GeoToWorld()

WorldToGeo()

LatLonToWorld()

WorldToLatLon()

ProjectedToWorld()

WorldToProjected()

Każda ważna lokacja może posiadać:

Latitude

Longitude

ProjectedCoordinate

WorldCoordinate.

Nie zapisuj ważnych miejsc wyłącznie jako losowych FVector bez informacji geograficznej.

# 124. LARGE WORLD COORDINATES

Projektuj świat z uwzględnieniem Large World Coordinates Unreal Engine 5.

UE5 używa współrzędnych o zwiększonej precyzji właśnie z myślą o dużych światach.

Nie buduj całego Wrocławia jako jednego przesuniętego zestawu meshów bez poprawnego systemu współrzędnych.

# 125. ŹRÓDŁA DANYCH

Preferuj otwarte i oficjalne dane przestrzenne.

Dopuszczalne źródła:

1. OpenStreetMap,
2. System Informacji Przestrzennej Wrocławia,
3. otwarte dane miasta Wrocławia,
4. państwowe dane przestrzenne posiadające odpowiednią licencję,
5. własne dane i modele.

Dane mogą obejmować:

- drogi,
- chodniki,
- buildings footprints,
- waterways,
- railways,
- tramways,
- parks,
- landuse,
- addresses,
- POI,
- terrain,
- elevation.

Przed wykorzystaniem konkretnego datasetu sprawdź jego licencję.

# 126. OPENSTREETMAP

OpenStreetMap może być jednym z podstawowych źródeł geometrii świata.

Importuj dane OSM zamiast ręcznie przepisywać wszystkie ulice.

OpenStreetMap udostępnia dane na licencji ODbL i wymaga odpowiedniej atrybucji. Nie traktuj publicznych kafelków mapy jako darmowej bazy tekstur do masowego pobrania.

Przygotuj pipeline:

OSM

→ ekstrakcja obszaru Wrocławia

→ preprocessing

→ klasyfikacja danych

→ konwersja współrzędnych

→ Unreal Engine

→ World Partition.

# 127. WROCLAW GIS IMPORTER

Stwórz narzędzie developerskie:

WroclawGISImporter

Importer powinien móc przetwarzać dane typu:

GeoJSON

OSM

PBF

CSV

SHP po wcześniejszej konwersji lub poprzez odpowiednią bibliotekę

oraz dane wysokościowe.

Nie uzależniaj gameplayu od działania internetowego API.

Dane należy importować podczas tworzenia świata i przygotowywać do wykorzystania offline.

# 128. PIPELINE DANYCH

Rozdziel:

SOURCE DATA

od:

GAME DATA.

Przykład:

data/source/wroclaw/

osm/

terrain/

roads/

buildings/

water/

rail/

poi/

następnie:

data/processed/wroclaw/

roads.json

buildings.json

districts.json

water.json

rail.json

landmarks.json.

Gra nie powinna parsować ogromnych plików GIS podczas każdego uruchomienia.

# 129. RZECZYWISTA SKALA

Domyślnie zachowuj skalę:

1 metr świata rzeczywistego = 1 metr świata gry.

Dotyczy to szczególnie:

- szerokości ulic,
- długości ulic,
- odległości pomiędzy skrzyżowaniami,
- rzek,
- mostów,
- wielkości placów,
- położenia budynków.

Nie zmniejszaj automatycznie odległości tylko dlatego, że jest to gra.

Jeżeli ze względów gameplayowych konieczne jest odstępstwo, oznacz je w danych:

GameplayAdjusted = true.

# 130. POZIOM DOKŁADNOŚCI

Nie wszystkie elementy muszą mieć taki sam poziom dokładności.

Użyj poziomów:

## GEO\_ACCURATE

Obiekt powinien znajdować się w prawdziwym miejscu i mieć możliwie prawdziwe proporcje.

Przykłady:

- ulice,
- Odra,
- mosty,
- Rynek,
- duże place,
- linie kolejowe.

## LANDMARK\_ACCURATE

Ważne obiekty powinny posiadać ręcznie przygotowany model o wysokiej zgodności wizualnej.

## APPROXIMATE

Budynki drugoplanowe zachowują footprint i wysokość, ale mogą korzystać z modularnej architektury.

## GAMEPLAY\_INTERIOR

Zewnętrzne położenie jest prawdziwe, ale wnętrze zostało zaprojektowane pod gameplay.

## FICTIONAL\_GAMEPLAY

Fikcyjny obiekt dodany celowo do gry.

# 131. BUDYNKI

Importuj rzeczywiste obrysy budynków.

Na ich podstawie można generować podstawową bryłę:

# footprint + height

procedural building shell.

Budynki dziel na:

DecorativeBuilding

ProceduralBuilding

LandmarkBuilding

AccessibleBuilding

QuestBuilding.

Nie modeluj ręcznie każdego budynku.

# 132. WYSOKOŚĆ BUDYNKÓW

Jeżeli dane zawierają wysokość, wykorzystuj ją.

Jeżeli brak wysokości:

spróbuj wykorzystać:

building\:levels

lub inne wiarygodne metadane.

Jeżeli nadal brak:

stosuj rozsądne profile zależne od typu zabudowy.

Nie ustawiaj wszystkich kamienic na identyczną wysokość.

# 133. TEREN

Teren nie może być idealnie płaski.

Wykorzystaj dane wysokościowe.

SIP Wrocławia posiada dane NMT/NMPT oraz LiDAR, które mogą stanowić źródło referencyjne po sprawdzeniu warunków wykorzystania konkretnego zestawu danych.

Pipeline:

terrain source

→ reprojection

→ crop

→ heightmap tiles

→ Unreal Landscape

→ World Partition.

# 134. ODRA I WODA

Odwzoruj rzeczywisty przebieg:

- Odry,
- kanałów,
- fosy miejskiej,
- wysp,
- nabrzeży.

Nie zastępuj Odry losowo wygenerowaną rzeką.

System wody powinien być dopasowany do rzeczywistej geometrii hydrograficznej.

# 135. MOSTY

Mosty są szczególnie ważne dla Wrocławia.

Każdy istotny most powinien posiadać:

BridgeDefinition

BridgeID

RealName

GeoLocation

RoadConnections

PedestrianConnections

GameplayTags.

Mosty powinny rzeczywiście łączyć odpowiadające im części miasta.

Mogą również mieć znaczenie gameplayowe:

- blokada,
- pościg,
- punkt kontrolny,
- quest,
- obserwacja,
- wyścig,
- zasadzka.

# 136. TRAMWAJE I TORY

Importuj rzeczywisty przebieg infrastruktury tramwajowej tam, gdzie dostępne są odpowiednie dane.

Na początku wystarczy geometria torów.

Później system może zostać rozszerzony o:

TramNetwork

TramStop

TramRoute

TramVehicle

PassengerSystem.

Tramwaj powinien poruszać się po prawdziwej sieci, a nie po fikcyjnej trasie niezwiązanej z mapą.

# 137. KOLEJ

Odwzoruj podstawową infrastrukturę kolejową:

- tory,
- dworce,
- wiadukty,
- przejazdy.

Będzie można wykorzystać ją później w kampanii związanej z próbą opuszczenia miasta.

# 138. ROAD GRAPH

Na podstawie rzeczywistych ulic wygeneruj RoadGraph.

Node:

Intersection

RoadSegment

ParkingEntrance

ServiceRoad

BridgeConnection.

RoadGraph wykorzystują:

- samochody,
- TrafficSystem,
- wyścigi,
- AI,
- GPS,
- pościgi,
- system wyznaczania tras.

Nie przygotowuj oddzielnej sieci drogowej dla każdego z tych systemów.

# 139. PRAWDZIWE NAZWY ULIC

StreetDefinition powinien przechowywać:

StreetID

RealName

RoadType

SpeedProfile

RoadGraphNodes

DistrictID

GameplayTags.

Mapa w telefonie gracza może dzięki temu pokazywać prawdziwe nazwy ulic.

# 140. NAWIGACJA GPS

Phone Map ma działać na podstawie tej samej geometrii świata.

Nie twórz niezależnej fikcyjnej mapy do telefonu.

GPS:

WorldPosition

→ GeoPosition

→ RoadGraph

→ Route.

Pozwala to później tworzyć rzeczywiste trasy przez Wrocław.

# 141. WYŚCIGI NA PRAWDZIWYCH ULICACH

RaceSystem ma korzystać z rzeczywistego RoadGraph.

Przykład:

RaceDefinition nie przechowuje wymyślonej planszy.

Przechowuje rzeczywiste punkty w świecie.

Wyścigi mogą przebiegać przez realne fragmenty miasta, ale należy respektować ich rzeczywistą topologię.

Nie twórz drogi między ulicami, które w rzeczywistym Wrocławiu nie są ze sobą połączone.

# 142. POŚCIGI

VehiclePursuitSystem także korzysta z RoadGraph.

Dzięki temu:

- przeciwnicy mogą próbować objechać gracza,
- blokady mogą powstawać na prawdziwych skrzyżowaniach,
- ucieczka boczną ulicą ma rzeczywisty sens,
- znajomość Wrocławia może być przewagą gracza.

# 143. PIESZA NAWIGACJA

Oddziel:

RoadGraph

od:

PedestrianGraph.

PedestrianGraph obejmuje:

- chodniki,
- alejki,
- przejścia,
- podwórka,
- kładki,
- schody,
- pasaże.

Dzięki temu gracze piesi i samochody nie korzystają z identycznej nawigacji.

# 144. LANDMARK SYSTEM

Stwórz LandmarkDefinition.

Przykładowe typy:

Square

Bridge

HistoricBuilding

Station

Church

Park

PublicBuilding

Monument.

Landmark może:

- odkryć fragment mapy,
- rozpocząć quest,
- zawierać zagadkę,
- być checkpointem,
- być punktem orientacyjnym.

# 145. DOKŁADNOŚĆ KONTRA GAMEPLAY

Rzeczywista mapa jest priorytetem, ale gameplay pozostaje nadrzędny we wnętrzach.

Nie przesuwaj ulic bez istotnej przyczyny.

Nie zmieniaj przebiegu rzeki.

Nie zmieniaj lokalizacji mostów.

Można natomiast:

- otworzyć fikcyjne wnętrze,
- dodać przejście przez budynek,
- stworzyć fikcyjny lokal,
- umieścić kryjówkę,
- dodać wejście gameplayowe,
- zmodyfikować wnętrze budynku.

Zewnętrzna geografia pozostaje możliwie zgodna z rzeczywistością.

# 146. WNĘTRZA PRYWATNYCH BUDYNKÓW

Nie próbuj odwzorowywać rzeczywistych prywatnych mieszkań 1:1.

Dla zwykłych budynków zachowaj:

- lokalizację,
- bryłę,
- charakter elewacji.

Wnętrza twórz jako fikcyjne GameplayInterior.

Pozwala to zachować wiarygodność miasta bez konieczności rekonstruowania rzeczywistych prywatnych przestrzeni.

# 147. LANDMARKI

Najważniejsze charakterystyczne obiekty wykonuj dokładniej niż zwykłą zabudowę.

Pipeline:

GIS footprint

→ reference photographs

→ accurate proportions

→ optimized game asset

→ LOD/HLOD

→ Unreal.

Nie wykorzystuj materiałów fotograficznych jako assetów, jeżeli ich licencja tego nie umożliwia.

# 148. AUTOMATYCZNA WALIDACJA MAPY

Dodaj narzędzie:

WroclawMapValidator.

Powinno wykrywać:

- drogi bez połączenia,
- błędnie ustawione budynki,
- nakładające się footprinty,
- przerwane mosty,
- problemy z NavMesh,
- błędne współrzędne,
- obiekty umieszczone poza właściwą dzielnicą,
- anomalie wysokości.

# 149. SOURCE METADATA

Każdy importowany dataset powinien posiadać metadata:

SourceName

SourceURL

SourceDate

SourceLicense

ImportDate

CRS

ProcessingVersion.

Nie usuwaj informacji o pochodzeniu danych.

# 150. LICENCJE

Przygotuj:

docs/DATA\_SOURCES.md

docs/GEO\_DATA\_LICENSES.md

Dokumentuj wszystkie źródła danych.

W przypadku OpenStreetMap zapewnij wymagane oznaczenie źródła i licencji zgodnie z ODbL.

Nie pobieraj geometrii lub tekstur z Google Maps, Google Street View ani innych źródeł bez odpowiednich praw do takiego wykorzystania.

# 151. MAP BUILD PIPELINE

Przygotuj automatyczny pipeline:

DOWNLOAD / IMPORT SOURCE DATA

→ VALIDATE LICENSE METADATA

→ REPROJECT

→ NORMALIZE

→ CLIP TO WROCLAW

→ GENERATE ROAD GRAPH

→ GENERATE BUILDING DATA

→ GENERATE WATER DATA

→ GENERATE RAIL DATA

→ GENERATE TERRAIN

→ SPLIT INTO WORLD PARTITION CELLS

→ IMPORT INTO UNREAL

→ VALIDATE.

Pipeline powinien być możliwy do ponownego uruchomienia po aktualizacji danych.

# 152. MAP VERSION

Dodaj:

WroclawMapDataVersion.

Przykład:

MapDataVersion = 2026.1

Pozwoli to później aktualizować świat wraz z danymi źródłowymi.

# 153. NIE BUDUJ OD RAZU CAŁEGO MIASTA W WYSOKIEJ JAKOŚCI

Docelowa mapa może obejmować rzeczywisty Wrocław, ale produkcję wykonuj sektorami.

Etap pierwszy:

jeden rzeczywisty fragment miasta.

Następnie rozszerzaj granice świata.

Każdy nowy sektor korzysta z tego samego GIS pipeline.

Nie twórz osobnych ręcznie przesuniętych map.

# 154. PIERWSZY REALNY SEKTOR

Pierwszy vertical slice powinien korzystać już z realnej geometrii wybranego obszaru Wrocławia.

Nawet jeżeli większość budynków będzie początkowo prostymi bryłami, zachowaj:

- prawdziwy przebieg ulic,
- rzeczywiste skrzyżowania,
- rzeczywiste odległości,
- rzeczywiste footprinty budynków,
- rzeczywiste położenie terenów zielonych,
- rzeczywiste położenie wody i mostów.

Następnie zastępuj proceduralne bryły finalnymi assetami.

# 155. WORLD PARTITION

Podziel Wrocław na komórki World Partition.

System powinien automatycznie streamować:

- geometrię,
- propsy,
- NPC,
- ruch uliczny,
- questowe elementy świata.

Dalekie części miasta nie mogą posiadać aktywnej pełnej symulacji.

# 156. HLOD

Dla dużego miasta obowiązkowo przygotuj HLOD.

Daleka zabudowa:

bardzo uproszczona.

Średni dystans:

LOD.

Blisko:

pełna geometria.

Nie używaj finalnego wysokiego LOD dla tysięcy budynków jednocześnie.

# 157. DATA LAYERS

Wykorzystuj Data Layers m.in. dla:

BaseCity

Gameplay

QuestChanges

Traffic

NightState

Construction

CampaignChanges.

Pozwoli to zmieniać miasto podczas kampanii bez duplikowania całej mapy.

# 158. RZECZYWISTE MIASTO + FIKCYJNA FABUŁA

Geografia ma być rzeczywista.

Fabuła