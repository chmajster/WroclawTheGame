# Realizm postaci i lokacji startowej

Zakres: istniejący start Pomorska/Paulińska, promień 100 m, mapa
`/Game/Maps/Nadodrze_Playable`. Dziesięć punktów zadania to dziesięć osobnych
gałęzi i PR do `main`. Nie przenosimy startu i nie przebudowujemy całego miasta.
UE w `.uproject`: 5.8; dostępny lokalnie build: 5.8.2, CL 56702186.

## 1. Rozpoznanie

Gracz: `ASliceCharacter`, Blueprint `/Game/CharacterCreator/BP_WTG_PlayerCharacter`.
Obie klasy trybu gry rozwiązują ten Blueprint w `InitGame`; zapisany poziom
nadpisuje domyślny tryb na `AGeoPreviewGameMode`. `UCharacterAppearanceComponent`
tworzy osobny `BodyMesh` i części modułowe; `ACharacter::GetMesh()` nie jest
renderowaną siatką bohatera. Katalog to `DA_CharacterAppearanceCatalog`.

`prepare_character_creator.py` przypisuje Quaternius Superhero FullBody do
Male.Standard/Female.Standard przy każdym wykonaniu. Aktualne ciało ma
przesadną muskulaturę. Nie jest realistycznym cywilem. Włosy i brwi pochodzą
z osobnych prototypów Quaternius. Ubrania/plecak w katalogu mogą być placeholderami
bez siatki; test `bUsingPlaceholder=false` oznacza jedynie dostępność ciała,
a nie jakość sylwetki ani kompletność ubioru.

Animacja używa pojedynczego węzła i `BS_Male_Standard` / `BS_Female_Standard`,
z Quaternius Idle/Walk/Jog/Sprint, oraz osobnych Crouch/Jump. Na tej ścieżce
nie ma Animation Blueprint ani Foot IK. Części otrzymują Leader Pose.
`LocomotionAssetLibrary::LinkCompatibleRig` sprawdza nazwy i rodziców kości,
ale zgodność hierarchii sama nie dowodzi zgodności orientacji rest pose.

Świat odtwarza `gis/build_playable_district.py` → `prepare_playable_district.py`
→ `prepare_district_presentation.py` → `assets/place_city_details.py` →
`assets/place_start_location.py` → `assets/prepare_start_location_quality.py`.
Położenie wynika z OSM/DEM i wersjonowanych Data, a nie z binarnej mapy.
Mapa/Generated są lokalnymi produktami ignorowanymi przez Git. Audyt bazowy
wykorzystuje kopię istniejących produktów oraz kod `main` w osobnym worktree.

`build_meshes.generate`: zielony poligon jest poziomą powierzchnią na maksimum
wysokości pierścienia. Jest to konkretny błąd dopasowania do DEM, niezależny od
cieni. Drogi używają wysokości osi na całej szerokości. Chodniki używają DEM
na wierzchołkach, lecz triangulacja może rozciągać się między odległymi próbkami.
Nakładają się teren, zieleń i pasy drogi; wymagają podziału według funkcji.
Przydział komórek według centroidu nie ogranicza ich rzeczywistych bounds.

`build_start_location_models.tree`: rzadkie płaskie liście, jednakowe drzewa,
wspólny materiał i UV projekcji 2 m. Gęstość/sylwetka, UV liścia i właściwy
shader wymagają przebudowy. Osiem bliskich budynków już ma geometryczne otwory,
ramy i detale, ale dachy oraz dalszy plan pozostają uproszczone.

Lumen GI/reflections=1, mesh distance fields włączone, VSM włączone,
D3D12/SM6. Nanite ustawiany indywidualnie przez importery, nie przez samo
ustawienie jakości. Świat ma siatki sektorowe i HISM; nie jest World Partition.
`SurfaceQualityDirector::BeginPlay` tworzy ekspozycję/postprocess oraz migruje
ogólne materiały. Osobne materiały artystyczne są chronione przez prefiks pakietu.
`WTGDayNightEnvironment::Tick` aktualizuje słońce/niebo/mgłę podczas gry.
Ustawienia menu mogą zmieniać skalowalność i limit FPS.

Dokładny komunikat VSM znajduje się w lokalnym
`VirtualShadowMapCacheManager.cpp:1081`, flaga `MARKING_JOB_QUEUE`.
Oznacza obciążenie znakowaniem rozległych non-Nanite instancji. Flaga
`PAGE_POOL` jest odrębnym przypadkiem. Audyt natywny wykazał 733 komponentów
non-Nanite, w tym duże komórki GIS. To kandydaci do optymalizacji, a nie dowód
przyczyny każdego artefaktu. Nie zwiększamy puli ani nie ukrywamy ostrzeżeń.

W załączniku nie ma oryginalnego screena. Poziomego elementu przypominającego
rurę nie można jednoznacznie wskazać na podstawie opisu. Rynny, poręcze mostów,
szyny i detale misji mają osobne źródła; nie usuwamy ich w ciemno.

Narzędzia: `assets/audit_realism.py` (przenośny odczyt),
`assets/inspect_realism_native.py` (natywne mesh/bounds/materiały/flag),
`WTG.RealismDiagnostics` (rzeczywista gra, sześć trybów diagnostycznych,
dwie istniejące kamery, bez zapisu profilu użytkownika).

## Źródła ocenianych zasobów

Oficjalna [paczka systemowa MakeHuman](https://static.makehumancommunity.org/assets/assetpacks/makehuman_system_assets.html)
podaje autora `makehuman_system` i CC0 osobno dla casualsuit01, shoes01 i
short01–04. CC0 pozwala na użycie w grze i redystrybucję źródeł. Licencja kodu
aplikacji MakeHuman jest osobna; nie importujemy jej kodu do gry.
Sama legalność nie stanowi akceptacji jakości modelu.

Wyników końcowych, FPS/GPU i fotorealizmu nie deklarujemy na podstawie kodu.
Każdy kolejny strumień dopisze faktycznie wykonane zmiany i dowody.
