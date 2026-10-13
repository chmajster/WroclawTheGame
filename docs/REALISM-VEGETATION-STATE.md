# Realizm — punkt 6/10

## Wykonane
Punkty 1–5 scalone (#165–169). Rozpoznano źródło płaskich fragmentów:
tree() ma po dwie przeciwne kopie rombu, UV projekcji świata zamiast liścia,
rzadką koronę i brak dedykowanego shaderu foliage.

## Do zrobienia
Trzy warianty drzewa, taper/smooth pni i gałęzi, autorski atlas liści,
dwustronne maskowanie/subsurface, LOD zachowujący całe liście. Źródłowe
osadzenie pni, wyposażenie ulicy, manifesty/import, native QA i zrzuty.

## Następny krok
Przebudować tree() i UV w Scripts/assets/build_start_location_models.py;
shader w prepare_start_location_assets.py, LOD w Pipeline/blender/build_asset.py.

## Stan
W toku. Osobna gałąź punktu 6 z main; bez CI i zmian w oryginalnym worktree.

Implementation: 3 authored trees (14680/14680/16280 triangles), leaf-card LOD, 512px leaf/bark atlas, masked TwoSidedFoliage, soil pit, modern lamp. Native materials/import PASS. Source plan: 18 trees (7 lime/7 maple/4 broad lime), 2 pits, 8 lamps; no new traffic subsystem. Next: finish native quality/assets/foliage checks (session 48499), run actual D3D12 RealismDiagnostics, inspect park and ordinary gameplay, archive evidence and merge #170.
