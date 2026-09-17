# Przebudzenie — droga odbiorowa

Dokument zawiera rozwiązania zagadek. To scenariusz testu, nie potwierdzenie wykonania playtestu.

1. Menu: N. Początek w mieszkaniu. Przeczytaj kartkę na łóżku przy pomocy E.
2. Obejdź ściankę działową przez przejście przy łóżku. Podejdź do biurka. E otwiera szufladę, kolejne E zabiera telefon.
3. Zabierz powerbank z kablem z kanapy. Kartka z PIN-em leży na komodzie w przeciwległej części mieszkania: **0417**. Kolejność znalezienia tych przedmiotów jest dowolna.
4. T, wpisz PIN, Enter. T ponownie otwiera wiadomość. Odczyt wiadomości jest osobnym celem.
5. Otwórz szafkę kuchenną E i kolejnym E zabierz bezpiecznik. Użyj rozdzielni przy wyjściu. Bezpiecznik zostaje zużyty; nie można go pobierać ponownie po przywróceniu zasilania.
6. Użyj lampy na biurku. Odczytaj **7319**.
7. E na szafce przy drzwiach, wpisz kod, Enter. Pierwszy checkpoint zagadki zostaje zapisany. Drugie E zabiera klucz.
8. Otwórz drzwi mieszkania. Przejdź przez próg: drugi checkpoint.
9. Zejdź 18 stopniami na parter, otwórz drzwi budynku, przejdź podwórko.
10. Wejdź na ulicę: trzeci checkpoint i aktywacja napastnika. Upewnij się, że rzeczywiście zauważył gracza — samo przejście ulicy nie zalicza uniknięcia napastnika.
11. Sprint pomaga zyskać dystans. Kieruj się do zaułka z szyldem lokalu. Zerwij kontakt za narożnikami/przeszkodami, przejdź w kucanie i przeczekaj przeszukiwanie ostatniej znanej pozycji. Alternatywnie pokonaj przeciwnika, korzystając z ataku i bloku. Nie próbuj tankować ataków bez staminy.
12. Gdy zagrożenie opadnie, otwórz drzwi lokalu i wejdź do środka. Trwa aktywny pościg? Drzwi i zakończenie misji nie powinny go ignorować.
13. Ekran zakończenia rozdziału, czwarty checkpoint. Po ponownym uruchomieniu L ma odtworzyć zakończony rozdział.

## Próby regresyjne

- Telefon/klucz nie mogą pojawiać się bez interakcji. Wpisanie kodu nie omija wymaganych wskazówek.
- Przedmioty można zbierać w innej kolejności. Podwójne kliknięcie nie dodaje duplikatów.
- Zapis po szafce: po wczytaniu szafka jest otwarta, ale trzeba jeszcze zabrać klucz.
- Zapis po mieszkaniu: klucz pozostaje w ekwipunku, drzwi są przechodnie.
- Zapis uliczny: gracz wraca przed spotkanie, napastnik jest aktywny i ma działającą nawigację.
- Śmierć podczas pościgu: Enter przywraca trzeci checkpoint z pełnym zdrowiem.
- Odczyt telefonu, pauza i ekwipunek zatrzymują rozgrywkę; nie wolno ginąć podczas wpisywania kodu.
- Błąd zapisu jest widoczny; UI nie może zapewniać o poprawnym zapisie po błędzie.
- Wyjście ze schronienia podczas ponownego pościgu nie może przedwcześnie kończyć rozdziału.
