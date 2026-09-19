# Dynamic city events

Dynamic events are now persistent runtime instances rather than one-shot catalogue selections.

## Runtime state

Eligibility still uses hour, weather and Heat, but selection honors event weights and cooldown history. A triggered event gets a stable instance ID, start/expiry time and a deterministic location from real city sectors or routable road edges.

Effects compile into world mutations such as:
- blocked edge / reroute request;
- local traffic slowdown;
- emergency service dispatch;
- Heat response;
- power lighting/security-camera disable.

Active instances and cooldown timestamps are saveable. Expired events and events whose streamed location unloads produce an explicit cleanup payload so mutations can be reverted without leaving stale roadblocks or disabled systems.
