#include "Data/CityDefinition.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool UCityDefinition::ImportCatalog(const FString &Json)
{
    TSharedPtr<FJsonObject> Root;
    const auto Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;
    int32 Version = 0;
    FString Fingerprint;
    const TArray<TSharedPtr<FJsonValue>> *Rows = nullptr;
    if (!Root->TryGetNumberField(TEXT("schema_version"), Version) || Version != 1 ||
        !Root->TryGetStringField(TEXT("fingerprint"), Fingerprint) || Fingerprint.IsEmpty() ||
        !Root->TryGetArrayField(TEXT("sectors"), Rows) || Rows->IsEmpty()) return false;
    TArray<FCitySectorDefinition> Parsed;
    TSet<FName> Ids;
    const TArray<FString> Statuses = {TEXT("Missing"), TEXT("GISOnly"), TEXT("Blockout"),
                                    TEXT("Playable"), TEXT("Detailed"), TEXT("Final")};
    for (const auto &Value : *Rows)
    {
        const TSharedPtr<FJsonObject> *Row = nullptr;
        if (!Value->TryGetObject(Row) || !Row->IsValid()) return false;
        FString Id, District, Name, Profile, Density, Status;
        const TArray<TSharedPtr<FJsonValue>> *Boundary = nullptr, *Blockers = nullptr;
        if (!(*Row)->TryGetStringField(TEXT("id"), Id) || Id.IsEmpty() ||
            !(*Row)->TryGetStringField(TEXT("district"), District) || District.IsEmpty() ||
            !(*Row)->TryGetStringField(TEXT("name"), Name) ||
            !(*Row)->TryGetStringField(TEXT("profile"), Profile) ||
            !(*Row)->TryGetStringField(TEXT("density"), Density) ||
            !(*Row)->TryGetStringField(TEXT("status"), Status) ||
            !(*Row)->TryGetArrayField(TEXT("boundary"), Boundary) || Boundary->Num() < 3 ||
            !(*Row)->TryGetArrayField(TEXT("blockers"), Blockers)) return false;
        const int32 StatusIndex = Statuses.Find(Status);
        if (StatusIndex == INDEX_NONE || Ids.Contains(FName(*Id))) return false;
        FCitySectorDefinition Sector;
        Sector.Id = FName(*Id); Sector.DistrictId = FName(*District);
        Sector.DisplayName = Name; Sector.ArchitectureProfile = FName(*Profile);
        Sector.ContentDensity = FName(*Density);
        Sector.Status = static_cast<ECityCoverageStatus>(StatusIndex);
        for (const auto &PointValue : *Boundary)
        {
            const TArray<TSharedPtr<FJsonValue>> *Point = nullptr;
            double X = 0, Y = 0;
            if (!PointValue->TryGetArray(Point) || Point->Num() != 2 ||
                !(*Point)[0]->TryGetNumber(X) || !(*Point)[1]->TryGetNumber(Y) ||
                !FMath::IsFinite(X) || !FMath::IsFinite(Y)) return false;
            Sector.Boundary.Add(FVector2D(X, Y));
        }
        for (const auto &Blocker : *Blockers)
        {
            FString Text;
            if (!Blocker->TryGetString(Text)) return false;
            Sector.Blockers.Add(Text);
        }
        const TSharedPtr<FJsonObject> *Counts = nullptr;
        if (!(*Row)->TryGetObjectField(TEXT("counts"), Counts) || !Counts->IsValid() ||
            !(*Counts)->TryGetNumberField(TEXT("buildings"), Sector.BuildingCount) ||
            !(*Counts)->TryGetNumberField(TEXT("roads"), Sector.RoadCount) ||
            Sector.BuildingCount < 0 || Sector.RoadCount < 0) return false;
        Ids.Add(Sector.Id); Parsed.Add(MoveTemp(Sector));
    }
    SourceFingerprint = Fingerprint;
    Sectors = MoveTemp(Parsed);
    return true;
}
