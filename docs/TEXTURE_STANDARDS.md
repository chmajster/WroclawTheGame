# Standard powierzchni WroclawTheGame

Cel artystyczny: współczesny, używany Wrocław; neutralne PBR, bez wypalonego światła,
plastikowej szorstkości i nadmiernego nasycenia. **Zestaw łączy sześć materiałów PBR
2K z Poly Haven (CC0) z autorskim starterem proceduralnym; nie jest ukończonym
pass'em fotorealistycznej geometrii.** Źródła: [SURFACE_ASSET_CREDITS.md](SURFACE_ASSET_CREDITS.md).
Materiały nie zastępują geometrii fasad, prawdziwych twarzy, groomów czy UV.

## Gęstość i rozdzielczości

Jednostka świata: 1 cm. Gęstość bazowa mierzona na powierzchni w najwyższym mipie;
nie mylić z rozdzielczością całego obiektu. Materiał kafelkowany na 2 m z mapą 1024
ma 512 texeli/m niezależnie od wielkości budynku. Tynk nie otrzymuje UV całej fasady
rozciągniętego od 0 do 1. Osobny detail normal dostarcza detal z bliska.

| Klasa | Bazowe texele/m | Typowe mapy | Limit |
|---|---:|---|---|
| HERO | 1024 | 2K–4K | 4K; 8K wyłącznie po pomiarze screen size/VRAM |
| HIGH | 512 | 2K–4K | 4K |
| STANDARD | 512 | 1K–2K | 2K |
| DISTANT | 128 | 512–1K / atlas | 1K |

Twarz/dłonie/napisy mogą otrzymać większą gęstość lokalnie. Wyjątek 8K wymaga
udokumentowanej różnicy w docelowej rozdzielczości, poprawnych UV i testu streamingu.
Nie skalować mapy 1K do 4K bez nowych danych. Pomiary UV robić checkerem 1 m,
w widokach Required Texture Resolution oraz Material Texture Scale Accuracy.

## Kontrakt plików i importu

- `T_*_BC`: sRGB, wyłącznie albedo; żadnych cieni, AO ani refleksów.
- `T_*_N`: liniowy DirectX, TC_NORMALMAP / BC5; relief w cm, normalizowane wektory.
- `T_*_ARM`: liniowy TC_MASKS; R=AO, G=roughness, B=metallic.
- Wszystkie mapy mają mipy SimpleAverage, repeat, streaming i limit 2K dla startera.
- Metal=1 tylko dla odsłoniętego metalu; farba, rdza, szkło, skóra i asfalt=0.
- Roughness nie jest stały. Parametr skaluje mapę, nie usuwa jej zmienności.
- Normal nie jest generowany z jasności albedo. Brak displacement w runtime;
  większe uskoki i krawężniki wymagają geometrii.

Proceduralny fallback Brick: kafel 200 cm, 8 cegieł w poziomie i 24 rzędy (moduł ok. 25×8,3 cm).
Cobble: moduł ok. 16,7 cm. Relief tynku 0,025 cm, betonu 0,06 cm, asfaltu 0,12 cm.
To ustawienia startowe, a nie deklaracja wymiarów każdego historycznego budynku.
Sześć zestawów Poly Haven używa wymiarów z `Data/surface_sources.json` (170–300 cm)
i map 2K; pozostałe profile mają mapy 1K. Nie mieszamy ich skali ze skalą fallbacku.
Drewno ma kierunkowe włókna; na obiektach wymagających konkretnego przebiegu słojów
użyj `UseUV=1` i poprawnych UV z oddzielnym masterem tangent-space, nie projekcji świata.

## Mastery i instancje

`/Game/SurfaceQuality/Masters/`: Surface, Building, Road, Glass, Metal, Wood, Ground,
CharacterClothing, Skin, Water oraz Decal. Wspólna implementacja shaderów znajduje się w
`Scripts/surface_shader.ush`; importer buduje grafy. Warianty to `MI_*`, nie kopie shaderów.
Zmian instancji nie resetuje ponowne przygotowanie (wyjątek: generowane instancje decali).
Nie edytować ręcznie generowanych masterów: zmiany źródła są autorytatywne.

Parametry: TileSizeCm, NormalStrength, DetailStrength/Tiling/FadeCm, ColorVariation,
RoughnessVariation, DirtAmount/HeightCm, GroundLevelCm, WearAmount, Wetness,
WeatherExposure, RainStreakAmount, OxidationAmount. Zakresy maskowania są ograniczone.
Makrovariation jest subtelna i nie zastępuje brudu wynikającego z użytkowania.

Środowisko używa projekcji trójosiowej w świecie, bez rozciągania na pionowych ścianach
GIS. Normal wyjściowy jest world-space. Tkaniny i skóra używają UV/tangent-space,
żeby detal nie przesuwał się po animowanej postaci. Szwy i fałdy muszą pochodzić z
autorskiej geometrii/normal; weave nie udaje ich obecności. Denim ma osobny splot.

Vertex color (biały oznacza brak autorskiego zużycia): R obniżany dla osadu,
G dla dotykanych krawędzi/uszkodzonej farby, B dla osłonięcia przed wodą, A dla odpływu.
RainStreakMask jest logicznie ograniczony do pomalowanych odpływów; nie maluje
losowych zacieków na całej fasadzie. GroundLevelCm ustawia lokalny poziom gruntu;
domyślne 0 nie wystarcza dla budynków na skarpie.

Rdza aktywna tylko przy `OxidationAmount>0` i autorskiej masce uszkodzeń G; obniża
metallic. Globalne moknięcie dotyczy domyślnie Road/Ground; nie zalewa wnętrz.
Wąskie przejścia pod zadaszeniem wymagają B=0 lub osobnej instancji WeatherExposure=0.
Nie ma symulacji przepływu wody ani automatycznego wykrywania zadaszeń.

Szkło jest niemetaliczne, translucent, Surface Forward Shading z mapą roughness.
Wymaga geometrii otworu/wnętrza; nie usuwa ściany za oknem. Nie ma jeszcze gotowego
interior mapping, wariantów firan ani finalnej rogówki bohatera. Skóra ma profil
Subsurface jako materiał startowy, nie finalny shader MetaHuman.

## Integracja i decale

`ASurfaceQualityDirector` działa w obu trybach gry. Podmienia wyłącznie znane materiały
z `/Game/Generated/M_*` i własne `MI_*`; nie rusza obcych assetów ani zapisu kampanii.
Obsługuje start, spawn aktorów i doczytanie leveli. Jeden MID na profil na świat.
Nowe bake'i map wskazują instancje bezpośrednio. World Partition nie jest przebudowywany
tylko z powodu zmian materiału. Regeneracja map pozostaje oddzielną operacją.

RoadPatchFresh/Old/Crack/Seal są projekcjami DBuffer, bez emissive. Rozmieszczenie ma
stały seed wynikający z komponentu, kontrolę powierzchni trafienia i limit 128 aktywnych
decali. Brak wpływu na kolizję/nawigację. RainStreak i RoadPaintWorn są dostępne do
świadomego rozmieszczenia; `PaintWearAmount` steruje ubytkiem farby.
Nie generujemy graffiti bez źródła ani nie niszczymy landmarków losowym noise.

## Oświetlenie i pamięć

Lumen GI/reflections i Virtual Shadow Maps pozostają aktywne. Bazowy dzień: 25 000 lx,
pochmurno/deszcz 30%; noc 0,2 lx. Skylight z atmosfery, ekspozycja histogramowa
w zakresie EV100 0–16, adaptacja góra 3/dół 1, bloom 0,15 i lekko stonowana saturacja.
Lokalne volume'y mają prawo nadpisać globalny baseline. Studio kreatora zachowuje
własną ekspozycję. Żarówki i wnętrza wymagają osobnego odbioru przy tych ustawieniach.

Streaming aktywny; pula ograniczana do VRAM, bez wymuszania uniwersalnej puli 8 GB.
Anizotropia 8×, brak NeverStream. Trzy mapy 1K to orientacyjnie 2,7 MiB z mipami przy
BC1+BC5+BC1 (dokładny format/platforma zmienia wynik). Nie włączamy SVT bez pomiaru:
obecny starter nie uzasadnia dodatkowego kosztu i odmiennego samplera custom shaderów.
Migracja do SVT wymaga zmiany masterów, weryfikacji cache i testu na docelowym GPU.

Master środowiska ma 12 odczytów tekstur (3 triplanar × BC/ARM/N/detail). UV skin/cloth
ma 4. Detail ma dystansowe wygaszanie amplitudy, nie odczytów. Wyższe klasy jakości
nie powinny bez pomiaru dodawać texture bombing/parallax do każdego budynku.

## Budowanie i odbiór

`make_surface_assets.py` tworzy oryginalne źródła TGA i manifest SHA-256 w Saved.
`prepare_surface_quality.py` uruchamiany w edytorze tworzy assety bez przebudowy map.
`WTG_SURFACE_REIMPORT=1` jawnie reimportuje tekstury; `WTG_SURFACE_RESET=1` resetuje
parametry instancji. Assety użytkownika poza własnym katalogiem nie są nadpisywane.
Build-Windows wymaga markera przygotowania przed packagingiem.

Testy CPU: `python -m unittest discover -s Tests -p test_surface_quality.py`.
Sprawdzić: deterministyczność, zakresy PBR, kanały, normalizację, ciągłość noise,
checksumy, limity rozdzielczości i zgodność starych ID. Oddzielnie wymagane są
kompilacja shaderów SM6, zrzuty w grze, stat streaming/RHI/GPU, przejście przez
granice WP, dzień/noc/deszcz, wnętrze/ulica i test migotania w ruchu.

Ograniczenia odbioru: proste bryły budynków i postaci nie stają się fotorealistyczne.
Indywidualne fasady Rynku, gzymsy, ramy/parapety, krawężniki, autorskie pęknięcia,
skany materiałów, oczy, włosy, finalne ubrania oraz budżet GPU wymagają dalszej pracy.
Załączone zlecenie urwało się w punkcie 49 dotyczącym pojazdów.

Źródła techniczne: [Epic — PBR](https://dev.epicgames.com/documentation/unreal-engine/physically-based-materials-in-unreal-engine),
[Epic — weryfikacja streamingu](https://dev.epicgames.com/documentation/unreal-engine/building-texture-streaming-data-in-unreal-engine).
