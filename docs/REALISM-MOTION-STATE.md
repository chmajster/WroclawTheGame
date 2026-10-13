# Realizm — punkt 3/10

## Wykonane
Dodano Character/CivilianAnimInstance.h/.cpp: istniejące klipy i blendspace,
stan Idle/Walk/Run/Sprint/Crouch/Jump, blend zmian klipu, korekta wzrostu/prędkości.
Proxy wykonuje Foot IK w przestrzeni komponentu, bez rozciągania nóg, ze
zmianą miednicy i dopasowaniem podeszew do normalnej podłoża. Trace świata
odbywa się na game thread; worker dostaje kopię danych. Pozostaje istniejący
leader pose ubrań. Retarget zachowuje ruch miednicy dla kucania.

## Do zrobienia
Zbudować kod i naprawić konkretne błędy API. Dodać autorski neutralny idle
w Blenderze i powtarzalny import do katalogu gracza. Zweryfikować chodzenie,
sprint, obrót, kucanie/skok, kontakt stóp i kamerę na Nadodrze_Playable.
Potwierdzić morphy ubrań przy zmianie sylwetki.

## Następny krok
Sprawdzić Saved/Realism/build-locomotion.log. Następnie dokończyć
Scripts/assets/build_civilian_idle.py i importer; ustawić AnimationClass
Male.Standard na /Script/WroclawTheGame.CivilianAnimInstance i sample 0
blendspace na neutralny idle. Uruchomić natywny smoke ruchu na prawdziwej mapie.

## Stan
W toku, draft. Punkty 1–2 scalone (#165, #166). Bez CI; bez ingerencji w PR #164.