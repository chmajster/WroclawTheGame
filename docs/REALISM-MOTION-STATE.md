# Realizm — punkt 3/10

PR: https://github.com/chmajster/WroclawTheGame/pull/167

## Wykonane
Natywna UCivilianAnimInstance zachowuje dotychczasowe klipy/blendspace i leader
pose ubrania. Steruje stanami Idle/Walk/Run/Sprint/Crouch/Jump, przejściami
klipów i prędkością zależną od wzrostu. Zapytania świata są na game thread,
worker otrzymuje kopię danych. Foot IK koryguje miednicę, zgięcie nóg i normalną
podeszwy; nie rozciąga nóg i zwalnia stopę podczas wymachu. Faza podporu blokuje
poziome przesunięcie. Uwzględniono skalę komponentu oraz rodzica kości FBX.

Autorski cywilny idle (6 s, 30 kl./s) ma źródła Blender/FBX i SHA256. Import
natywnego źródła oraz osobnej animacji wynikowej normalizuje skalę i długości
bind pose. Retarget zachowuje ruch miednicy, normalizując jednostki klipów.
Kadencja używa zmierzonej medianowej prędkości stopy podczas podporu: walk
98,76 cm/s, jog 563,13 cm/s; rate 1,8226 / 0,6215. Pochodny sprint używa run
z rate 0,9412, bo oryginalny sprint wymagał nieprawidłowego przyspieszenia 5,45×.

Heavy/Slim deformują ciało, koszulę, spodnie, buty i plecak zgodnie (+0,25/-0,2).
Karty włosów/brwi pozostają na niezmienionej głowie; ich zerowe morphy masy
są pomijane przez importer. Obniżono punkt celowania kamery z 55 do 25 cm,
zachowując odległość 250 cm i kolizję spring arm.

Walidacja: kompilacja UE 5.8.2 PASS; ponowny import siedmiu siatek PASS,
39 natywnych artefaktów bez rozmnażania ścieżek. WTG.CivilianMotionSmoke na
Nadodrze_Playable PASS: 432 próbki, wszystkie sześć stanów, 23,46 m rzeczywistego
przemieszczenia, maksymalna korekta IK 17,29 cm, minimalna odległość kamera–głowa
195,15 cm. Kontrola morphów pięciu deformowanych powierzchni PASS. Trzynaście
rzeczywistych zrzutów i JSON: docs/qa/realism-2026-10-13/motion.
Po subtelnej korekcie palców ponownie uruchomiono WTG.CivilianHeroSmoke:
PASS, siedem widocznych siatek, pięć zrzutów final-idle obejrzanych bez błędu
skali/rozciągnięcia dłoni. Python py_compile i git diff --check PASS.

Ograniczenia weryfikacji: są to rzeczywiste testy gry i oglądane klatki,
nie deklaracja fotorealizmu ani pełnego QA wszystkich możliwych tras/wyglądów.
Nie ukrywano VSM; nawierzchnie, infrastruktura, zieleń i ostrzeżenie są zakresem
kolejnych punktów 4–8. Regresja całej misji/zapisu pozostaje w punkcie 10.

## Do zrobienia
Brak w zakresie punktu 3. Punkty 4–10 pozostają w zakresie całego zlecenia.

## Następny krok
Scalić kompletny PR #167 do main; z aktualnego main utworzyć oddzielną gałąź
punktu 4. Zidentyfikować rzeczywisty komponent poziomego obiektu (runtime
trace/inwentaryzacja), następnie poprawić Scripts/gis/build_meshes.py i
build_playable_district.py: wysokości, nakładanie powierzchni i podział sektorów.

## Stan
Punkt 3 ukończony. Punkty 1–2 scalone (#165, #166). Bez CI; bez zmian w PR #164.