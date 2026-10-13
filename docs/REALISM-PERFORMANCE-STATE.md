# Realizm — punkt 8/10

## Wykonane
Punkty 1–7 scalone (#165–171). Źródła używanego UE 5.8.2 rozróżniają MARKING_JOB_QUEUE (128 wpisów na grupę 64 wątków, duże zadania >8 stron) od PAGE_POOL. Overflow używa wolniejszej ścieżki bez zmiany geometrii. Log i on-screen delegate umożliwiają rzeczywisty odczyt ostrzeżeń.

## Do zrobienia
Pomiar przed/po na tej samej trasie, sprzęt/CPU/GPU/RAM/tekstury, kontrola bounds/WPO/cieni; zgodne Nanite dla sektorowych GIS, LOD/culling dla pozostałych, native regeneracja i porównanie widoków.

## Następny krok
Dodać WTG.RealismPerformance w Tests/RealismPerformance.h włączonym przez istniejący StartLocationSmoke.cpp. Najpierw zbudować i zmierzyć mapę po punkcie 7, potem zmieniać GeoMeshLibrary::BakeMesh oraz prepare_playable_district.py.

## Stan
W toku. Osobna gałąź z main. Bez CI. Bez zmian oryginalnego worktree.
