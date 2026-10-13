# Realizm — punkt 4/10

## Wykonane
Punkty 1–3 scalone (#165–167). Dodano WTG.SurfaceDiagnostics: rzeczywista
inwentaryzacja komponentów i instancji w promieniu 150 m, bounds/transformy,
rzuty na ekran i trafienia promieni w kamerze StartLocation_Start_Street.
Diagnostyka nie zmienia mapy, infrastruktury ani elementów misji.

## Do zrobienia
Zidentyfikować poziomy obiekt, poprawić generator ciągłego podłoża/chodników,
nakładanie powierzchni, osadzenie torów i krawężników oraz granice sektorów.
Wygenerować/importować rzeczywistą mapę, sprawdzić geometrię i zrzuty.

## Następny krok
Zbudować Tests/SurfaceDiagnostics.cpp. Uruchomić WTG.SurfaceDiagnostics na
Nadodrze_Playable, odczytać Saved/Realism/surface-runtime.json. Na podstawie
trafionego komponentu poprawić właściwe źródło. Następnie przebudować
Scripts/gis/build_meshes.py i build_playable_district.py.

## Stan
W toku, draft. Bez CI; bez zmian w pierwotnym worktree i PR #164.
