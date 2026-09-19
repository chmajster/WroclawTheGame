# Economy core

The economy now has a native `UWTGEconomySubsystem` and a matching deterministic offline model.

## Runtime guarantees

- money is stored in integer grosz values in C++ to avoid floating-point currency drift;
- every transaction requires a stable transaction ID;
- repeated transaction IDs are idempotent and cannot duplicate rewards/purchases;
- credit and maximum-transaction limits are enforced atomically;
- purchases and rewards resolve through central price/reward keys;
- balance and ledger export/import as `FWTGEconomySaveState`.

The central catalogue already covers fuel, basic/major repairs, parking, tickets, tow release, medkits and quest/race/investigation rewards. Parking and garage systems reference the same keys.

The Python model mirrors ledger/idempotency behavior for balancing, data validation and non-UE tests.
