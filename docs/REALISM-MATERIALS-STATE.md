# Realizm — punkt 7/10

## Wykonane
Punkty 1–6 scalone (#165–170). Geometria/postać/zieleń mają rzeczywiste dowody.

## Do zrobienia
Sprawdzić rzeczywiste przypisania/parametry/tekstury PBR i poprawić skalę
oraz różnice materiałów. Czytelne zwykłe dzienne światło, oszczędny postprocess,
deterministyczny czas tylko w procesie QA. Native kontrola i ujęcia.

## Następny krok
Native probe MI_Grass/Stone/Roof i komponentów mapy. Przejrzeć raport przed
zmianami w prepare_start_location_materials.py/place_start_location.py.

## Stan
W toku. Gałąź punktu 7 z main; bez CI; oryginalny worktree poza zakresem.

Batch: native probes confirm correct BC paths, no overrides, sRGB and TC_DEFAULT already correct (HDR hypothesis rejected). Separate neutral concrete curb and coated lamp metal interfaces, subdued stone/roof/grass palette, glass metallic=0, painted surfaces dielectric, cobble physical scale .72, sky fill 1.65, no chromatic aberration/DOF/motion blur. Native build PASS; materials/foliage/bindings/assets42/foliage QA PASS. Session8248 captures actual D3D12 after at fixed 13:00. Before capture report also13:00 (PowerShell decimal parsing corrected by quoted flag). Next inspect fresh 18 views, actual material-runtime command if needed, ordinary gameplay and PBR report, archive then merge171.
