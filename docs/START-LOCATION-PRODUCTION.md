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

## 7. Jakość blisko gracza i budżet

Polityka w `Data/start_location.json` jest wykonywana przez importer i
`prepare_start_location_quality.py`. Budynki zachowują Nanite i pełny fallback;
progi konwencjonalnych LOD wynoszą 1/0,22/0,06. Drzewa utrzymują drobne gałęzie
blisko gracza (1/0,01/0,003), pozostałe rekwizyty mają 1/0,16/0,04.
Instancje wygaszają się między 180 a 260 m. 21 współdzielonych map PBR ma
włączony streaming i limit 2k; trawa pozostaje w źródłowym 1k.

Rzeczywista kontrola UE: 2272 instancje, 29 HISM, 466628 trójkątów LOD0
nowego otoczenia razem z instancjami (limit 1600000), 2259 prostych kształtów
kolizji. Kontrola zapisanych progów LOD zakończyła się PASS. Nie jest to
pomiar VRAM ani FPS całej dzielnicy; istniejący świat, postacie i stare detale
mają odrębny koszt. Mapa nadal korzysta z komórek GIS, bez World Partition.

## 8. Rzeczywiste testy gry i obrazy

Uruchomiono grę z rendererem D3D12 na RTX 5070 Ti, 1280×720. Po 50 s
rozgrzewania zebrano 1163 odstępy klatek z zegara monotonicznego: średnio
16,764 ms / 59,65 FPS, p95 17,684 ms. Raport końcowy potwierdził limit
`t.MaxFPS=60`, pomimo wcześniejszej próby wyłączenia go przez konsolę.
Nie przedstawiamy tego wyniku jako benchmarku bez limitu ani pomiaru GPU.
Wcześniejsza scena miała 16,78 ms przy limicie 60, z innym testem i kamerą.

PASS: gracz bez modelu zastępczego, poprawna podłoga, osiem blokujących ścian,
kamera poza kolizją i 2,01 m zwykłego chodzenia. Zapisano i obejrzano świeże
obrazy gracza, chodzenia, ulicy i parku. Gałęzie drzew są widoczne po korekcie
LOD; nie występują wcześniejsze artefakty pierwszego kadru Nanite.

PASS: cztery natywne testy `WTG.District`, zapis/odzyskiwanie/pojazd,
pełna misja z fizycznym przejazdem 460,04 m w 147,56 s oraz odczyt sesji
w kolejnym procesie. 17 ogólnych błędów testów startowych samego silnika
poprzedzało kolejkę; żaden nie pochodził z tych czterech testów projektu.
Ich treść pozostała w lokalnym logu; archiwum zawiera wyłącznie wyniki projektu.

Edytor ponownie otwarty przez użytkownika blokował główną DLL (LNK1104).
Kompilacja i testy odbyły się w izolowanym projekcie z identycznymi 171 plikami
C++ i tymi samymi zasobami przez junction Content/Data. Zmieniono jedynie
ustawienie adaptive unity celu kompilacji kopii. Katalog zapisów był osobny;
test używa świeżego profilu wyglądu postaci. Nie zmieniono zapisów użytkownika.
Źródła, natywne zasoby, obrazy i raporty mają sumy kontrolne w
`docs/qa/start-location-2026-10-12/`. Wszystkie testy faktycznie wykonano;
surowe logi z identyfikatorami usług pozostają lokalnie w Saved.

## 9. Organizacja i odtwarzanie

Źródła w `SourceAssets/Models/Wroclaw/StartLocation/` i natywne zasoby w
`Content/Wroclaw/StartLocation/` rozdzielają Buildings, Roads, Props,
Vegetation i Materials/Textures. Pobrane mapy pozostają oddzielnie w
`SourceAssets/Textures/PolyHaven/`. Generator bpy i narzędzia planowania są
w `Scripts/assets/`, manifesty w `Pipeline/assets/start_location/`.
`Data/start_location_asset_catalog.json` wiąże 13 siatek, siedem zestawów CC0
i trzy typy ponownie użytych modeli z autorami, licencjami i ścieżkami.
Wszystkie manifesty wskazują istniejące kamery QA z prefiksem StartLocation.

Nowy punkt wejścia:

```powershell
Scripts/Invoke-StartLocationProduction.ps1 -Mode Validate
Scripts/Invoke-StartLocationProduction.ps1 -Mode Source
Scripts/Invoke-StartLocationProduction.ps1 -Mode Full -GameplayQA
```

Validate rzeczywiście wykonano: 21 plików z sumami, 28 manifestów i katalog
PASS. Source odtwarza GIS, oryginalne modele i 39 FBX. Full dodatkowo buduje
moduł, sprowadza istniejącą bibliotekę do UE, jeśli brakuje jej mapy importu,
importuje 13 modeli, wypieka mapę, stosuje rozmieszczenie i jakość oraz zapisuje
obrazy. Każdy etap edytora ma oddzielny proces i świeży marker; raport PASS
dotyczy wyłącznie wybranego trybu i wykonanych etapów. Obrazy wymagają osobnego
obejrzenia. Istniejący importer biblioteki korzysta z bezpiecznego zakończenia
po inicjalizacji edytora, tak jak importer produkcyjny.

Wymagane lokalnie: UE 5.8, Blender 5.2 i Python z zależnościami
`Scripts/gis/requirements.txt` oraz `Scripts/assets/requirements.txt`.
Domyślnie wykrywana jest istniejąca venv `Saved/GISPython`; ścieżki narzędzi
można podać parametrami EngineRoot, BlenderExe, PythonExe. Brakujące, już
zatwierdzone tekstury pobiera opcja `-DownloadReviewed`. Produkcja Source/Full
wymaga gałęzi zadania, a Full możliwości podmiany DLL i zapisu zasobów.
W tym przebiegu Full nie powtórzono jako jednego polecenia przy ponownie
otwartym edytorze: wszystkie jego etapy produkcyjne wykonano wcześniej
oddzielnie, a testy gry na zgodnej kopii kodu. Nowy punkt wejścia sprawdzono
parserem PowerShell i rzeczywistym trybem Validate; skrypty Python parsują się.
