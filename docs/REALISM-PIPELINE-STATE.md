# Realizm — punkt 9/10

## Wykonane
Punkty 1–8 scalone (#165–172). Istniejące źródła/modele i manifesty są natywne, GIS pozostaje źródłem placement. Sprawdzono osobno licencję graficznych danych MakeHuman (CC0; kod aplikacji ma inną licencję) i Poly Haven (CC0 dla assetów; nie dla zdjęć strony).

## Do zrobienia
Włączyć Source/Full bohatera do istniejącego pipeline; katalog autorów/źródeł/licencji i osobne prawa gry/publicznego repo dla każdego zasobu, infrastruktura i atlas folii. Uruchomić Source/Full, sprawdzić ponowny import, niepowielanie instancji i zachowanie poprawionych materiałów/animacji.

## Następny krok
Invoke-StartLocationProduction.ps1: Source build_hero_models.py oraz import_batch z manifestem player/civilian; write_start_location_catalog.py: provenance kosza, infrastructure.json i hero_source_catalog; następnie pełne odtworzenie.

## Stan
W toku, osobna gałąź z main, bez CI, oryginalny worktree poza zakresem.
