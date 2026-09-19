# Day/night environment

The day/night system now has both a simulation policy and a native runtime environment actor.

## Runtime actor

`AWTGDayNightEnvironment` owns movable Sun and Moon directional lights, SkyLight real-time capture, SkyAtmosphere, VolumetricCloud and ExponentialHeightFog components. The 24-hour clock can run automatically or be set from Blueprint.

The actor continuously derives daylight/night alpha, rotates Sun/Moon, changes realistic directional-light intensity/color, adjusts SkyLight and fog, and publishes global material parameters:
- `TimeOfDay01`
- `SunAlpha`
- `NightAlpha`
- `StreetLight`
- `WindowEmissive`

This makes street lamps, windows, signs and other emissive materials respond without per-object ticking.

## Simulation policy

City activity phases now interpolate during the configured transition window instead of jumping at phase boundaries. The same state exposes traffic, crowd and shop multipliers plus lighting values (sun/moon lux, SkyLight, fog, exposure and window emissive).

Season and weather systems can modify these values as separate layers without replacing the stable day/night clock.
