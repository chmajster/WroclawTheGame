# Realizm — punkt 2/10

PR: https://github.com/chmajster/WroclawTheGame/pull/166

## Wykonane
Siedem ważonych siatek: anatomia MakeHuman z oczami, dopasowana koszula i spodnie
z oficjalnej paczki, buty, maskowane włosy short02, brwi oraz własny plecak.
32 morphy ciała i twarzy; ubranie ma wewnętrzne powierzchnie i obszycia.
Źródłowy .blend, generator, metadane i 84 sprawdzone wpisy pochodzenia/CC0.
Manifest Pipeline/assets/player/civilian.asset.json korzysta z istniejącego
punktu wejścia Blendera i natywnego importera, rozszerzonych o skeletal_mesh.
Przestrzeń /Game/Wroclaw/Hero i osobny katalog wyglądu chronią istniejący
katalog kreatora. Części zależą od BodyPreset, bufor uwzględnia ciało.
Materiał oczu zachowuje własny shader; druga warstwa tej samej koszuli nie jest
rysowana pod koszulą w slocie Outerwear. Istniejąca kapsuła i systemy gry pozostają.

## Sprawdzenia i ograniczenia
Rzeczywisty Blender 5.2.2: PASS, 71724 trójkąty wszystkich siedmiu siatek.
Manifest i build C++: PASS. Natywny import UE 5.8.2: PASS, siedem siatek,
trzy LOD każdej, 32 morphy ciała, wspólny 65-kościowy szkielet, Physics Asset,
sześć osobnych retargetowanych klipów i blendspace. Redukcje 65%/35%, progi
0.20/0.065 i remap morphów. Naprawiono usuwanie prefiksu morphów przez importer
FBX oraz brak przypisania szkieletu. Źródłowe animacje Quaternius są zachowane.
Tworzenie szkieletu i Physics Asset używa narzędzi silnika bez dialogów.
Licencja danych graficznych MakeHuman jest osobna od licencji jego aplikacji;
nie skopiowano kodu MakeHuman. Pochodzenie istniejącego rigu/animacji Quaternius
jest w docs/FREE_CHARACTER_ANIMATION_ASSETS.md.

## Do zrobienia
Ocena pięciu rzeczywistych kadrów na mapie startowej, poprawa ujawnionych problemów
materiałów/dopasowania/deformacji. Powtórzenie importu i kontrola braku duplikatów.

## Następny krok
Sprawdzić Saved/Realism/hero-game.log, Saved/Realism/Hero/integration.json
i pięć PNG z WTG.CivilianHeroSmoke. Poprawić konkretny problem w generatorze,
importerze lub renderowaniu części i ponowić manifest. Foot IK i dalsza praca
animacji należą do punktu 3.

## Stan
W toku. Nie uruchamiano CI. Nie naruszono niezatwierdzonych zmian PR #164.