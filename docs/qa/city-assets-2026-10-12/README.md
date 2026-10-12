# First city asset batch — native evidence

All five PNGs were captured by Unreal Engine 5.8.2, not source mockups:

- [Kit front](Kit_Front.png)
- [Kit oblique](Kit_Oblique.png)
- [Cornice close-up](Kit_Cornice_Close.png)
- [District entrance](CityKit_Street.png)
- [District balcony](CityKit_Balcony.png)

[Explicit inspection record](review.json) includes screenshot hashes and the
native kit/map fingerprints. PASS_WITH_LIMITATIONS applies to this enrichment
batch: geometry, front orientation, supported balcony and portal clearance
inspected. The wider district still uses prototype facades/windows/ground,
with bright sun/deep shadow contrast and temporal noise visible in captures.
Generic details are not surveyed reconstruction, decorative doors add no
interiors, and this evidence does not establish AAA or whole-city FPS.

Integrated execution run 20261012-201713 passed all stages:
15 manifests, 16 focused source tests (27 broader city tests separately), five
Blender FBX products, editor module build, five native imports, 904 placed
instances on 44 buildings in 28 HISM clusters, native quality policy, the
five-asset WTG.Assets.TenementKit test, five fresh captures and district regression.
The latter passed DeliveryRules, GeneratedMeshCollision, SaveValidation,
WantedDecay, save smoke, full delivery session (147.35 s / 459.49 m physical
distance), and resume in a new process (5.01 s). Logic regressions used NullRHI;
images were rendered with the normal graphics backend. No CI was invoked.

Local logs/reports: Saved/CityDetails/production.json, budget.json, quality.json,
Saved/Pipeline/automation/city-production-20261012-201713/index.json and
Saved/Logs/CityProduction*, DistrictRulesTest, DistrictSaveSmoke,
DistrictSessionSmoke, DistrictSessionReplay. Reproduce using
Scripts/Invoke-CityAssetProduction.ps1; the report distinguishes execution
success from explicit visual review.
