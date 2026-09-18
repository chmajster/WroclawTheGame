## Runtime Asset QA — obowiązkowa bramka

Odbiór assetów 3D jest zautomatyzowany w `Scripts/Build-Windows.ps1` i `Scripts/Build-Geography.ps1`.

Przed packagingiem wymagane są rzeczywiste raporty z UE 5.8:

- [ ] `Saved/RuntimeAssetQA/model_quality.json` — PASS: bounds/skala, LOD, simple collision, materiały, skeleton i Physics Asset bazowych postaci.
- [ ] `Saved/RuntimeAssetQA/retarget.json` — PASS: IK Rig, chains, Full Body IK oraz retarget wszystkich semantic animations na męski i żeński target Quaternius.
- [ ] `Saved/RuntimeAssetQA/scene.json` — PASS: transformy i visual meshes kampanii.
- [ ] `Saved/RuntimeAssetQA/geography_scene.json` — PASS dla builda GIS.
- [ ] `Saved/RuntimeAssetQA/character_screenshots/capture.json` — PASS: sześć krytycznych póz 1920×1080, wariant męski i żeński.
- [ ] `Saved/RuntimeAssetQA/visual_review.json` — PASS po faktycznej inspekcji screenshotów pod kątem skali, deformacji, IK i clippingu.
- [ ] `Saved/RuntimeAssetQA/final.json` — PASS i aktualny fingerprint.
- [ ] `Saved/RuntimeAssetQAPass.ok` istnieje z bieżącego przebiegu.

Brak któregokolwiek raportu, FAIL albo nieaktualny visual review blokuje `BuildCookRun`.

Szczegóły: [RUNTIME_ASSET_QA.md](RUNTIME_ASSET_QA.md).

# Bramka odbioru 0.3 — niezaliczona

Żaden punkt wymagający działającego silnika lub Windows nie został uznany za wykonany. Testy modelu opisano w VALIDATION.md. Poniższa lista uzupełnia historyczny odbiór rozdziału.

- [ ] UHT/UBT UE 5.8 bez błędów; również Shipping.
- [ ] Assety i obie mapy zapisują się, a commandlety kończą bez błędów.
- [ ] Data Layers i World Partition rzeczywiście streamują aktorów; brak spadania przez niezaładowaną podłogę.
- [ ] HLOD, nawigacja, powrót AI do patrolu, wykrywanie z profili i lokalne alarmowanie sprawdzone w grze.
- [ ] Pełna pętla menu → mieszkanie → zagadki → ulica → pościg → bezpieczny punkt → koniec rozdziału w EXE.
- [ ] Zapis v3 na dysku, odczyt v2, zgon i restart, uszkodzony zapis, pełny inventory i brak uprawnień do zapisu.
- [ ] Pomiar czasu rozgrywki oraz 1080p: FPS, frame times, hitching, draw calls i pamięć na nazwanym sprzęcie.
- [ ] Misja umieszczona w rzeczywistych budynkach; migracja starych współrzędnych checkpointów.
- [ ] GIS: kontrola punktów odniesienia, footprintów, skrzyżowań, wysokości mostów, kolizji i przejść dla pieszych.
- [ ] Import PBF/GeoJSON/SHP, tiled Landscape, graf pasów i relacje zakazów skrętu.
- [ ] Jazda, stabilność zawieszenia, kolizje, wsiadanie/wysiadanie, kamera i sterowanie sprawdzone na Windows.
- [ ] Ruch AI, LOD symulacji ruchu, co najmniej dwa pojazdy ścigające, utrata kontaktu, przeszukiwanie i blokady drogowe.
- [ ] Pozostałe przykłady: ucieczka od dwóch aut, śledzenie pojazdu, trasa ze wskazówek; nagrody i dowody powiązane z kampanią.
- [ ] Zapis własności/uszkodzeń/pozycji pojazdów, spójny checkpoint kampanii podczas jazdy.
- [ ] Raster zdjęć telefonu, rzeczywista mapa GIS w telefonie i komplet wymaganych producentów typów celów.
- [ ] Akceptacja licencji/atrybucji danych w dystrybuowanym pakiecie i dołączenie źródłowej bazy ODbL.

PR pozostaje draft. `Nadodrze_GIS` jest laboratorium integracji, nie ukończonym rozdziałem w rzeczywistym mieście. Sześć scenariuszy jest zaimplementowanych w danych/logice, ale nie zalicza bramki odbioru bez kompilacji UE, playtestu i powiązania nagród z kampanią.

---

# Bramki odbioru v2 — OTWARTE

Implementacja 0.2 rozszerza zakres do 45–90 minut. Nie uznawać etapu za ukończony na podstawie samych klas, katalogu treści lub testów.

| Bramka | Wymagany dowód | Stan |
|---|---|---|
| UE5.8 Editor | UHT/UBT i uruchomienie edytora | Niewykonane |
| Content | Import materiałów/audio, mapa i poprawny navmesh całego obszaru | Niewykonane w UE |
| Windows x64 | Development/Shipping EXE uruchomione bez edytora | Niewykonane |
| Pełny rozdział | Menu → mieszkanie → jedna z dwóch dróg → sklep → paczka → zasadzka → garaż → warsztat → rozdział 2 | Niewykonane w grze |
| Alternatywy | Obie drogi i oba wejścia do sklepu bez zadań pobocznych | Logika przeszła; fizyczna mapa niesprawdzona |
| Czas 45–90 min | Pierwsze przejścia co najmniej 3 osób bez solucji | Nie zmierzono |
| Pościg 3–5 min | Pomiar rzeczywistej sekcji, korekta układu/tuningu po testach | Nie zmierzono |
| Panele i UI | Czytelne wskazówki, klawiatura, światła, alarm, lockout i przewijanie w 1080p | Logika przeszła; UI niesprawdzone |
| AI | Brak widzenia przez ściany, fizyczne drogi navmesh, słuch rzuconego obiektu, powrót | Niewykonane w silniku |
| Walka/ucieczka | Ogłuszenie lub bezpieczna ucieczka, brak obowiązku zabicia | Niewykonane w silniku |
| Checkpointy | Restart EXE z każdego punktu, stan drzwi/strażników/zużycia/wariantu | Model przeszedł; serializacja UE niesprawdzona |
| Dowody/osiągnięcia | Pełny zestaw, pomijanie pobocznych, profil zachowany po nowej grze | Logika przeszła; zapis profilu UE niesprawdzony |
| Wydajność/pamięć | GPU/CPU/RAM, frame-time, FPS, draw calls i Unreal Insights | Nie zmierzono |

W Development zbierz `stat unit`, `stat gpu`, `stat rhi`, `stat memory` i ślad Unreal Insights. Uwzględnij pierwszą generację nawigacji, wejścia do pomieszczeń, wszystkie trzy spotkania, śmierć i kilkukrotne wczytanie. Cel roboczy: 1080p/60 FPS na nazwanym komputerze. To cel, nie deklarowany wynik.

## Ograniczenia oprawy i implementacji

Geometria i postacie są bryłowymi proxy. Dokumenty, fotografie, monitoring i finał przekazują obecnie informacje głównie tekstowo; nie ma finalnych fotografii, nagranego materiału CCTV, dialogów głosowych ani filmowej sceny finałowej. Wizjer korzysta z rzeczywistej kamery korytarza. Audio jest syntezowane. Wstawanie jest interakcją rozpoczynającą zadanie, bez finalnej animacji. Źródłowa animacja CC0 wstawania (`Lie_StandUp`) jest dodana i zmapowana; retargeting na docelowy szkielet oraz odbiór jej odtwarzania w UE pozostają niewykonane.

AI używa natywnej maszyny stanów, bez Behavior Tree/Blackboard. Drugi rozdział jest odblokowany logicznie, ale nie ma jeszcze zawartości. Mapa nie używa World Partition ani level streamingu — dotyczy zamkniętego fragmentu miasta. Zgodność API UE, kolizji, rozmieszczenia interakcji oraz trudności wymaga kompilacji i przejścia w silniku. Nie dodano sztucznych pauz wydłużających czas do 45–90 minut.
