# CommonUI + Enhanced Input

CommonUI is enabled alongside the existing Enhanced Input module. `Data/input_actions.json` is the versioned action/context contract for gameplay, vehicle and UI controls.

## Remaining
1. create actual InputAction/InputMappingContext assets in UE 5.8 from the catalog;
2. route PlayerMenu/phone/pause through CommonUI activation stacks;
3. implement persistent rebinding and conflict detection;
4. keyboard/mouse/gamepad glyphs and focus navigation;
5. accessibility: hold/toggle, sensitivity, deadzone, UI scale;
6. verify seamless gameplay↔vehicle↔UI context switching in packaged Windows build.
