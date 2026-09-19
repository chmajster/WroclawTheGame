# Automated world validation

This validator aggregates cheap deterministic checks before Unreal is launched:
- duplicate/non-finite/outlier GIS features;
- road edge IDs, endpoints and lengths;
- orphan building entrances;
- duplicate quality decisions;
- quest stages referencing unknown districts;
- streaming plans referencing unknown sectors;
- unresolved hero landmarks.

It writes one PASS/FAIL report and returns a failing exit code for structural errors.

## Remaining
1. ingest parking, Smart Object, crowd/traffic, HLOD and Data Layer outputs;
2. floor/terrain intersection and floating-object checks;
3. route every critical quest objective and service response;
4. validate collision/nav/World Partition/Data Layers in UE commandlet;
5. ingest screenshot/Automation/performance reports;
6. attach this validator to the local full-build gate without manually dispatching CI.
