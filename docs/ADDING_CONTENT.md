# Dodawanie zawartości

Po każdej zmianie danych uruchom `compile_chapter.py`, `compile_world.py`, `compile_tags.py` i testy. Następnie ponownie przygotuj content w UE. Nie zmieniaj istniejących ID w wydanym zapisie bez migracji.

## Quest i przedmiot

Dodaj przedmiot do `items` z typem, limitem stosu i tagami. W akcji użyj `item`, `consume`, `needs`; warunki i nagrody należą do danych. Quest w `quests` lub `side_quests` potrzebuje ID, tytułu, prerequisites i celów. Wbudowane evaluatory obejmują zdarzenie, posiadanie przedmiotu, dowód oraz licznik. Cele FollowNPC/SurviveTime potrzebują rzeczywistego producenta, który zapisuje licznik; samo wpisanie etykiety typu nie wystarczy. Nie przyznawaj `Campaign.Main.Completed` za koniec rozdziału 1.

## Zagadka

Dodaj akcję z istniejącym `kind`, odpowiedzią, referencjami wskazówek i trzema poziomami podpowiedzi. Uzupełnij `position`, `size`, prezentację, wymagane tagi i nagrody. Panele blokują błędne próby we wspólnym modelu. Nowy rodzaj panelu wymaga nowego renderowania/obsługi inputu w UI i testu modelu; Data Asset nie wygeneruje go automatycznie.

## Przeciwnik

Dodaj profil do `guards`, stabilne ID, patrol, wymagania aktywacji, frakcję i parametry ruchu. Generator edytora umieszcza `SliceEnemy` z ID. Nie rozszerzaj listy ifów w GameMode. Nowa broń/archetyp wymaga własnej implementacji zachowania. Aktualna konfiguracja wzroku/słuchu wymaga dalszego przeniesienia wszystkich parametrów z konstruktora AI do profilu.

## Dzielnica i zdarzenie

Dzielnice blockoutu używają bounds w centymetrach. Produkcyjne dzielnice GIS powinny korzystać z importowanych granic i referencji geograficznych; obecny schemat katalogu nie przełącza ich automatycznie. Dodaj odkrywalne lokacje, stan dostępności i punkty odrodzenia dopiero po odbiorze podłoża/nawigacji.

Zdarzenie wymaga ID, miejsca, przedziału czasu, pogody, Heat, cooldownu, szansy i ewentualnej akcji. Wybór jest lokalny oraz ograniczony stanem gracza. Nowy efekt wydarzenia musi mieć odbiorcę event bus; tekst powiadomienia nie zastępuje fizycznego zdarzenia.

## Pojazd i próba drogowa

`VehicleDefinition` ustala masę, przyspieszenie, hamowanie, prędkości i przyczepność. Nie zwiększaj prędkości bez testu streamingu i hamowania. Wysiadać wolno dopiero po zatrzymaniu i przy wolnym miejscu na kapsułę.

`make_routes.py` wyznacza trasy na rzeczywistym grafie. Zdefiniuj punkty docelowe, oblicz legalną trasę, zachowaj `source_nodes` i wykonaj test wszystkich kolejnych krawędzi. `RaceDefinition` przechowuje checkpointy, transform startu, limit czasu i wariant dostawy. Ręczne narysowanie skrótu przez budynek nie jest dopuszczalną trasą. Pościg i śledzenie pojazdu oczekują implementacji AI; nie oznaczaj zwykłej próby czasu jako tych trybów.
