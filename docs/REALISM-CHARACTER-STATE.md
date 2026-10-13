# Realizm — punkt 2/10

## Wykonane
Siedem ważonych siatek: anatomia MakeHuman z oczami, dopasowana koszula i spodnie
z oficjalnej paczki, buty, maskowane włosy short02, brwi oraz własny plecak.
32 morphy ciała i twarzy. Ubranie ma prawdziwe wewnętrzne powierzchnie i obszycia.
Źródłowy `.blend`, generator, metadane i 84 sprawdzone wpisy pochodzenia/CC0.
Manifest `Pipeline/assets/player/civilian.asset.json` korzysta z istniejącego
punktu wejścia Blendera i natywnego importera, rozszerzonych o skeletal_mesh.
Przestrzeń zasobów `/Game/Wroclaw/Hero` i osobny katalog wyglądu chronią istniejący
katalog kreatora. Części mogą być przypisane per BodyPreset, a bufor części
uwzględnia ciało. Materiał oczu nie zostaje zastąpiony skórą; pod kurtką nie
rysujemy drugiej siatki tej samej koszuli. Istniejąca kapsuła i systemy gry pozostają.

## Sprawdzenia i ograniczenia
Rzeczywisty Blender 5.2.2: PASS, 71724 trójkąty wszystkich siedmiu siatek.
Manifest: PASS. Build C++: PASS przed ostatnią zmianą domyślnego wyglądu.
Źródła przekopiowano z lokalnie sprawdzonych danych MakeHuman oraz pobrano
wybrane pliki oficjalnej paczki przez HTTP Range; SHA256 i licencje są jawne.
Nie deklarujemy jakości końcowego obrazu bez importu i kadrów z gry.

## Do zrobienia
Natywny import, weryfikacja materiałów/LOD/morphów/Physics Asset, kadry bohatera
na mapie startowej, powtórzenie importu i poprawa ujawnionych problemów.

## Następny krok
Zbudować moduł po zmianie `ApplyModernHeroProfile`, uruchomić
`Pipeline/unreal/import_asset.py` z `WTG_ASSET_MANIFEST=Pipeline/assets/player/civilian.asset.json`.
Sprawdzić `Saved/Pipeline/import/wtg-civilian.json` i log shaderów, następnie
rzeczywistą grę. Foot IK i dalsza praca animacji należą do punktu 3.

## Stan
W toku. Nie uruchamiano CI. Nie naruszono niezatwierdzonych zmian PR #164.
