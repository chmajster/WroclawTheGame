# Realizm — punkt 5/10

## Wykonane
Punkty 1–4 scalone. Źródła 8 kamienic + 24 budynków tła do 210 m (32 razem),
737261 trójkątów, zachowane obrysy/wysokości/kotwice/trasy. Osiem dodatkowych
obrysów wynika z rzeczywistej oceny Start_Park: nearest-only pozostawiał blockout.
Przestrzenne dachy i flat tags, cokoły do DEM, poprawne otwory, wnęki wejściowe,
gzymsy, obróbki, otwarte rynny. Detale są interpretacją fasad, nie pomiarem.
Pierwsze 24 budynki: native import i kontrakt mapy PASS; 29 assets QA PASS;
kompilacja PASS. 13 regresji geometrii/dzielnicy + 4 kontrakty dachów/obrysów PASS.
GIS odtworzony: 440 komórek/279224 trójkątów. Plany CityDetails/start odtworzone.
18 rzeczywistych ujęć z D3D12, ocena ulicy/parku/portalu. Bufor jednego modelu
ma 119 zerowych tangentów, pozostałe 23 poprawne. GLB ma poprawne UV.
Eksport/import zachowuje teraz jawną bazę, Nanite architektury explicit_tangents.
GetAuthoredMeshBounds mierzy zapisane LOD0; culling bounds Nanite są konserwatywne.


Aktualizacja 16:35 UTC: native QA 37 zasobów i kontrakt 32 aktorów PASS.
Wszystkie 7 faz mapy PASS. 1654 instancje, 31 HISM, 1169968 trójkątów,
1525 kolizji — limit 1,6 mln zachowany. Trwa sesja 35586: D3D12 diagnostics,
potem StartLocationSmoke. Oceniać tylko świeże capture.json/18 PNG, sprawdzić
32 basis records i Start_Park. Eksport explicit rozpoznawany przez report,
starsze FBX zachowują normalne i wyliczają brakujące tangenty; 2 takie assety
nadodrze ponownie zaimportowane PASS. Końcowy obraz/baza nadal przed oceną.
## Do zrobienia
Dokończyć import 8 nowych + 1 korekty tangentów. Native QA 37 assets/32 aktorów,
odtworzyć mapę, 18 ujęć i pomiar bufora, zwykłe chodzenie. Dowody i merge #169.

## Następny krok
Trwa eksport/import 9 manifestów (sesja 47787), architecture-import-panorama.log,
lista Saved/Realism/architecture-panorama-import.json. Po zakończeniu usunąć filtr
stages w Saved/Realism/run_architecture_native.ps1 (teraz tylko details/placement/
quality/contract), wykonać też assets/base/presentation. Potem RealismDiagnostics
i StartLocationSmoke; ocenić architecture_render_basis oraz Start_Park.

## Stan
W toku — draft #169. Źródła 32 budynków gotowe; nowe 8 i korekta przed końcową oceną.
Bez CI; oryginalny dirty worktree i PR #164 poza zadaniem.