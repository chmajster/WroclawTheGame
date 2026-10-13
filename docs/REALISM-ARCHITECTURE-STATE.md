# Realizm — punkt 5/10

## Wykonane
Wspólna selekcja 8 kamienic + 16 budynków tła do 210 m. Źródła 24 budynków
mają zachowane obrysy i wysokości, fundamenty do DEM, przestrzenne dachy,
wnęki wejściowe, obróbki i otwarte rynny. Tagi płaskich dachów zachowane.
Cztery testy kontraktów, walidacja manifestów i eksport 24 × 3 LOD PASS. 569 311 trójkątów budynków;
kolizje maks. 29 hulls/budynek. Detale/dachy są interpretacją, nie pomiarem fasad.

## Do zrobienia
Eksport FBX/LOD, natywny import/QA, regeneracja mapy, rzeczywista ocena
panoramy i elewacji oraz normalnego sterowania. Zapis dowodów i scalenie #169.

## Następny krok
Trwa import_batch.py dla 24 budynków. Po zamknięciu importu skompilować
RealismDiagnostics.cpp (6 dodatkowych widoków kamery elewacji) i uruchomić
verify_start_location_assets.py.
GIS/base/presentation/details/start-placement/quality, RealismDiagnostics i StartLocationSmoke.

## Stan
W toku, draft #169. Nowe budynki jeszcze bez natywnego importu/oceny w grze.
Bez CI; pierwotny worktree i PR #164 poza zadaniem.