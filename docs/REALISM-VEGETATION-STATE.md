# Realizm — punkt 6/10

## Wykonane
Trzy autorskie drzewa: młoda lipa, klon i szersza lipa parkowa. Taperowane,
wygładzone pnie/gałęzie, korzenie; 6000/6000/6800 pojedynczych kart liści,
bez zdublowanych backfaces. Dedykowany atlas 512 px z maską/żyłkami,
TwoSidedFoliage/subsurface, rough bark i ziemia. Trzy LOD usuwają całe karty
z zachowaniem UV zamiast cięcia ich na odłamki; zero niepoprawnych baz
we wszystkich dziewięciu natywnych LOD. LOD0 14680/14680/16280 trójkątów.

18 drzew (7/7/4), deterministyczna skala .93–1.07 i obrót z ID; zachowane
mapped nodes, uzupełnienia jawnie estymowane w źródłowej zieleni. Podstawy
na DEM+nawierzchnia, wykluczenia jezdni/torów/wejść i kotwic misji. Dwie misy
ziemne; kolizja dotyczy tylko pni. Osiem współczesnych latarni zamiast starych
lamp. Kosz był uchwytem powiększonym do 85 cm: zastąpiony kompletnym korpusem,
pokrywą i uchwytami GurJas Studios/Poly Haven CC0 (6428 trójkątów). Użycie
w grze i publiczna redystrybucja źródeł zapisane w metadanych/manifestach.
Pasy przejść leżą 0,2–0,6 mm nad nawierzchnią i nie rzucają cienia.

Źródła Blender/GLB, manifesty, trzy LOD, tekstury i rzeczywiste .uasset
podłączone do mapy. Source/Full odtwarza je i sprawdza foliage. Native QA
42 zasobów oraz foliage PASS; 1656 instancji, 37 HISM, 1327346 trójkątów
nowych obiektów, 1525 kolizji, limit 1,6 mln zachowany. Zwykły spacer PASS
17:12 UTC: 201,40 cm, podłoże, osiem ścian i kamera bez overlap.
Dowody: docs/qa/realism-2026-10-13/vegetation (18 diagnostycznych PNG sprzed
ostatniej wymiany kosza i cztery świeże zwykłe ujęcia po niej; jawna proweniencja).
Lit/Unlit parku pokazują pełne nieregularne korony bez dawnych odłamków.
Materiały ogólne, światło i końcowa pełna misja/zapis należą do punktów 7/10.

## Do zrobienia
Nic w zakresie punktu 6.

## Następny krok
Scalić #170. Utworzyć z main gałąź punktu 7; sprawdzić rzeczywiste parametry
MI_Grass/MI_Roof oraz materiał komponentu, następnie dopracować PBR i dzienne
światło w prepare_start_location_materials.py, place_start_location.py,
SurfaceQualityDirector.cpp/WTGDayNightEnvironment.cpp. Zrzuty utrwalić o 13:30
tylko w procesie testowym, zachowując dynamiczny dzień zwykłej gry.

## Stan
Punkt 6 zakończony; 1–5 scalone. Bez CI i zmian oryginalnego dirty worktree.
