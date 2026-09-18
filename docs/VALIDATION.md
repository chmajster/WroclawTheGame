# Weryfikacja 0.3 — 18.09.2026

## Wykonano w środowisku Linux

- Katalog rozdziału, świata i Gameplay Tags zgodny z generatorami.
- Produkcyjny model C++20 z AddressSanitizer i UndefinedBehaviorSanitizer: 12 ścieżek rozdziału, opcjonalne nagrody/pominięcia, warunki zapisów, blokady i 50 000 mieszanych interakcji.
- Event bus (w tym usunięcie subskrypcji podczas publikacji), rozszerzenie evaluatorów, zależności questów, Heat, pogoda, odkrycia, dyrektor lokalnych zdarzeń i round trip serializacji świata.
- Model wyścigu: kolejność bramek, przejazd w złym kierunku, szybkie przekroczenie kilku bramek, limit czasu, utrata ładunku i restart.
- Testy Python: generatory danych/assetów oraz GIS: źródła, dwukierunkowe przeliczenie współrzędnych, skala, kierunki jazdy, graf pieszy, niepoprawne referencje i trasy prób na rzeczywistych krawędziach.
- Import rzeczywistych źródeł OSM/raster, wygenerowanie 281 grup siatek do wypieku w edytorze.
- Kontrola składni Python i białych znaków diffu.
- Walidator manifestów produkcyjnych jest częścią `Scripts/test.sh`; przykład manifestu oraz wszystkie pliki `Pipeline/assets/**/*.asset.json` są sprawdzane bez uruchamiania UE.

Lokalne uruchomienie: `ASAN_OPTIONS=detect_leaks=0 bash Scripts/test.sh`. Wyłączono wyłącznie LeakSanitizer ze względu na środowisko wykonawcze. Nie jest to dowód braku wycieków w grze. CI używa domyślnych ustawień sanitizerów.

## Nie wykonano

Nie ma tutaj instalacji UE/Windows toolchain ani Blendera. Nowa warstwa `Pipeline/` została zweryfikowana statycznie, ale nie wykonano jeszcze rzeczywistego przebiegu Blender → Unreal → Automation → screenshot na maszynie produkcyjnej. Nie skompilowano klas Unreal ani nagłówków refleksji. Nie wykonano testu `WTG.Save.Version3MemoryRoundTrip`, generacji binarnych assetów w edytorze, konwersji WP, HLOD, cookingu, pakowania, testu gameplayu, migracji pliku SaveGame z dysku ani profilowania GPU/CPU.

Nie wolno wnioskować o działaniu UE na podstawie kompilacji przenośnych modeli C++. Procedury Python/PowerShell oraz nowy kod komponentów mogą wymagać poprawek po pierwszym rzeczywistym uruchomieniu silnika. To jest jawna bramka przed odbiorem, a nie wynik zaliczony.

## Odtworzenie

```bash
python3 -m pip install -r Scripts/gis/requirements.txt
bash Scripts/test.sh
python3 Scripts/gis/build_meshes.py
```

Na Windows użyj Build-Windows.ps1 lub Build-Geography.ps1 zgodnie z README. Dla assetów 3D użyj `Pipeline/Invoke-WTGAssetPipeline.ps1`; rzeczywisty PASS musi pochodzić z raportu wygenerowanego przez ten przebieg, nie z samej walidacji składni. Test silnikowy uruchom w Session Frontend → Automation → WTG.Save. Wyniki powinny trafić do raportu odbioru wraz z logami i identyfikatorem commitu. Workflow Windows wymaga własnego runnera z UE 5.8 i zmienną UE_ROOT; nie uruchamiano go na nieistniejącym runnerze.

## Runtime Asset QA — implementacja

Dodano bramki wykonywane przez UE 5.8: `prepare_runtime_asset_quality.py`, `prepare_animation_retargeting.py`, `validate_runtime_asset_scene.py`, `validate_geography_asset_scene.py`, `capture_runtime_character_qa.py`, `record_runtime_asset_visual_review.py` i `build_runtime_asset_qa_report.py`.

Obejmują LOD, kolizje, materiały, bounds, Skeletal LOD, Physics Asset, IK Rig, retarget chains, Full Body IK, dwa targety Quaternius, batch retarget semantic animations, transformy kampanii/GIS oraz deterministyczne screenshoty clippingu. Packaging Windows/GIS wymaga `Saved/RuntimeAssetQAPass.ok`.

W bieżącym środowisku **nie ma UE 5.8/Windows**, więc nie wygenerowano ani nie zadeklarowano PASS tych raportów. To implementacja procesu odbioru, a nie sfabrykowany wynik. Pierwszy rzeczywisty przebieg na maszynie z UE może ujawnić różnice API importera, auto-characterization szkieletu lub korekty transformów; każdy taki problem ma zatrzymać build przed PASS.
