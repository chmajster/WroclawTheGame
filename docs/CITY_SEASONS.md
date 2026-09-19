# City seasons

Season state now drives a persistent campaign calendar layer instead of exposing only four static labels. Values remain gameplay/art-direction parameters, not weather forecasts.

## Seasonal runtime state

For a game date the system produces:
- current season and smooth transition to the next season;
- foliage density plus foliage/material/PCG variants;
- daylight hours and derived sunrise/sunset;
- crowd and wetness bias;
- snow allowance and accumulation rate;
- NPC clothing profile;
- seasonal decoration Data Layer;
- save payload containing date/season/transition state.

`AWTGDayNightEnvironment` now accepts `SeasonalDaylightHours`, so sunrise and sunset shift with the season while preserving the same 24-hour game clock. The value is also published to the material parameter collection as `SeasonDaylightHours`.

Snow remains conditional: winter permits accumulation, while actual precipitation is still controlled by the weather layer.
