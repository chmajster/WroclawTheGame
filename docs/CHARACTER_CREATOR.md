# Kreator postaci

## Uruchomienie

W menu kampanii wybierz `N — Nowa gra`. Otwiera to `WBP_CharacterCreator`.
Wybierz preset, ustaw wygląd, przejdź do podsumowania i naciśnij `ROZPOCZNIJ KAMPANIĘ`.
Kampania startuje dopiero po udanym zapisie. Powrót do menu nie nadpisuje zapisu.
Szybki start i postać domyślna również wymagają potwierdzenia na podsumowaniu.

Interfejs zawiera tryb podstawowy/zaawansowany, kategorie twarzy, seed (Enter generuje),
reset kategorii, reset całości, undo/redo (50 kroków), ubrania i kolory, profil głosu,
trzy widoki kamery, obrót prawym przyciskiem myszy, zoom, oświetlenie i animacje podglądu.
Panel przycisków i ustawień jest przewijany. Pole seeda przyjmuje int32.

## Assety i odpowiedzialności

`/Game/CharacterCreator/`:

* `BP_WTG_PlayerCharacter` — dziedziczy po `ASliceCharacter`, używany przez oba game mode.
* `BP_WTG_CharacterCreator` — studio z render target i `CharacterAppearanceComponent`.
* `WBP_CharacterCreator` — Blueprint oparty o runtime UMG `UCharacterCreatorWidget`.
* `DA_CharacterAppearanceCatalog` — edytowalne definicje, palety, cztery presety i fallback.
* `M_CharacterPlaceholder` — materiał z parametrami `BaseColor` i `Roughness`.
* `M_CharacterPreview` — materiał UI wyświetlający obraz kamery studia.

`Scripts/prepare_character_creator.py` tworzy tylko brakujące assety. Nie nadpisuje
autorskiego katalogu ani nie przebudowuje map. Uruchamia walidację i zapisuje marker
`Saved/CharacterCreatorReady.ok`; `Build-Windows.ps1` wymaga go przed packagingiem.
Katalog i klasy znajdują się w `DirectoriesToAlwaysCook`.

`UCharacterCreatorSubsystem` przechowuje edytowaną i zatwierdzoną postać, historię,
normalizację i losowanie. `UCharacterAppearanceComponent` renderuje ten sam zestaw
danych w podglądzie i na graczu. Komponent może być dodany również do NPC lub aktora
używanego w cinematicu; przekaż mu bieżące dane, zamiast korzystać z osobnego modelu.

## Zapis i garderoba

`FCharacterCustomizationSaveData` ma wersję niezależną od wersji kampanii oraz zawiera
`PlayerAppearanceData` i `OwnedClothing`. Trafia do `USliceSave` oraz `UCityProgressSave`.
ID nie są ścieżkami do assetów. Brakujące ID są zastępowane kompatybilnym fallbackiem;
przyszła wersja wyglądu wyświetla postać domyślną. Wczytanie/respawn tworzy komponent
z zatwierdzonych danych. Wiek wizualny nie zmienia statystyk.

`UWardrobeComponent::GrantClothing` dodaje znane ID do ekwipunku garderoby; najbliższy
checkpoint zapisuje własność. `Equip` sprawdza własność, slot i zgodność, zapisuje wynik
w aktywnym trybie gry i cofa zmianę przy błędzie zapisu. `UInventoryComponent::Has`
uwzględnia te ID. Ubrania są widoczne w panelu ekwipunku.

`GenerateRandomNPCAppearance(Seed)` nie zmienia edytowanej postaci. Ważny NPC powinien
otrzymywać stały seed zapisany w jego definicji/zapisie, a nie `FMath::Rand()` podczas
każdego spawnu. Algorytm iteruje tablice katalogu w stałej kolejności. Ten sam seed
odtwarza wynik przy tej samej wersji i kolejności katalogu; zapis pełnych danych
zabezpiecza tożsamość po rozbudowie katalogu.

## Podłączenie realistycznych assetów

Runtime korzysta teraz z audytowanych modeli Quaternius CC0 dla baz męskiej i żeńskiej oraz dostępnych włosów, zarostu i brwi. Proceduralny `M_CharacterPlaceholder` pozostaje wyłącznie awaryjnym fallbackiem, gdy źródłowy mesh nie może zostać załadowany. Modele nie są fotorealistycznymi MetaHumanami; finalny retarget animacji, LOD, IK, clipping i odbiór materiałów nadal wymagają QA w Unreal.

1. Dodaj `FBodyPresetDefinition`: ID, płeć, tag zgodności, body/face mesh, referencyjny
   wzrost, obrót modelu, AnimBP ciała i twarzy oraz animacje Idle/Walk/Jog/Crouch.
   Ustaw `bPlaceholder=false`. Komponent nie uruchamia MetaHuman Creator w runtime.
2. Wypełnij `Morphs`: logiczne ID, etykieta, kategoria, nazwa morph targetu i bezpieczne
   zakresy. UI korzysta z definicji; nazwy targetów nie są wpisane w widget.
   Morphy dotyczą face mesh (albo body, gdy brak oddzielnej twarzy).
3. Rig powinien oferować `Body_Slim`, `Body_Average`, `Body_Athletic`, `Body_Heavy` lub
   używać osobnych body presetów. AnimBP może odczytać `HeightRatio` i komponent
   appearance z właściciela. Wzrost skaluje tylko visual root, nigdy Actor.
   Stała kapsuła 200 cm obejmuje zakres 160–195 cm, a kamera uwzględnia wzrost.
   Checkpointy kampanii zachowują dawny układ wysokości przez kompensację +10 cm.
4. Przygotuj materiały z parametrami `SkinTone`, `EyeColor`, `Roughness`, `Freckles`,
   `Imperfections`, `AgeDetail`. Dobierz tekstury i ich mieszanie w materiale do danej
   bazy. Nie należy podłączać wszystkich slotów skomplikowanej twarzy do jednego
   materiału bez sprawdzenia wymagań jej shaderów.
5. Dodawaj fryzury/zarost/brwi do odpowiednich tablic definicji, a ubrania do
   `ClothingDefinitions`: mesh skeletal (zgodny szkielet i leader pose) albo statyczny
   mesh na sockecie, transformacja, materiał, LOD, palety i tagi zgodności.
   Ta implementacja korzysta z meshów/hair cards; groom wymaga adaptera specyficznego
   dla danej bazy i bindingu — nie generuje automatycznie bindingów MetaHuman.
6. `HiddenBodyRegions` wskazuje nazwy slotów materiałowych posegmentowanego ciała.
   Przygotuj body tak, aby sekcje odpowiadały regionom. Nie ukrywaj sekcji całej twarzy.
7. Uruchom `CharacterCreatorValidator::ValidateCatalog` i testy animacji na każdej bazie.
   Zakresy i wspólny budżet morphów są zabezpieczeniem liczbowym; nie zastępują
   przeglądu geometrii, włosów, oczu, LOD i ubrań.

Ciężkie assety ładowane są przez StreamableManager asynchronicznie; numer żądania
chroni przed zastosowaniem spóźnionego wyniku. Komponenty i dynamiczne materiały
są ponownie używane. Zmiana koloru nie przeładowuje mapy ani całego modelu.

## Weryfikacja

Testy Unreal: `Automation RunTests WTG.CharacterCreator` (EditorContext).
Obejmują wymagane sześć scenariuszy oraz historię/odrzucenie zmian.
`WTG.CreatorSmoke` w Development uruchamia kreator w grze, losuje seed 12345,
wykonuje undo/redo, sprawdza dane renderer/subsystem, robi zrzut UI i kończy proces.
Nie rozpoczyna kampanii i nie zapisuje danych użytkownika.
Zrzut: `Saved/Tests/CharacterCreatorPreview.png`.

Weryfikacja implementacji: 7/7 testów `WTG.CharacterCreator` zakończonych sukcesem;
kompilacja Win64 Development dla edytora i gry; podgląd sprawdzony w uruchomionej
grze na mapie `Przebudzenie_Source`. Nie wykonano pełnego cook/package ani odbioru
realistycznych assetów. Test zapisu używa serializacji SaveGame w pamięci;
test wizualny nie nadpisuje istniejącego zapisu kampanii.

Przy odbiorze finalnych assetów sprawdź również: wszystkie cztery presety, 160/195 cm,
crouch pod sufitem, checkpoint/respawn, wszystkie sloty ubrań i kolory, oba szkielety,
materiały oczu, cutsceny/reflections, lodowanie z dużej odległości i brakujące assety.
