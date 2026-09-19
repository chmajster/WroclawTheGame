# CommonUI + Enhanced Input

CommonUI and Enhanced Input remain the execution layer. The repository now also has a versioned player input profile contract and a native `UWTGInputProfileSubsystem`.

## Player profile

The profile stores mouse/gamepad sensitivity, deadzone, UI scale, hold/toggle preferences, glyph family and keyboard/gamepad overrides. Rebind operations reject duplicate keys inside the same runtime profile.

`Scripts/input/build_input_profile.py` validates catalogue-based overrides, non-rebindable actions and context-local key conflicts before a profile is distributed or imported.

The action catalogue remains the authoritative list of actions/contexts. Binary InputAction/InputMappingContext assets may bind to these IDs without changing save/profile semantics.
