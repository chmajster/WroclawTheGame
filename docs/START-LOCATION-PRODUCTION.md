# Lokacja początkowa: Pomorska / Paulińska

## 1. Audyt

UE 5.8, C++20, Enhanced Input, istniejący GIS/DEM, Python Editor oraz Blender
5.2.2 LTS. Mapa gry: `/Game/Maps/Nadodrze_Playable`. Źródłem położenia jest
`Data/playable_district.json`; wygenerowany punkt chodnika ma współrzędne
`44611.8, 62830.93, 143.77` cm. PlayerStart dodaje 104 cm dla kapsuły.
Przebudowa obejmuje najbliższe 100 m, bez przenoszenia startu lub trasy misji.

`Scripts/assets/audit_start_location.py` zapisuje rzeczywisty wykaz obiektów
do `Saved/StartLocation/audit.json`. Najbliższe kamienice: Pomorska 45, 47,
43. Źródłowe obrysy są z OSM; wiele wysokości to domyślne 12 m, a nie pomiar.
Jezdnia Pomorskiej zawiera cztery pasy z torowiskiem, oddzielną drogę rowerową;
Paulińska i Kaszubska mają źródłowy `surface=sett`. Nie należy pokrywać całej
lokacji asfaltem ani dodawać sygnalizacji na każdym skrzyżowaniu.

Stan obrazu przed pracami: proste bryły z płaskimi dachami, powtarzalne okna,
zbyt duża cegła, bardzo ciemna jezdnia, pasy gołej ziemi zamiast uporządkowanego
chodnika. Dotychczasowy TenementKit dodaje detale na trasie misji, ale nie
zastępuje geometrii fasad. Istnieją działające pojazdy, piesi, misja, zapisy,
menu, pogoda i cykl dobowy. Zachowujemy te systemy.

GIS ma przestrzenne komórki siatek oraz HISM okien; mapa nie jest World
Partition. Renderer korzysta z Lumen i VSM. Nowe zasoby przechodzą istniejący
pipeline manifest → Blender/FBX/LOD → natywny import → źródłowe rozmieszczenie.
Screenshot i pomiar klatek mają pochodzić z gry, nie z renderu Blendera.

## 2. Referencje i źródła położenia

Obejrzano zdjęcia miejskie przebudowy torowiska oraz Pomorskiej 47 w remoncie
z 2022 r. Kremowy tynk, białe ramy, trójkątne nadokienniki, żelazne balkony,
parter usługowy i mniejsze okna poddasza zastępują przypadkową cegłę.
Linki, autorzy i zakres wykorzystania są w `Data/start_location.json`.
Fotografie służą jako referencje; nie są dystrybuowane ani kopiowane do tekstur.
OSM/DEM zachowuje istniejące przypisanie autorstwa w grze.

Osiem budynków ma dokładne źródłowe obrysy w
`Data/start_location_buildings.json`, odtwarzane przez
`Scripts/assets/build_start_location_inputs.py`. Wysokości bez pomiaru oraz
układ okien są oszacowaniem na podstawie architektury okolicy, jawnie opisanym
w polityce. Nie jest to geodezyjny model wszystkich adresów. Układ dróg,
torowiska, kierunki ruchu i rozróżnienie nawierzchni pozostają źródłowe.

## Strumienie pracy

Każdy numer wymaga osobnej gałęzi/PR: 1 audyt, 2 referencje i układ,
3 legalne zasoby, 4 modele Blender, 5 integracja otoczenia, 6 start/kolizje,
7 optymalizacja, 8 testy gry, 9 organizacja, 10 odbiór i raport.
Szczegóły wykonania i aktualny następny krok: `IMPLEMENTATION-STATE.md` oraz PR.

## 3. Materiały i istniejące modele

Pozyskane z Poly Haven materiały CC0: Painted Plaster Wall (Amal Kumar),
Asphalt 03 (Dario Barresi / Charlotte Baglioni), Concrete Pavement 03
(Charlotte Baglioni), Cobblestone Floor 08 i Roof Tiles 14 (Rob Tuytel).
Każdy zestaw zawiera BC, normal DirectX oraz ORM w 2k. Dokładne adresy,
autorzy, licencje, rozmiary oraz sumy MD5/SHA256 zapisane są w
`Data/city_asset_acquisition.json`; pobranie i kontrolę wykonuje istniejący
`Scripts/assets/acquire_city_assets.py`. Pozostaje wcześniejszy Sandstone Cracks.

Wykorzystamy już sprowadzone lampy, ławki i kosze z biblioteki 66 modeli.
Internetowe modele kamienic nie odpowiadają źródłowym obrysom i rytmowi okien
Pomorskiej. Modele adresów, krawężnik, wpust, stojak rowerowy, młode drzewo
oraz samochód dekoracyjny wymagają własnego autorstwa, z odtwarzalnym bpy.
Parkowany model nie zastępuje sterowalnego samochodu misji.

## 4. Oryginalne modele produkcyjne

`Scripts/assets/build_start_location_models.py` rzeczywiście uruchomiono
w Blenderze 5.2.2 LTS. Wynik: osiem modeli budynków na obrysach OSM oraz
krawężnik, wpust, stojak, drzewo i samochód — 13 modeli, 257206 trójkątów
LOD0 łącznie. Są to pełne siatki z UV i materiałami; okna mają otwory w ścianie
i cofnięte szkło. Budynki zawierają 74 proste hull-e ścian, zachowujące kształt
obrysu, zamiast jednego collidera zamykającego podwórze. Dachy są uproszczone,
wnętrza nie są dostępne; budynki nie są modelami z pomiarów fotogrametrycznych.

Wykonano 39 eksportów FBX przez pipeline, natywny import wszystkich 13 siatek
oraz 14 współdzielonych interfejsów materiałów. Powstało 59 wersjonowanych
uasset. `verify_start_location_assets.py` sprawdził w UE metrową skalę,
UV w każdym z trzech LOD, nazwy i przypisanie materiałów, sekcje LOD oraz
liczbę colliderów. PASS dla 13 siatek. Naprawiono import LOD, który pomijał
materiał usuniętego drobnego detalu i mógł scalać sekcje na domyślnym materiale.

## 5. Integracja otoczenia

W mapie rzeczywiście zastąpiono osiem brył GIS szczegółowymi budynkami.
Pozostałe systemy, punkty misji i sterowalny samochód zachowano. Rozmieszczenie
odtwarza `build_start_location_plan.py`: 2272 instancje w 29 grupach HISM,
w tym 2098 modułów krawężnika, 87 wpustów, 3 stojaki, 15 ławek,
18 młodych drzew, kosz, 8 lamp i 6 samochodów dekoracyjnych. 36 pasów malowania
odpowiada przejściom oznaczonym w OSM; nie dodano niepotwierdzonej sygnalizacji.
50 odcinków stalowych główek szyn zachowuje źródłowe osie i rozstaw 1435 mm.
Samochody dekoracyjne omijają trasę dostawy i ruchu ulicznego.

29 komórek nawierzchni przecinających obszar startu otrzymuje PBR odpowiedni
do klasy źródłowej: 6 asfaltu, 8 kostki, 12 chodnika i 3 zieleni. Grass Ground
1k (Charlotte Baglioni, Poly Haven CC0) ma również sprawdzone sumy i licencję.
Cienkie siatki gruntu nie uczestniczą w mesh SDF Lumen, ponieważ ich zgrubne
pola odległości nadmiernie zaciemniały sąsiednie nawierzchnie. Budynki zachowują
mesh SDF, a grunt zachowuje zwykłe cienie i śledzenie ekranowe.

Natywny build modułu, wypiekanie 439 komórek, izolowane rozmieszczenie i trzy
nowe zrzuty edytora zakończyły się powodzeniem. Obejrzano każdy kadr:
dwusekundowe doczytywanie kamery usuwa artefakty pierwszego widoku Nanite;
wyłączenie SDF cienkiego gruntu usuwa czarny pas ulicy. Materiały mają zapisane
flagi Nanite i HISM. To kontrola architektury w edytorze; rozgrywka i FPS
są odrębnym strumieniem 8. Dachy pozostają uproszczone, a architektura opiera
się na referencjach i oszacowaniach, bez deklarowania dokładności pomiarowej.

## 6. Start, kamera i kolizje

Gracz pozostaje na oryginalnym chodniku. Spawn i odzyskiwanie po utknięciu
sprawdzają nachylenie trafionej podłogi, ustawiają kapsułę ponad nią i kierują
kamerę na punkt spotkania; powrót zeruje także prędkość. Istniejący spring arm
z próbą kolizji kamery pozostaje aktywny. Teleport sprawdza blokujące obiekty.

Blender wygenerował osobny, wąski UCX pnia młodego drzewa; liście nie blokują
ruchu. Trzy nowe FBX drzewa rzeczywiście zaimportowano i umieszczono w mapie.
Natywna kontrola 13 zasobów potwierdziła jeden hull drzewa, 74 hulle ścian
budynków oraz dotychczasowe kolizje krawężników, stojaków i aut dekoracyjnych.
Build C++ zakończył się powodzeniem. Kontrola rozgrywki i kamery na obrazach
następuje w strumieniu 8; nie zastępujemy jej kontrolą samej siatki.
