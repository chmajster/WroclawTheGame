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

## Do zrobienia
Zbudować nowy smoke i sprawdzić jego wyniki oraz rzeczywiste zrzuty.
Skorygować konkretne błędy deformacji/kontaktu stóp. Potwierdzić morphy ubrań.

## Następny krok
Sprawdzić Saved/Realism/build-motion-smoke.log, uruchomić grę na
/Game/Maps/Nadodrze_Playable z ExecCmds=WTG.CivilianMotionSmoke i RenderOffscreen.
Odczytać Saved/Realism/motion/motion.json i obejrzeć dziesięć prawdziwych PNG.

## Stan
W toku, draft. Punkty 1–2 scalone (#165, #166). Bez CI; bez ingerencji w PR #164.
