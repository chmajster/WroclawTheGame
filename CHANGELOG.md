## 2026-09-18 — nowoczesne menu gry

- Przebudowano centrum gracza na skalowany layout 1600×900 z zachowaniem proporcji na niższych rozdzielczościach, ultrawide i 4K.
- Wprowadzono system wizualny „urban glass”: zaokrąglone karty, nową paletę, stany hover/pressed/disabled oraz wyraźniejszą hierarchię typografii.
- Przeprojektowano branding WROCŁAW / THE GAME / PRZEBUDZENIE, pasek kontekstu i stopkę.
- Ekran GRA rozróżnia aktywną i nieaktywną sesję, eksponuje właściwą akcję główną i porządkuje nową grę, zapis, profil oraz ustawienia.
- Podgląd postaci otrzymał nową oprawę, badge „Podgląd na żywo” i czytelne skróty sterowania.
- Panel profilu/statusu otrzymał kartę aktualnego celu i uporządkowane metryki sesji.
- Zmiana dotyczy runtime UMG/C++; końcowy odbiór wizualny i kompilacja w UE 5.8 nadal są wymagane.

## 2026-09-18 — okna uliczne sceny startowej i pełny zestaw Kenney

- Dodano 8 wariantów zwykłych modułów okiennych z Kenney Building Kit: prostokątne i łukowe, standardowe oraz detailed, w wersjach zwykłych i szerokich.
- Każdy model jest zapisany osobnym commitem, ma wpis w katalogu CC0 i manifest pipeline'u produkcyjnego.
- Domyślne mieszkanie startowe otrzymało dwa szerokie, detaliczne okna na ścianie od strony ulicy.
- Pełną ścianę od strony ulicy podzielono na nadproże, podokiennik i filary, dzięki czemu za modelami okien nie pozostaje pełna bryła blokująca otwór.
- Źródło Kenney jest przypięte do konkretnego commita publicznego mirrora; licencja Building Kit: CC0 1.0.
- Geometria i rozmieszczenie wymagają końcowego odbioru w UE 5.8: widok z wnętrza i ulicy, materiały, kolizja, LOD oraz screenshot QA.

## 2026-09-18 — oficjalne modele 3D budynków Wrocławia

- Dodano pobieranie i konwersję oficjalnych modeli budynków GUGiK/Geoportal z CityGML do układu GIS gry.
- Import rozszerzono z sześciu landmarków na wszystkie bezpiecznie dopasowane budynki w aktywnych sektorach miasta; sześć landmarków pozostaje zestawem hero-QA.
- Preferowana jest aktualna ogólnopolska warstwa LoD1-2024; starsza paczka TERYT 0264 jest fallbackiem.
- Dopasowanie GUGiK→OSM korzysta głównie z pokrycia footprintów, a nie wyłącznie dystansu centroidów.
- Bryły OSM zastępowanych budynków są wykluczane przed wypiekiem, a oficjalna geometria jest grupowana w komórki 128 m i trafia do `/Game/Generated/OfficialBuildings`.
- Pełny build włącza `OfficialBuildings`; runtime/visual QA w UE 5.8 nadal jest wymagane.

## 2026-09-18 — nowoczesna postać i ustawienia FPS

- Domyślny profil głównej postaci przebudowano na współczesny wariant miejski: atletyczna sylwetka, nowocześniejsza fryzura, zarost, subtelnie dopracowana twarz oraz spójny zestaw ubrań i dodatków.
- Podgląd postaci w menu i kreatorze dostał profil `Modern` z trzypunktowym oświetleniem key/fill/rim, dającym wyraźniejszą sylwetkę i separację od tła.
- Dodano zakładkę `USTAWIENIA` w centrum gracza z przełącznikiem licznika FPS oraz VSync.
- Dodano trwały limit klatek: bez limitu, 30, 60, 90, 120, 144, 165 i 240 FPS. Wybrana wartość jest zapisywana w ustawieniach użytkownika i stosowana przy uruchomieniu gry.
- Licznik FPS jest rysowany przez HUD jako wygładzony odczyt i może działać także w menu pauzy.
- Istniejące zapisane wyglądy postaci nie są nadpisywane; nowy profil dotyczy wartości domyślnej i nowych kampanii.

## 2026-09-18 — kompletne powiązanie modeli runtime

- Model coverage obejmuje 115 audytowanych modeli: Poly Haven, Kenney, OpenGameArt, Quaternius i oryginalne fallbacki CC0.
- Wszystkie 15 pozycji inventory i 82 fizyczne akcje rozdziału mają binding modelu.
- Usunięto widoczne `/Engine/BasicShapes/Cube` z klas gameplayowych: propsy, drzwi, aktywności, kryjówki, NPC, przeciwnicy, populacja, gracz i pojazd.
- Player/NPC/guard korzystają z rigowanych baz Quaternius; Character Creator wiąże także dostępne włosy, zarost i brwi.
- Samochód GIS korzysta z body i czterech osobnych kół Kenney; ambient traffic i piesi mają własne modele.
- Generator kampanii umieszcza modele dla wszystkich fizycznych actionów, CCTV i siedmiu lamp ulicznych.
- Wnętrza GIS otrzymały bazowe umeblowanie CC0; geometryczna powłoka pomieszczeń nadal jest generowana proceduralnie.
- Dodano `Scripts/audit_model_coverage.py`; test źródłowy wymusza zero brakujących bindingów i zero cube-proxy w runtime.
- Odbiór w UE nadal wymaga sprawdzenia transformacji, LOD, kolizji, retargetingu i clippingu; obecność źródła nie jest dowodem finalnej jakości wizualnej.

## 2026-09-18 — pipeline produkcyjny Astra / Blender / Unreal

- Dodano manifestowy pipeline assetów: referencje/źródło 3D → Blender headless → LOD i kolizja → Unreal/PBR/Nanite → Automation → screenshot QA → visual review → raport.
- Dodano wznawialny orchestrator PowerShell zapisujący bieżący etap w `Saved/Pipeline/status` oraz odrzucający kolejne bramki po błędzie.
- Import Unreal sprawdza faktyczną liczbę LOD-ów i kolizję, ustawia Nanite oraz buduje materiał z BaseColor/Normal/ORM.
- Dodano jawny zapis PASS/FAIL kontroli wizualnej i końcowy raport QA; publikator Git/PR odmawia działania bez `done/PASS`.
- Build Windows i lokalny zestaw testów walidują produkcyjne manifesty; `AGENTS.md` wymaga tego pipeline'u dla nowych lub istotnie zmienianych assetów 3D.

## 2026-09-18 — domknięcie darmowych animacji KayKit

- Dodano cały darmowy KayKit Character Animations 1.1 dla `Rig_Medium`: 8 bibliotek GLB i 139 rzeczywistych klipów.
- Dodano `Lie_Down`, `Lie_Idle` i `Lie_StandUp`, zamykając jawny brak źródłowej animacji wstawania.
- Rozszerzono importer UE o katalog KayKit oraz semantyczne bindingi m.in. dla uników, crawl/sneak, lockpickingu, narzędzi i siadania.
- Wszystkie GLB są przypięte do konkretnego commita publicznego mirrora i sprawdzane przez Git blob SHA-1; licencja źródłowa: CC0 1.0.
- Retargeting KayKit `Rig_Medium` na docelowy szkielet postaci pozostaje bramką odbioru UE, a nie brakiem źródłowego assetu.

## 2026-09-18 — darmowe postacie i animacje CC0

- Dodano reprodukowalny pipeline dla Quaternius Universal Base Characters [Standard]: męska i żeńska baza, fryzury, zarost i brwi.
- Dodano Universal Animation Library 1/2 [Standard] w wariantach in-place i Root Motion; katalog klipów jest generowany bezpośrednio z GLB.
- Każdy plik źródłowy jest przypięty do konkretnego commita i weryfikowany przez Git blob SHA-1.
- Build Windows importuje modele oraz animacje przed walidacją Character Creator i zatrzymuje packaging przy błędzie importu.

## Player menu — 2026-09-18

- Dodano pełnoekranowe centrum gracza inspirowane układem lobby: górne zakładki, boczne akcje, centralny podgląd postaci i panel statusu.
- Zakładki Gra, Postać, Ekwipunek, Dziennik, Mapa i Statystyki korzystają z bieżącego stanu kampanii zamiast danych demonstracyjnych.
- Podgląd postaci renderuje aktualny zapis wyglądu w osobnej scenie i obsługuje obrót, zoom oraz kadry całej sylwetki, górnej części i twarzy.
- Menu przejmuje pauzę, kursor i fokus wejścia; ESC/ENTER wznawia rozgrywkę, a start nowej gry przechodzi do istniejącego wyboru postaci.
- Stary tekstowy panel pauzy pozostaje awaryjnie dostępny, lecz jest pomijany, gdy nowe menu działa.

## 0.3.1 — 2026-09-18 — realizm sceny startowej

- Zastąpiono blockoutowe bryły mebli sceny otwierającej modelami CC0/PBR: łóżko, sofa, biurko, krzesło, szafka, kuchenka, zabudowa kuchenna i drobne wyposażenie.
- Interaktywne rekwizyty korzystają z właściwych modeli zamiast sześcianów: telefon, ładowarka, kabel USB, gniazdko, rozdzielnia, bezpiecznik, książki, laptop i latarka.
- Generator mapy zachowuje materiały i skalę importowanych modeli; fallback do BasicShapes pozostaje tylko dla elementów bez dedykowanego modelu.
- Przebudowano światło mieszkania na cieplejsze źródła z miękkimi cieniami i osobnym światłem kuchennym.
- Dodano `AGENTS.md`: każda zmiana ma powstawać na gałęzi, przez PR do `main`, a następnie być scalana.
- Usunięto repozytoryjne workflowy GitHub Actions; agent nie ma ich ręcznie uruchamiać bez jawnego polecenia.

## Wave 1 — gameplay i ciągłość sektorów

- Sześć pomostów blockoutu z zachowaniem współrzędnych OSM i interpolacją wysokości przyczółków; spójność grafu i geometrii oraz limit nachylenia. Połączenia car/foot dla wszystkich sześciu sektorów.
- 32 stabilne aktywności: 4 zadania z trzema tropami, 4 sekrety i 12 jednorazowych zdarzeń; obserwacja z samochodu i skrytka wymagająca kucania.
- Trzy wnętrza Hub i kryjówka Borka, powiązane z BuildingID, klatki ze schodami i przejścia oczekujące na streaming oraz kolizję.
- Dziennik miasta, interakcje, własny wersjonowany zapis postępu i samochodu, zachowanie nieznanych ID oraz blokada nadpisania nieprawidłowego/nowszego zapisu.
- Osiem tras populacji wyznaczanych na grafie, stała pula 24 agentów i zatrzymywanie przed przeszkodami; dodatkowe źródło streamingu przed jadącym samochodem.
- Wyłączanie fizyki zaparkowanego pojazdu, przywracanie jego pozycji i brak kolizji tafli wody.
- Testy logiki i danych przechodzą; odbiór UE pozostaje wymagany. Status Playable nie jest przyznawany automatycznie.

## Wave 1 — city GIS foundation

- Import rzeczywistych danych południa Wrocławia i łącznika z Nadodrzem: 26 wycinków OSM, dwa kafle terenu, metadane/licencje/checksumy, build offline.
- Stały origin i rozszerzenie istniejącej mapy GIS przez `Build-Geography.ps1 -City`; dodatkowa kontrola World Partition przed pakowaniem.
- Katalog sektorów i dzielnic, ringi, gęstości contentu, palety materiałów, słownik ulic/adresów i wykrywanie wewnętrznych dziedzińców.
- `WroclawMapSubsystem`, `CityCoverageSubsystem`, importowany katalog UE, lista sektorów w telefonie i developerski overlay.
- Routing portalowy offline z kierunkami jazdy i raport osiągalności; wykluczenie niezweryfikowanych tuneli także przy layer=0.
- Raport gotowości blokujący Playable bez rzeczywistych odbiorów; testy routingu, integralności źródeł i bramek odbioru.
- Zakres: GISOnly. Brak kompilacji/playtestu UE, gotowych aktywności nowych dzielnic i integracji kampanii z GIS. Nie jest to ukończony Wave 1.

# Changelog

## 0.2.0 — 2026-09-17 (implementacja UE do walidacji)

- Rozszerzono zakres Przebudzenia do docelowych 45–90 minut: 17 etapów, 4 poboczne, 5 sekretów i 14 paneli/zagadek z wejściem gracza.
- Dodano katalog JSON z 83 stabilnymi zdarzeniami, zależnościami, wyborami, nagrodami i 40 wpisami śledztwa.
- Przebudowano zasilanie telefonu i łańcuch schowek → prąd → telefon → komputer → TARGET.
- Dodano piwnicę i trasę techniczną, sklep z dwoma wejściami, parking, opuszczony lokal, garaż i warsztat.
- Dodano warianty świateł, blokady paneli, podpowiedzi, dziennik questów, tablicę śledztwa, zakładki telefonu i lokalne osiągnięcia.
- Dodano wizjer z kamerą, rzucane przedmioty, latarkę, opatrunki, unik i ogłuszenie oraz trzy spotkania z jednym archetypem AI.
- Zapis v2 odtwarza zdarzenia, wariant, wybory, blokady, strażników i zużycie przedmiotów; v1 pozostaje odrębnym nieodczytywanym slotem.
- Rozszerzono testy logiki i walidację katalogu. Brak kompilacji i przejścia gry w Unreal — odbiór pozostaje otwarty.

## 0.1.0 — 2026-09-17

- Początkowy projekt UE5.6, mały prototyp mieszkania i pościgu, inventory, 13 celów, cztery checkpointy i skrypt Windows.
- Pierwsze testy stanu i generatorów assetów; implementacja nie była weryfikowana w silniku.
