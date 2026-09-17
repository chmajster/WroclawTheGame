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
