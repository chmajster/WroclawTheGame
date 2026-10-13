# Realizm — punkt 5/10

## Wykonane
Wspólna selekcja 8 kamienic + 16 budynków tła do 210 m. Źródła 24 budynków
mają zachowane obrysy i wysokości, fundamenty do DEM, przestrzenne dachy,
wnęki wejściowe, obróbki i otwarte rynny. Tagi płaskich dachów zachowane.
Cztery testy kontraktów i walidacja manifestów PASS. 569 311 trójkątów budynków;
kolizje maks. 29 hulls/budynek. Detale/dachy są interpretacją, nie pomiarem fasad.

## Do zrobienia
Eksport FBX/LOD, natywny import/QA, regeneracja mapy, rzeczywista ocena
panoramy i elewacji oraz normalnego sterowania. Zapis dowodów i scalenie #169.

## Następny krok
Pipeline/blender/build_asset.py dla Buildings, potem import_batch.py.
GIS/base/presentation/details/start-placement/quality, RealismDiagnostics i StartLocationSmoke.

## Stan
W toku, draft #169. Nowe budynki jeszcze bez natywnego importu/oceny w grze.
Bez CI; pierwotny worktree i PR #164 poza zadaniem.