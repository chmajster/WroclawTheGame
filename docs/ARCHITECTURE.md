# Architektura 0.3

`Data/chapter1.json` jest źródłem stabilnych ID akcji, questów, inventory, dowodów, wskazówek i warunków. `compile_chapter.py` generuje `ChapterCatalog.h`. `ChapterDefinition` importuje ten katalog do Data Asset i pozwala zmieniać obsługiwane pola akcji; nie zastępuje edycji całego schematu JSON. Nie edytować wygenerowanych nagłówków ręcznie.

`Progress` jest bezsilnikowym modelem wykonania i walidacji. `ObjectiveRegistry` rozdziela evaluatory zdarzeń, inventory, dowodów i liczników. Nowy evaluator wymaga również producenta zdarzenia/licznika w runtime. Nazwa typu w danych sama nie implementuje nowej mechaniki.

`USliceMission` łączy model z UE, interakcjami, checkpointami i prezentacją. Zapis v3 odtwarza uporządkowany `Timeline`, w tym użycie przedmiotów pomiędzy nagrodami. Dzięki temu limit stosu nie zmienia wyniku odtworzenia. Zachowany `History` i `UsedItems` muszą być zgodne z osią czasu. Przed zastosowaniem zapisu walidowane są referencje, liczby, stan świata i NPC. Odczyt v2 zachowuje stary slot; test na rzeczywistym pliku UE pozostaje otwarty.

`Data/openworld.json` definiuje dzielnice, lokacje, strażników, hałas, pogodę, frakcje, NPC, aplikacje telefonu i zdarzenia. `WorldState` obsługuje Heat, czas, odkrycia, cooldowny i deterministyczny generator zdarzeń. `UOpenWorldSubsystem` wiąże je z pozycją i zdarzeniami świata. Dyrektor wybiera zdarzenia tylko w pobliżu gracza; nie tworzy dynamicznego tłumu ani ruchu pojazdów.

`UGameplayEventBus` publikuje typowane tagami zdarzenia świata. `EnemyAI` wykorzystuje widoczność, słuch, stan ostatniej znanej pozycji, lokalne alarmowanie, frakcję i dystans aktywności. Patrol i poszukiwanie wymagają faktycznie zbudowanej nawigacji. Harmonogram mieszkańca używa `MoveTo`; teleportowanie NPC do odległych miejsc nie jest symulacją życia miasta.

`Data/environment.json` opisuje fikcyjny blockout. `prepare_content.py` zapisuje aktorów do mapy źródłowej, po czym commandlet konwertuje ją do World Partition. Runtime nie odtwarza już całej geometrii każdego uruchomienia. `ASliceWorld` steruje globalnym oświetleniem i ambience. NavMesh jest dynamiczny wokół invokerów. HLOD i Data Layers wymagają odbioru silnikowego.

`Geography` jest osobnym torem importu danych. Źródłowa topologia OSM służy do budowy zarówno siatek, jak i tras. `GeoReferenceLibrary` korzysta z GeoReferencing UE; skrypt edytora porównuje konwersję z wynikiem pyproj przed zapisaniem mapy. Nie ma jeszcze migracji koordynatów questów z blockoutu.

`ADriveableVehicle` jest prototypem fizycznego pawna z czterema promieniami podparcia i placeholderem nadwozia. Nie jest implementacją Chaos Wheeled Vehicle z kompletnym modelem opon. `URaceSession` używa wspólnego `RaceProgress`, trzech Data Assets i osobnego slotu rekordów. Nie ma jeszcze powiązania nagród wyścigów z kampanią ani AI kierowców. Próby nie udają nieistniejących przeciwników.

`Pipeline/` jest nadrzędną warstwą produkcji assetów, a nie alternatywnym runtime. Manifest `*.asset.json` opisuje źródło, budżet geometrii, LOD, kolizję, tekstury PBR, cel importu UE i bramki QA. Blender tworzy deterministyczne eksporty w `Saved/Pipeline/generated`, skrypt Unreal importuje mesh, weryfikuje liczbę LOD-ów, kolizję i Nanite, a Automation/screenshot QA zapisują dowody do `Saved/Pipeline`. Wygenerowane produkty i raporty robocze nie są źródłem prawdy repozytorium. Umieszczenie assetu w świecie nadal powinno wynikać z istniejących danych/generatorów kampanii lub GIS, dzięki czemu mapy binarne pozostają odtwarzalne.

Publikacja assetu jest osobną bramką. `Pipeline/git/Publish-Asset.ps1` odmawia publikacji bez końcowego raportu PASS i stanu `done/PASS`; dla assetów wymagających kontroli wizualnej wynik porównania screenshotów z referencją musi zostać jawnie zapisany przed utworzeniem PR.
