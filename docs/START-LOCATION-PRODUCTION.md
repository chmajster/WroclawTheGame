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

## Strumienie pracy

Każdy numer wymaga osobnej gałęzi/PR: 1 audyt, 2 referencje i układ,
3 legalne zasoby, 4 modele Blender, 5 integracja otoczenia, 6 start/kolizje,
7 optymalizacja, 8 testy gry, 9 organizacja, 10 odbiór i raport.
Szczegóły wykonania i aktualny następny krok: `IMPLEMENTATION-STATE.md` oraz PR.
