# Wrocław public transport

The extractor preserves OSM tram/bus relations and now converts them into a runtime-ready topology model. It remains source data for simulation, not a claim about the current public timetable.

## Runtime model

Every route exposes:
- ordered relation members and stable way sequence;
- ordered stop sequence;
- stable `service_id`;
- continuity status with exact way-to-way breaks;
- gameplay headway, logical update interval and physicalization radius.

Stops preserve name plus wheelchair/shelter metadata when present. Missing accessibility data remains `unknown`.

The configured headway is explicitly a gameplay simulation policy. An authoritative GTFS source can later replace the headway without changing route/stop identity.

## Integration

The model can drive off-screen logical vehicle progress and physicalize a limited number of vehicles near the player. Smart Object transit slots bind naturally by stop ID, while save state can persist `service_id + route progress` instead of transient vehicle actors.
