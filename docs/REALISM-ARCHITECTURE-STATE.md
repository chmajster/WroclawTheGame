# Realizm — punkt 5/10

## Wykonane
Punkty 1–4 scalone (#165–168). Rozpoznano aktualny generator:
osiem bliskich budynków ma geometryczne otwory/ramy/parapety, lecz płaskie
dachy i uproszczone wejścia; dalszy plan nadal korzysta z brył GIS.
Referencje miejskie pozostają odniesieniem wizualnym, nie teksturami.
Źródłem obrysów i wysokości jest wersjonowany OSM/polityka istniejącego startu.

## Do zrobienia
Dopracować osiem bliskich budynków: dachy, głębię wejść, gzymsy i obróbki.
Zastąpić ograniczony najbliższy dalszy plan modelami na tych samych obrysach,
bez zmiany kotwic, układu ulic i całej architektury świata.
Przepuścić źródła/manifesty przez pipeline, przypisać do mapy i ocenić grę.

## Następny krok
Dodać wspólny wybór ograniczonych budynków tła używany przez GIS i
build_start_location_inputs.py. Rozbudować build_start_location_models.py::building
o dachy i wejścia; zachować maksymalną wysokość i tag roof:shape=flat.
Następnie build_asset/import_batch, bake i rzeczywista kontrola startu.

## Stan
W toku, draft. Bez CI; pierwotny worktree/PR #164 poza zadaniem.
