# Realizm — punkt 2/10

PR: https://github.com/chmajster/WroclawTheGame/pull/166

## Wykonane
Cywilny bohater na rzeczywistym graczu Nadodrze_Playable: anatomia MakeHuman
z oczami, ciemna koszula, szare spodnie, buty, krótkie włosy, brwi i plecak.
Siedem ważonych siatek, 71724 trójkąty, 32 rzeczywiste morphy ciała i twarzy,
trzy LOD (65%/35%, remap morphów), wspólny 65-kościowy szkielet i Physics Asset.
Ubranie ma wewnętrzne powierzchnie i obszycia; plecak panele, kieszeń, uchwyt,
szwy oraz paski związane z tułowiem. Wersjonowane źródła .blend, FBX pipeline,
tekstury i manifest. 84 wpisy pochodzenia z SHA256 i osobnymi decyzjami o użyciu
w grze / publicznym repo; szczegóły w docs/CIVILIAN-ASSET-CREDITS.md.

Osobny katalog /Game/Wroclaw/Hero/DA_CivilianAppearance jest wybierany przez
istniejący subsystem. BodyMeshes wiążą części z ciałem; materiał oczu zachowuje
własny shader. Nie rysujemy dwóch identycznych koszul. Oryginalny katalog,
architektura kreatora, kapsuła i źródłowe animacje Quaternius są zachowane.
Osobne pochodne sześciu klipów oraz blendspace mają retarget rotacji w przestrzeni
komponentu i zachowują długości kości celu. Generator poprawnie obejmuje końce
palców w konwersji A/T-pose i kontroluje anatomiczne długości paliczków.

Materiały mają wymagane usage flags, skórę z subsurface, maskowane włosy,
normalne ubrań/butów, subtelny splot plecaka i poprawne sRGB. Import sprawdza
każde połączenie shaderów zamiast ignorować odrzucone piny.

## Sprawdzenia i ograniczenia
Blender 5.2.2, manifest, build C++ i natywny import UE 5.8.2: PASS.
Rzeczywisty renderer D3D12/SM6: pięć ujęć gracza (przód, tył, twarz, ubranie,
plecak), 1280x720; siedem widocznych siatek, bez błędów kompilacji materiałów
i brakujących flags MorphTargets. Dowody: docs/qa/realism-2026-10-13/hero.
Powtórny import: 37 zasobów przed i po, zero nowych ścieżek, PASS.
Naprawiono błędy odkryte w rendererze: szare materiały domyślne, rozciągane
boki koszuli, palce i paski. Nie deklarujemy fotorealizmu.
Widoczny VSM marking queue overflow i geometria otoczenia pozostają pracą
punktów 4–8; Foot IK i naturalniejsza animacja spoczynku należą do punktu 3.

## Do zrobienia
Brak w zakresie punktu 2.

## Następny krok
Po scaleniu utworzyć z main gałąź punktu 3. W Character/CivilianAnimInstance
dodać runtime locomotion i Foot IK, autorski neutralny idle oraz sprawdzenie
chodu, sprintu, obrotu, kucania, skoku i kamery na Nadodrze_Playable.

## Stan
Ukończony punkt 2. Punkty 3–10 pozostają w zakresie całego zadania.
Nie uruchamiano CI. Nie naruszono niezatwierdzonych zmian PR #164.