# Darmowe postacie i animacje — Quaternius CC0

## Zakres

Projekt ma już szeroki katalog modeli środowiskowych, pojazdów i przedmiotów. Ten pakiet uzupełnia brakującą warstwę humanoidalną:

- Universal Base Characters [Standard]: bazowa postać męska i żeńska,
- fryzury: Simple Parted, Buzzed, Buzzed Female, Long i Buns,
- zarost i dwa warianty brwi,
- Universal Animation Library 1 [Standard] — in-place i Root Motion,
- Universal Animation Library 2 [Standard] — in-place i Root Motion.

Źródła są objęte CC0 1.0. Oficjalne strony autora:

- https://quaternius.com/packs/universalbasecharacters.html
- https://quaternius.com/packs/universalanimationlibrary.html
- https://quaternius.com/packs/universalanimationlibrary2.html

## Powtarzalność pobrania

Oficjalne darmowe pobieranie postaci na itch.io jest interaktywne. Dlatego
`Scripts/fetch_free_character_animation_assets.py` pobiera te same pliki z publicznego
mirrora GitHub przypiętego do commita
`1a9c05693f705cd7ebe0dd7af022d6b9e250d5c5`.

Każdy plik ma zapisany oczekiwany Git blob SHA-1. Skrypt przed zapisem i w trybie
`--check` oblicza identyfikator obiektu Git z bajtów i odrzuca każdą zmianę.

## Import do Unreal Engine

Modele postaci są dołączane do zwykłego katalogu `/Game/FreeModels`.
Animacje importuje osobno `Scripts/import_free_animations.py` do
`/Game/FreeAnimations/Quaternius`.

Build Windows wymaga dwóch markerów:

- `Saved/FreeModelsReady.ok`,
- `Saved/FreeAnimationsReady.ok`.

Import zapisuje mapy rzeczywistych obiektów UE w:

- `Saved/FreeModelImportMap.json`,
- `Saved/FreeAnimationImportMap.json`.

## Granica odpowiedzialności

Pobrane postacie są darmowymi, rigowanymi modelami bazowymi i zastępują blocky fallback
w warstwie źródłowej. Nie są fotorealistycznymi skanami ludzi. Finalne użycie jako
postaci gracza/NPC wymaga odbioru skeletonu, retargetingu, materiałów, LOD i clippingu
w UE 5.8. Skrypt importu celowo zatrzymuje packaging, jeśli biblioteka nie tworzy
żadnego `AnimationAsset`.

Katalog `Data/free_animation_catalog.json` jest generowany z samych GLB i zawiera
rzeczywistą listę nazw klipów oraz ich liczbę, zamiast deklarowanej liczby z opisu
produktu.

## KayKit — domknięcie biblioteki ruchu

Dodatkowo vendored jest kompletny darmowy zestaw KayKit Character Animations 1.1 dla
`Rig_Medium`: osiem bibliotek i 139 klipów odczytanych z GLB. Zestaw dostarcza m.in.
`Lie_Down`, `Lie_Idle`, `Lie_StandUp`, czterokierunkowe uniki, crawl/sneak,
lockpicking, narzędzia, walkę melee/ranged oraz animacje społeczne.

Katalog: `Data/free_kaykit_animation_catalog.json`.
Szczegóły provenance i retargetingu: `docs/KAYKIT_CHARACTER_ANIMATIONS.md`.
