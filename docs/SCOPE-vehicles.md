# 79. SYSTEM POJAZDÓW I WYŚCIGÓW

Rozbuduj WroclawTheGame o system pojazdów oraz wyścigów działający jako integralna część otwartego świata.

Wyścigi nie mogą być oderwaną minigrą.

Mają służyć:

- kampanii,
- poznawaniu miasta,
- zdobywaniu informacji,
- kontaktom z NPC,
- zdobywaniu wyposażenia,
- odblokowywaniu nowych obszarów,
- zdobywaniu pieniędzy,
- pościgom,
- ucieczkom przed antagonistami.

System ma być modularny i data-driven.

# 80. VEHICLE SYSTEM

Stwórz rozszerzalny VehicleSystem.

Pojazdy powinny obsługiwać:

- wsiadanie,
- wysiadanie,
- uruchamianie silnika,
- przyspieszanie,
- hamowanie,
- cofanie,
- skręcanie,
- hamulec ręczny,
- zmianę biegów automatycznych,
- światła,
- klakson,
- uszkodzenia,
- kolizje,
- paliwo jako opcjonalny system przyszłości.

W pierwszym etapie wystarczą samochody osobowe.

Architektura ma pozwalać później dodać:

- motocykle,
- rowery,
- skutery,
- dostawczaki,
- samochody terenowe.

# 81. VEHICLE DEFINITION

Pojazdy mają być konfigurowane przez VehicleDefinition.

Przykładowe parametry:

VehicleID

DisplayName

VehicleClass

MaxSpeed

Acceleration

Braking

Grip

Mass

SteeringResponse

Durability

EnginePower

TransmissionType

GameplayTags

Nie hardcoduj parametrów konkretnych samochodów.

# 82. VEHICLE DAMAGE

Dodaj VehicleDamageComponent.

Uszkodzenia mogą wpływać na:

- maksymalną prędkość,
- prowadzenie,
- przyspieszenie,
- stan silnika,
- możliwość dalszej jazdy.

Nie buduj od razu pełnego symulatora mechanicznego.

Na początek wystarczą stany:

Healthy

Damaged

Critical

Disabled.

# 83. WYŚCIGI OPEN WORLD

Wyścigi mają pojawiać się jako aktywności open world.

Nie wszystkie muszą być dostępne od początku.

Niektóre powinny wymagać:

- określonego etapu kampanii,
- poznania NPC,
- znalezienia miejsca,
- odpowiedniego pojazdu,
- ukończenia wcześniejszego wyścigu.

# 84. RACE MANAGER

Stwórz RaceManager.

RaceManager nie może zawierać logiki konkretnego wyścigu.

Wyścigi definiuj przez RaceDefinition.

RaceDefinition:

RaceID

RaceName

RaceType

StartLocation

Checkpoints

FinishLocation

RequiredVehicleTags

RequiredQuest

EntryRequirements

Reward

TimeLimit

OpponentProfiles

TrafficProfile

WeatherProfile

TimeOfDayProfile

FailureConditions.

# 85. TYPY WYŚCIGÓW

Przygotuj możliwość dodawania wielu typów.

Minimum:

## CHECKPOINT RACE

Klasyczny wyścig przez serię punktów kontrolnych.

## SPRINT

Start i meta bez okrążeń.

## CIRCUIT

Kilka okrążeń po przygotowanej trasie.

## TIME TRIAL

Gracz jedzie sam przeciwko czasowi.

## ESCAPE RACE

Nie chodzi tylko o osiągnięcie mety.

Gracz musi zgubić pościg.

## DELIVERY RACE

Gracz musi przewieźć przedmiot do miejsca docelowego przed końcem czasu.

## PURSUIT RACE

Gracz jest ścigany podczas przejazdu.

## NAVIGATION RACE

Trasa nie jest pokazana w całości.

Gracz otrzymuje wskazówki i musi znaleźć odpowiednią drogę.

## ENDURANCE

Dłuższa trasa wymagająca utrzymania samochodu w dobrym stanie.

# 86. WYŚCIGI FABULARNE

Wybrane wyścigi mają być częścią kampanii.

Przykład:

## QUEST — NOCNY KIEROWCA

Gracz potrzebuje informacji od lokalnego kierowcy.

NPC nie chce przekazać informacji za darmo.

Warunek:

wygraj wyścig.

Po zwycięstwie gracz otrzymuje:

- lokalizację,
- kontakt,
- informację o przeciwnikach,
- nowy quest.

Wyścig nie jest więc tylko aktywnością sportową.

Jest elementem śledztwa.

# 87. QUEST — OSTATNI KURS

Gracz musi dostarczyć tajemniczą paczkę do drugiej części miasta.

Po drodze przeciwnicy zaczynają go ścigać.

Gameplay:

START

→ jazda po mieście

→ zmiana trasy

→ blokada

→ alternatywna droga

→ pościg

→ uszkodzenie samochodu

→ dotarcie do mety.

# 88. QUEST — UCIEKAJ

Gracz zostaje zaatakowany podczas spotkania.

Musi dostać się do samochodu.

Następnie:

- ucieka ulicami,
- unika przeciwników,
- musi zgubić samochody ścigające,
- może wykorzystać boczne uliczki,
- może schować samochód.

Nie wystarczy po prostu przekroczyć linię mety.

Warunkiem jest:

PursuitState == Lost.

# 89. QUEST — CZARNY SAMOCHÓD

Gracz zauważa pojazd, który wcześniej pojawiał się na monitoringu.

Cel:

Śledź samochód.

Nie podjeżdżaj za blisko.

Nie zgub celu.

To ma być mission type:

VehicleTailObjective.

Parametry:

MinDistance

MaxDistance

DetectionLevel

LostTargetTime.

# 90. ŚLEDZENIE POJAZDU

AI kierowcy powinno móc zauważyć, że jest śledzone.

Jeżeli gracz:

- jedzie zbyt blisko,
- długo utrzymuje identyczną trasę,
- powoduje kolizje,

SuspicionLevel wzrasta.

Po przekroczeniu limitu:

NPC próbuje zgubić gracza.

# 91. WYŚCIG Z ZAGADKĄ

Niektóre wyścigi mogą łączyć prowadzenie z mechaniką escape room.

Przykład:

gracz otrzymuje tylko trzy wskazówki:

"Most"

"Stacja"

"Czerwone drzwi"

Musi podczas jazdy ustalić kolejne punkty trasy.

Nie pokazuj standardowej linii GPS.

# 92. WYŚCIG NA PODSTAWIE ZDJĘĆ

Gracz otrzymuje kilka zdjęć miejsc we Wrocławiu.

Każde zdjęcie wskazuje następny checkpoint.

Gracz musi rozpoznać lokalizację.

Mechanika może wykorzystywać system:

LocationDiscovery.

# 93. ROAD NETWORK

Stwórz RoadNetworkSystem.

Sieć drogowa powinna określać:

- możliwe drogi AI,
- skrzyżowania,
- pasy ruchu,
- kierunki jazdy,
- ograniczenia,
- miejsca parkingowe,
- punkty spawnów.

Nie zapisuj tras każdego NPC jako pojedynczej ręcznie przygotowanej spline.

# 94. TRAFFIC SYSTEM

Wprowadź podstawowy ruch uliczny.

W pierwszej wersji:

- niewielka liczba pojazdów,
- proste zachowania,
- zatrzymywanie na przeszkodzie,
- poruszanie po pasach.

Architektura ma pozwalać później dodać:

- sygnalizację,
- skrzyżowania,
- ustępowanie pierwszeństwa,
- zaawansowane AI.

# 95. TRAFFIC LOD

Nie symuluj pełnej fizyki samochodów znajdujących się daleko od gracza.

Traffic Simulation Level:

Full

Simplified

Virtual.

Full:
samochody blisko gracza.

Simplified:
dalsza okolica.

Virtual:
logiczna symulacja bez fizycznego Actor.

# 96. AI KIEROWCÓW

Dodaj DriverAI.

Profile:

Civilian

Aggressive

Racer

Enemy

Cautious.

Parametry:

Aggression

RiskTolerance

MaxSpeedFactor

OvertakeProbability

CollisionAvoidance.

# 97. AI WYŚCIGOWE

AI nie może oszukiwać poprzez teleportowanie.

Można zastosować ograniczony system catch-up, ale nie może być oczywisty.

Różni przeciwnicy mają mieć różne style:

- agresywny,
- techniczny,
- szybki,
- ostrożny,
- ryzykowny.

# 98. CHECKPOINT SYSTEM

Checkpointy wyścigowe powinny posiadać:

CheckpointID

Order

Radius

RequiredDirection

Optional

TimeBonus.

Nie twórz osobnego Blueprintu dla każdej trasy.

# 99. DYNAMICZNE TRASY

RaceDefinition może wybierać wariant trasy.

Przykład:

Race\_WroclawNight

RouteA

RouteB

RouteC.

Pozwala to ponownie rozgrywać aktywność bez identycznego przebiegu.

# 100. WYDARZENIA PODCZAS WYŚCIGU

Dodaj RaceEventSystem.

Możliwe wydarzenia:

- zamknięcie ulicy,
- przeciwnik dołącza do pościgu,
- zmiana celu,
- uszkodzony samochód,
- telefon od NPC,
- zmiana pogody,
- dodatkowy checkpoint.

Dla głównych questów wydarzenia mogą być przygotowane ręcznie.

# 101. POŚCIGI SAMOCHODOWE

Połącz VehicleSystem z istniejącym PursuitSystem.

Przeciwnicy mogą ścigać gracza pojazdami.

Stany:

Search

Locate

Follow

Chase

Intercept

LostTarget.

# 102. INTERCEPT AI

Część przeciwników nie musi jechać bezpośrednio za graczem.

System może próbować przewidzieć kierunek jazdy i wysłać samochód na inną drogę.

Nie korzystaj z wiedzy o dokładnym przyszłym położeniu gracza.

Bazuj na:

- aktualnym kierunku,
- RoadNetwork,
- ostatnich checkpointach,
- prawdopodobnej trasie.

# 103. ROADBLOCKS

Przy wysokim HeatLevel mogą pojawiać się blokady.

RoadBlockSystem może wybierać miejsca z:

RoadBlockSpawnPoints.

Gracz może:

- znaleźć inną drogę,
- zawrócić,
- ominąć blokadę.

# 104. VEHICLE HEAT

Jazda samochodem może wpływać na HeatLevel.

Przykładowe wydarzenia:

kolizje

→ niewielki wzrost zainteresowania.

wyścig fabularny

→ większy Heat.

ucieczka przed antagonistami

→ znaczący Heat.

# 105. POJAZDY GRACZA

Dodaj PlayerVehicleRegistry.

Gracz może odkrywać i zdobywać samochody.

VehicleOwnershipState:

Unknown

Discovered

Temporary

Owned.

Nie wszystkie pojazdy stojące na ulicy muszą być dostępne.

# 106. GARAŻ

Safehouse może później posiadać garaż.

Funkcje:

- przechowywanie pojazdów,
- wybór pojazdu,
- podstawowe naprawy,
- późniejsze modyfikacje.

# 107. ULEPSZENIA POJAZDÓW

Nie buduj rozbudowanego tuningu w pierwszej wersji.

Przygotuj jednak VehicleUpgradeSystem pod przyszłe:

- silnik,
- hamulce,
- opony,
- zawieszenie,
- trwałość.

Każde ulepszenie ma być data-driven.

# 108. NOCNE WYŚCIGI

Dodaj kilka aktywności dostępnych tylko nocą.

Przykład:

NOCNY SPRINT.

Start pojawia się pomiędzy:

22:00–04:00.

Dzięki temu TimeOfDaySystem ma realny wpływ na gameplay.

# 109. WYŚCIGI W DESZCZU

Pogoda może modyfikować parametry nawierzchni.

Deszcz:

- zmniejsza przyczepność,
- zwiększa drogę hamowania,
- zmienia zachowanie AI.

Nie implementuj osobnych modeli jazdy.

Stosuj multiplikatory powierzchni.

# 110. WYŚCIGI POBOCZNE

Dodaj minimum kilka przykładowych wyścigów do vertical slice.

### WYŚCIG 1 — NOCNY SPRINT

Krótki sprint przez otwartą część mapy.

Cel:
pierwsze miejsce.

### WYŚCIG 2 — PRÓBA CZASOWA

Gracz musi pokonać trasę w określonym czasie.

### WYŚCIG 3 — PRZESYŁKA

Dostarcz paczkę przed końcem czasu.

### WYŚCIG 4 — UCIECZKA

Zgub dwa samochody przeciwników.

### WYŚCIG 5 — ŚLEDZENIE

Śledź podejrzany pojazd.

### WYŚCIG 6 — BEZ MAPY

Gracz dostaje tylko wskazówki dotyczące kolejnych punktów.

# 111. DODATKOWE AKTYWNOŚCI SAMOCHODOWE

Poza wyścigami dodaj możliwość późniejszego tworzenia:

- dostaw,
- transportowania NPC,
- śledzenia samochodu,
- ucieczek,
- pościgów,
- przewozu dowodów,
- przejazdów na czas.

Wszystkie powinny korzystać z tego samego Vehicle Objective Framework.

# 112. VEHICLE OBJECTIVES

Dodaj typy objective:

EnterVehicle

ReachDestinationByVehicle

CompleteRace

LoseVehiclePursuit

FollowVehicle

DeliverCargo

KeepVehicleHealthAbove

ReachDestinationBeforeTime

DriveThroughCheckpoints.

# 113. INTEGRACJA ZE ŚLEDZTWEM

Wyścigi mogą odblokowywać dowody.

Przykład:

gracz wygrywa wyścig

→ poznaje nowego NPC

→ NPC przekazuje numer rejestracyjny

→ numer trafia do InvestigationSystem

→ pojawia się nowy trop.

# 114. INTEGRACJA Z KAMPANIĄ

Nie wszystkie wyścigi są obowiązkowe.

Kategorie:

Race.Main

Race.Side

Race.World

Race.Challenge.

Main:
element kampanii.

Side:
quest poboczny.

World:
aktywność świata.

Challenge:
powtarzalne wyzwanie.

# 115. SYSTEM REKORDÓW

Dla TimeTrial i wyścigów powtarzalnych zapisuj:

BestTime

BestPosition

Completed

Attempts.

Nie buduj online leaderboardów w pierwszym etapie.

# 116. RESTART WYŚCIGU

Gracz powinien mieć możliwość:

- restartu wyścigu,
- rezygnacji,
- ponownego rozpoczęcia.

Nie restartuj całej mapy.

Resetuj tylko stan konkretnej aktywności.

# 117. WYŚCIGI A OPEN WORLD

Wyścigi powinny wykorzystywać normalną mapę open world.

Nie twórz dla każdego wyścigu osobnego levelu.

Trasa powinna prowadzić przez istniejące ulice i obszary świata.

Dzięki temu inwestycja w mapę jednocześnie zwiększa:

- eksplorację,
- kampanię,
- pościgi,
- wyścigi,
- questy.

# 118. PRZYSZŁA ROZBUDOWA RACE SYSTEM

Architektura musi umożliwiać później dodanie:

- większej liczby typów wyścigów,
- motocykli,
- rankingów,
- serii zawodów,
- lig,
- specjalnych NPC kierowców,
- tuningowania samochodów,
- bardziej zaawansowanego ruchu drogowego.

Dodanie nowej RaceDefinition istniejącego typu nie może wymagać zmiany RaceManager.cpp.

# 119. KOLEJNY ETAP OPEN WORLD

Po ukończeniu podstawowego vertical slice rozbuduj świat w następującej kolejności:

1. samochód gracza,
2. podstawowy RoadNetwork,
3. podstawowy TrafficSystem,
4. Vehicle AI,
5. RaceManager,
6. CheckpointRace,
7. TimeTrial,
8. VehiclePursuit,
9. FollowVehicleObjective,
10. pierwsze questy samochodowe,
11. nocne wyścigi,
12. integracja RaceSystem z CampaignSystem,
13. integracja z HeatSystem,
14. dynamiczne blokady,
15. kolejne pojazdy.

# 120. DALSZE QUESTY OPEN WORLD

Oprócz wyścigów dodaj kolejne aktywności wykorzystujące istniejące systemy.

## OBSERWOWANY

Gracz odkrywa, że od kilku minut jedzie za nim ten sam samochód.

Musi ustalić:

- czy rzeczywiście jest śledzony,
- kto znajduje się w aucie,
- jak zgubić obserwatora.

## KURIER

Nieznany kontakt prosi o przewiezienie paczki.

Gracz nie wie, co znajduje się w środku.

Podczas misji może:

- dostarczyć ją,
- sprawdzić zawartość,
- przekazać innemu NPC.

Decyzja wpływa na WorldState.

## ZNIKNIĘCIE

Samochód powiązany ze śledztwem zostaje odnaleziony porzucony.

Gracz musi przeszukać:

- samochód,
- okolicę,
- pobliski budynek.

## BLOKADA

Główna droga zostaje zajęta przez przeciwników.

Gracz musi odnaleźć alternatywną trasę przez open world.

## TRANSPORT

Gracz musi przewieźć NPC do kryjówki.

NPC może zostać zauważony przez antagonistów.

## PUŁAPKA

Kontakt umawia gracza na spotkanie.

Na miejscu okazuje się, że jest to zasadzka.

Gracz musi wydostać się z obszaru pieszo lub samochodem.

# 121. ZASADA DOTYCZĄCA WYŚCIGÓW

WroclawTheGame nie ma zmieniać się w czystą grę wyścigową.

Samochody i wyścigi mają być kolejnym elementem sandboxu.

Podstawowa tożsamość pozostaje:

OPEN WORLD

-

THRILLER

-

ESCAPE ROOM

-

ŚLEDZTWO

-

SKRADANIE

-

WALKA

-

POŚCIGI

-

POJAZDY I WYŚCIGI

-

KAMPANIA FABULARNA.

Każdy większy system powinien współpracować z pozostałymi zamiast funkcjonować jako osobna minigra.