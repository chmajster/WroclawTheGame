# Asset Production Pipeline

Ta warstwa nie zastępuje istniejących skryptów kampanii, GIS ani importu darmowych modeli. Spina je z deterministycznym procesem produkcji pojedynczych assetów. Istniejące assety pozostają zgodne z dotychczasowym importem; gdy są istotnie przebudowywane, należy przenieść je pod manifest produkcyjny zamiast utrzymywać drugi równoległy proces.

reference -> source 3D -> Blender -> PBR/LOD/collision -> Unreal -> gameplay automation -> screenshot QA -> poprawka -> commit -> PR -> merge.

## Zasady

- Źródłem prawdy dla assetu jest plik `*.asset.json`.
- Dane robocze, raporty, markery i eksporty trafiają do `Saved/Pipeline/`; katalog `Saved/` jest ignorowany przez Git.
- Blender działa headless. GUI może być używane przez Astrę do modelowania, ale wynik musi przejść ten sam pipeline.
- Unreal importuje gotowy LOD0, podpina kolejne LOD-y przez `StaticMeshEditorSubsystem`, ustawia Nanite i zapisuje raport.
- Screenshot QA wymaga nazwanych kamer z manifestu. Asset jakości `hero` nie przejdzie walidacji bez co najmniej jednej kamery.
- Publikacja do Git jest oddzielną bramką i może nastąpić dopiero po zakończeniu QA.
- Pipeline produkuje asset, ale nie przejmuje źródła prawdy mapy. Pozycje i użycie w świecie zapisuj w istniejących `Data/*`/generatorach kampanii lub GIS, a następnie odtwarzaj mapę tym samym procesem co obecnie.

## Manifest

Skopiuj `Pipeline/examples/asset.example.json` do:

`Pipeline/assets/<obszar>/<asset>.asset.json`

i ustaw co najmniej:

- `source.model` — źródłowy .blend/.fbx/.obj/.glb/.gltf,
- `mesh.max_triangles`,
- `lod.ratios`,
- `collision.mode`,
- tekstury PBR: BaseColor / Normal / ORM,
- `unreal.destination`,
- `unreal.qa_map`,
- `qa.automation_filter`,
- `qa.cameras`.

ORM ma kanały: R=AO, G=Roughness, B=Metallic.

## Uruchomienie

Walidacja przenośna:

```powershell
python Pipeline/qa/validate_manifest.py Pipeline/assets/rynek/latarnia_001.asset.json
```

Pełny pipeline na Windows:

```powershell
.\Pipeline\Invoke-WTGAssetPipeline.ps1 -Manifest Pipeline/assets/rynek/latarnia_001.asset.json -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' -BlenderExe 'C:\Program Files\Blender Foundation\Blender 4.5\blender.exe'
```

Wznowienie od ostatniego zaliczonego etapu:

```powershell
.\Pipeline\Invoke-WTGAssetPipeline.ps1 ... -Resume
```

Publikacja po QA:

```powershell
.\Pipeline\git\Publish-Asset.ps1 -Manifest Pipeline/assets/rynek/latarnia_001.asset.json -Merge
```

## Artefakty robocze

Dla assetu `foo`:

```text
Saved/Pipeline/
  status/foo.json
  generated/foo/
    SM_foo_LOD0.fbx
    SM_foo_LOD1.fbx
    blender-report.json
  import/foo.json
  screenshots/foo/
  automation/foo/
  reports/foo.md
```

## Definition of Done

Asset jest zakończony dopiero, gdy:

1. manifest jest poprawny,
2. Blender zakończył eksport bez przekroczenia budżetu LOD0,
3. Unreal zaimportował mesh i wymagane LOD-y,
4. ustawienie Nanite odpowiada manifestowi,
5. test Automation zakończył się kodem 0,
6. wymagane screenshoty istnieją,
7. raport końcowy ma status `PASS`,
8. dopiero wtedy można utworzyć PR i go scalić.

Sama kompilacja kodu lub sam import FBX nie oznacza ukończenia assetu.

Kontrakt pracy autonomicznego agenta: [ASTRA_AGENT.md](ASTRA_AGENT.md).

## Runtime integration QA

Pipeline pojedynczego assetu nie zastępuje odbioru całej gry. Modele po podłączeniu do kampanii/GIS przechodzą dodatkową bramkę opisaną w [docs/RUNTIME_ASSET_QA.md](../docs/RUNTIME_ASSET_QA.md): LOD, collision, materiały, transformy, IK/FBIK, retarget oraz clipping w kontekście runtime. Packaging wymaga aktualnego `Saved/RuntimeAssetQAPass.ok`.
