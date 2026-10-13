# Realizm — punkt 3/10

## Wykonane
Dodano Character/CivilianAnimInstance.h/.cpp: istniejące klipy i blendspace,
stan Idle/Walk/Run/Sprint/Crouch/Jump, blend zmian klipu, korekta wzrostu/prędkości.
Proxy wykonuje Foot IK w przestrzeni komponentu, bez rozciągania nóg, ze
zmianą miednicy i dopasowaniem podeszew do normalnej podłoża. Trace świata
odbywa się na game thread; worker dostaje kopię danych. Pozostaje istniejący
leader pose ubrań. Retarget zachowuje ruch miednicy dla kucania.

Kompilacja instancji animacji zakończona powodzeniem. Autorski neutralny idle
(6 s, 30 kl./s) ma wersjonowane źródła Blender/FBX i SHA256; import przez
dotychczasowy manifest zakończył się PASS dla siedmiu siatek. Katalog wskazuje
natywną klasę, blendspace używa nowego idle. Budowa ciała deformuje też ubranie.
Dodano WTG.CivilianMotionSmoke: rzeczywiste wejście ruchu, obrót, sprint,
kucanie/skok, próbki stóp/kamery i zrzuty na mapie startowej bez zapisu zmian.
Pierwszy rzeczywisty test ujawnił zmianę skali kości przez FBX idle i błędne
jednostki przesunięcia miednicy w retargetingu. Poprawiono normalizację względem
bind pose; oryginalny import idle pozostaje osobnym źródłem dla powtarzalnego
retargetu. Kadencję wyznacza medianowa prędkość stopy w fazie podporu: walk
98,76 cm/s, jog 563,13 cm/s. Pochodny sprint używa dopasowanego cyklu run,
bo oryginalny sprint wymagał nieprawidłowego przyspieszenia 5,45×.
Foot IK blokuje kontakt poziomy podczas podporu, zwalnia stopę w fazie wymachu.
Smoke rozszerzono o kontrolę bounds, zapadnięcia stóp oraz zgodność wag morphów
w siedmiu częściach dla Heavy/Slim, po czym odtwarza pierwotny wygląd.

## Do zrobienia
Zbudować rozszerzony smoke i sprawdzić jego wyniki oraz rzeczywiste zrzuty.
Skorygować konkretne błędy deformacji/kontaktu stóp. Potwierdzić morphy ubrań.

## Następny krok
Sprawdzić Saved/Realism/motion-game-2.log oraz zrzuty po normalizacji.
Zbudować rozszerzony Tests/CivilianMotionSmoke.cpp; uruchomić grę na
/Game/Maps/Nadodrze_Playable z ExecCmds=WTG.CivilianMotionSmoke i RenderOffscreen.
Odczytać Saved/Realism/motion/motion.json i obejrzeć dziesięć prawdziwych PNG.

## Stan
W toku, draft. Punkty 1–2 scalone (#165, #166). Bez CI; bez ingerencji w PR #164.
