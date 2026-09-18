# KayKit Character Animations 1.1 — Rig_Medium

Projekt vendoruje osiem darmowych bibliotek GLB z aktualnego pakietu KayKit Character
Animations 1.1 dla szkieletu `Rig_Medium`:

- General,
- MovementBasic,
- MovementAdvanced,
- CombatMelee,
- CombatRanged,
- Tools,
- Simulation,
- Special.

Autor: Kay Lousberg (KayKit). Licencja: CC0 1.0 Universal.

Oficjalna strona: https://kaylousberg.itch.io/kaykit-character-animations

Bajty są pobierane przez
`Scripts/fetch_kaykit_character_animations.py` z publicznego mirrora przypiętego do
commita `11d7df978c63b9e375707bd8d9431b4c8358cda8`. Każdy GLB ma zapisany oczekiwany
Git blob SHA-1 i jest odrzucany przy jakiejkolwiek zmianie bajtów.

Katalog `Data/free_kaykit_animation_catalog.json` jest generowany bezpośrednio z GLB.
Szczególnie istotna biblioteka Simulation zawiera `Lie_Down`, `Lie_Idle` i
`Lie_StandUp`, czyli zamyka jawny brak źródłowej animacji wstawania z dokumentu
odbiorowego.

KayKit korzysta z własnego `Rig_Medium`; użycie tych klipów na modelach Quaternius
wymaga retargetingu w Unreal Engine. Obecność źródła nie jest równoznaczna z zaliczonym
odbiorami retargetingu, IK ani clippingu.
