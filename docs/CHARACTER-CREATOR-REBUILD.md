# Przebudowa kreatora postaci

## 1. Audyt

Silnik UE 5.8.2, C++20, UMG/Slate (programowy WidgetTree), CommonUI jest
włączone, rendering D3D12/SM6/Lumen. Widoczny opis użytkownika obejmuje trzy
strumienie: audyt, interfejs/studio GTA IV, działającą personalizację. Dalszy
opis menu GTA V jest kontekstem integracji, zgodnie z odpowiedzią użytkownika.

`UI/CharacterCreatorWidget` tworzy formularze i techniczne polecenia.
`Character/CharacterCreator` renderuje osobnego aktora przez SceneCapture2D.
`CharacterCreatorSubsystem` normalizuje stabilne ID, losuje przez FRandomStream,
przechowuje szkic/undo/redo i serializuje CharacterCustomizationSaveData.
`CharacterAppearanceComponent` ładuje asynchronicznie siatki i części.
`SliceController::StartCreatedCampaign` zapisuje przed rozpoczęciem nowej
sesji, cofa zatwierdzenie po błędzie i stosuje wygląd do gracza. Garderoba,
zapis dzielnicy/miasta/kampanii i powrót do menu już istnieją.

### Rzeczywiste sprawdzenia bazowe

- Uruchomiono istniejący `WTG.CreatorSmoke` w grze 1280×720: otwarcie,
  losowanie/undo/redo, zgodność szkicu z podglądem i zamknięcie — exit 0.
- Obraz UI i sam target 720×1000 są całkowicie białe. Przyczyna leży przed
  kompozycją UMG. Manual exposure bez fizycznej ekspozycji oraz globalne
  efekty atmosfery/oświetlenia wymagają izolacji i kalibracji studia.
- Dwa zaimportowane modele Quaternius CC0 są pełnymi, rigowanymi ciałami,
  ale nie mają ani jednego morph targetu. Pole `placeholder` katalogu jest
  prawdziwe. 19 nominalnych morphów nie zmienia tych siatek.
- Wszystkie 11 wpisów ubrań nie ma siatki. Cztery wpisy zarostu wskazują
  ten sam model. Kolory twarzy wymagają rzeczywistych parametrów materiału.
- Render target ma stały pionowy format, mimo że UI go rozciąga. Obsługa
  obrotu/koła reaguje także poza podglądem; przebudowa musi ograniczyć ją
  do centrum. Zmiana każdej klatki suwaka tworzy osobny wpis historii.
- Narzędzia lokalne: Blender 5.2.2 LTS, UE 5.8.2, Python; dostępne rzeczywiście.

Dowody bazowe: `docs/qa/creator-rebuild/before-ui.png`, `before-capture.png`
oraz `baseline-assets.json`. Surowe logi pozostają w Saved.

### Decyzje

Zachować stabilne ID/zapis, przepływ menu i istniejące animacje. Kategorie
zostaną osobnym panelem, edycja będzie kontekstowa, a centrum otrzyma
skalowany target, izolowane studio, obrót/zoom i automatyczne kadrowanie.
Brakujące zdolności modelu mają być wykonane, nie udawane przez suwaki.
Licencja potencjalnej realistycznej bazy MakeHuman została sprawdzona:
rdzeniowe siatki, targets i skóry są CC0, kod narzędzia ma inną licencję.
Nie kopiujemy kodu MakeHuman ani zasobów GTA/The Sims.

Źródła licencji: [MakeHuman](https://static.makehumancommunity.org/about/license.html),
[LICENSE.md](https://github.com/makehumancommunity/makehuman/blob/master/LICENSE.md).

## Do zrobienia

2. Wdrożyć nowe UI, izolowane studio, interakcje i zweryfikować rzeczywisty render.
3. Wdrożyć siatki z morphami, dopasowany ubiór, materiały i personalizację,
   rozszerzyć historię/zapis, sprawdzić przepływ do gry i udokumentować wyniki.

Stan: audyt zakończony, implementacja kolejnych strumieni w toku.
