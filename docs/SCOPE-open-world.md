Rozbuduj istniejący projekt **WroclawTheGame** w Unreal Engine 5.

Nie twórz projektu od nowa, jeżeli repozytorium już istnieje. Najpierw przeanalizuj aktualną architekturę, systemy, kod C++, Blueprinty, Data Assets, mapy i zależności. Następnie wykonuj refaktoryzację i implementację w sposób kompatybilny z istniejącym projektem.

# 1. GŁÓWNY CEL

WroclawTheGame ma docelowo być realistyczną grą:

- single-player,
- Windows PC,
- Unreal Engine 5,
- third-person,
- open world,
- action-adventure,
- thriller,
- survival,
- escape room,
- investigation,
- stealth,
- combat,
- exploration,
- campaign-driven.

Akcja rozgrywa się we Wrocławiu.

Gracz może swobodnie eksplorować miasto, ale nie może ukończyć gry poprzez zwykłe wyjechanie poza mapę.

Aby naprawdę opuścić Wrocław i zakończyć grę, musi przejść główną kampanię.

Open world ma być przestrzenią służącą:

- eksploracji,
- questom,
- śledztwu,
- zagadkom,
- wydarzeniom losowym,
- spotkaniom,
- pościgom,
- skradaniu,
- walce,
- odkrywaniu historii.

# 2. NAJWAŻNIEJSZA ZASADA ARCHITEKTURY

Każdy system implementuj tak, aby można było go później rozszerzać bez przebudowy całej gry.

Nie hardcoduj:

- questów,
- dialogów,
- zagadek,
- NPC,
- przedmiotów,
- dzielnic,
- przeciwników,
- wydarzeń,
- kodów,
- nagród,
- celów misji.

Dane gameplayowe powinny być konfigurowalne przez:

- Data Assets,
- Data Tables,
- Gameplay Tags,
- komponenty,
- interfejsy,
- subsystemy,
- konfigurację.

Unikaj konstrukcji:

if MissionID == 17

if District == "Rynek"

if ItemName == "RedKey"

Zamiast tego używaj architektury data-driven.

# 3. OPEN WORLD

Docelowo WroclawTheGame ma posiadać jeden spójny świat Wrocławia.

Wykorzystaj:

- World Partition,
- streaming,
- Data Layers,
- HLOD,
- One File Per Actor,
- Level Instances,
- PCG tam, gdzie będzie przydatne.

Świat powinien być ładowany dynamicznie.

Nie utrzymuj całego miasta jednocześnie w pamięci.

# 4. STRUKTURA MIASTA

Podziel świat logicznie na dzielnice/strefy.

Przykład:

- Rynek,
- Stare Miasto,
- Nadodrze,
- Ostrów Tumski,
- okolice Dworca Głównego,
- Śródmieście,
- osiedla mieszkaniowe,
- tereny przemysłowe,
- obrzeża miasta,
- okolice Odry,
- strefy magazynowe.

Każda strefa powinna posiadać DistrictDefinition.

Przykładowe dane:

DistrictID

DisplayName

GameplayTags

ThreatLevel

AvailableActivities

RandomEvents

QuestDefinitions

Safehouses

EnemySpawnProfiles

AmbientProfiles

TimeProfiles

WeatherProfiles

Nie koduj mechanik osobno dla każdej dzielnicy.

# 5. DISTRICT MANAGER

Stwórz system odpowiedzialny za aktywną dzielnicę.

DistrictManager powinien:

- wykrywać aktualną strefę gracza,
- aktywować odpowiednie zdarzenia,
- zmieniać poziom zagrożenia,
- dobierać NPC,
- dobierać przeciwników,
- uruchamiać ambient,
- obsługiwać questy dzielnicy,
- aktywować lokalne encountery.

# 6. OPEN WORLD NIE MOŻE BYĆ PUSTY

Miasto ma zawierać aktywności.

Przykłady:

- wydarzenia losowe,
- questy poboczne,
- kryjówki,
- zamknięte pomieszczenia,
- mini escape roomy,
- dokumenty,
- ukryte przejścia,
- przeciwników,
- neutralnych NPC,
- spotkania fabularne,
- miejsca śledztwa,
- punkty obserwacyjne,
- opcjonalne zagadki.

Nie wypełniaj świata setkami znaczników.

Eksploracja powinna nagradzać obserwację.

# 7. WORLD EVENT SYSTEM

Stwórz uniwersalny WorldEventSystem.

Typy wydarzeń:

- Ambush,
- SuspiciousVehicle,
- InjuredNPC,
- EnemyPatrol,
- Chase,
- RoadBlock,
- PhoneCall,
- HiddenCache,
- InvestigationLead,
- NPCEncounter,
- EnvironmentalEvent.

Każde wydarzenie ma posiadać własną definicję danych.

WorldEventSystem powinien uwzględniać:

- dzielnicę,
- godzinę,
- pogodę,
- HeatLevel,
- etap kampanii,
- wcześniejsze decyzje,
- cooldown wydarzenia,
- szansę wystąpienia.

# 8. DIRECTOR AI

Dodaj globalny GameplayDirector.

Jego zadaniem jest kontrolowanie napięcia.

Director nie może oszukiwać gracza.

Powinien zarządzać:

- częstotliwością spotkań,
- patrolami,
- intensywnością zagrożenia,
- okresami ciszy,
- zasadzkami,
- przypadkowymi wydarzeniami.

Przykład:

długi brak zagrożenia
→ możliwość wygenerowania podejrzanego wydarzenia.

intensywny pościg
→ zmniejszenie prawdopodobieństwa natychmiastowego kolejnego ataku.

Nie teleportuj przeciwników bezpośrednio przed graczem.

# 9. HEAT SYSTEM

Stwórz globalny system zainteresowania przeciwników graczem.

HeatLevel:

0 — brak zainteresowania

1 — obserwacja

2 — zwiększona liczba patroli

3 — aktywne poszukiwania

4 — blokady i zasadzki

5 — intensywne polowanie

Heat powinien wzrastać m.in. przez:

- głośną walkę,
- wykrycie,
- pozostawienie świadków,
- ukończenie istotnej misji,
- działania fabularne.

Powinien spadać poprzez:

- ukrywanie się,
- zmianę dzielnicy,
- wykorzystanie kryjówki,
- utratę pościgu.

# 10. AI PERCEPTION

Rozbuduj AI.

Przeciwnik nie może posiadać permanentnej wiedzy o pozycji gracza.

Stany:

Idle

Patrol

Suspicious

Investigate

Search

Alert

Chase

Combat

LostTarget

ReturnToPatrol

AI powinno wykorzystywać:

- wzrok,
- słuch,
- ostatnią znaną pozycję,
- pamięć zdarzeń.

# 11. SYSTEM HAŁASU

Stwórz NoiseSystem.

Każde działanie może generować:

NoiseEvent.

Przykłady:

- chodzenie,
- sprint,
- otwieranie drzwi,
- rozbijanie szkła,
- przewracanie przedmiotu,
- walka,
- rzucanie przedmiotem.

NoiseEvent powinien posiadać:

Location

Radius

Intensity

NoiseType

Source

AI reaguje odpowiednio do natężenia.

# 12. UKRYWANIE

Dodaj HideableComponent.

Przykładowe kryjówki:

- szafy,
- kontenery,
- ciemne pomieszczenia,
- samochody,
- miejsca pod łóżkiem.

AI może sprawdzić kryjówkę, jeżeli posiada wystarczające podejrzenie.

Nie może automatycznie wiedzieć, gdzie gracz się schował.

# 13. ŚWIATŁO I WIDOCZNOŚĆ

Wprowadź podstawowy VisibilitySystem.

Na możliwość wykrycia powinny wpływać:

- odległość,
- światło,
- pozycja gracza,
- ruch,
- kucanie,
- przeszkody,
- pogoda.

Nie wymagaj pełnej symulacji fotonów.

System ma być przewidywalny gameplayowo.

# 14. QUEST SYSTEM

Przebuduj questy na w pełni data-driven system.

QuestDefinition:

QuestID

Title

Description

Category

RequiredQuests

Objectives

Rewards

FailureConditions

District

CampaignChapter

GameplayTags

QuestState:

Locked

Available

Active

Completed

Failed

Suspended

# 15. QUEST GRAPH

Questy nie muszą być liniowe.

Obsłuż:

- rozgałęzienia,
- alternatywne cele,
- questy opcjonalne,
- konsekwencje,
- różne rozwiązania,
- warunki ukryte.

Przykład:

WEJŚCIE DO BIURA

Metoda A:
znajdź klucz.

Metoda B:
ukradnij kartę dostępu.

Metoda C:
wyłącz alarm.

Metoda D:
wejdź przez dach.

Wszystkie rozwiązania prowadzą do wspólnego kolejnego etapu.

# 16. OBJECTIVE SYSTEM

Każdy quest może posiadać wiele typów objective.

Nie implementuj każdego typu bezpośrednio w MissionManager.

Stwórz rozszerzalny system klas objective.

Przykłady:

ReachLocation

FindItem

CollectEvidence

Interact

TalkToNPC

SolvePuzzle

EscapeArea

LosePursuit

DefeatEnemy

HackTerminal

PhotographObject

FollowNPC

ProtectNPC

SurviveTime

SearchArea

UseItem

Każdy nowy typ objective powinien być możliwy do dodania bez przebudowy MissionManager.

# 17. PUZZLE FRAMEWORK

Zagadki są jednym z głównych filarów gry.

Stwórz modularny Puzzle Framework.

BasePuzzleComponent.

PuzzleDefinition.

PuzzleState:

Inactive

Available

InProgress

Solved

Failed

Typy:

NumericCode

SymbolSequence

Electrical

LightSequence

Terminal

ComputerPassword

AudioPuzzle

FrequencyPuzzle

ItemCombination

EnvironmentalPuzzle

MechanicalLock

InvestigationPuzzle

ObservationPuzzle

# 18. ZAGADKI WIELOETAPOWE

Puzzle mogą posiadać dependencies.

Przykład:

znajdź zdjęcie

→ zauważ datę

→ uruchom komputer

→ sprawdź wiadomość

→ odnajdź lokalizację

→ zdobądź klucz

→ otwórz sejf.

Nie traktuj zagadki wyłącznie jako pojedynczego Actor.

# 19. SYSTEM PODPOWIEDZI

HintSystem powinien działać niezależnie od konkretnej zagadki.

PuzzleDefinition zawiera:

HintLevel1

HintLevel2

HintLevel3

HintDelay.

Podpowiedzi powinny być opcjonalne.

# 20. RANDOMIZACJA PUZZLI

Puzzle mogą posiadać warianty.

Randomizowane mogą być:

- kody,
- sekwencje,
- lokalizacje przedmiotów,
- symbole.

Losowanie musi być zapisane w SaveGame.

Wczytanie zapisu nie może generować nowej odpowiedzi.

# 21. INVESTIGATION SYSTEM

Stwórz rozbudowany system śledztwa.

EvidenceDefinition:

EvidenceID

Category

Description

RelatedPersons

RelatedLocations

RelatedEvents

DiscoveryRequirements

Evidence może należeć do kategorii:

- Person,
- Location,
- Document,
- Photo,
- Recording,
- Message,
- Object,
- Vehicle.

# 22. ŁĄCZENIE DOWODÓW

Gracz powinien móc łączyć dowody.

Przykład:

zdjęcie samochodu

-

numer rejestracyjny

-

nagranie monitoringu

\=

nowy trop.

Stwórz EvidenceCombinationDefinition.

Po poprawnym połączeniu system może:

- uruchomić quest,
- odkryć lokalizację,
- odblokować dialog,
- dodać nowy dowód.

# 23. TELEFON

Telefon ma być jednym z podstawowych narzędzi.

PhoneSystem powinien obsługiwać moduły/aplikacje.

Aplikacje:

Messages

Calls

Contacts

Camera

Gallery

Map

Notes

Investigation

Objectives

Moduły telefonu nie powinny być bezpośrednio sprzężone ze sobą.

# 24. WIADOMOŚCI

MessageDefinition:

Sender

Receiver

Timestamp

Text

Attachments

TriggerCondition

QuestEffects

Messages mogą pojawiać się dynamicznie w zależności od:

- questa,
- miejsca,
- czasu,
- decyzji,
- HeatLevel.

# 25. SAFEHOUSE

Dodaj SafehouseSystem.

Kryjówki mogą służyć do:

- zapisu,
- odpoczynku,
- analizy dowodów,
- wyboru tropów,
- przeglądania mapy,
- przygotowania wyposażenia.

Safehouse powinien być typem danych, nie specjalnym hardcodowanym levelem.

Docelowo gracz może posiadać kilka kryjówek.

# 26. WYBORY I KONSEKWENCJE

Stwórz prosty system WorldState.

Przechowuj flagi:

NPC\_X\_Saved

NPC\_X\_Dead

Evidence\_Y\_Found

Quest\_Z\_AlternativeSolution

District\_A\_Unlocked

Nie używaj setek oddzielnych booli w GameInstance.

Zastosuj Gameplay Tags lub odpowiednią strukturę danych.

# 27. NPC

Przygotuj rozszerzalny NPC Framework.

NPCDefinition:

NPCID

Name

Role

Faction

Schedule

DialogueProfile

BehaviorProfile

QuestProfile

NPC może być:

- neutralny,
- sojuszniczy,
- podejrzany,
- informator,
- przeciwnik,
- cywil.

# 28. NPC SCHEDULE

NPC mogą mieć harmonogram.

Przykład:

08:00 — dom

09:00 — sklep

12:00 — ulica

18:00 — dom.

Nie symuluj setek postaci poza zasięgiem gracza.

Dla nieaktywnych NPC wystarczy symulacja logiczna.

# 29. FACTIONS

Dodaj FactionSystem.

Przykładowe relacje:

Friendly

Neutral

Suspicious

Hostile

Relacje powinny wynikać z danych.

# 30. COMBAT

CombatSystem musi być osobnym modułem.

Minimum:

- LightAttack,
- HeavyAttack,
- Block,
- Dodge,
- Stamina,
- Damage,
- Knockdown.

Nie wiąż systemu bezpośrednio z jednym typem postaci.

Użyj:

HealthComponent

StaminaComponent

CombatComponent.

# 31. WALKA NIE JEST JEDYNYM ROZWIĄZANIEM

Projektuj encountery tak, aby często istniały:

- stealth,
- distraction,
- escape,
- avoidance,
- combat.

# 32. INVENTORY

InventoryComponent nie powinien znać konkretnych przedmiotów.

ItemDefinition:

ItemID

DisplayName

Description

ItemType

Weight

StackSize

GameplayTags

Icon

UsableComponent

Kategorie:

QuestItem

Key

Tool

Evidence

Consumable

Weapon

Document

# 33. ITEM INTERACTIONS

Przedmioty mogą reagować poprzez tagi.

Przykład:

Door.Requires.Key.Red

Item ma:

Key.Red

Nie wpisuj:

if PlayerHasRedKey.

# 34. INTERACTION SYSTEM

Wszystkie interaktywne obiekty wykorzystują wspólny framework.

IInteractable.

InteractionComponent.

InteractionDefinition.

Obsłuż:

- Use,
- Examine,
- Take,
- Open,
- Close,
- Unlock,
- Talk,
- Hack,
- Hide.

# 35. SYSTEM DRZWI

DoorComponent powinien obsługiwać:

Unlocked

Locked

KeyLocked

CodeLocked

ElectronicLocked

QuestLocked

Broken

Nie twórz osobnej klasy drzwi dla każdej misji.

# 36. SYSTEM ELEKTRYCZNOŚCI

Dodaj PowerNetworkSystem.

Obiekty mogą implementować PowerConsumerComponent.

Przykłady:

- drzwi,
- terminal,
- światło,
- monitoring,
- winda,
- alarm.

PowerSource może być:

- sieć,
- generator,
- bezpiecznik,
- bateria.

Pozwoli to tworzyć dziesiątki zagadek bez pisania osobnych systemów.

# 37. CCTV

Dodaj CCTVSystem.

Kamery mogą:

- działać,
- zostać wyłączone,
- wykryć gracza,
- przesyłać obraz do terminala,
- nagrywać materiał fabularny.

# 38. ALARM SYSTEM

Obiekty powinny generować AlarmEvent.

Alarm może aktywować:

- przeciwników,
- zamknięcie drzwi,
- większy HeatLevel,
- dodatkowy patrol.

# 39. ŚWIAT OTWARTY I KAMPANIA

Open world nie oznacza braku kampanii.

CampaignManager kontroluje główną historię.

Kampania odblokowuje:

- kolejne dzielnice,
- nowe aktywności,
- kontakty,
- kryjówki,
- typy przeciwników,
- informacje.

Gracz może swobodnie eksplorować odblokowane obszary.

# 40. OPUSZCZENIE WROCŁAWIA

Gracz może próbować dostać się do:

- dróg wylotowych,
- dworca,
- lotniska,
- innych miejsc prowadzących poza miasto.

Przed zakończeniem kampanii musi istnieć logiczne uzasadnienie, dlaczego ucieczka jest niemożliwa.

Nie stosuj wyłącznie niewidzialnej ściany.

Możliwe:

- blokady,
- kontrola przeciwników,
- wydarzenie fabularne,
- uszkodzona infrastruktura,
- ryzyko natychmiastowego wykrycia.

FinalEscapeMission może zostać aktywowana tylko po:

IsMainCampaignCompleted() == true.

# 41. FAST TRAVEL

Nie implementuj teleportacji od początku.

Jeżeli później zostanie dodana, powinna wymagać:

- odkrycia lokalizacji,
- braku aktywnego pościgu,
- odpowiedniego punktu podróży.

# 42. POJAZDY

Architektura świata musi pozwalać na późniejsze dodanie pojazdów.

Nie muszą być częścią pierwszego etapu.

Przygotuj VehicleInteractionInterface i miejsca parkingowe/spawn profile bez budowania całego VehicleSystem na początku.

# 43. POGODA

Przygotuj WeatherManager.

Profile:

Clear

Cloudy

Rain

Fog

Storm.

Pogoda może wpływać na:

- widoczność,
- NPC,
- ambient,
- AI perception.

Nie jest konieczne implementowanie wszystkich efektów od razu.

# 44. DZIEŃ/NOC

TimeOfDayManager.

System powinien być niezależny od questów.

Quest może posiadać requirement:

Time.Morning

Time.Night

ale nie może bezpośrednio sterować całą logiką czasu.

# 45. SAVE SYSTEM

SaveGame musi przechowywać:

- CampaignState,
- QuestStates,
- ObjectiveStates,
- PuzzleStates,
- WorldState,
- Evidence,
- Inventory,
- HeatLevel,
- DistrictStates,
- RandomPuzzleSeeds,
- discovered locations,
- NPC state,
- safehouses,
- ustawienia.

Zaprojektuj wersjonowanie SaveGame.

Dodaj SaveVersion.

Przyszłe aktualizacje gry nie mogą automatycznie niszczyć starszych zapisów.

# 46. GAMEPLAY TAGS

Wykorzystuj Gameplay Tags szeroko.

Przykłady:

State.Player.Hidden

State.Player.InCombat

State.World.Alert

Quest.Main

Quest.Side

Objective.Stealth

Item.Key

Item.Evidence

District.Rynek

Enemy.Assassin

Noise.Glass

Door.Locked.Electronic

Nie używaj tagów jako zamiennika całej architektury, ale wykorzystuj je do luźnego sprzęgania systemów.

# 47. EVENT BUS

Systemy nie powinny mieć bezpośrednich twardych zależności.

Dodaj Gameplay Event Bus lub odpowiednie Unreal Subsystemy.

Przykład:

PuzzleSolvedEvent

→ Mission System aktualizuje objective.

→ Investigation System może dodać evidence.

→ Audio System odtwarza efekt.

Puzzle nie powinno ręcznie wywoływać wszystkich tych systemów.

# 48. SUBSYSTEMS

Rozważ użycie:

UGameInstanceSubsystem

UWorldSubsystem

ULocalPlayerSubsystem

dla globalnych managerów.

Nie wrzucaj wszystkich managerów do GameMode.

# 49. KOMPONENTY

Preferuj composition over inheritance.

Przykład EnemyCharacter może posiadać:

HealthComponent

CombatComponent

PerceptionComponent

InvestigationComponent

FactionComponent

Nie twórz hierarchii:

Character

EnemyCharacter

AssassinEnemy

AssassinEnemyWithKnife

AssassinEnemyWithKnifeNight.

# 50. DEPENDENCY MANAGEMENT

Minimalizuj circular dependencies.

Warstwy:

Core

Data

Gameplay Framework

World Systems

Character Systems

Quest Systems

UI.

Core nie może zależeć od UI.

# 51. UI

UI ma pobierać stan systemów.

Nie umieszczaj logiki gameplayu bezpośrednio w widgetach.

Zastosuj:

ViewModel / Presenter / event binding.

HUD:

- zdrowie,
- stamina,
- aktualny objective,
- status zagrożenia,
- kontekst interakcji.

# 52. DEBUG TOOLS

Dodaj development/debug panel.

Komendy:

SetHeatLevel

CompleteQuest

StartQuest

TeleportToDistrict

GiveItem

AddEvidence

SetTime

SetWeather

ToggleAI

ShowNoiseEvents

ShowAIPerception

Debug narzędzia mają działać tylko w Development Build.

# 53. LOGOWANIE

Utwórz osobne Log Categories.

Przykłady:

LogWTGQuest

LogWTGAI

LogWTGPuzzle

LogWTGWorld

LogWTGSave

LogWTGInventory.

Nie używaj wszędzie LogTemp.

# 54. AUTOMATED TESTS

Dodaj testy tam, gdzie jest to możliwe.

Szczególnie:

- QuestState,
- prerequisite quests,
- Inventory,
- SaveGame serialization,
- Evidence combinations,
- HeatLevel,
- Puzzle state,
- Objective completion.

# 55. PERFORMANCE

Open world projektuj od początku pod wydajność.

Używaj:

- World Partition,
- HLOD,
- instancing,
- cull distance,
- pooling tam, gdzie ma sens,
- async loading.

Nie uruchamiaj pełnej AI wszystkich NPC w całym mieście.

AI Simulation Level:

Full

Simplified

Dormant.

W zależności od odległości od gracza.

# 56. PIERWSZY OPEN WORLD VERTICAL SLICE

Nie buduj od razu całego Wrocławia.

Pierwsza mapa open world powinna posiadać niewielki fragment miasta.

Przykładowo:

około kilka–kilkanaście przecznic reprezentujących jedną część miasta.

Powinna zawierać:

- mieszkanie,
- kamienicę,
- podwórko,
- sklep,
- warsztat,
- garaż,
- kilka ulic,
- zaułki,
- niewielki park/skwer,
- bezpieczną kryjówkę,
- zamknięte budynki dekoracyjne,
- kilka dostępnych wnętrz.

Gracz musi móc swobodnie poruszać się po tym obszarze.

Nie prowadź go ciągle korytarzem.

# 57. QUESTY OPEN WORLD

Vertical slice powinien posiadać:

- quest główny,
- kilka większych etapów kampanii,
- 5–10 questów pobocznych,
- mini-zagadki,
- sekrety,
- wydarzenia losowe.

Przykładowe questy poboczne:

## ZAGINIONY TELEFON

Znajdź telefon należący do tajemniczej osoby.

Odblokowanie telefonu ujawnia zdjęcie bohatera.

## ŚWIATŁO W OKNIE

Gracz zauważa sygnały świetlne w mieszkaniu.

Musi ustalić kod Morse'a.

## ZAMKNIĘTY GARAŻ

Seria wskazówek prowadzi do ukrytego magazynu.

## RANNY CZŁOWIEK

Gracz decyduje, czy pomóc NPC.

Decyzja wpływa na późniejsze wydarzenie.

## PORZUCONY SAMOCHÓD

Znajdują się w nim dokumenty związane z antagonistami.

## KAMERA

Wyłącz monitoring w określonej części dzielnicy.

## DZIWNY SYGNAŁ

Telefon wykrywa sygnał radiowy.

Gracz musi ustawić właściwą częstotliwość.

## UKRYTE MIESZKANIE

Znalezienie serii symboli prowadzi do sekretnego mieszkania.

# 58. PUZZLE OPEN WORLD

Dodaj również zagadki, które nie należą bezpośrednio do questów.

Przykład:

gracz zauważa trzy symbole na różnych budynkach.

Po znalezieniu wszystkich otrzymuje lokalizację ukrytego pomieszczenia.

# 59. RANDOM ENCOUNTERS

Pierwsza wersja może posiadać kilka encounter definitions.

Przykłady:

- dwóch przeciwników przechodzi ulicą,
- NPC prosi o pomoc,
- samochód obserwuje gracza,
- przeciwnicy przeszukują kamienicę,
- słychać strzał z bocznej ulicy,
- ktoś zostawia paczkę.

Nie każde wydarzenie powinno kończyć się walką.

# 60. DYNAMICZNE PATROLE

PatrolRoute nie powinien być zawsze identyczny.

AI może wybierać punkty z PatrolNetwork.

Dzięki temu gracz nie może nauczyć się każdego patrolu na pamięć po jednym przejściu.

# 61. SYSTEM INFORMACJI AI

Nie wszyscy przeciwnicy mają automatycznie wspólną wiedzę.

Dodaj możliwość przesłania AlertEvent.

Przeciwnik może:

- zauważyć gracza,
- poinformować innych,
- uruchomić alarm.

Jeżeli zostanie unieszkodliwiony przed wysłaniem informacji, inni nie powinni automatycznie wiedzieć o graczu.

# 62. FAŁSZYWE TROPY

Investigation System musi obsługiwać:

EvidenceValidity:

Unknown

Confirmed

Questionable

False.

Niektóre dowody mogą być celowo podłożone.

Gra nie powinna automatycznie mówić graczowi, który trop jest fałszywy.

# 63. DISCOVERY SYSTEM

Nie pokazuj wszystkich lokacji od początku.

Gracz odkrywa:

- miejsca,
- safehouse,
- questy,
- przejścia,
- sekrety.

DiscoveryComponent / LocationDefinition powinny obsługiwać stan:

Unknown

Discovered

Visited

Completed.

# 64. ŚWIAT NIE MOŻE ZATRZYMYWAĆ SIĘ DLA QUESTA

Quest powinien korzystać z istniejącego świata.

Przykład:

przeciwnik może patrolować ulicę również zanim aktywujesz quest.

Quest zmienia parametry świata zamiast tworzyć sztuczną osobną rzeczywistość.

# 65. MODULARNE BUDYNKI

Stwórz modularny system środowiska.

Elementy:

- ściany,
- drzwi,
- okna,
- schody,
- dachy,
- elewacje,
- balkony,
- wejścia.

Pozwoli to szybciej budować Wrocław.

# 66. WNĘTRZA

Nie każdy budynek musi być dostępny.

Stosuj klasy:

Decorative

PartialInterior

GameplayInterior

QuestInterior.

Pozwoli to kontrolować zakres projektu.

# 67. PROCEDURAL CONTENT

PCG wykorzystuj do elementów pomocniczych:

- roślinność,
- śmieci,
- drobne propsy,
- parkowanie,
- niektóre dekoracje.

Nie generuj proceduralnie głównych questów fabularnych.

# 68. ROADMAPA IMPLEMENTACJI

Implementuj w tej kolejności:

FAZA 1 — FUNDAMENT

- architektura modułowa,
- Gameplay Tags,
- SaveGame,
- Event Bus,
- subsystemy.

FAZA 2 — GRACZ

- ruch,
- interakcje,
- inventory,
- health,
- stamina.

FAZA 3 — OPEN WORLD

- World Partition,
- District System,
- Discovery System,
- streaming.

FAZA 4 — QUESTY

- Quest Framework,
- Objective Framework,
- CampaignManager.

FAZA 5 — PUZZLE

- Puzzle Framework,
- Hint System,
- randomizacja.

FAZA 6 — AI

- perception,
- Noise System,
- suspicion,
- search,
- chase,
- combat,
- patrol network.

FAZA 7 — ŚLEDZTWO

- Evidence,
- clue combinations,
- investigation UI.

FAZA 8 — ŚWIAT DYNAMICZNY

- World Events,
- Heat,
- Gameplay Director,
- random encounters.

FAZA 9 — PHONE

- Messages,
- Camera,
- Gallery,
- Map,
- Investigation integration.

FAZA 10 — CONTENT

- quest główny,
- questy poboczne,
- zagadki,
- world events.

FAZA 11 — WINDOWS

- optimization,
- QA,
- Windows build.

# 69. DEFINITION OF DONE DLA SYSTEMU

System nie jest ukończony tylko dlatego, że kod się kompiluje.

Musi:

- działać w grze,
- posiadać konfigurację data-driven,
- mieć przykładową implementację,
- obsługiwać zapis,
- obsługiwać błędne dane,
- nie powodować crasha,
- być możliwy do dalszego rozszerzenia.

# 70. CODE QUALITY

Kod ma być:

- czytelny,
- modularny,
- zgodny z praktykami Unreal Engine,
- bez wielkich klas typu God Object,
- bez powielania logiki,
- bez niepotrzebnych Singletonów,
- bez hardcoded content.

Używaj:

- Interfaces,
- Actor Components,
- Subsystems,
- Data Assets,
- Gameplay Tags,
- Delegates,
- Events.

# 71. DOKUMENTACJA ARCHITEKTURY

Utwórz:

docs/ARCHITECTURE.md

Opisuj:

- moduły,
- zależności,
- event flow,
- dodawanie questa,
- dodawanie puzzla,
- dodawanie NPC,
- dodawanie dzielnicy,
- dodawanie WorldEvent.

Przykładowo:

"Jak dodać nowy quest bez modyfikowania QuestManager.cpp".

# 72. EXTENSION GUIDES

Dodaj:

docs/ADDING\_QUEST.md

docs/ADDING\_PUZZLE.md

docs/ADDING\_ENEMY.md

docs/ADDING\_ITEM.md

docs/ADDING\_DISTRICT.md

docs/ADDING\_WORLD\_EVENT.md

Każdy dokument powinien pokazywać minimalny proces dodania nowej zawartości.

# 73. REGUŁA ROZWOJU

Przy każdej nowej implementacji zadawaj techniczne pytanie:

"Czy dodanie drugiego, dziesiątego i setnego elementu tego typu będzie wymagało zmiany istniejącego kodu?"

Jeżeli tak, rozważ przebudowę na rozwiązanie data-driven.

Przykład:

Dodanie nowego questa nie powinno wymagać modyfikowania QuestManager.

Dodanie nowej zagadki konkretnego istniejącego typu nie powinno wymagać modyfikowania PuzzleManager.

Dodanie nowego przedmiotu nie powinno wymagać modyfikowania InventoryComponent.

Dodanie nowej dzielnicy nie powinno wymagać modyfikowania DistrictManager.

# 74. NIE OVERENGINEERUJ

Rozszerzalność nie oznacza tworzenia abstrakcji bez zastosowania.

Nie implementuj ogromnego frameworka dla funkcji, która obecnie posiada jedno proste zastosowanie.

Stosuj zasadę:

prosta implementacja

-

jasne granice modułu

-

możliwość rozszerzenia.

# 75. PIERWSZY CEL PRODUKCYJNY

Najpierw doprowadź do działającego open-world vertical slice.

Gracz:

uruchamia grę

→ rozpoczyna kampanię

→ budzi się w mieszkaniu

→ rozwiązuje serię zagadek

→ wydostaje się z budynku

→ trafia do otwartego fragmentu Wrocławia

→ może swobodnie eksplorować

→ odkrywa questy poboczne

→ znajduje dowody

→ spotyka dynamiczne wydarzenia

→ unika patroli

→ zostaje wykryty

→ rozpoczyna się pościg

→ ukrywa się

→ kontynuuje śledztwo

→ dociera do kryjówki

→ odblokowuje następny etap kampanii.

Po wydostaniu się z pierwszego mieszkania gra nie może już działać jak liniowy korytarz.

Gracz otrzymuje ograniczony, ale rzeczywisty fragment open world.

# 76. DALSZA ROZBUDOWA

Architektura ma umożliwić w przyszłości dodanie bez dużej przebudowy:

- kolejnych dzielnic Wrocławia,
- pojazdów,
- ruchu drogowego,
- tramwajów,
- większej liczby NPC,
- większej liczby antagonistów,
- różnych frakcji,
- reputacji,
- zaawansowanej pogody,
- pełnego dnia/nocy,
- kolejnych kryjówek,
- nowych typów zagadek,
- kilkudziesięciu questów,
- dużej kampanii,
- dodatkowych zakończeń.

# 77. WARUNEK KOŃCOWY

Nie traktuj open world jako celu samego w sobie.

WroclawTheGame ma być przede wszystkim:

THRILLEREM

-

ESCAPE ROOMEM

-

GRĄ ŚLEDCZĄ

-

GRĄ AKCJI

umieszczoną w otwartym świecie Wrocławia.

Świat ma służyć gameplayowi.

Nie buduj ogromnej pustej mapy.

Lepiej stworzyć mniejszy, gęsty i interaktywny fragment miasta niż wielki obszar bez zawartości.

# 78. INSTRUKCJA DLA CODEX

Nie kończ na analizie.

Wykonuj zmiany w repozytorium.

Dla każdego etapu:

1. przeanalizuj aktualną implementację,
2. sprawdź zależności,
3. zaprojektuj rozszerzalne rozwiązanie,
4. zaimplementuj je,
5. skompiluj projekt,
6. napraw błędy,
7. wykonaj testy,
8. dodaj przykładowe dane/content,
9. sprawdź integrację,
10. zaktualizuj dokumentację.

Jeżeli istniejący kod utrudnia dalszą rozbudowę, wykonaj kontrolowaną refaktoryzację zamiast dokładania kolejnych wyjątków i hardcodowanej logiki.

Nie pozostawiaj pustych TODO zamiast wymaganej implementacji.

Nie twórz metod zwracających sztuczne dane wyłącznie po to, żeby kod się kompilował.

Nie przebudowuj działających elementów bez potrzeby.

Priorytety:

1. stabilność,
2. działający gameplay,
3. modularność,
4. rozszerzalność,
5. wydajność,
6. jakość grafiki.

WroclawTheGame ma być budowane jako projekt możliwy do rozwijania przez kolejne miesiące i lata, a nie jednorazowy prototyp.