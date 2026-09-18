# Runtime Asset QA — Unreal Engine 5.8

Ten dokument opisuje obowiązkowy odbiór modeli i animacji używanych przez runtime. Samo istnienie FBX/GLTF/GLB w repozytorium nie jest PASS.

## Zakres

QA obejmuje:

- skalę i rotację actor/component,
- LOD Static Mesh i Skeletal Mesh,
- simple collision Static Mesh,
- Physics Asset bazowych postaci,
- kompletność materiałów,
- retarget wszystkich semantic animations na docelowy męski i żeński szkielet Quaternius,
- automatyczne IK Retarget Chains i Full Body IK,
- obecność modeli w 82 fizycznych akcjach kampanii,
- obecność modeli na aktorach systemowych kampanii,
- modele pojazdu, kół, populacji, drzwi i umeblowania GIS,
- screenshot QA sześciu krytycznych póz dla postaci męskiej i żeńskiej,
- jawny visual review clippingu.

Polityka progów znajduje się w `Data/runtime_asset_qa.json`.

## Kolejność bramek

`Scripts/Build-Windows.ps1` wykonuje kolejno:

1. import darmowych modeli,
2. `prepare_runtime_asset_quality.py`,
3. import darmowych animacji,
4. `prepare_animation_retargeting.py`,
5. `prepare_character_creator.py`,
6. generację kampanii,
7. `validate_runtime_asset_scene.py`,
8. konwersję/odbiór World Partition,
9. dla builda pakowanego: `capture_runtime_character_qa.py`,
10. kontrolę aktualnego `visual_review.json`,
11. `build_runtime_asset_qa_report.py`,
12. dopiero wtedy `BuildCookRun`.

`-PrepareOnly` kończy się po bramkach strukturalnych. Nie udaje zaliczonego visual review.

`Scripts/Build-Geography.ps1` uruchamia dodatkowo `validate_geography_asset_scene.py`. Przy `-Package` wymaga również pełnego visual review i raportu końcowego z opcją `--require-geography`.

## LOD, kolizje i materiały

`prepare_runtime_asset_quality.py` pracuje na rzeczywistych obiektach zaimportowanych do Unreal.

Static Mesh:

- dobiera minimalną liczbę LOD do liczby wierzchołków,
- generuje brakujące LOD,
- włącza generowanie lightmap UV,
- dodaje simple box collision, jeżeli mesh nie ma simple collision,
- wymaga co najmniej jednego poprawnego materiału,
- naprawia puste sloty neutralnym materiałem QA,
- odrzuca zerowe lub absurdalnie duże bounds.

Skeletal Mesh:

- wymaga skeletonu,
- wymaga co najmniej dwóch LOD,
- regeneruje brakujący LOD,
- wymaga materiałów,
- dla bazowych modeli ciała wymaga Physics Asset i generuje go, gdy go brakuje,
- sprawdza zakres wymiarów postaci.

Raport:

`Saved/RuntimeAssetQA/model_quality.json`

Marker PASS:

`Saved/RuntimeAssetQualityReady.ok`

## Retarget i IK

`prepare_animation_retargeting.py` nie opiera się na założeniu, że dwa osobno zaimportowane GLB współdzielą ten sam `USkeleton`.

Dla każdej biblioteki animacji:

- tworzy IK Rig źródłowy,
- uruchamia Auto Generated Retarget Definition,
- uruchamia Auto FBIK,
- sprawdza wymagane chains: Spine, LeftArm, RightArm, LeftLeg, RightLeg,
- tworzy osobny IK Retargeter na męski i żeński model Quaternius,
- wykonuje fuzzy auto-map chains,
- automatycznie wyrównuje target pose,
- włącza IK,
- batch-retargetuje wszystkie semantic bindings, również Quaternius → docelowy Quaternius, aby nie polegać na przypadkowym współdzieleniu `USkeleton`,
- sprawdza, czy wynikowy AnimationAsset używa dokładnie skeletonu target mesh.

Wyniki:

- `Saved/RuntimeAssetQA/retarget.json`,
- `Saved/RetargetedAnimationMap.json`,
- `Saved/AnimationRetargetReady.ok`.

Character Creator pobiera z tej mapy Idle, Walk, Jog i Crouch dla obu płci. Gdy BodyPreset nie ma AnimBP, runtime odtwarza te retargetowane animacje bezpośrednio.

## Skala, rotacja i runtime placement

`validate_runtime_asset_scene.py` otwiera wygenerowaną mapę `Przebudzenie_Source` i sprawdza:

- transformy actorów i komponentów,
- brak pustych visual meshes,
- materiały na visual components,
- 82/82 fizyczne action actors,
- guard/NPC/hide/CCTV actor counts,
- obecność wszystkich AnimationAsset wymagających retargetingu.

Raport:

`Saved/RuntimeAssetQA/scene.json`

GIS ma osobny raport:

`Saved/RuntimeAssetQA/geography_scene.json`

## Clipping i visual review

Clipping nie jest wiarygodnie zaliczany przez samą kontrolę bounds.

`capture_runtime_character_qa.py` tworzy deterministyczną mapę `/Game/RuntimeAssetQA/CharacterQA` i renderuje męską oraz żeńską postać w pozach:

- creator idle,
- walk,
- jog,
- crouch,
- stand-up,
- dodge forward.

Włosy, brwi i zarost są follower meshes prowadzone przez body pose.

Screenshoty 1920×1080 trafiają do:

`Saved/RuntimeAssetQA/character_screenshots/`

`capture.json` zawiera SHA-256 każdego obrazu oraz fingerprint wszystkich wejść QA. Zmiana modelu, retargetu, sceny, polityki lub kodu QA unieważnia wcześniejszy visual review.

Po obejrzeniu wszystkich screenshotów zapis PASS:

```powershell
.\Scripts\Review-RuntimeAssetQA.ps1 `
  -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' `
  -Status PASS `
  -Notes 'Skala poprawna; brak widocznego clippingu głowy, dłoni i stóp w sześciu pozach.'
```

FAIL zapisuje się analogicznie z `-Status FAIL` i opisem problemu. FAIL blokuje raport końcowy.

## Raport końcowy

`build_runtime_asset_qa_report.py` wymaga PASS z:

- model quality,
- retarget/IK,
- scene,
- screenshot capture,
- visual review.

Dla pakietu GIS dochodzi `geography_scene`.

Dodatkowo review musi wskazywać dokładnie ten sam fingerprint i te same hashe screenshotów co bieżący capture.

Końcowe pliki:

- `Saved/RuntimeAssetQA/final.json`,
- `Saved/RuntimeAssetQAPass.ok`.

Bez `RuntimeAssetQAPass.ok` packaging nie startuje.

## Granica automatyzacji

Strukturalne i techniczne warunki są automatyczne. Visual clipping wymaga obejrzenia wyrenderowanych klatek. Nie wolno tworzyć `visual_review.json` z PASS bez faktycznej inspekcji obrazów.

Po zmianie assetu lub kodu wpływającego na render fingerprint się zmieni i wcześniejszy PASS przestaje obowiązywać.
