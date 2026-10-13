# Realizm — punkt 5/10

## Wykonane
Jeden wybór dla GIS/Blender: 8 kamienic + 16 budynków tła do 210 m.
Obrysy, wysokości i kotwice zachowane. Dachy przestrzenne z tagami flat,
fundamenty do DEM, dopasowane okna, głębokie wejścia, obróbki i otwarte rynny.
569311 trójkątów budynków; maks. 29 kolizji. Źródła i manifesty wersjonowane.
Eksport 24 × 3 LOD, import 24 + 12 korekt oraz natywna kontrola 29 zasobów PASS
(skala/UV/LOD/materiały/kolizje). Kompilacja UE 5.8.2 PASS. 13 regresji
geometrii/dzielnicy + 4 kontrakty dachów i obrysów PASS. GIS: 440 komórek,
279530 trójkątów, te same kotwice i trasy. Detale są interpretacją fasad.
Nowa kamera elewacji i 6 widoków diagnostycznych na właściwej mapie.
Kontrola pozycji/wysokości/Nanite/liczby aktorów oraz renderowanych tangentów.


Aktualizacja natywna: regeneracja mapy i kontrakt 24 aktorów PASS. 1654 instancje,
31 HISM, 1002018 trójkątów, 1424 kolizje. GetAuthoredMeshBounds mierzy zapisane
wierzchołki LOD0; bounds Nanite do cullingu są konserwatywne, nie wysokością dachu.
Trwa D3D12 WTG.RealismDiagnostics, sesja 47830, architecture-game-diagnostics.log.
Po zakończeniu ocenić 18 PNG i architecture_render_basis, potem StartLocationSmoke.
## Do zrobienia
Regeneracja mapy i kontrola aktorów; 18 rzeczywistych ujęć i normalna rozgrywka.
Sprawdzić renderowane tangenty: import ostrzega o prawie zerowych tangentach
dwóch modeli tła, źródłowy GLB nie ma zdegenerowanych UV. Korekty i dowody.

## Następny krok
Trwa Saved/Realism/run_architecture_native.ps1 (sesja 27553): base → presentation
→ details → placement → quality → verify_start_architecture.py.
Po świeżych PASS uruchomić D3D12 WTG.RealismDiagnostics i WTG.StartLocationSmoke.
Przeczytać architecture_render_basis w Saved/Realism/Diagnostics/capture.json,
obejrzeć PNG i rozstrzygnąć ostrzeżenia tangentów. Potem scalić #169 i punkt 6.

## Stan
W toku — draft #169. Nowe zasoby natywnie zweryfikowane; mapa/obraz przed oceną.
Bez CI; pierwotny worktree i PR #164 poza zadaniem.