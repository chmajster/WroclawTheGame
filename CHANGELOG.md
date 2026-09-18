## 2026-09-18 — darmowe postacie i animacje CC0

- Dodano reprodukowalny pipeline dla Quaternius Universal Base Characters [Standard]: męska i żeńska baza, fryzury, zarost i brwi.
- Dodano Universal Animation Library 1/2 [Standard] w wariantach in-place i Root Motion; katalog klipów jest generowany bezpośrednio z GLB.
- Każdy plik źródłowy jest przypięty do konkretnego commita i weryfikowany przez Git blob SHA-1.
- Build Windows importuje modele oraz animacje przed walidacją Character Creator i zatrzymuje packaging przy błędzie importu.

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
