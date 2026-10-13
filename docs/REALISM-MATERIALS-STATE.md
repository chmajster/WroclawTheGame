# Realizm — punkt 7/10

## Wykonane
Spójne PBR otoczenia: neutralniejszy kamień i dachówka, stonowana trawa, osobny beton krawężnika, jasna stolarka, ciemny lakierowany metal latarni i balkonów. Metal bazowy ma metallic=1; powłoki, szkło i farba metallic=0. Skala kostki: 72% poprzedniej długości kafla. Tekstury do 2048 px; BaseColor sRGB, Normal liniowy/TC_NORMALMAP, ORM liniowy/TC_MASKS. Native probes wykluczyły błędny import BaseColor jako HDR.

Bohater: licencjonowane AO ubrania, liniowe próbkowanie ORM plecaka, subtelna zmienność roughness skóry z istniejącej tekstury. Natywne materiały i AO zapisane; szkielet, animacje, katalog i funkcja plecaka zachowane. Pipeline Full odświeża te materiały.

Słońce 110000, światło nieba 2.2, balans bieli 6500 K. Aberracja, DOF i motion blur wyłączone; bloom oszczędny. Flaga QA zamraża czas tylko w procesie testowym, zwykła gra zachowuje cykl dnia i nocy.

Walidacja: kompilacja natywna PASS; materiały, folie, przypisania, 42 assety, osiem powierzchni PBR i osiem materiałów bohatera PASS. WTG.MaterialDiagnostics: 44 rekordy rzeczywistych materiałów. 18 ujęć przed i 18 po z tych samych kamer o 13:00, D3D12 1280×720, oraz cztery ujęcia zwykłej gry po finalnym PBR bohatera. Test gry 17:53:44 UTC PASS: ruch 201.40 cm, prawidłowe podłoże, kamera bez kolizji, osiem blokujących ścian. Dowody/SHA256: docs/qa/realism-2026-10-13/materials/evidence.json.

Ograniczenia: nieprzezroczyste przybliżenie szkła bez wnętrz; architektura odtwarza GIS i lokalny charakter, nie pełny pomiar elewacji. NPC Maja nie została przebudowana. Nie deklarujemy fotorealizmu ani gwarantowanego FPS na podstawie tych kontroli.

## Do zrobienia
Nic w zakresie punktu 7. Punkty 8–10 pozostają osobnymi strumieniami.

## Następny krok
Scalić PR #171, utworzyć gałąź punktu 8 z main. Przeanalizować MARKING_JOB_QUEUE w VirtualShadowMapBuildPerPageDrawCommands.usf UE 5.8; zmierzyć tę samą trasę przed/po, zinwentaryzować non-Nanite i sektorowe bounds.

## Stan
Punkt 7 ukończony. Bez uruchamiania CI. Oryginalny worktree poza zakresem.
