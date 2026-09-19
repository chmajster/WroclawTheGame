# Weather gameplay and runtime layer

Weather is now represented by a native runtime component plus a versioned gameplay/VFX policy.

## Runtime component

`UWTGWeatherRuntimeComponent` supports Clear, Cloudy, Rain, Fog and Storm presets with eased transitions. It exposes traction, visibility, AI sight, crowd and traffic multipliers while publishing global material parameters for wetness, cloud coverage, precipitation, fog, wind and lightning.

This lets road materials, puddle masks, cloud/rain materials and environment effects share one state instead of each system inventing weather independently.

## Persistence and simulation

The policy stores current weather, target weather and transition alpha so save/load can resume an in-progress transition. The Python layer provides the same interpolation contract for offline generation/tests and city simulation.

Weather-specific quests/events can consume the stable weather ID while vehicle, AI, crowd and traffic systems use the numeric multipliers.
