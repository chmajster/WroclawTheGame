# Realizm — punkt 5/10

## Wykonane
Zastąpiono blockout 8 kamienic i 24 budynków panoramy do 210 m: 32 modele,
737261 trójkątów źródłowych. Zachowano obrysy, wysokości, kotwice i trasy GIS.
Dodano przestrzenne dachy, wnęki, wejścia, gzymsy, cokoły do DEM i rynny.
Detale fasad i dachy bez roof:shape są interpretacją, nie dokumentacją pomiarową.
GIS wyklucza wszystkie zastąpione bryły: 440 komórek/279224 trójkątów.
Nowe źródła Blender/GLB, manifesty i rzeczywiste natywne zasoby są w pipeline.

Rozpoznano zerowe tangenty MikkTSpace na wąskich trójkątach dachu także
w źródłowym Blenderze (149). Import zachowuje jawną bazę i naprawia tylko
brakujące tangenty na podstawie UV i normalnej. Naprawiono 230 wektorów w 27
LOD; końcowy pomiar buforów renderera: 32/32 modeli, zero niepoprawnych
tangentów i normalnych. GetAuthoredMeshBounds odróżnia rzeczywiste wierzchołki
od konserwatywnego bounds Nanite, bez zmiany wymaganej wysokości budynku.

Kompilacja i 22 testy statyczne PASS. Wszystkie siedem faz mapy PASS;
końcowe native QA: 37 zasobów oraz 32 aktorów/obrysów/wysokości PASS.
1654 instancje, 31 HISM, 1169968 trójkątów nowych obiektów, 1525 kolizji,
limit 1,6 mln zachowany. StartLocationSmoke PASS: rzeczywisty spacer 201,40 cm,
podłoże, osiem ścian, kamera bez overlap, bohater bez placeholdera.
18 diagnostycznych i cztery zwykłe ujęcia, raporty oraz SHA obrazów:
docs/qa/realism-2026-10-13/architecture. Materiały, światło i zieleń pozostają
zakresem kolejnych punktów; samej geometrii nie opisujemy jako fotorealizmu.
Czas dnia jest dynamiczny; porównania geometrii używają tych samych kamer Unlit.

## Do zrobienia
Nic nie pozostaje w zakresie punktu 5.

## Następny krok
Scalić #169, następnie utworzyć z main gałąź punktu 6. Przebudować tree()
w Scripts/assets/build_start_location_models.py, materiały foliage i osadzenie
w build_start_location_plan.py/place_start_location.py. Zachować mapped nodes.

## Stan
Punkt 5 zakończony. Punkty 1–4 scalone. Bez uruchamiania CI.
Oryginalny dirty worktree i PR #164 pozostają poza zakresem.
