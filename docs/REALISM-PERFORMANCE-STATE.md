# Realizm — punkt 8/10

## Wykonane
W UE 5.8.2 MARKING_JOB_QUEUE to 128 wpisów w grupie 64 wątków, dla zadań obejmujących >8 stron. Przepełnienie uruchamia wolniejszą ścieżkę znakowania; nie jest PAGE_POOL i nie wyjaśnia błędów geometrii. Sprawdzone lokalne VirtualShadowMapBuildPerPageDrawCommands.usf oraz VirtualShadowMapCacheManager.cpp; [dokumentacja Epic](https://dev.epicgames.com/documentation/unreal-engine/virtual-shadow-maps-in-unreal-engine) opisuje osobno koszty non-Nanite i bounds.

GeoMeshLibrary::BakeMesh otrzymał jawne Nanite/SDF; generator włącza Nanite dla 426 nieprzezroczystych sektorów GIS, pełny fallback kolizji i jawne tangenty. 14 wodnych siatek zachowuje raster. Mesh SDF zachowany dla 128 brył architektonicznych, pomijany dla otwartych cienkich nawierzchni. Rasterowe cienie pozostają. Native kontrola 440 siatek: szerokość <=128 m, normalne/tangenty, Nanite i SDF PASS. Sektorowe HISM pozostają; agregowane bounds HISM nie są bounds pojedynczych casterów. Detale elewacji mają culling 180–260 m; drzewa zachowują 100–160 m i trzy LOD. Bez nowych WPO, World Partition/HLOD nie są używane w tej mapie. Limity stron VSM 2048 i komunikaty pozostają włączone.

Natywna kompilacja i sześć faz regeneracji PASS. WTG.RealismPerformance wykonuje prawdziwy ruch, zbiera RHI GPU cycles, aktywne czasy CPU, working set RAM, alokacje tekstur oraz ostrzeżenia przez delegate silnika. Raport porównania i osiem rzeczywistych PNG: docs/qa/realism-2026-10-13/performance. 24 odcinki/sześć pętli, 92.87 m przed i 92.89 m po; podłoże/kamera PASS.

Sprzęt: Ryzen 7 7800X3D, RTX 5070 Ti, 63.08 GiB RAM. D3D12 1280×720, jakości 2 (High), r.ScreenPercentage=0/sg.ResolutionQuality=0 automatyczne, dynamic resolution=0, VSync=1, cap60. Po zmianie publiczny heuristic silnika zwraca primary=1/secondary=1; przed nie zapisano rozstrzygniętej frakcji, jedynie te same ustawienia automatyczne. Godzina 13:00 w obu procesach.

| Średni czas / pamięć | Przed | Po |
|---|---:|---:|
| Wall frame ms | 16.869 | 16.883 |
| Game thread active ms | 1.829 | 1.944 |
| Render thread active ms | 4.505 | 4.700 |
| RHI thread active ms | 1.594 | 1.681 |
| GPU frame ms | 3.071 | 3.159 |
| Process RAM GiB | 3.810 | 3.390 |
| Nanite components | 112 | 538 |
| Non-Nanite shadow components | 756 | 330 |

3854/3851 klatek, 65 s próbkowania. Nie wykazano przyspieszenia GPU ani FPS; wall time obejmuje pacing i zrzuty, inne aplikacje mogą korzystać z GPU. Tekstury ~1.046 GiB w obu przebiegach; to nie całkowite użycie VRAM. Ostrzeżenia VSM 0 w obu, również podczas rozgrzewania; wcześniejsze punkty już mogły usunąć pierwotne przepełnienie. Brak komunikatu na tej trasie nie gwarantuje braku we wszystkich widokach.

## Do zrobienia
Nic w zakresie punktu 8. Punkty 9–10 osobno.

## Następny krok
Scalić #172, utworzyć gałąź punktu 9 z main. Invoke-StartLocationProduction.ps1: dodać bohatera do Source/Full i uzupełnić write_start_location_catalog.py o rzeczywiste autorstwo infrastruktury, kosza i osobne prawa gry/publicznego źródła; wykonać powtórne importy/regenerację.

## Stan
Punkt 8 ukończony. Bez CI. Oryginalny worktree poza zakresem.
