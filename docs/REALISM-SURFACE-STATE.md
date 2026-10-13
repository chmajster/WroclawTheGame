# Realizm — punkt 4/10

## Wykonane
Punkty 1–3 scalone (#165–167). Dodano WTG.SurfaceDiagnostics: rzeczywista
inwentaryzacja komponentów i instancji w promieniu 150 m, bounds/transformy,
rzuty na ekran i trafienia promieni w kamerze StartLocation_Start_Street.
Diagnostyka nie zmienia mapy, infrastruktury ani elementów misji.
Ustalono przyczynę „rury”: StartLocation_Instances_*_seating używał
Poly Haven modular_street_seating/crossbar zamiast kompletnej ławki. Wysokość
poprzeczki 4 cm powodowała scale=20 i obiekty o długości około 37 m.
Złożono prawdziwą ławkę z dziesięciu licencjonowanych części (Stuart Attenborrow,
CC0, użycie w grze i publiczna redystrybucja potwierdzone na polyhaven.com/license).
Ławka ma 2,067 × 0,670 × 0,867 m i zachowuje mapped nodes oraz funkcję wyposażenia.

Dodano autorski profil szyny 18 cm z główką 6,8 cm, środnikiem i stopką.
Dwa nowe źródła Blender, manifesty i natywne .uasset przeszły istniejący pipeline:
3 LOD, trzy materiały i dwie kolizje ławki; szyna bez kolizji, 52 trójkąty.
Placement używa pełnej ławki oraz odcinków szyny 2 m zgodnych z OSM,
rozstawem między wewnętrznymi krawędziami i lokalnym DEM (główka 3 mm nad brukiem).

Nowy surface_mesh.py daje priorytet nawierzchniom i oddziela je od zieleni/terenu;
eliminuje koplanarne road/rail/footway. Zielone poligony próbkują DEM zamiast
max wysokości obwodu. Chodniki/jezdnie mają tessellację constrained Delaunay
z zachowaniem otworów i poprzecznego spadku. Trójkąty są dokładnie przycinane
do 128 m sektorów z interpolacją Z (także pionowe ściany), zamiast centroidów.
Regeneracja źródeł: 440 komórek, 280208 trójkątów poniżej budżetu 320000.
Trzynaście testów dzielnicy i geometrii PASS. Plan ma 422 odcinki szyn,
1058 segmentów krawężnika i 1654 instancje, poniżej budżetu 2400. Pipeline
Source/Full odtwarza oba nowe modele. BakeMesh zachowuje fizyczne normalne,
projektuje UV ścian w ich płaszczyźnie i jawnie buduje ortogonalne tangenty.
Rzeczywisty probe potwierdził 15 pełnych ławek i 422 szyny; brak dawnego
crossbar. Placement/quality: PASS, 31 HISM, 650217 trójkątów, 1234 kolizje.
Próba regeneracji dała identyczne SHA wszystkich 442 plików geometrii.
Widoki silnika ujawniły CW z constrained Delaunay: generator jawnie ustawia
CCW i odrzuca slivery odwrócone/zerowe po kwantyzacji do 0,001 cm. Kontrakty
sprawdzają teraz kierunek wszystkich rzeczywiście zapisanych nawierzchni.
Diagnostyka i test startu czekają na zakończenie kompilacji zasobów,
aby zrzuty nie przedstawiały niedokończonych tekstur/SDF/kart Lumen.

Nawierzchnie mają pionowe krawędzie do DEM−3 cm: usunięto widoczne w Unlit
prześwity między zielenią (+4 cm), jezdnią (+8 cm) i chodnikami (+20 cm).
Końce krawędzi pokrywają się z siatką próbkowania, także przy otworach.
Usunięto redundantną drugą triangulację rastra. Budżet 320 tys. uwzględnia
rzeczywiste domknięcie geometrii; sektory nadal 128 m.
Krawężnik to dwie metrowe sztuki z fugą, a nie jedna sztuka rozciągnięta do 2 m:
48/26/10 trójkątów w LOD, źródło Blender/GLB, manifest i import natywny PASS.
Odpływy cofnięto 30 cm na jezdnię i osadzono 2 mm nad lokalną nawierzchnią.
Przed domknięciem krawędzi zwykły start/chodzenie/8 ścian/kamera PASS.

## Do zrobienia
Dokończyć ostatni bake/placement i kontrolę renderowania po domknięciu krawędzi.
Zapisać finalne dowody, ocenić kolizję i pozostałe ograniczenia powierzchni.

## Następny krok
Zakończyć trwający proces Saved/Realism/*-surfaces-accepted.log
(base → presentation → details → placement → quality → RealismDiagnostics
→ StartLocationSmoke). Proces narzędzia 18683. Sprawdzić 12 świeżych PNG
Saved/Realism/Diagnostics i raport Saved/StartLocation/gameplay-report.json.
Potem skopiować zaakceptowane dowody do docs/qa/.../surfaces i zakończyć PR.

## Stan
W toku, draft. Bez CI; bez zmian w pierwotnym worktree i PR #164.
