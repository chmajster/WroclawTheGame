# Realizm — punkt 8/10

## Wykonane
Punkty 1–7 scalone (#165–171). Źródła używanego UE 5.8.2 rozróżniają MARKING_JOB_QUEUE (128 wpisów na grupę 64 wątków, duże zadania >8 stron) od PAGE_POOL. Overflow używa wolniejszej ścieżki bez zmiany geometrii. Log i on-screen delegate umożliwiają rzeczywisty odczyt ostrzeżeń.

## Do zrobienia
Pomiar przed/po na tej samej trasie, sprzęt/CPU/GPU/RAM/tekstury, kontrola bounds/WPO/cieni; zgodne Nanite dla sektorowych GIS, LOD/culling dla pozostałych, native regeneracja i porównanie widoków.

## Następny krok
Dodać WTG.RealismPerformance w Tests/RealismPerformance.h włączonym przez istniejący StartLocationSmoke.cpp. Najpierw zbudować i zmierzyć mapę po punkcie 7, potem zmieniać GeoMeshLibrary::BakeMesh oraz prepare_playable_district.py.

## Stan
W toku. Osobna gałąź z main. Bez CI. Bez zmian oryginalnego worktree.

Pomiar bazowy PASS 18:08:19 UTC: 92.87 m, 24 odcinki, prawidlowe podloze i kamera; ostrzezenia VSM 0 w logu/delegate. 882 statyczne komponenty, 112 z danymi Nanite, 756 non-Nanite rzucajacych cienie. CPU Ryzen 7800X3D, GPU RTX 5070 Ti. Mean ms game 1.829/render 4.505/RHI 1.594/GPU 3.071; cap60/VSync1, D3D12 1280x720, screenpercentage0(auto). Native kompilacja Nanite/SDF PASS. Nastepny krok: Saved/Realism/regenerate8.ps1 fazy 0-5, potem identyczny pomiar po i archiwum. Bez CI.
