## 28. REALISTYCZNY ZAKRES PIERWSZEGO ETAPU PRODUKCJI

Pierwszy etap ma być zamkniętym, kompletnym vertical slice gry WroclawTheGame.

Nie buduj jeszcze całego Wrocławia ani pełnej kampanii.

Celem pierwszego etapu jest stworzenie grywalnego fragmentu o długości około 45–90 minut przy pierwszym przejściu.

Vertical slice ma pokazać wszystkie podstawowe filary gry:

- eksplorację,
- zagadki escape room,
- questy,
- zbieranie informacji,
- śledztwo,
- skradanie,
- AI przeciwników,
- pościgi,
- prostą walkę,
- alternatywne rozwiązania,
- wybory gracza,
- checkpointy,
- rozwój kampanii.

# 28.1. OBSZAR PIERWSZEGO ETAPU

Zbuduj ograniczony fragment miasta.

Obszar powinien zawierać:

- mieszkanie bohatera,
- klatkę schodową,
- piwnicę,
- strych lub pomieszczenie techniczne,
- podwórko kamienicy,
- niewielką ulicę,
- boczny zaułek,
- mały sklep,
- opuszczony lokal,
- garaż,
- fragment parkingu,
- punkt docelowy kampanii.

Nie twórz jeszcze pełnego open world.

Poszczególne obszary powinny być połączone tak, żeby gracz miał poczucie eksplorowania fragmentu prawdziwego miasta.

# 28.2. STRUKTURA PIERWSZEGO ROZDZIAŁU

Nazwa rozdziału:

ROZDZIAŁ 1 — PRZEBUDZENIE

Rozdział powinien składać się z kilku questów.

## QUEST 1 — PRZEBUDZENIE

Gracz budzi się w mieszkaniu.

Nie pamięta dokładnie, co wydarzyło się wcześniej.

Cele:

1. wstań,
2. rozejrzyj się po mieszkaniu,
3. sprawdź drzwi wejściowe,
4. znajdź telefon,
5. spróbuj go uruchomić,
6. znajdź źródło zasilania.

Telefon jest rozładowany.

Gracz musi znaleźć:

- ładowarkę,
- kabel,
- działające gniazdko.

Ale mieszkanie nie ma prądu.

To prowadzi do kolejnego questa.

## QUEST 2 — BRAK ZASILANIA

Cele:

1. sprawdź skrzynkę bezpieczników,
2. znajdź brakujący bezpiecznik,
3. sprawdź kuchnię,
4. sprawdź schowek,
5. znajdź bezpiecznik,
6. zamontuj go,
7. przywróć zasilanie.

Bezpiecznik może być ukryty w zamkniętym schowku.

Żeby otworzyć schowek, gracz musi znaleźć kod.

# 28.3. ZAGADKA — KOD DO SCHOWKA

Kod nie powinien leżeć bezpośrednio na kartce.

Gracz powinien znaleźć kilka wskazówek.

Przykład:

na lodówce znajduje się zdjęcie,

na zdjęciu data:

17.06.2024

obok znajduje się wiadomość:

"Nie zapomnij, że zmieniłem kod. Jak zawsze — dzień i miesiąc."

Prawidłowy kod:

1706

Dodaj możliwość wpisania kodu na panelu numerycznym.

Po trzech błędnych próbach:

- panel blokuje się na kilka sekund,
- pojawia się dźwięk ostrzegawczy.

# 28.4. QUEST 3 — WIADOMOŚĆ

Po odzyskaniu prądu gracz może naładować telefon.

Telefon uruchamia się.

Pojawia się wiadomość:

"NIE WYCHODŹ GŁÓWNYM WEJŚCIEM. ONI JUŻ TAM SĄ."

Gracz otrzymuje nowy cel:

Dowiedz się, kto wysłał wiadomość.

Telefon powinien umożliwiać:

- czytanie SMS,
- listę kontaktów,
- zdjęcia,
- notatki,
- historię połączeń.

# 28.5. ZAGADKA — ZABLOKOWANY TELEFON

Telefon wymaga PIN-u.

Kod można odkryć poprzez eksplorację mieszkania.

Przykładowe wskazówki:

- zdjęcie psa,
- data na odwrocie fotografii,
- kalendarz,
- przypomnienie zapisane na komputerze.

Nie podawaj rozwiązania bezpośrednio.

# 28.6. QUEST 4 — KOMPUTER

Telefon zawiera informację:

"Sprawdź komputer."

Komputer jest zabezpieczony hasłem.

Gracz musi znaleźć hasło.

Możliwe wskazówki:

- książka,
- notatnik,
- zdjęcie,
- wcześniejsza wiadomość.

Po zalogowaniu komputer pokazuje:

- e-mail,
- zaszyfrowany plik,
- zdjęcie,
- adres,
- fragment monitoringu.

# 28.7. ZAGADKA — ZASZYFROWANY PLIK

Plik wymaga czterocyfrowego kodu.

Kod jest ukryty w zdjęciu.

Na zdjęciu widoczny jest np.:

numer budynku + numer mieszkania.

Gracz musi połączyć informacje.

Po otwarciu pliku pojawia się:

- zdjęcie bohatera wykonane z ukrycia,
- lista jego codziennych tras,
- oznaczenie "TARGET".

To jest pierwszy poważny element fabularny.

# 28.8. QUEST 5 — KTOŚ JEST NA KLATCE

Gracz słyszy:

- kroki,
- rozmowę,
- uderzenie w drzwi sąsiada.

Pojawia się cel:

Nie daj się zauważyć.

Gracz może spojrzeć przez wizjer.

Na korytarzu pojawia się przeciwnik.

AI przeciwnika powinno:

- patrolować,
- nasłuchiwać,
- reagować na hałas,
- sprawdzać drzwi.

Gracz nie powinien jeszcze być zmuszony do walki.

# 28.9. QUEST 6 — ZNAJDŹ INNE WYJŚCIE

Główne drzwi są niebezpieczne.

Gracz musi znaleźć alternatywną drogę.

Możliwe opcje:

A. piwnica,

B. dach i druga klatka,

C. wyjście techniczne.

W pierwszym vertical slice wystarczy zaimplementować dwie realne drogi.

# 28.10. OPCJONALNY QUEST — SĄSIAD

Gracz słyszy wołanie o pomoc.

Quest jest opcjonalny.

Cele:

1. znajdź mieszkanie sąsiada,
2. otwórz drzwi,
3. sprawdź mieszkanie,
4. znajdź rannego sąsiada,
5. zdecyduj, czy mu pomóc.

Nagroda:

- dodatkowa informacja fabularna,
- klucz do piwnicy,
- przedmiot leczniczy.

Quest powinien pokazywać system questów pobocznych.

# 28.11. QUEST 7 — PIWNICA

Jeżeli gracz wybiera piwnicę:

musi odnaleźć drogę przez ciemny korytarz.

Potrzebna jest latarka.

Latarka nie ma baterii.

Gracz musi znaleźć baterie.

# 28.12. ZAGADKA — PIWNICA

W piwnicy znajduje się zamknięta krata.

Zamek posiada cztery symbole.

W kilku pomieszczeniach znajdują się wskazówki:

- graffiti,
- stary plan budynku,
- symbole na licznikach,
- notatka administratora.

Gracz musi ustalić właściwą kolejność symboli.

# 28.13. OPCJONALNA ZAGADKA — SEJF

W jednym z pomieszczeń znajduje się sejf.

Nie jest wymagany do ukończenia gry.

Rozwiązanie wymaga połączenia trzech wskazówek.

W sejfie znajduje się:

- gotówka,
- zdjęcie,
- dokument,
- dodatkowy dowód do systemu śledztwa.

Takie zagadki mają nagradzać dokładną eksplorację.

# 28.14. QUEST 8 — WYJDŹ Z BUDYNKU

Po rozwiązaniu zagadek gracz wydostaje się na podwórko.

Rozpoczyna się pierwsza sekcja zewnętrzna.

Cel:

Dotrzyj niezauważony do ulicy.

Na podwórku znajduje się przeciwnik.

Gracz może:

- przekraść się,
- odwrócić jego uwagę,
- rzucić przedmiot,
- schować się,
- zaatakować.

# 28.15. SYSTEM ODWRACANIA UWAGI

Dodaj możliwość podnoszenia prostych przedmiotów.

Przykłady:

- butelka,
- puszka,
- kamień.

Gracz może rzucić przedmiot.

AI słyszy dźwięk i idzie sprawdzić jego źródło.

# 28.16. QUEST 9 — PIERWSZY KONTAKT

Po dotarciu na ulicę gracz otrzymuje wiadomość:

"Idź do sklepu na końcu ulicy. Nikomu nie ufaj."

Gracz musi dotrzeć do lokalu.

Po drodze może eksplorować fragment ulicy.

# 28.17. QUEST POBOCZNY — ZAMKNIĘTY SAMOCHÓD

Na parkingu znajduje się samochód.

W środku widać:

- torbę,
- telefon,
- dokument.

Gracz może znaleźć sposób na otwarcie samochodu.

Quest nie jest wymagany.

Nagroda:

- dokument,
- dodatkowa wskazówka,
- przedmiot leczniczy.

# 28.18. QUEST POBOCZNY — TAJEMNICZY TELEFON

Gracz znajduje porzucony telefon.

Telefon wymaga kodu.

W pobliżu znajdują się wskazówki pozwalające go odblokować.

Po odblokowaniu:

gracz odkrywa zdjęcie własnej postaci wykonane kilka godzin wcześniej.

Informacja trafia do systemu śledztwa.

# 28.19. QUEST 10 — SKLEP

Drzwi sklepu są zamknięte.

Gracz musi dostać się do środka.

Możliwe rozwiązania:

- odnalezienie klucza,
- wejście od zaplecza,
- rozwiązanie zagadki z zamkiem.

Nie pozwalaj na zwykłe otwarcie drzwi przyciskiem.

# 28.20. ZAGADKA — ZAPLECZE SKLEPU

W sklepie znajduje się:

- terminal,
- monitoring,
- kasa,
- zaplecze.

Terminal wymaga kodu.

Kod można ustalić na podstawie:

- paragonu,
- grafiku pracowników,
- numeru magazynu.

Po zalogowaniu gracz uzyskuje dostęp do monitoringu.

# 28.21. QUEST 11 — MONITORING

Gracz ogląda zapis monitoringu.

Na nagraniu widzi:

- samochód przeciwników,
- dwóch ludzi obserwujących budynek,
- tajemniczą osobę zostawiającą paczkę.

Cel:

Znajdź paczkę.

# 28.22. QUEST 12 — PACZKA

Paczka znajduje się w opuszczonym lokalu.

Drzwi są zabezpieczone.

Wymagana jest kolejna zagadka.

# 28.23. ZAGADKA — PANEL ELEKTRONICZNY

Panel posiada:

- cztery przyciski,
- diody,
- sekwencję świetlną.

Gracz musi odtworzyć sekwencję.

Sekwencja powinna być losowana przy rozpoczęciu nowej gry z kilku przygotowanych wariantów.

# 28.24. ZAWARTOŚĆ PACZKI

W środku znajdują się:

- klucz,
- pendrive,
- zdjęcie,
- krótka wiadomość.

Wiadomość:

"Jeżeli to czytasz, wiedzą już, że żyjesz."

# 28.25. QUEST 13 — PENDRIVE

Pendrive można podłączyć do komputera w opuszczonym lokalu.

Zawiera zaszyfrowany katalog.

Gracz musi rozwiązać prostą zagadkę logiczną.

Przykład:

kilka plików posiada daty,

jedna wiadomość sugeruje:

"Zacznij od najstarszego."

Gracz musi ustalić sekwencję.

# 28.26. QUEST 14 — ZASADZKA

Po odczytaniu danych uruchamia się wydarzenie fabularne.

Przeciwnicy przyjeżdżają pod budynek.

Cel:

Uciekaj.

Rozpoczyna się pościg.

# 28.27. POŚCIG

Sekcja powinna trwać około 3–5 minut.

Gracz musi:

- wybiec z budynku,
- przebiec przez zaułek,
- przeskoczyć przeszkodę,
- przejść przez podwórko,
- zgubić przeciwnika,
- dotrzeć do garażu.

AI powinno znać ostatnią pozycję gracza, a nie jego dokładną lokalizację przez ściany.

# 28.28. QUEST 15 — GARAŻ

Drzwi garażu są zamknięte.

Gracz jest pod presją czasu.

Musi szybko rozwiązać zagadkę.

Przykład:

panel ma trzy przewody:

- czerwony,
- biały,
- niebieski.

Wcześniej znaleziony dokument zawiera wskazówkę:

"2 → 1 → 3"

Gracz musi aktywować je w odpowiedniej kolejności.

# 28.29. PIERWSZA WALKA

W garażu jeden przeciwnik odnajduje gracza.

Rozpoczyna się tutorial walki.

Dostępne:

- lekki atak,
- blok,
- unik,
- stamina,
- otrzymywanie obrażeń.

Gracz może:

- pokonać przeciwnika,
- ogłuszyć go,
- uciec.

Zabijanie przeciwnika nie powinno być obowiązkowe.

# 28.30. QUEST 16 — UCIECZKA

Po wydostaniu się z garażu gracz musi dotrzeć do bezpiecznego punktu.

Otrzymuje SMS:

"Stary warsztat. 300 metrów. Wejdź tyłem."

# 28.31. QUEST 17 — BEZPIECZNY PUNKT

Gracz dociera do warsztatu.

Drzwi są zamknięte.

Ostatnia zagadka rozdziału wymaga wykorzystania informacji zdobytych wcześniej.

Kod powinien być możliwy do ustalenia na podstawie danych znajdujących się w systemie śledztwa.

# 28.32. FINAŁ ROZDZIAŁU

Po wejściu do środka uruchom scenę fabularną.

Gracz znajduje:

- mapę Wrocławia,
- zdjęcia kilku osób,
- oznaczone punkty miasta,
- informacje o blokadach wyjazdowych.

Na mapie znajdują się zaznaczone:

- drogi wyjazdowe,
- dworzec,
- lotnisko,
- punkty kontrolowane przez przeciwników.

Gracz dowiaduje się:

NIE MOŻE JESZCZE OPUŚCIĆ WROCŁAWIA.

Żeby wydostać się z miasta, musi przejść kampanię.

Odblokuj:

ROZDZIAŁ 2.

# 28.33. LISTA ZAGADEK PIERWSZEGO ETAPU

Pierwszy vertical slice powinien zawierać około 10–15 zagadek.

Obowiązkowe:

1. kod do telefonu,
2. znalezienie brakującego bezpiecznika,
3. kod do schowka,
4. hasło do komputera,
5. szyfr pliku,
6. symbole w piwnicy,
7. wejście do sklepu,
8. kod terminala sklepu,
9. sekwencja elektroniczna,
10. szyfr pendrive,
11. awaryjne otwarcie garażu,
12. kod do bezpiecznego punktu.

Opcjonalne:

13. sejf w piwnicy,
14. telefon znaleziony na ulicy,
15. samochód na parkingu.

# 28.34. QUESTY

Pierwszy etap powinien zawierać około:

- 12–17 questów głównych i etapów głównej misji,
- 3–5 questów pobocznych,
- kilka ukrytych interakcji.

Questy poboczne nie mogą blokować kampanii.

Powinny dawać:

- informacje,
- dowody,
- przedmioty,
- alternatywne rozwiązania.

# 28.35. SYSTEM PODPOWIEDZI

Nie pokazuj od razu rozwiązania.

System powinien działać stopniowo.

Po określonym czasie gracz może otrzymać:

Poziom 1:
ogólną wskazówkę.

Poziom 2:
bardziej konkretną wskazówkę.

Poziom 3:
bardzo wyraźne naprowadzenie.

Nie pokazuj bezpośrednio kodu, jeżeli gracz sam może go wywnioskować.

# 28.36. RANDOMIZACJA ZAGADEK

Niektóre zagadki powinny mieć kilka możliwych wariantów.

Przy rozpoczęciu nowej gry można losować:

- kod,
- położenie klucza,
- sekwencję świateł,
- lokalizację jednego z przedmiotów.

Nie randomizuj wszystkiego.

Fabuła musi pozostać spójna.

# 28.37. SYSTEM ŚLEDZTWA

Już w pierwszym etapie dodaj prostą tablicę śledztwa.

Kategorie:

LUDZIE
MIEJSCA
DOWODY
WIADOMOŚCI

Przykładowe dowody:

- zdjęcie bohatera,
- plik TARGET,
- tajemniczy SMS,
- nagranie monitoringu,
- pendrive,
- mapa blokad miasta.

# 28.38. UKRYTE SEKRETY

Dodaj minimum 5 opcjonalnych sekretów.

Przykłady:

- ukryta notatka,
- zdjęcie,
- dokument,
- zamknięta szuflada,
- dodatkowe nagranie,
- wiadomość na komputerze.

Sekrety mogą później wpływać na osiągnięcia lub alternatywne elementy fabuły.

# 28.39. OSIĄGNIĘCIA PIERWSZEGO ETAPU

Przygotuj obsługę lokalnych achievementów.

Przykłady:

Pierwsze kroki
— rozpocznij kampanię.

Escape Artist
— opuść mieszkanie.

Bez śladu
— przejdź podwórko bez wykrycia.

Detektyw
— odnajdź wszystkie dowody rozdziału.

Pacyfista
— ukończ rozdział bez zabicia przeciwnika.

Szybkie myślenie
— rozwiąż zagadkę garażu podczas pościgu.

# 28.40. DEFINITION OF DONE

Vertical slice jest ukończony, gdy można wykonać pełną sekwencję:

MENU

→ PRZEBUDZENIE

→ EKSPLORACJA MIESZKANIA

→ TELEFON

→ PRĄD

→ PIERWSZE ZAGADKI

→ KOMPUTER

→ ODKRYCIE, ŻE BOHATER JEST CELEM

→ UNIKANIE PRZECIWNIKA

→ UCIECZKA Z KAMIENICY

→ PIWNICA / ALTERNATYWNA DROGA

→ PODWÓRKO

→ SKRADANIE

→ ULICA

→ QUESTY POBOCZNE

→ SKLEP

→ MONITORING

→ PACZKA

→ PENDRIVE

→ ZASADZKA

→ POŚCIG

→ GARAŻ

→ PIERWSZA WALKA

→ UCIECZKA

→ BEZPIECZNY PUNKT

→ ODKRYCIE MAPY BLOKAD

→ INFORMACJA, ŻE UCIECZKA Z WROCŁAWIA WYMAGA UKOŃCZENIA KAMPANII

→ ODBLOKOWANIE ROZDZIAŁU 2

Pierwszy etap nie jest ukończony, jeżeli istnieją tylko mechaniki techniczne.

Musi istnieć kompletna, grywalna, połączona fabularnie sekwencja z questami i zagadkami.

# 28.41. ELEMENTY POZA PIERWSZYM ETAPEM

Na tym etapie nadal NIE implementuj:

- pełnego Wrocławia,
- kilkudziesięciu pojazdów,
- rozbudowanej symulacji ruchu drogowego,
- multiplayera,
- pełnej ekonomii,
- dużego systemu policji,
- setek NPC,
- pełnego cyklu pogodowego,
- rozbudowanego craftingu,
- ogromnego arsenału,
- kilkunastu godzin kampanii.

Pierwszy etap ma być dopracowanym fragmentem gry, a nie wielkim, niedokończonym open worldem.

Docelowa pętla pierwszego etapu:

EKSPLORACJA
→ WSKAZÓWKA
→ ZAGADKA
→ QUEST
→ DOWÓD
→ NOWA INFORMACJA FABULARNA
→ SKRADANIE
→ ZAGROŻENIE
→ POŚCIG
→ WALKA LUB UCIECZKA
→ KOLEJNA ZAGADKA
→ BEZPIECZNY PUNKT
→ POSTĘP KAMPANII.