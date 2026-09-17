# Bramki odbioru v2 — OTWARTE

Implementacja 0.2 rozszerza zakres do 45–90 minut. Nie uznawać etapu za ukończony na podstawie samych klas, katalogu treści lub testów.

| Bramka | Wymagany dowód | Stan |
|---|---|---|
| UE5.6 Editor | UHT/UBT i uruchomienie edytora | Niewykonane |
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

Geometria i postacie są bryłowymi proxy. Dokumenty, fotografie, monitoring i finał przekazują obecnie informacje głównie tekstowo; nie ma finalnych fotografii, nagranego materiału CCTV, dialogów głosowych ani filmowej sceny finałowej. Wizjer korzysta z rzeczywistej kamery korytarza. Audio jest syntezowane. Wstawanie jest interakcją rozpoczynającą zadanie, bez finalnej animacji.

AI używa natywnej maszyny stanów, bez Behavior Tree/Blackboard. Drugi rozdział jest odblokowany logicznie, ale nie ma jeszcze zawartości. Mapa nie używa World Partition ani level streamingu — dotyczy zamkniętego fragmentu miasta. Zgodność API UE, kolizji, rozmieszczenia interakcji oraz trudności wymaga kompilacji i przejścia w silniku. Nie dodano sztucznych pauz wydłużających czas do 45–90 minut.
