# Realizm — punkt 4/10

## Wykonane
Naprawiono źródło nawierzchni na Nadodrze_Playable: wspólny DEM, priorytety
road/rail/footway/sidewalk/green, constrained Delaunay z otworami, jawny CCW,
odrzucanie zerowych/odwróconych sliverów po kwantyzacji i dokładne przycinanie
trójkątów do sektorów 128 m. Zielone poligony nie używają maksymalnej wysokości
obwodu. Pionowe krawędzie do DEM−3 cm domykają uskoki 4/8/20 cm.
Usunięto redundantną triangulację rastra. Wynik: 440 komórek, 280208 trójkątów;
budżet 320000 uwzględnia fizyczne krawędzie. BakeMesh zachowuje normalne,
projektuje UV ścian w ich płaszczyźnie i zapisuje ortogonalne tangenty.

„Rura” była poprzeczką modular_street_seating/crossbar powiększoną 20 razy
do około 37 m. Zastąpiono ją pełną ławką z dziesięciu części Poly Haven:
Stuart Attenborrow, CC0, potwierdzone użycie w grze i publiczna redystrybucja.
Ławka 2,067 × 0,670 × 0,867 m zachowuje źródłowe mapped nodes i funkcję.
Dodano autorski profil szyny 18 cm z główką 6,8 cm, osadzony 3 mm nad brukiem;
422 odcinki do 2 m, układ OSM i gauge 1435 mm między wewnętrznymi krawędziami.
Krawężnik jest parą metrowych kamieni z fugą (48/26/10 trójkątów w LOD).
Odpływy osadzono 2 mm nad jezdnią i odsunięto od krawężnika.
Nowe/zmienione źródła Blender i manifesty przeszły rzeczywisty pipeline:
natywne .uasset, trzy LOD, materiały i kolizje. Source/Full odtwarza modele.
Współdzielone materiały zapisuje się tylko przy brakujących wymaganych flagach,
co usuwa zaobserwowany błąd 32 przy powtarzanych zapisach pakietu.

Walidacja: kompilacja UE 5.8.2 PASS; 13 testów geometrii/dzielnicy PASS;
powtórzona regeneracja daje identyczne SHA wszystkich 440 referencjonowanych
komórek. Bake → presentation → city details → placement → quality PASS.
1654 instancje, 31 HISM, 675609 trójkątów nowych obiektów i 1234 kolizje.
Rzeczywista inwentaryzacja: 15 pełnych ławek, 422 szyny, brak crossbar.
D3D12, 1280×720: przejrzano 12 widoków diagnostycznych oraz zwykłą kamerę.
W Unlit zniknęły prześwity; Lit zachowuje normalne cienie budynków.
Zwykłe chodzenie PASS: 201,43 cm, podłoże, osiem ścian i kamera poprawne,
gracz bez placeholdera. Zrzuty i raporty: docs/qa/realism-2026-10-13/surfaces.

Ograniczenia: profil szyny jest autorską interpretacją, nie pomiarem lokalnego
typu szyny. DEM/OSM mają źródłową dokładność; próbki powierzchni co 6 m.
Czas dnia jest dynamiczny, więc porównanie geometrii opiera się przede wszystkim
na tych samych kamerach Unlit/normalne. Istniejący widok Collision_Park ma
taki sam bliski ekranowy obszar kolizji jak baseline; zwykła kamera ma niezależny
test braku overlap. Architektura, roślinność, materiały/oświetlenie, dokładna
diagnoza VSM i pełny scenariusz misja/zapis należą do kolejnych punktów.
Logi startowe silnika zawierają własne self-testy/ostrzeżenia SDK; wykonane
skrypty mają świeże PASS i kończą się bez LogPython Error/Fatal.

## Do zrobienia
Nic nie pozostaje w zakresie punktu 4.

## Następny krok
Scalić ten PR, utworzyć z aktualnego main gałąź dla punktu 5.
Rozpocząć od Scripts/assets/build_start_location_models.py::building:
poprawić dachy, wejścia i obróbki oraz bliski dalszy plan, zachowując źródłowe
obrysy i wysokości. Zweryfikować przez istniejący pipeline i rzeczywistą mapę.

## Stan
Punkt 4 ukończony. Punkty 1–3 scalone (#165–167).
Bez CI; pierwotny worktree i PR #164 pozostają poza tym zadaniem.
